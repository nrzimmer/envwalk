#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <unistd.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>

#include "path.h"
#include "string_utils.h"
#include "nob.h"

bool path_eq(const Path a, const Path b) {
    if (a.count != b.count)
        return false;
    for (size_t i = 0; i < a.count; ++i) {
        if (!sv_eq(a.parts[i], b.parts[i]))
            return false;
    }
    return true;
}

PathType get_path_type(const Path path) {
    if (path.type != PT_UNKNOWN)
        return path.type;

    String_Builder sb = {0};
    sb_append(&sb, '/');
    for (size_t i = 0; i < path.count; ++i) {
        nob_sb_append_sv(&sb, path.parts[i]);
        if (i + 1 < path.count)
            sb_append(&sb, '/');
    }
    sb_append_null(&sb);

    struct stat st;
    if (stat(sb.data, &st) != 0)
        return PT_NOT_EXISTS;
    if (S_ISDIR(st.st_mode))
        return PT_DIR;
    return PT_FILE;
}

Path normalize(const Path path) {
    Path normalized = {0};
    normalized.first_char = path.first_char;
    normalized.type = path.type;

    for (size_t i = 0; i < path.count; ++i) {
        if (sv_eq_cstr(path.parts[i], "."))
            continue;

        if (sv_eq_cstr(path.parts[i], "..")) {
            if (normalized.count > 0)
                normalized.count--;
            continue;
        }

        da_append(&normalized, path.parts[i]);
    }
    return normalized;
}

Path expand(const Path path) {
    if (path.first_char == '/')
        return path;

    NOB_ASSERT(path.count != 0 && "Cannot expand empty relative path.");

    if (path.parts[0].count > 0) {

        if (sv_eq_cstr(path.parts[0], "~")) {
            const char *env_home = getenv("HOME");
            NOB_ASSERT(env_home != nullptr && "Could not expand ~. HOME is not set.");

            Path home = path_from_cstr(env_home);
            home.type = path.type;
            for (size_t i = 1; i < path.count; ++i) {
                da_append(&home, path.parts[i]);
            }
            return home;
        }

        Path pwd = get_pwd();
        pwd.type = path.type;
        for (size_t i = 0; i < path.count; ++i) {
            da_append(&pwd, path.parts[i]);
        }
        return pwd;
    }

    UNREACHABLE("Should not have an empty Path part");
}

Path path_from_sv(String_View sv) {
    bool is_absolute = sv.count > 0 && sv.data[0] == '/';
    sv_chop_prefix(&sv, sv_from_cstr("/"));
    sv_chop_suffix(&sv, sv_from_cstr("/"));

    Path path = {0};
    path.first_char = is_absolute ? '/' : (sv.count > 0 ? sv.data[0] : '\0');
    path.type = PT_UNKNOWN;

    String_View part = sv_chop_by_delim(&sv, '/');
    while (part.count != 0) {
        da_append(&path, string_from_sv(part));
        part = sv_chop_by_delim(&sv, '/');
    }
    Path expanded = expand(path);
    expanded.type = get_path_type(expanded);
    return normalize(expanded);
}

String_Builder sb_from_path_with_file(const Path path, const String_View file) {
    if (get_path_type(path) == PT_DIR) {
        String_Builder sb = sb_from_path(path, false);
        sb_append_sv(&sb, file);
        sb_append_null(&sb);
        return sb;
    }

    String_Builder sb = sb_from_path(path, true);
    nob_log(NOB_ERROR, "sb_from_path_with_file: \"%s\" is not a directory (type=%d)", sb.data, path.type);
    UNREACHABLE("Should not append a file to a path that is not a directory");
}

String_Builder sb_from_path(Path path, bool null_terminated) {
    if (path.type == PT_UNKNOWN)
        path.type = get_path_type(path);

    String_Builder sb = {0};
    sb_append(&sb, '/');
    for (size_t i = 0; i < path.count; ++i) {
        nob_sb_append_sv(&sb, path.parts[i]);
        sb_append(&sb, '/');
    }

    sb.count--;

    if (path.type == PT_NOT_EXISTS)
        nob_log(NOB_WARNING, "Path \""SV_Fmt"\" does not exists.", SV_Arg(sb));

    if (path.type != PT_FILE)
        sb.count++;

    if (null_terminated)
        sb_append_null(&sb);

    return sb;
}

char *get_pwd_cstr(void) {
    char *buf = nullptr;

    buf = getcwd(nullptr, 0);
    if (buf != nullptr) {
        return buf;
    }

    int err = errno;

    buf = realpath(".", nullptr);
    if (buf != nullptr) {
        return buf;
    }

#if defined(__linux__)
    {
        char tmp[PATH_MAX];
        ssize_t len = readlink("/proc/self/cwd", tmp, sizeof(tmp) - 1);
        if (len != -1) {
            tmp[len] = '\0';
            buf = malloc(len + 1);
            if (buf != nullptr) {
                memcpy(buf, tmp, len + 1);
                return buf;
            }
        }
    }
#endif

    errno = err;
    fprintf(stderr, "get_pwd failed (errno=%d): %s\n", errno, strerror(errno));
    UNREACHABLE("Without PWD this cannot work");
}

Path get_pwd(void) {
    return path_from_cstr(get_pwd_cstr());
}