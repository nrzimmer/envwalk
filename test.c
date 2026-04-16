#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define NOB_IMPLEMENTATION
#include "nob.h"
#undef NOB_IMPLEMENTATION

#include <sys/stat.h>

#include "dotenv.h"
#include "path.h"
#include "cli.h"
#include "types.h"
#include "config.h"

// --- Minimal test framework ---

static int tests_run = 0;
static int tests_passed = 0;
static const char *current_suite = "";

#define ASSERT(expr) do { \
    tests_run++; \
    if (expr) { \
        tests_passed++; \
    } else { \
        fprintf(stderr, "  FAIL [%s:%d]: %s\n", __FILE__, __LINE__, #expr); \
    } \
} while(0)

#define ASSERT_STR_EQ(a, b) do { \
    tests_run++; \
    const char *_a = (a); const char *_b = (b); \
    if (strcmp(_a, _b) == 0) { \
        tests_passed++; \
    } else { \
        fprintf(stderr, "  FAIL [%s:%d]: expected \"%s\", got \"%s\"\n", \
                __FILE__, __LINE__, _b, _a); \
    } \
} while(0)

#define ASSERT_SV_EQ(sv, cstr) do { \
    tests_run++; \
    String_View _sv = (sv); const char *_cs = (cstr); \
    if (_sv.count == strlen(_cs) && strncmp(_sv.data, _cs, _sv.count) == 0) { \
        tests_passed++; \
    } else { \
        fprintf(stderr, "  FAIL [%s:%d]: expected \"%s\", got \"%.*s\"\n", \
                __FILE__, __LINE__, _cs, (int)_sv.count, _sv.data); \
    } \
} while(0)

#define SUITE(name) do { current_suite = name; printf("  %s\n", name); } while(0)

// --- Helpers ---

static char *write_temp_file(const char *content) {
    char *path = strdup("/tmp/envwalk_test_XXXXXX");
    int fd = mkstemp(path);
    write(fd, content, strlen(content));
    close(fd);
    return path;
}

// --- dotenv tests ---

