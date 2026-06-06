#ifndef ENVWALK_CONFIG_H
#define ENVWALK_CONFIG_H

#include "nob.h"
#include "path.h"

void parse_config();
void save_config();
bool is_path_allowed(Path path);
int allow_path(Path path);
int deny_path(Path path);
int list_paths();
void Config_free(void);

#ifdef TESTING
void config_reset_for_testing(void);
#endif

#endif //ENVWALK_CONFIG_H
