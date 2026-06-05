// Need this because i am using c23
#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#define NOB_IMPLEMENTATION
#include "nob.h"
#undef NOB_IMPLEMENTATION

#include "path.h"
#include "config.h"
#include "dotenv.h"
#include "cli.h"
#include "stack_trace.h"

static int run(StringList *unsets) {
    Path pwd = get_pwd();
    Variables dot_env = {0};
    while (pwd.count > 0) {
        if (is_path_allowed(pwd)) {
            if (!parse_dotenv(&dot_env, pwd)) {
                String_Builder sb = sb_from_path(pwd, true);
                nob_log(NOB_ERROR, "Failed to parse .env file at %s", sb.data);
                return 1;
            }
        }
        --pwd.count;
    }

    if (unsets != nullptr) {
        for (size_t i = 0; i < unsets->count; ++i) {
            bool found = false;
            for (size_t j = 0; j < dot_env.count; ++j) {
                if (strncmp(unsets->items[i], dot_env.items[j].key.data, dot_env.items[j].key.count) == 0) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                printf("unset %s\n", unsets->items[i]);
            }
        }
    }

    String_Builder sb = {0};
    for (size_t i = 0; i < dot_env.count; ++i) {
        String_View value = dot_env.items[i].value;
        char *value_str;
        if (value.count > 0 && value.data[0] == '~') {
            const Path path = path_from_sv(value);
            sb = sb_from_path(path, true);
            value_str = sb.data;
        } else {
            value_str = strndup(value.data, value.count);
        }
        printf("export "SV_Fmt"=\"%s\"\n", SV_Arg(dot_env.items[i].key), value_str);
    }

    return 0;
}

int chpwd(Path old_path) {
    // First we need to traverse the old_path back until we find a common folder with the current path
    // If there is an allowed folder in each partial path, we need to unset the variables on that folder env

    const Path pwd = get_pwd();

    const size_t max = old_path.count < pwd.count ? old_path.count : pwd.count;
    size_t same = 0;
    for (size_t i = 0; i < max; ++i) {
        if (nob_sv_eq(old_path.items[i], pwd.items[i])) {
            same = i + 1;
        } else {
            break;
        }
    }

    StringList *unset = calloc(1, sizeof(StringList));

    while (old_path.count > same) {
        if (is_path_allowed(old_path)) {
            Variables dot_env = {0};
            if (parse_dotenv(&dot_env, old_path)) {
                for (size_t j = 0; j < dot_env.count; ++j) {
                    da_append(unset, strndup(dot_env.items[j].key.data, dot_env.items[j].key.count));
                }
            }
        }
        --old_path.count;
    }

    // Now we run normally to set all the variables in current path
    // This may be unnecessary because it will be run before the next command too
    return run(unset);
}

extern const char _binary_hook_zsh_start[];
extern const char _binary_hook_zsh_end[];

static void hook_zsh(const Path bin) {
    const size_t size = _binary_hook_zsh_end - _binary_hook_zsh_start;
    char *format = strndup(_binary_hook_zsh_start, size);
    const String_Builder sb = sb_from_path(bin, true);
    printf(format, sb.data, sb.data);
    free(format);
}

extern const char _binary_hook_bash_start[];
extern const char _binary_hook_bash_end[];

static void hook_bash(const Path bin) {
    const size_t size = _binary_hook_bash_end - _binary_hook_bash_start;
    char *format = strndup(_binary_hook_bash_start, size);
    const String_Builder sb = sb_from_path(bin, true);
    printf(format, sb.data, sb.data);
    free(format);
}

int hook(const Path bin, const String_View str) {
    const Shell shell = parse_shell(str);
    switch (shell) {
        case ZSH:
            hook_zsh(bin);
            return 0;
        case BASH:
            hook_bash(bin);
            return 0;
        default:
            fprintf(stderr, "Unsupported shell: "SV_Fmt"\n", SV_Arg(str));
            return 1;
    }
}

int main(const int argc, const char **argv) {
    setup_handler();
    Params *params = parse_params(argc, argv);

    if (params->action == RUN || params->action == CHPWD)
        nob_minimal_log_level = NOB_ERROR;

    parse_config();

    if (params->text.count == 0 && (params->action == ALLOW || params->action == DENY)) {
        params->path = get_pwd();
    }

    switch (params->action) {
        case ALLOW:
            return allow_path(params->path);
        case DENY:
            return deny_path(params->path);
        case LIST:
            return list_paths();
        case RUN:
            return run(nullptr);
        case CHPWD:
            return chpwd(params->path);
        case HOOK:
            return hook(path_from_cstr(argv[0]), params->text);
        case HELP:
        default:



    }

    UNREACHABLE("This should not happen...");
}
