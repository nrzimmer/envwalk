#ifndef ENVWALK_DOTENV_H
#define ENVWALK_DOTENV_H

#include "nob.h"
#include "path.h"

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

bool parse_dotenv(Variables *variables, Path folder);

#endif
