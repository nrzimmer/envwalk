#ifndef ENVWALK_TYPES_H
#define ENVWALK_TYPES_H

#include <stddef.h>
#include "nob.h"

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} StringList;

String_Builder sb_from_string_list(const StringList *da);

#endif