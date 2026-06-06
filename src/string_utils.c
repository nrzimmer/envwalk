#include "string_utils.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>

String_Builder sb_from_string_list(const StringList *da) {
    String_Builder sb = {0};
    sb_append(&sb, '/');
    for (size_t i = 0; i < da->count; ++i) {
        sb_append_cstr(&sb, da->items[i]);
        sb_append(&sb, '/');
    }
    return sb;
}

void String_Builder_free(String_Builder *sb) {
    sb_free(*sb);
}

void char_free(char **p) {
    free(*p);
}

void *sv_data_dup(const String_View sv) {
    void *dst = malloc(sv.count);
    assert(dst != NULL);
    memcpy(dst, sv.data, sv.count);
    return dst;
}

String_View string_from_sv(const String_View sv) {
    const String_View str = {.count = sv.count, .data = sv_data_dup(sv)};
    return str;
}

bool sv_eq_cstr_ci(const String_View sv, const char *cstr) {
    const size_t len = strlen(cstr);
    if (sv.count != len) return false;
    return strncasecmp(sv.data, cstr, len) == 0;
}
