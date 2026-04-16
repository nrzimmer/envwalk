#ifndef ENVWALK_PATH_H
#define ENVWALK_PATH_H

#include "types.h"

char *expand_path(const char *path);
char *expand_path_file(const char *path);
int is_directory(const char *path);
StringList *get_path_parts(const char *pwd);

#endif