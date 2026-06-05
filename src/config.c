#include "config.h"
#include "nob.h"
#include "path.h"

static Path config_path;

typedef struct PathList {
    Path *items;
    size_t count;
    size_t capacity;
} PathList;

typedef struct {
    PathList allowed_paths;
} Config;

static Config config = {0};

void parse_config() {
    path_free(&config_path);
    config_path = path_from_cstr("~/.config/");
    NOB_ASSERT(config_path.count > 0 && "config_path not set");
    const String_Builder sb_config_file = sb_from_path_with_file(config_path, sv_from_cstr("envwalk"));

    PathList *allowed_paths = &config.allowed_paths;
    String_Builder sb = {0};
    if (!read_entire_file(sb_config_file.data, &sb)) {
        nob_log(NOB_INFO, "Generating a new config file");
        sb_free(sb);
        sb_free(sb_config_file);
        return;
    }
    sb_free(sb_config_file);

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

        const Path path = path_from_sv(line);
        da_append(allowed_paths, path);
    }
    sb_free(sb);
}

void save_config() {
    path_free(&config_path);
    config_path = path_from_cstr("~/.config/");
    NOB_ASSERT(config_path.count > 0 && "config_path not set");
    const String_Builder sb_config_file = sb_from_path_with_file(config_path, sv_from_cstr("envwalk"));

    String_Builder sb = {0};
    for (size_t i = 0; i < config.allowed_paths.count; ++i) {
        const String_Builder path_sb = sb_from_path(config.allowed_paths.items[i], false);
        sb_append_cstr(&sb, "allowed=");
        sb_append_buf(&sb, path_sb.data, path_sb.count);
        sb_append_cstr(&sb, "\n");
        sb_free(path_sb);
    }
    if (!write_entire_file(sb_config_file.data, sb.data, sb.count)) {
        nob_log(NOB_ERROR, "Failed to write config file: %s", sb_config_file.data);
    }
    sb_free(sb);
    sb_free(sb_config_file);
}

bool is_path_allowed(const Path path) {
    if (get_path_type(path) != PT_DIR) {
        const String_Builder sb = sb_from_path(path, true);
        nob_log(NOB_WARNING, "\"%s\" must be a folder", sb.data);
        sb_free(sb);
        return false;
    }
    for (size_t i = 0; i < config.allowed_paths.count; ++i) {
        if (path_eq(config.allowed_paths.items[i], path)) {
            return true;
        }
    }
    return false;
}

// Takes ownership of `path`: it is stored in the allowlist, or freed if the
// path is rejected or already present.
int allow_path(Path path) {
    const String_Builder sb = sb_from_path(path, true);
    if (path.type != PT_DIR) {
        nob_log(NOB_ERROR, "%s must be a folder", sb.data);
        sb_free(sb);
        path_free(&path);
        return 1;
    }

    nob_log(NOB_INFO, "Adding %s to allowed paths", sb.data);
    sb_free(sb);

    if (!is_path_allowed(path)) {
        da_append(&config.allowed_paths, path);
    } else {
        path_free(&path);
    }

    save_config();
    return 0;
}

// Takes ownership of `path`, which is only used for comparison and freed here.
int deny_path(Path path) {
    const String_Builder sb = sb_from_path(path, true);
    nob_log(NOB_INFO, "Removing %s from allowed paths", sb.data);
    sb_free(sb);

    for (size_t i = 0; i < config.allowed_paths.count; ++i) {
        if (path_eq(path, config.allowed_paths.items[i])) {
            path_free(&config.allowed_paths.items[i]);
            da_remove_unordered(&config.allowed_paths, i);
            break;
        }
    }

    path_free(&path);
    save_config();
    return 0;
}

int list_paths() {
    printf("List of paths to be autoloaded:\n");
    for (size_t i = 0; i < config.allowed_paths.count; ++i) {
        const String_Builder sb = sb_from_path(config.allowed_paths.items[i], true);
        printf("- %s\n", sb.data);
        sb_free(sb);
    }
    return 0;
}

void config_free(void) {
    for (size_t i = 0; i < config.allowed_paths.count; ++i)
        path_free(&config.allowed_paths.items[i]);
    da_free(config.allowed_paths);
    config = (Config){0};
    path_free(&config_path);
}

#ifdef TESTING
void config_reset_for_testing(void) {
    config_free();
}
#endif