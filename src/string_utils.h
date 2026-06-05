#ifndef ENVWALK_STRING_UTILS_H
#define ENVWALK_STRING_UTILS_H

#ifdef _WIN32
#error "envwalk does not support Windows"
#endif

#include <stddef.h>
#include <stdbool.h>
#include <strings.h>
#include "nob.h"

typedef struct {
    char **items;
    size_t count;
    size_t capacity;
} StringList;

String_Builder sb_from_string_list(const StringList *da);

void *sv_data_dup(String_View sv);
String_View string_from_sv(String_View sv);

#define string_from_sb(sb) (string_from_sv(sb_to_sv(sb)))

bool sv_eq_cstr_ci(String_View sv, const char *cstr);

#endif
