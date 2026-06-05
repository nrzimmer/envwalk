#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "path.h"
#include "framework.h"

static void test_expand_path_empty_string(void)
{
    // Empty string → treated as relative → expands to cwd/
    char *cwd = get_pwd();
    char expected[4096];
    snprintf(expected, sizeof(expected), "%s/", cwd);
    char *result = expand_path("");
    ASSERT_STR_EQ(result, expected);
    free(cwd);
}

static void test_expand_path_root(void)
{
    char *result = expand_path("/");
    ASSERT_STR_EQ(result, "/");
}

static void test_expand_path_file_absolute(void)
{
    char *result = expand_path_file("/usr/local/bin/envwalk");
    ASSERT_STR_EQ(result, "/usr/local/bin/envwalk");
}

static void test_get_path_parts_empty_string(void)
{
    StringList *parts = get_path_parts("");
    ASSERT(parts->count == 0);
}

static void test_get_pwd_matches_getcwd(void)
{
    char buf[4096];
    getcwd(buf, sizeof(buf));
    char *pwd = get_pwd();
    ASSERT(pwd != nullptr);
    ASSERT_STR_EQ(pwd, buf);
    free(pwd);
}

static void test_get_pwd_is_absolute(void)
{
    char *pwd = get_pwd();
    ASSERT(pwd != nullptr);
    ASSERT(pwd[0] == '/');
    free(pwd);
}

static void test_get_path_parts_three_segments(void)
{
    StringList *parts = get_path_parts("/foo/bar/baz");
    ASSERT(parts->count == 3);
    ASSERT_STR_EQ(parts->items[0], "foo");
    ASSERT_STR_EQ(parts->items[1], "bar");
    ASSERT_STR_EQ(parts->items[2], "baz");
}

static void test_get_path_parts_single_segment(void)
{
    StringList *parts = get_path_parts("/foo");
    ASSERT(parts->count == 1);
    ASSERT_STR_EQ(parts->items[0], "foo");
}

static void test_get_path_parts_root(void)
{
    StringList *parts = get_path_parts("/");
    ASSERT(parts->count == 0);
}

static void test_get_path_parts_trailing_slash(void)
{
    StringList *parts = get_path_parts("/foo/bar/");
    ASSERT(parts->count == 2);
    ASSERT_STR_EQ(parts->items[0], "foo");
    ASSERT_STR_EQ(parts->items[1], "bar");
}

static void test_expand_path_tilde(void)
{
    const char *home = getenv("HOME");
    char expected[4096];
    snprintf(expected, sizeof(expected), "%s/projects/", home);
    char *result = expand_path("~/projects");
    ASSERT_STR_EQ(result, expected);
}

static void test_expand_path_dotdot(void)
{
    char *result = expand_path("/foo/bar/../baz");
    ASSERT_STR_EQ(result, "/foo/baz/");
}

static void test_expand_path_dot(void)
{
    char *result = expand_path("/foo/./bar");
    ASSERT_STR_EQ(result, "/foo/bar/");
}

static void test_expand_path_multiple_dotdot(void)
{
    char *result = expand_path("/a/b/c/../../d");
    ASSERT_STR_EQ(result, "/a/d/");
}

static void test_expand_path_dotdot_past_root(void)
{
    // Can't go above root — extra '..' is a no-op
    char *result = expand_path("/foo/../../bar");
    ASSERT_STR_EQ(result, "/bar/");
}

static void test_expand_path_absolute_no_tilde(void)
{
    char *result = expand_path("/usr/local/bin");
    ASSERT_STR_EQ(result, "/usr/local/bin/");
}

static void test_expand_path_file_tilde(void)
{
    const char *home = getenv("HOME");
    char expected[4096];
    snprintf(expected, sizeof(expected), "%s/.env", home);
    char *result = expand_path_file("~/.env");
    ASSERT_STR_EQ(result, expected);
}

static void test_expand_path_file_dotdot(void)
{
    char *result = expand_path_file("/foo/bar/../baz.txt");
    ASSERT_STR_EQ(result, "/foo/baz.txt");
}

static void test_expand_path_relative(void)
{
    char *cwd = get_pwd();
    char expected[4096];
    snprintf(expected, sizeof(expected), "%s/foo/bar/", cwd);
    char *result = expand_path("foo/bar");
    ASSERT_STR_EQ(result, expected);
    free(cwd);
}

static void test_expand_path_file_relative(void)
{
    char *cwd = get_pwd();
    char expected[4096];
    snprintf(expected, sizeof(expected), "%s/foo/bar.txt", cwd);
    char *result = expand_path_file("foo/bar.txt");
    ASSERT_STR_EQ(result, expected);
    free(cwd);
}

static void test_is_directory_with_dir(void)
{
    ASSERT(is_directory("/tmp") == 1);
}

static void test_is_directory_with_file(void)
{
    char *path = write_temp_file("x");
    ASSERT(is_directory(path) == 0);
    unlink(path);
    free(path);
}

static void test_is_directory_nonexistent(void)
{
    ASSERT(is_directory("/tmp/envwalk_no_such_dir_xyz") == 0);
}

void run_path_tests(void)
{
    printf("path:\n");
    SUITE("expand: empty string → cwd");
    test_expand_path_empty_string();
    SUITE("expand: root");
    test_expand_path_root();
    SUITE("expand_file: absolute no tilde");
    test_expand_path_file_absolute();
    SUITE("parts: empty string → zero segments");
    test_get_path_parts_empty_string();
    SUITE("get_pwd: matches getcwd");
    test_get_pwd_matches_getcwd();
    SUITE("get_pwd: is absolute path");
    test_get_pwd_is_absolute();
    SUITE("parts: three segments");
    test_get_path_parts_three_segments();
    SUITE("parts: single segment");
    test_get_path_parts_single_segment();
    SUITE("parts: root");
    test_get_path_parts_root();
    SUITE("parts: trailing slash");
    test_get_path_parts_trailing_slash();
    SUITE("expand: tilde");
    test_expand_path_tilde();
    SUITE("expand: dotdot");
    test_expand_path_dotdot();
    SUITE("expand: dot");
    test_expand_path_dot();
    SUITE("expand: multiple dotdot");
    test_expand_path_multiple_dotdot();
    SUITE("expand: dotdot past root");
    test_expand_path_dotdot_past_root();
    SUITE("expand: absolute");
    test_expand_path_absolute_no_tilde();
    SUITE("expand_file: tilde");
    test_expand_path_file_tilde();
    SUITE("expand_file: dotdot");
    test_expand_path_file_dotdot();
    SUITE("expand: relative");
    test_expand_path_relative();
    SUITE("expand_file: relative");
    test_expand_path_file_relative();

    printf("is_directory:\n");
    SUITE("directory");
    test_is_directory_with_dir();
    SUITE("file");
    test_is_directory_with_file();
    SUITE("nonexistent");
    test_is_directory_nonexistent();
}
