#ifndef ENVWALK_PATH_H
#define ENVWALK_PATH_H

#include <stdbool.h>
#include "string_utils.h"

typedef enum PathType {
    PT_UNKNOWN = 0,
    PT_DIR = 1,
    PT_FILE = 2,
    PT_NOT_EXISTS = 3,
} PathType;

typedef struct Path{
    union {
        String_View *parts;
        String_View *items;
    };
    size_t count;
    size_t capacity;
    char first_char;
    PathType type;
} Path;

String_Builder sb_from_path(Path path, bool null_terminated);
String_Builder sb_from_path_with_file(Path path, String_View file);
PathType get_path_type(Path path);
bool path_eq(Path a, Path b);

#define path_from_cstr(cstr) (path_from_sv(sv_from_cstr(cstr)))
#define path_from_sb(sb) (path_from_sv(sb_to_sv(sb)))
#define sv_eq_cstr(sv, cstr) (sv_eq(sv, sv_from_cstr(cstr)))
#define sb_eq_cstr(sb, cstr) (sv_eq(sb_to_sv(sb), sv_from_cstr(cstr)))

Path get_pwd(void);
Path path_from_sv(String_View sv);
void path_free(Path *path);

#endif
