#ifndef ENVWALK_DOTENV_H
#define ENVWALK_DOTENV_H

#include "nob.h"

typedef struct {
    String_View key;
    String_View value;
    String_View path;
} KeyValuePair;

typedef struct {
    KeyValuePair *items;
    size_t count;
    size_t capacity;
} Variables;

typedef struct {
    char *path;
    Variables *variables;
} DotEnv;

bool parse_dotenv(Variables *variables, char *filepath);

#endif
