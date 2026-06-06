#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "path.h"
#include "framework.h"

// Build a Path, render it null-terminated, free the Path, return the builder.
// Caller frees the returned String_Builder.
static String_Builder sb_of(const char *cstr)
{
    Defer(Path) p = path_from_cstr(cstr);
    return sb_from_path(p, true);
}

static void test_path_parts_root(void)
{
    nob_set_log_handler(nob_null_log_handler);
    Defer(Path) p = path_from_cstr("/");
    nob_set_log_handler(nob_default_log_handler);
    ASSERT(p.count == 0);
}

static void test_path_parts_absolute(void)
{
    nob_set_log_handler(nob_null_log_handler);
    Defer(Path) p = path_from_cstr("/foo/bar/baz");
    nob_set_log_handler(nob_default_log_handler);
    ASSERT(p.count == 3);
    ASSERT_SV_EQ(p.parts[0], "foo");
    ASSERT_SV_EQ(p.parts[1], "bar");
    ASSERT_SV_EQ(p.parts[2], "baz");
}

static void test_path_parts_trailing_slash(void)
{
    nob_set_log_handler(nob_null_log_handler);
    Defer(Path) p = path_from_cstr("/foo/bar/");
    nob_set_log_handler(nob_default_log_handler);
    ASSERT(p.count == 2);
    ASSERT_SV_EQ(p.parts[0], "foo");
    ASSERT_SV_EQ(p.parts[1], "bar");
}

static void test_path_parts_single(void)
{
    nob_set_log_handler(nob_null_log_handler);
    Defer(Path) p = path_from_cstr("/foo");
    nob_set_log_handler(nob_default_log_handler);
    ASSERT(p.count == 1);
    ASSERT_SV_EQ(p.parts[0], "foo");
}

static void test_path_normalize_dotdot(void)
{
    nob_set_log_handler(nob_null_log_handler);
    Defer(String_Builder) sb = sb_of("/foo/bar/../baz");
    nob_set_log_handler(nob_default_log_handler);
    ASSERT_STR_EQ(sb.data, "/foo/baz/");
}

static void test_path_normalize_dot(void)
{
    nob_set_log_handler(nob_null_log_handler);
    Defer(String_Builder) sb = sb_of("/foo/./bar");
    nob_set_log_handler(nob_default_log_handler);
    ASSERT_STR_EQ(sb.data, "/foo/bar/");
}

static void test_path_normalize_multiple_dotdot(void)
{
    nob_set_log_handler(nob_null_log_handler);
    Defer(String_Builder) sb = sb_of("/a/b/c/../../d");
    nob_set_log_handler(nob_default_log_handler);
    ASSERT_STR_EQ(sb.data, "/a/d/");
}

static void test_path_normalize_dotdot_past_root(void)
{
    nob_set_log_handler(nob_null_log_handler);
    Defer(String_Builder) sb = sb_of("/foo/../../bar");
    nob_set_log_handler(nob_default_log_handler);
    ASSERT_STR_EQ(sb.data, "/bar/");
}

static void test_path_normalize_absolute(void)
{
    nob_set_log_handler(nob_null_log_handler);
    Defer(String_Builder) sb = sb_of("/usr/local/bin");
    nob_set_log_handler(nob_default_log_handler);
    ASSERT_STR_EQ(sb.data, "/usr/local/bin/");
}

static void test_path_tilde_expansion(void)
{
    const char *home = getenv("HOME");
    char expected[4096];
    snprintf(expected, sizeof(expected), "%s/projects/", home);
    nob_set_log_handler(nob_null_log_handler);
    Defer(String_Builder) sb = sb_of("~/projects");
    nob_set_log_handler(nob_default_log_handler);
    ASSERT_STR_EQ(sb.data, expected);
}

