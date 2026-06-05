#ifndef ENVWALK_CLI_H
#define ENVWALK_CLI_H
#include "path.h"

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
    String_View text;
    Path path;
} Params;

Params *parse_params(int argc, const char **argv);
Shell parse_shell(String_View shell);

#endif
