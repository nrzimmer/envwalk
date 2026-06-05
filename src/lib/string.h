#ifndef STRING_H
#define STRING_H

#include <stddef.h>

typedef struct {
    size_t count;
    char *data;
} String;

#define String_Fmt "%.*s"
#define String_Arg(sv) (int) (sv).count, (sv).data


#endif //STRING_H
