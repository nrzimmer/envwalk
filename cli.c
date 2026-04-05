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
        printf("Usage: zenv [action] [path]\n");
        printf("Actions:\n");
        printf("  allow: Add a path to the allowed paths list\n");
        printf("  deny: Remove a path from the allowed paths list\n");
        printf("  help: Show this help message\n");
        printf("Path:\n");
        printf("  If a folder path is provided, it will use FOLDER/.env as path\n");
        printf("  If no path is provided, it will use PWD/.env as path\n");
        printf("\n");
        printf("If no action is provided, it will print the changed .env vars to STDOUT.\n\n");
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
            params->text = expand_path(sb.items);
        }
    }
    return params;
}