static void test_path_relative_expansion(void)
{
    char buf[4096];
    getcwd(buf, sizeof(buf));
    char expected[8192];
    snprintf(expected, sizeof(expected), "%s/foo/bar/", buf);
    nob_set_log_handler(nob_null_log_handler);
    Defer(String_Builder) sb = sb_of("foo/bar");
    nob_set_log_handler(nob_default_log_handler);
    ASSERT_STR_EQ(sb.data, expected);
}

static void test_path_type_file(void)
{
    Defer(char) *f = write_temp_file("x");
    Defer(Path) p = path_from_cstr(f);
    ASSERT(p.type == PT_FILE);
    Defer(String_Builder) sb = sb_from_path(p, true);
    ASSERT_STR_EQ(sb.data, f);
    unlink(f);
}

static void test_path_type_dir(void)
{
    Defer(Path) p = path_from_cstr("/tmp");
    ASSERT(p.type == PT_DIR);
    Defer(String_Builder) sb = sb_from_path(p, true);
    ASSERT_STR_EQ(sb.data, "/tmp/");
}

static void test_path_type_nonexistent(void)
{
    nob_set_log_handler(nob_null_log_handler);
    Defer(Path) p = path_from_cstr("/tmp/envwalk_no_such_xyz/");
    nob_set_log_handler(nob_default_log_handler);
    ASSERT(p.type == PT_NOT_EXISTS);
}

static void test_get_pwd_is_absolute(void)
{
    Defer(Path) pwd = get_pwd();
    Defer(String_Builder) sb = sb_from_path(pwd, true);
    ASSERT(sb.data[0] == '/');
}

static void test_get_pwd_matches_getcwd(void)
{
    char buf[4096];
    getcwd(buf, sizeof(buf));
    char expected[8192];
    snprintf(expected, sizeof(expected), "%s/", buf);
    Defer(Path) pwd = get_pwd();
    Defer(String_Builder) sb = sb_from_path(pwd, true);
    ASSERT_STR_EQ(sb.data, expected);
}

static void test_path_eq_same(void)
{
    nob_set_log_handler(nob_null_log_handler);
    Defer(Path) a = path_from_cstr("/foo/bar");
    Defer(Path) b = path_from_cstr("/foo/bar");
    nob_set_log_handler(nob_default_log_handler);
    ASSERT(path_eq(a, b));
}

static void test_path_eq_different(void)
{
    nob_set_log_handler(nob_null_log_handler);
    Defer(Path) a = path_from_cstr("/foo/bar");
    Defer(Path) b = path_from_cstr("/foo/baz");
    nob_set_log_handler(nob_default_log_handler);
    ASSERT(!path_eq(a, b));
}

void run_path_tests(void)
{
    printf("path:\n");
    SUITE("parts: root → 0 segments");
    test_path_parts_root();
    SUITE("parts: absolute 3 segments");
    test_path_parts_absolute();
    SUITE("parts: trailing slash stripped");
    test_path_parts_trailing_slash();
    SUITE("parts: single segment");
    test_path_parts_single();
    SUITE("normalize: dotdot");
    test_path_normalize_dotdot();
    SUITE("normalize: dot");
    test_path_normalize_dot();
    SUITE("normalize: multiple dotdot");
    test_path_normalize_multiple_dotdot();
    SUITE("normalize: dotdot past root");
    test_path_normalize_dotdot_past_root();
    SUITE("normalize: absolute no tilde");
    test_path_normalize_absolute();
    SUITE("expand: tilde");
    test_path_tilde_expansion();
    SUITE("expand: relative");
    test_path_relative_expansion();
    SUITE("type: file detected");
    test_path_type_file();
    SUITE("type: dir detected");
    test_path_type_dir();
    SUITE("type: nonexistent");
    test_path_type_nonexistent();
    SUITE("get_pwd: absolute");
    test_get_pwd_is_absolute();
    SUITE("get_pwd: matches getcwd");
    test_get_pwd_matches_getcwd();
    SUITE("path_eq: same paths");
    test_path_eq_same();
    SUITE("path_eq: different paths");
    test_path_eq_different();
}
