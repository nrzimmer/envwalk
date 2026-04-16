#include "config.h"
#include "nob.h"
#include "path.h"

static const char *configPath = nullptr;

typedef struct {
    StringList allowedPaths;
} Config;

static Config config = {0};

void parse_config() {
    configPath = expand_path_file("~/.config/envwalk");
    NOB_ASSERT(configPath != nullptr && "config_path not set");

    StringList *allowedPaths = &config.allowedPaths;
    String_Builder sb = {0};
    if (!read_entire_file(configPath, &sb)) {
        nob_log(NOB_INFO, "Generating a new config file");
        return;
    }

    String_View sv = sb_to_sv(sb);
    while (sv.count > 0) {
        String_View line = sv_chop_by_delim(&sv, '\n');
        line = sv_trim(line);

        if ((line.count == 0) ||
            (line.count > 0 && line.data[0] == '#') ||
            (line.count > 1 && line.data[0] == '/' && line.data[1] == '/'))
            continue;

        const String_View key = sv_chop_by_delim(&line, '=');
        if (line.count == 0) {
            nob_log(NOB_WARNING, "Missing value for key: %.*s", (int) key.count, key.data);
            continue;
        }

        if (!sv_starts_with(key, sv_from_cstr("allowed"))) {
            nob_log(NOB_WARNING, "Unknown key: %.*s", (int) key.count, key.data);
            continue;
        }

        char *path = strndup(line.data, line.count);
        da_append(allowedPaths, path);
    }
}

void save_config() {
    NOB_ASSERT(configPath != nullptr && "config_path not set");

    String_Builder sb = {0};
    for (size_t i = 0; i < config.allowedPaths.count; ++i) {
        char *path = config.allowedPaths.items[i];
        sb_append_cstr(&sb, "allowed=");
        sb_append_cstr(&sb, path);
        sb_append_cstr(&sb, "\n");
    }
    write_entire_file(configPath, sb.items, sb.count);
}

bool is_path_allowed(const char *path) {
    const char *expath = expand_path(path);
    for (size_t i = 0; i < config.allowedPaths.count; ++i) {
        if (strcmp(config.allowedPaths.items[i], expath) == 0) {
            return true;
        }
    }
    return false;
}

bool is_path_allowed_sb(const String_Builder *path) {
    return is_path_allowed(strndup(path->items, path->count));
}

int allow_path(const char *path) {
    if (!is_directory(path)) {
        nob_log(NOB_ERROR, "%s must be a folder", path);
        return 1;
    }

    nob_log(NOB_INFO, "Adding %s to allowed paths", path);

    if (!is_path_allowed(path)) {
        da_append(&config.allowedPaths, strdup(path));
    }

    save_config();
    return 0;
}

int deny_path(const char *path) {
    if (!is_directory(path)) {
        nob_log(NOB_ERROR, "%s must be a folder", path);
        return 1;
    }

    nob_log(NOB_INFO, "Removing %s from allowed paths", path);

    for (size_t i = 0; i < config.allowedPaths.count; ++i) {
        if (strcmp(config.allowedPaths.items[i], path) == 0) {
            da_remove_unordered(&config.allowedPaths, i);
            break;
        }
    }

    save_config();
    return 0;
}

int list_paths() {
    printf("List of paths to be autoloaded:\n");
    for (size_t i = 0; i < config.allowedPaths.count; ++i) {
        printf("- %s\n", config.allowedPaths.items[i]);
    }
    return 0;
}

#ifdef TESTING
void config_reset_for_testing(void) {
    config = (Config){0};
    configPath = nullptr;
}
#endif