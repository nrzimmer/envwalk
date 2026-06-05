#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <unistd.h>

#include "dotenv.h"
#include "framework.h"

static void test_dotenv_basic(void)
{
    char *path = write_temp_file("KEY=value\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "KEY");
    ASSERT_SV_EQ(vars.items[0].value, "value");
    unlink(path);
    free(path);
}

static void test_dotenv_quoted_value(void)
{
    char *path = write_temp_file("KEY=\"hello world\"\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].value, "hello world");
    unlink(path);
    free(path);
}

static void test_dotenv_comments(void)
{
    char *path = write_temp_file("# hash comment\n// slash comment\nKEY=value\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "KEY");
    unlink(path);
    free(path);
}

static void test_dotenv_empty_lines(void)
{
    char *path = write_temp_file("\nA=1\n\nB=2\n\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 2);
    unlink(path);
    free(path);
}

static void test_dotenv_multiple_keys(void)
{
    char *path = write_temp_file("A=1\nB=2\nC=3\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 3);
    ASSERT_SV_EQ(vars.items[0].key, "A");
    ASSERT_SV_EQ(vars.items[1].key, "B");
    ASSERT_SV_EQ(vars.items[2].key, "C");
    unlink(path);
    free(path);
}

static void test_dotenv_duplicate_keeps_first(void)
{
    int saved = nob_minimal_log_level;
    nob_minimal_log_level = NOB_ERROR;
    char *path = write_temp_file("KEY=first\nKEY=second\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].value, "first");
    unlink(path);
    free(path);
    nob_minimal_log_level = saved;
}

static void test_dotenv_nonexistent_file(void)
{
    nob_set_log_handler(nob_null_log_handler);
    Variables vars = {0};
    ASSERT(!parse_dotenv(&vars, "/tmp/envwalk_no_such_file_xyz"));
    ASSERT(vars.count == 0);
    nob_set_log_handler(nob_default_log_handler);
}

static void test_dotenv_tilde_value_preserved(void)
{
    char *path = write_temp_file("MYPATH=~/projects\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].value, "~/projects");
    unlink(path);
    free(path);
}

static void test_dotenv_empty_file(void)
{
    char *path = write_temp_file("");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 0);
    unlink(path);
    free(path);
}

static void test_dotenv_no_trailing_newline(void)
{
    char *path = write_temp_file("KEY=value");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "KEY");
    ASSERT_SV_EQ(vars.items[0].value, "value");
    unlink(path);
    free(path);
}

static void test_dotenv_value_contains_equals(void)
{
    char *path = write_temp_file("KEY=foo=bar=baz\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "KEY");
    ASSERT_SV_EQ(vars.items[0].value, "foo=bar=baz");
    unlink(path);
    free(path);
}

static void test_dotenv_path_recorded(void)
{
    char *path = write_temp_file("KEY=value\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].path, path);
    unlink(path);
    free(path);
}

static void test_dotenv_empty_key(void)
{
    // Line starting with '=' has empty key and non-empty value — stored as-is
    char *path = write_temp_file("=value\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT(vars.items[0].key.count == 0);
    ASSERT_SV_EQ(vars.items[0].value, "value");
    unlink(path);
    free(path);
}

static void test_dotenv_missing_value_skipped(void)
{
    int saved = nob_minimal_log_level;
    nob_minimal_log_level = NOB_ERROR;
    char *path = write_temp_file("KEYONLY\nA=1\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "A");
    unlink(path);
    free(path);
    nob_minimal_log_level = saved;
}

static void test_dotenv_single_quoted_value_not_stripped(void)
{
    // Only double quotes are stripped; single quotes remain in value
    char *path = write_temp_file("KEY='hello world'\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].value, "'hello world'");
    unlink(path);
    free(path);
}

static void test_dotenv_export_prefix_not_stripped(void)
{
    // parse_dotenv does not strip 'export ' prefix;
    // the key becomes "export KEY" and value becomes the rest
    int saved = nob_minimal_log_level;
    nob_minimal_log_level = NOB_ERROR;
    char *path = write_temp_file("export KEY=value\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "export KEY");
    ASSERT_SV_EQ(vars.items[0].value, "value");
    unlink(path);
    free(path);
    nob_minimal_log_level = saved;
}

static void test_dotenv_whitespace_trimmed_from_line(void)
{
    // sv_trim strips leading/trailing whitespace from the whole line
    char *path = write_temp_file("  KEY=value   \n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "KEY");
    ASSERT_SV_EQ(vars.items[0].value, "value");
    unlink(path);
    free(path);
}

static void test_dotenv_key_equals_empty_value_skipped(void)
{
    // "KEY=" has equals but empty value — treated same as missing value, skipped
    int saved = nob_minimal_log_level;
    nob_minimal_log_level = NOB_ERROR;
    char *path = write_temp_file("KEY=\nA=1\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "A");
    unlink(path);
    free(path);
    nob_minimal_log_level = saved;
}

static void test_dotenv_whitespace_only_line_skipped(void)
{
    // Line containing only spaces is trimmed to empty and skipped
    char *path = write_temp_file("   \nA=1\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "A");
    unlink(path);
    free(path);
}

static void test_dotenv_accumulates_across_calls(void)
{
    // Two parse_dotenv calls into the same Variables accumulate entries
    char *path1 = write_temp_file("A=1\n");
    char *path2 = write_temp_file("B=2\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path1));
    ASSERT(parse_dotenv(&vars, path2));
    ASSERT(vars.count == 2);
    ASSERT_SV_EQ(vars.items[0].key, "A");
    ASSERT_SV_EQ(vars.items[1].key, "B");
    unlink(path1);
    unlink(path2);
    free(path1);
    free(path2);
}

void run_dotenv_tests(void)
{
    printf("dotenv:\n");
    SUITE("basic key=value");
    test_dotenv_basic();
    SUITE("quoted value");
    test_dotenv_quoted_value();
    SUITE("comments");
    test_dotenv_comments();
    SUITE("empty lines");
    test_dotenv_empty_lines();
    SUITE("multiple keys");
    test_dotenv_multiple_keys();
    SUITE("duplicate keeps first");
    test_dotenv_duplicate_keeps_first();
    SUITE("nonexistent file");
    test_dotenv_nonexistent_file();
    SUITE("tilde value preserved");
    test_dotenv_tilde_value_preserved();
    SUITE("empty file");
    test_dotenv_empty_file();
    SUITE("no trailing newline");
    test_dotenv_no_trailing_newline();
    SUITE("value contains =");
    test_dotenv_value_contains_equals();
    SUITE("path recorded");
    test_dotenv_path_recorded();
    SUITE("empty key stored");
    test_dotenv_empty_key();
    SUITE("missing value skipped");
    test_dotenv_missing_value_skipped();
    SUITE("single-quoted value not stripped");
    test_dotenv_single_quoted_value_not_stripped();
    SUITE("export prefix not stripped");
    test_dotenv_export_prefix_not_stripped();
    SUITE("whitespace trimmed from line");
    test_dotenv_whitespace_trimmed_from_line();
    SUITE("key= empty value skipped");
    test_dotenv_key_equals_empty_value_skipped();
    SUITE("whitespace-only line skipped");
    test_dotenv_whitespace_only_line_skipped();
    SUITE("accumulates across calls");
    test_dotenv_accumulates_across_calls();
}
