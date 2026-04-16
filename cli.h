#ifndef ENVWALK_CLI_H
#define ENVWALK_CLI_H

typedef enum {
    ALLOW,
    DENY,
    HELP,
    RUN,
    CHPWD,
    LIST,
    HOOK
} Action;

typedef enum {
    ZSH,
    BASH,
    UNKNOWN,
} Shell;

typedef struct {
    Action action;
    char *text;
} Params;

Params *parse_params(int argc, const char **argv);
Shell parse_shell(const char *shell);

#endif
