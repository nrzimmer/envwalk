#ifndef ZENV_CONFIG_H
#define ZENV_CONFIG_H

#include "nob.h"

void parse_config();
void save_config();
bool is_path_allowed_sb(const String_Builder *path);
bool is_path_allowed(const char *path);
int allow_path(const char *path);
int deny_path(const char *path);
int list_paths();

#ifdef TESTING
void config_reset_for_testing(void);
#endif

#endif //ZENV_CONFIG_H