static void test_dotenv_basic(void) {
    char *path = write_temp_file("KEY=value\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "KEY");
    ASSERT_SV_EQ(vars.items[0].value, "value");
    unlink(path); free(path);
}

static void test_dotenv_quoted_value(void) {
    char *path = write_temp_file("KEY=\"hello world\"\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].value, "hello world");
    unlink(path); free(path);
}

static void test_dotenv_comments(void) {
    char *path = write_temp_file("# hash comment\n// slash comment\nKEY=value\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "KEY");
    unlink(path); free(path);
}

static void test_dotenv_empty_lines(void) {
    char *path = write_temp_file("\nA=1\n\nB=2\n\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 2);
    unlink(path); free(path);
}

static void test_dotenv_multiple_keys(void) {
    char *path = write_temp_file("A=1\nB=2\nC=3\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 3);
    ASSERT_SV_EQ(vars.items[0].key, "A");
    ASSERT_SV_EQ(vars.items[1].key, "B");
    ASSERT_SV_EQ(vars.items[2].key, "C");
    unlink(path); free(path);
}

static void test_dotenv_duplicate_keeps_first(void) {
    // nob_log warnings go to stderr — suppress during this test
    int saved = nob_minimal_log_level;
    nob_minimal_log_level = NOB_ERROR;
    char *path = write_temp_file("KEY=first\nKEY=second\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].value, "first");
    unlink(path); free(path);
    nob_minimal_log_level = saved;
}

static void test_dotenv_nonexistent_file(void) {
    nob_set_log_handler(nob_nullptr_log_handler);
    Variables vars = {0};
    ASSERT(!parse_dotenv(&vars, "/tmp/envwalk_no_such_file_xyz"));
    ASSERT(vars.count == 0);
    nob_set_log_handler(nob_default_log_handler);
}

static void test_dotenv_tilde_value_preserved(void) {
    // parse_dotenv stores the raw value; ~ expansion happens in the caller
    char *path = write_temp_file("MYPATH=~/projects\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].value, "~/projects");
    unlink(path); free(path);
}

static void test_dotenv_empty_file(void) {
    char *path = write_temp_file("");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 0);
    unlink(path); free(path);
}

static void test_dotenv_no_trailing_newline(void) {
    char *path = write_temp_file("KEY=value");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "KEY");
    ASSERT_SV_EQ(vars.items[0].value, "value");
    unlink(path); free(path);
}

static void test_dotenv_value_contains_equals(void) {
    // Only the first '=' is the separator; rest goes into the value
    char *path = write_temp_file("KEY=foo=bar=baz\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "KEY");
    ASSERT_SV_EQ(vars.items[0].value, "foo=bar=baz");
    unlink(path); free(path);
}

static void test_dotenv_path_recorded(void) {
    char *path = write_temp_file("KEY=value\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    // kv.path should hold the filepath we passed in
    ASSERT_SV_EQ(vars.items[0].path, path);
    unlink(path); free(path);
}

// --- path tests ---

static void test_get_path_parts_three_segments(void) {
    StringList *parts = get_path_parts("/foo/bar/baz");
    ASSERT(parts->count == 3);
    ASSERT_STR_EQ(parts->items[0], "foo");
    ASSERT_STR_EQ(parts->items[1], "bar");
    ASSERT_STR_EQ(parts->items[2], "baz");
}

static void test_get_path_parts_single_segment(void) {
    StringList *parts = get_path_parts("/foo");
    ASSERT(parts->count == 1);
    ASSERT_STR_EQ(parts->items[0], "foo");
}

static void test_get_path_parts_root(void) {
    StringList *parts = get_path_parts("/");
    ASSERT(parts->count == 0);
}

static void test_expand_path_tilde(void) {
    const char *home = getenv("HOME");
    char expected[4096];
    snprintf(expected, sizeof(expected), "%s/projects/", home);
    char *result = expand_path("~/projects");
    ASSERT_STR_EQ(result, expected);
}

static void test_expand_path_dotdot(void) {
    char *result = expand_path("/foo/bar/../baz");
    ASSERT_STR_EQ(result, "/foo/baz/");
}

static void test_expand_path_dot(void) {
    char *result = expand_path("/foo/./bar");
    ASSERT_STR_EQ(result, "/foo/bar/");
}

static void test_expand_path_file_tilde(void) {
    const char *home = getenv("HOME");
    char expected[4096];
    snprintf(expected, sizeof(expected), "%s/.env", home);
    char *result = expand_path_file("~/.env");
    ASSERT_STR_EQ(result, expected);
}

static void test_expand_path_file_dotdot(void) {
    char *result = expand_path_file("/foo/bar/../baz.txt");
    ASSERT_STR_EQ(result, "/foo/baz.txt");
}

static void test_expand_path_relative(void) {
    char *cwd = getcwd(nullptr, 0);
    char expected[4096];
    snprintf(expected, sizeof(expected), "%s/foo/bar/", cwd);
    char *result = expand_path("foo/bar");
    ASSERT_STR_EQ(result, expected);
    free(cwd);
}

static void test_expand_path_multiple_dotdot(void) {
    char *result = expand_path("/a/b/c/../../d");
    ASSERT_STR_EQ(result, "/a/d/");
}

static void test_expand_path_dotdot_past_root(void) {
    // Can't go above root — extra '..' is a no-op
    char *result = expand_path("/foo/../../bar");
    ASSERT_STR_EQ(result, "/bar/");
}

static void test_expand_path_absolute_no_tilde(void) {
    char *result = expand_path("/usr/local/bin");
    ASSERT_STR_EQ(result, "/usr/local/bin/");
}

static void test_get_path_parts_trailing_slash(void) {
    StringList *parts = get_path_parts("/foo/bar/");
    ASSERT(parts->count == 2);
    ASSERT_STR_EQ(parts->items[0], "foo");
    ASSERT_STR_EQ(parts->items[1], "bar");
}

// --- types tests ---

static void test_sb_from_string_list_empty(void) {
    StringList list = {0};
    String_Builder sb = sb_from_string_list(&list);
    // empty list → just the leading "/"
    ASSERT(sb.count == 1);
    ASSERT(sb.items[0] == '/');
}

static void test_sb_from_string_list_single(void) {
    StringList list = {0};
    da_append(&list, "foo");
    String_Builder sb = sb_from_string_list(&list);
    sb_append_nullptr(&sb);
    ASSERT_STR_EQ(sb.items, "/foo/");
}

static void test_sb_from_string_list_multiple(void) {
    StringList list = {0};
    da_append(&list, "a");
    da_append(&list, "b");
    da_append(&list, "c");
    String_Builder sb = sb_from_string_list(&list);
    sb_append_nullptr(&sb);
    ASSERT_STR_EQ(sb.items, "/a/b/c/");
}

// --- is_directory tests ---

static void test_is_directory_with_dir(void) {
    ASSERT(is_directory("/tmp") == 1);
}

static void test_is_directory_with_file(void) {
    char *path = write_temp_file("x");
    ASSERT(is_directory(path) == 0);
    unlink(path); free(path);
}

static void test_is_directory_nonexistent(void) {
    ASSERT(is_directory("/tmp/envwalk_no_such_dir_xyz") == 0);
}

// --- parse_params tests ---

static void test_parse_params_no_args(void) {
    const char *argv[] = {"envwalk"};
    Params *p = parse_params(1, argv);
    ASSERT(p->action == RUN);
    ASSERT(p->text == nullptr);
}

static void test_parse_params_allow_no_path(void) {
    const char *argv[] = {"envwalk", "allow"};
    Params *p = parse_params(2, argv);
    ASSERT(p->action == ALLOW);
    // cwd expansion for missing path happens in main(), not parse_params()
    ASSERT(p->text == nullptr);
}

static void test_parse_params_allow_with_path(void) {
    const char *argv[] = {"envwalk", "allow", "/tmp"};
    Params *p = parse_params(3, argv);
    ASSERT(p->action == ALLOW);
    ASSERT_STR_EQ(p->text, "/tmp/");
}

static void test_parse_params_deny_with_path(void) {
    const char *argv[] = {"envwalk", "deny", "/tmp"};
    Params *p = parse_params(3, argv);
    ASSERT(p->action == DENY);
    ASSERT_STR_EQ(p->text, "/tmp/");
}

static void test_parse_params_list(void) {
    const char *argv[] = {"envwalk", "list"};
    Params *p = parse_params(2, argv);
    ASSERT(p->action == LIST);
}

static void test_parse_params_chpwd(void) {
    const char *argv[] = {"envwalk", "cd", "/old/path"};
    Params *p = parse_params(3, argv);
    ASSERT(p->action == CHPWD);
    ASSERT_STR_EQ(p->text, "/old/path/");
}

static void test_parse_params_hook(void) {
    const char *argv[] = {"envwalk", "hook", "zsh"};
    Params *p = parse_params(3, argv);
    ASSERT(p->action == HOOK);
    // HOOK text is not path-expanded
    ASSERT_STR_EQ(p->text, "zsh");
}

static void test_parse_params_actions_case_insensitive(void) {
    const char *argv_allow[] = {"envwalk", "ALLOW"};
    ASSERT(parse_params(2, argv_allow)->action == ALLOW);

    const char *argv_deny[] = {"envwalk", "Deny"};
    ASSERT(parse_params(2, argv_deny)->action == DENY);

    const char *argv_list[] = {"envwalk", "LIST"};
    ASSERT(parse_params(2, argv_list)->action == LIST);

    const char *argv_cd[] = {"envwalk", "CD"};
    ASSERT(parse_params(2, argv_cd)->action == CHPWD);

    const char *argv_hook[] = {"envwalk", "HOOK", "zsh"};
    ASSERT(parse_params(3, argv_hook)->action == HOOK);
}

static void test_parse_params_quoted_path(void) {
    // Surrounding quotes are stripped before path expansion
    const char *argv[] = {"envwalk", "allow", "\"/tmp\""};
    Params *p = parse_params(3, argv);
    ASSERT(p->action == ALLOW);
    ASSERT_STR_EQ(p->text, "/tmp/");
}

static void test_parse_params_hook_multi_arg(void) {
    // Extra args are concatenated with a space for HOOK
    const char *argv[] = {"envwalk", "hook", "bash", "--norc"};
    Params *p = parse_params(4, argv);
    ASSERT(p->action == HOOK);
    ASSERT_STR_EQ(p->text, "bash --norc");
}

// --- config tests ---

// Creates a temp dir for HOME with a .config/ subdir inside.
// The caller must restore HOME and free the returned pointer.
static char *config_setup(const char *config_content) {
    char *home = strdup("/tmp/envwalk_home_XXXXXX");
    mkdtemp(home);
    char config_dir[4096];
    snprintf(config_dir, sizeof(config_dir), "%s/.config", home);
    mkdir(config_dir, 0755);

    if (config_content != nullptr) {
        char config_path[4096];
        snprintf(config_path, sizeof(config_path), "%s/.config/envwalk", home);
        int fd = open(config_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        write(fd, config_content, strlen(config_content));
        close(fd);
    }

    setenv("HOME", home, 1);
    config_reset_for_testing();
    return home;
}

static void config_teardown(const char *orig_home, char *home) {
    setenv("HOME", orig_home, 1);
    free(home);
}

// Suppress logs, call parse_config(), restore logs.
// Used when the config file is expected to be absent.
static void parse_config_quiet(void) {
    nob_set_log_handler(nob_nullptr_log_handler);
    parse_config();
    nob_set_log_handler(nob_default_log_handler);
}

static void test_config_empty(void) {
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    ASSERT(!is_path_allowed("/tmp/"));
    config_teardown(orig, home);
}

static void test_config_parse_allowed_paths(void) {
    const char *orig = getenv("HOME");
    // Paths stored in config must be in expanded (trailing-slash) form,
    // matching what allow_path() writes via main()'s expand_path().
    char *home = config_setup("allowed=/tmp/\nallowed=/usr/local/\n");
    parse_config();
    ASSERT(is_path_allowed("/tmp/"));
    ASSERT(is_path_allowed("/usr/local/"));
    ASSERT(!is_path_allowed("/home/"));
    config_teardown(orig, home);
}

static void test_config_parse_skips_comments_and_unknowns(void) {
    const char *orig = getenv("HOME");
    char *home = config_setup("# comment\n\nunknown=foo\nallowed=/tmp/\n");
    parse_config();
    ASSERT(is_path_allowed("/tmp/"));
    ASSERT(!is_path_allowed("foo"));
    config_teardown(orig, home);
}

static void test_config_allow_path(void) {
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    ASSERT(!is_path_allowed("/tmp/"));
    allow_path("/tmp/");  // pre-expanded, as main() would pass it
    ASSERT(is_path_allowed("/tmp/"));
    config_teardown(orig, home);
}

static void test_config_allow_path_non_directory(void) {
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    char *f = write_temp_file("x");
    nob_set_log_handler(nob_nullptr_log_handler);
    ASSERT(allow_path(f) == 1);  // must fail — not a directory
    nob_set_log_handler(nob_default_log_handler);
    unlink(f); free(f);
    config_teardown(orig, home);
}

static void test_config_allow_path_duplicate(void) {
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    allow_path("/tmp/");
    allow_path("/tmp/");  // second call should be a no-op

    // Verify by reading back the saved config file
    char config_path[4096];
    snprintf(config_path, sizeof(config_path), "%s/.config/envwalk", home);
    Variables vars = {0};
    parse_dotenv(&vars, config_path);
    size_t count = 0;
    for (size_t i = 0; i < vars.count; ++i) {
        if (sv_eq(vars.items[i].key, sv_from_cstr("allowed"))) count++;
    }
    ASSERT(count == 1);
    config_teardown(orig, home);
}

static void test_config_deny_path(void) {
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    allow_path("/tmp/");
    ASSERT(is_path_allowed("/tmp/"));
    deny_path("/tmp/");
    ASSERT(!is_path_allowed("/tmp/"));
    config_teardown(orig, home);
}

static void test_config_deny_path_not_present(void) {
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    nob_set_log_handler(nob_nullptr_log_handler);
    ASSERT(deny_path("/tmp/") == 0);  // no-op — path was never added
    nob_set_log_handler(nob_default_log_handler);
    config_teardown(orig, home);
}

// --- cli tests ---

static void test_parse_shell_zsh(void) {
    ASSERT(parse_shell("zsh") == ZSH);
    ASSERT(parse_shell("ZSH") == ZSH);
    ASSERT(parse_shell("Zsh") == ZSH);
}

static void test_parse_shell_bash(void) {
    ASSERT(parse_shell("bash") == BASH);
    ASSERT(parse_shell("BASH") == BASH);
}

static void test_parse_shell_unknown(void) {
    ASSERT(parse_shell("fish") == UNKNOWN);
    ASSERT(parse_shell("") == UNKNOWN);
}

// --- main ---

int main(void) {
    nob_minimal_log_level = NOB_ERROR;

    printf("dotenv:\n");
    SUITE("basic key=value");       test_dotenv_basic();
    SUITE("quoted value");          test_dotenv_quoted_value();
    SUITE("comments");              test_dotenv_comments();
    SUITE("empty lines");           test_dotenv_empty_lines();
    SUITE("multiple keys");         test_dotenv_multiple_keys();
    SUITE("duplicate keeps first"); test_dotenv_duplicate_keeps_first();
    SUITE("nonexistent file");      test_dotenv_nonexistent_file();
    SUITE("tilde value preserved"); test_dotenv_tilde_value_preserved();
    SUITE("empty file");            test_dotenv_empty_file();
    SUITE("no trailing newline");   test_dotenv_no_trailing_newline();
    SUITE("value contains =");      test_dotenv_value_contains_equals();
    SUITE("path recorded");         test_dotenv_path_recorded();

    printf("path:\n");
    SUITE("parts: three segments"); test_get_path_parts_three_segments();
    SUITE("parts: single segment"); test_get_path_parts_single_segment();
    SUITE("parts: root");              test_get_path_parts_root();
    SUITE("parts: trailing slash");    test_get_path_parts_trailing_slash();
    SUITE("expand: tilde");            test_expand_path_tilde();
    SUITE("expand: dotdot");           test_expand_path_dotdot();
    SUITE("expand: dot");              test_expand_path_dot();
    SUITE("expand: multiple dotdot");  test_expand_path_multiple_dotdot();
    SUITE("expand: dotdot past root"); test_expand_path_dotdot_past_root();
    SUITE("expand: absolute");         test_expand_path_absolute_no_tilde();
    SUITE("expand_file: tilde");       test_expand_path_file_tilde();
    SUITE("expand_file: dotdot");      test_expand_path_file_dotdot();
    SUITE("expand: relative");         test_expand_path_relative();

    printf("types:\n");
    SUITE("sb_from_list: empty");    test_sb_from_string_list_empty();
    SUITE("sb_from_list: single");   test_sb_from_string_list_single();
    SUITE("sb_from_list: multiple"); test_sb_from_string_list_multiple();

    printf("is_directory:\n");
    SUITE("directory");    test_is_directory_with_dir();
    SUITE("file");         test_is_directory_with_file();
    SUITE("nonexistent");  test_is_directory_nonexistent();

    printf("parse_params:\n");
    SUITE("no args");          test_parse_params_no_args();
    SUITE("allow no path");    test_parse_params_allow_no_path();
    SUITE("allow with path");  test_parse_params_allow_with_path();
    SUITE("deny with path");   test_parse_params_deny_with_path();
    SUITE("list");             test_parse_params_list();
    SUITE("chpwd");            test_parse_params_chpwd();
    SUITE("hook");             test_parse_params_hook();
    SUITE("case insensitive"); test_parse_params_actions_case_insensitive();
    SUITE("quoted path");      test_parse_params_quoted_path();
    SUITE("hook multi-arg");   test_parse_params_hook_multi_arg();

    printf("config:\n");
    SUITE("empty config");               test_config_empty();
    SUITE("parse allowed paths");        test_config_parse_allowed_paths();
    SUITE("skips comments/unknowns");    test_config_parse_skips_comments_and_unknowns();
    SUITE("allow path");                 test_config_allow_path();
    SUITE("allow non-directory fails");  test_config_allow_path_non_directory();
    SUITE("allow duplicate no-op");      test_config_allow_path_duplicate();
    SUITE("deny path");                  test_config_deny_path();
    SUITE("deny missing no-op");         test_config_deny_path_not_present();

    printf("cli:\n");
    SUITE("shell: zsh");            test_parse_shell_zsh();
    SUITE("shell: bash");           test_parse_shell_bash();
    SUITE("shell: unknown");        test_parse_shell_unknown();

    printf("\n%d/%d passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
