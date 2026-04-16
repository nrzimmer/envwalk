#include "cli.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "nob.h"
#include "path.h"

static Action parse_action(const char *action) {
    if (strcasecmp(action, "allow") == 0) return ALLOW;
    if (strcasecmp(action, "deny") == 0) return DENY;
    if (strcasecmp(action, "list") == 0) return LIST;
    if (strcasecmp(action, "cd") == 0) return CHPWD;
    if (strcasecmp(action, "hook") == 0) return HOOK;
    return HELP;
}

Shell parse_shell(const char *shell) {
    if (strcasecmp(shell, "zsh") == 0) return ZSH;
    if (strcasecmp(shell, "bash") == 0) return BASH;
    return UNKNOWN;
}

Params *parse_params(const int argc, const char **argv) {
    Action action = RUN;
    if (argc > 1) {
        action = parse_action(argv[1]);
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
    params->text = nullptr;
    String_Builder sb = {0};
    if (argc > 2) {
        for (int i = 2; i < argc; ++i) {
            sb_append_cstr(&sb, argv[i]);
            sb_append_cstr(&sb, " ");
        }
        sb.items[sb.count - 1] = 0;
        if (action == HOOK) {
            params->text = strdup(sb.items);
        } else {
            String_View sv = sv_from_cstr(sb.items);
            sv_chop_prefix(&sv, sv_from_cstr("\""));
            sv_chop_suffix(&sv, sv_from_cstr("\""));
            params->text = expand_path(strndup(sv.data, sv.count));
        }
    }
    return params;
}
