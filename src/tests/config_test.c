#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "config.h"
#include "dotenv.h"
#include "path.h"
#include "framework.h"

static char *config_setup(const char *config_content)
{
    char *home = strdup("/tmp/envwalk_home_XXXXXX");
    mkdtemp(home);
    char config_dir[4096];
    snprintf(config_dir, sizeof(config_dir), "%s/.config", home);
    mkdir(config_dir, 0755);

    if (config_content != nullptr)
    {
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

static void config_teardown(const char *orig_home, char *home)
{
    Config_free();
    setenv("HOME", orig_home, 1);
    free(home);
}

// Build a temp Path, query the allowlist, and free it (is_path_allowed does
// not take ownership).
static bool allowed_c(const char *cstr)
{
    Defer(Path) p = path_from_cstr(cstr);
    bool r = is_path_allowed(p);
    return r;
}

// Build a temp Path, run allow/deny, and free it (neither takes ownership).
static int allow_c(const char *cstr)
{
    Defer(Path) p = path_from_cstr(cstr);
    return allow_path(p);
}

static int deny_c(const char *cstr)
{
    Defer(Path) p = path_from_cstr(cstr);
    return deny_path(p);
}

static void parse_config_quiet(void)
{
    nob_set_log_handler(nob_null_log_handler);
    parse_config();
    nob_set_log_handler(nob_default_log_handler);
}

static void test_config_parse_missing_value_line(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup("keyonly\nallowed=/tmp/\n");
    int saved = nob_minimal_log_level;
    nob_minimal_log_level = NOB_ERROR;
    parse_config();
    nob_minimal_log_level = saved;
    ASSERT(allowed_c("/tmp/"));
    config_teardown(orig, home);
}

static void test_config_is_path_allowed_relative(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();

    allow_c(".");

    ASSERT(allowed_c("."));

    config_teardown(orig, home);
}

static void test_config_empty(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    ASSERT(!allowed_c("/tmp/"));
    config_teardown(orig, home);
}

static void test_config_parse_allowed_paths(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup("allowed=/tmp/\nallowed=/usr/local/\n");
    parse_config();
    ASSERT(allowed_c("/tmp/"));
    ASSERT(allowed_c("/usr/local/"));
    ASSERT(!allowed_c("/home/"));
    config_teardown(orig, home);
}

static void test_config_parse_skips_comments_and_unknowns(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup("# comment\n\nunknown=foo\nallowed=/tmp/\n");
    parse_config();
    ASSERT(allowed_c("/tmp/"));
    ASSERT(!allowed_c("foo"));
    config_teardown(orig, home);
}

static void test_config_allow_path(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    ASSERT(!allowed_c("/tmp/"));
    allow_c("/tmp/");
    ASSERT(allowed_c("/tmp/"));
    config_teardown(orig, home);
}

static void test_config_allow_path_non_directory(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    char *f = write_temp_file("x");
    nob_set_log_handler(nob_null_log_handler);
    ASSERT(allow_c(f) == 1); // must fail — not a directory
    nob_set_log_handler(nob_default_log_handler);
    unlink(f);
    free(f);
    config_teardown(orig, home);
}

static void test_config_allow_path_duplicate(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    allow_c("/tmp/");
    allow_c("/tmp/"); // second call should be a no-op

    char config_path[4096];
    snprintf(config_path, sizeof(config_path), "%s/.config/envwalk", home);
    Defer(String_Builder) sb = {0};
    ASSERT(read_entire_file(config_path, &sb));
    sb_append_null(&sb);
    size_t count = 0;
    const char *p = sb.items;
    while ((p = strstr(p, "allowed=")) != NULL) { count++; p++; }
    ASSERT(count == 1);
    config_teardown(orig, home);
}

static void test_config_deny_path(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    allow_c("/tmp/");
    ASSERT(allowed_c("/tmp/"));
    deny_c("/tmp/");
    ASSERT(!allowed_c("/tmp/"));
    config_teardown(orig, home);
}

static void test_config_deny_path_not_present(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    nob_set_log_handler(nob_null_log_handler);
    ASSERT(deny_c("/tmp/") == 0); // no-op — path was never added
    nob_set_log_handler(nob_default_log_handler);
    config_teardown(orig, home);
}

static void test_config_save_config_writes_file(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    allow_c("/tmp/");

    char config_path[4096];
    snprintf(config_path, sizeof(config_path), "%s/.config/envwalk", home);

    Defer(String_Builder) sb = {0};
    ASSERT(read_entire_file(config_path, &sb));
    sb_append_null(&sb);
    ASSERT(strstr(sb.items, "allowed=/tmp/") != NULL);

    config_teardown(orig, home);
}

static void test_config_save_config_removes_denied_path(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    allow_c("/tmp/");
    allow_c("/usr/local/");
    deny_c("/tmp/");

    char config_path[4096];
    snprintf(config_path, sizeof(config_path), "%s/.config/envwalk", home);

    Defer(String_Builder) sb = {0};
    ASSERT(read_entire_file(config_path, &sb));
    sb_append_null(&sb);
    ASSERT(strstr(sb.items, "allowed=/tmp/") == NULL);
    ASSERT(strstr(sb.items, "allowed=/usr/local/") != NULL);

    config_teardown(orig, home);
}

static void test_config_save_config_empty_when_no_paths(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();

    save_config();

    char config_path[4096];
    snprintf(config_path, sizeof(config_path), "%s/.config/envwalk", home);

    Defer(String_Builder) sb = {0};
    ASSERT(read_entire_file(config_path, &sb));
    ASSERT(sb.count == 0);

    config_teardown(orig, home);
}

static void test_config_save_config_persists_across_reload(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    allow_c("/tmp/");
    allow_c("/usr/local/");

    config_reset_for_testing();
    parse_config();

    ASSERT(allowed_c("/tmp/"));
    ASSERT(allowed_c("/usr/local/"));

    config_teardown(orig, home);
}

static void test_config_is_path_allowed_sb(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    allow_c("/tmp/");

    Defer(String_Builder) sb = {0};
    sb_append_cstr(&sb, "/tmp/");

    Defer(Path) p1 = path_from_sb(sb);
    ASSERT(is_path_allowed(p1));

    sb.count = 0;
    sb_append_cstr(&sb, "/nonexistent_xyz/");
    Defer(Path) p2 = path_from_sb(sb);
    ASSERT(!is_path_allowed(p2));

    config_teardown(orig, home);
}

static void test_config_list_paths_output(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup("allowed=/tmp/\nallowed=/usr/local/\n");
    parse_config();

    char tmp[] = "/tmp/envwalk_list_XXXXXX";
    int tmpfd = mkstemp(tmp);

    int saved_stdout = dup(STDOUT_FILENO);
    fflush(stdout);
    dup2(tmpfd, STDOUT_FILENO);
    close(tmpfd);

    list_paths();
    fflush(stdout);

    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);

    int fd = open(tmp, O_RDONLY);
    char buf[1024] = {0};
    read(fd, buf, sizeof(buf) - 1);
    close(fd);
    unlink(tmp);

    ASSERT(strstr(buf, "/tmp/") != NULL);
    ASSERT(strstr(buf, "/usr/local/") != NULL);

    config_teardown(orig, home);
}

static void test_config_list_paths_empty(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();

    char tmp[] = "/tmp/envwalk_list_empty_XXXXXX";
    int tmpfd = mkstemp(tmp);

    int saved_stdout = dup(STDOUT_FILENO);
    fflush(stdout);
    dup2(tmpfd, STDOUT_FILENO);
    close(tmpfd);

    list_paths();
    fflush(stdout);

    dup2(saved_stdout, STDOUT_FILENO);
    close(saved_stdout);

    int fd = open(tmp, O_RDONLY);
    char buf[1024] = {0};
    read(fd, buf, sizeof(buf) - 1);
    close(fd);
    unlink(tmp);

    // header is always printed; no path lines
    ASSERT(strstr(buf, "List of paths") != NULL);
    ASSERT(strstr(buf, "- ") == NULL);

    config_teardown(orig, home);
}

static void test_config_deny_removes_only_target(void)
{
    const char *orig = getenv("HOME");
    char *home = config_setup(nullptr);
    parse_config_quiet();
    allow_c("/tmp/");
    allow_c("/usr/local/");
    deny_c("/tmp/");
    ASSERT(!allowed_c("/tmp/"));
    ASSERT(allowed_c("/usr/local/"));
    config_teardown(orig, home);
}

void run_config_tests(void)
{
    printf("config:\n");
    SUITE("parse: missing value line skipped");
    test_config_parse_missing_value_line();
    SUITE("is_path_allowed: relative path");
    test_config_is_path_allowed_relative();
    SUITE("empty config");
    test_config_empty();
    SUITE("parse allowed paths");
    test_config_parse_allowed_paths();
    SUITE("skips comments/unknowns");
    test_config_parse_skips_comments_and_unknowns();
    SUITE("allow path");
    test_config_allow_path();
    SUITE("allow non-directory fails");
    test_config_allow_path_non_directory();
    SUITE("allow duplicate no-op");
    test_config_allow_path_duplicate();
    SUITE("deny path");
    test_config_deny_path();
    SUITE("deny missing no-op");
    test_config_deny_path_not_present();
    SUITE("save_config writes file");
    test_config_save_config_writes_file();
    SUITE("save_config removes denied path");
    test_config_save_config_removes_denied_path();
    SUITE("save_config empty when no paths");
    test_config_save_config_empty_when_no_paths();
    SUITE("save_config persists across reload");
    test_config_save_config_persists_across_reload();
    SUITE("is_path_allowed_sb");
    test_config_is_path_allowed_sb();
    SUITE("list_paths output");
    test_config_list_paths_output();
    SUITE("list_paths empty");
    test_config_list_paths_empty();
    SUITE("deny removes only target");
    test_config_deny_removes_only_target();
}
