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

// GCC/Clang scope-exit cleanup ("poor man's defer"). Declare an owning local as
//   Defer(String_Builder) sb = ...;
// and it is freed automatically when the variable leaves scope. Only use it for
// locals that are NOT moved out (returned / stored / ownership transferred).
#define Defer(type) __attribute__((cleanup(type##_free))) type

void String_Builder_free(String_Builder *sb);
void char_free(char **p);

#define string_from_sb(sb) (string_from_sv(sb_to_sv(sb)))

bool sv_eq_cstr_ci(String_View sv, const char *cstr);

#endif
