#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "nob.h"
#include "path.h"

static Action parse_action(const String_View action) {
    if (sv_eq_cstr_ci(action, "allow")) return ALLOW;
    if (sv_eq_cstr_ci(action, "deny")) return DENY;
    if (sv_eq_cstr_ci(action, "list")) return LIST;
    if (sv_eq_cstr_ci(action, "cd")) return CHPWD;
    if (sv_eq_cstr_ci(action, "hook")) return HOOK;
    return HELP;
}

Shell parse_shell(const String_View shell) {
    if (sv_eq_cstr_ci(shell, "zsh")) return ZSH;
    if (sv_eq_cstr_ci(shell, "bash")) return BASH;
    return UNKNOWN;
}

Params *parse_params(const int argc, const char **argv) {
    Action action = RUN;
    if (argc > 1) {
        action = parse_action(sv_from_cstr(argv[1]));
    }

    if (action == HELP) {
        printf("Usage: envwalk [action] [args]\n\n");
        printf("Actions:\n");
        printf("  allow [path]   Allow a directory to auto-load its .env file (defaults to current directory)\n");
        printf("  deny  [path]   Remove a directory from the allowed list (defaults to current directory)\n");
        printf("  list           Show all allowed directories\n");
        printf("  hook <shell>   Print the shell hook to be eval'd in your shell config (zsh, bash)\n");
        printf("  help           Show this help message\n\n");
        printf("If no action is provided, envwalk prints the current environment exports to stdout.\n\n");
        exit(0);
    }

    Params *params = calloc(1, sizeof(Params));
    params->action = action;
    String_Builder sb = {0};
    if (argc > 2) {
        for (int i = 2; i < argc; ++i) {
            sb_append_cstr(&sb, argv[i]);
            sb_append_cstr(&sb, " ");
        }
        sb.count--;
        if (action == HOOK) {
            params->text = string_from_sb(sb);
        } else {
            sb_append_null(&sb);
            sb.count--;
            String_View sv = nob_sv_from_parts(sb.data, sb.count);
            sv_chop_prefix(&sv, sv_from_cstr("\""));
            sv_chop_suffix(&sv, sv_from_cstr("\""));
            params->path = path_from_sv(sv);
        }
    }
    sb_free(sb);
    return params;
}

void params_free(Params *params) {
    if (params == nullptr) return;
    if (params->action == HOOK)
        free((void *) params->text.data);
    path_free(&params->path);
    free(params);
}
