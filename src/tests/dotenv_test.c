#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <unistd.h>

#include "dotenv.h"
#include "path.h"
#include "framework.h"

static void test_dotenv_basic(void)
{
    char *dir = write_temp_dotenv_dir("KEY=value\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "KEY");
    ASSERT_SV_EQ(vars.items[0].value, "value");
    free(dir);
}

static void test_dotenv_quoted_value(void)
{
    char *dir = write_temp_dotenv_dir("KEY=\"hello world\"\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].value, "hello world");
    free(dir);
}

static void test_dotenv_comments(void)
{
    char *dir = write_temp_dotenv_dir("# hash comment\n// slash comment\nKEY=value\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "KEY");
    free(dir);
}

static void test_dotenv_empty_lines(void)
{
    char *dir = write_temp_dotenv_dir("\nA=1\n\nB=2\n\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 2);
    free(dir);
}

static void test_dotenv_multiple_keys(void)
{
    char *dir = write_temp_dotenv_dir("A=1\nB=2\nC=3\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 3);
    ASSERT_SV_EQ(vars.items[0].key, "A");
    ASSERT_SV_EQ(vars.items[1].key, "B");
    ASSERT_SV_EQ(vars.items[2].key, "C");
    free(dir);
}

static void test_dotenv_duplicate_keeps_first(void)
{
    int saved = nob_minimal_log_level;
    nob_minimal_log_level = NOB_ERROR;
    char *dir = write_temp_dotenv_dir("KEY=first\nKEY=second\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].value, "first");
    free(dir);
    nob_minimal_log_level = saved;
}

static void test_dotenv_nonexistent_file(void)
{
    nob_set_log_handler(nob_null_log_handler);
    Variables vars = {0};
    ASSERT(!parse_dotenv(&vars, path_from_cstr("/tmp/envwalk_no_such_dir_xyz/")));
    ASSERT(vars.count == 0);
    nob_set_log_handler(nob_default_log_handler);
}

static void test_dotenv_tilde_value_preserved(void)
{
    char *dir = write_temp_dotenv_dir("MYPATH=~/projects\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].value, "~/projects");
    free(dir);
}

static void test_dotenv_empty_file(void)
{
    char *dir = write_temp_dotenv_dir("");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 0);
    free(dir);
}

static void test_dotenv_no_trailing_newline(void)
{
    char *dir = write_temp_dotenv_dir("KEY=value");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "KEY");
    ASSERT_SV_EQ(vars.items[0].value, "value");
    free(dir);
}

static void test_dotenv_value_contains_equals(void)
{
    char *dir = write_temp_dotenv_dir("KEY=foo=bar=baz\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "KEY");
    ASSERT_SV_EQ(vars.items[0].value, "foo=bar=baz");
    free(dir);
}

static void test_dotenv_path_recorded(void)
{
    char *dir = write_temp_dotenv_dir("KEY=value\n");
    char dotenv_path[4096];
    snprintf(dotenv_path, sizeof(dotenv_path), "%s.env", dir);
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].path, dotenv_path);
    free(dir);
}

static void test_dotenv_empty_key(void)
{
    // Line starting with '=' has empty key and non-empty value — stored as-is
    char *dir = write_temp_dotenv_dir("=value\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 1);
    ASSERT(vars.items[0].key.count == 0);
    ASSERT_SV_EQ(vars.items[0].value, "value");
    free(dir);
}

static void test_dotenv_missing_value_skipped(void)
{
    int saved = nob_minimal_log_level;
    nob_minimal_log_level = NOB_ERROR;
    char *dir = write_temp_dotenv_dir("KEYONLY\nA=1\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "A");
    free(dir);
    nob_minimal_log_level = saved;
}

static void test_dotenv_single_quoted_value_not_stripped(void)
{
    // Only double quotes are stripped; single quotes remain in value
    char *dir = write_temp_dotenv_dir("KEY='hello world'\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].value, "'hello world'");
    free(dir);
}

static void test_dotenv_export_prefix_not_stripped(void)
{
    // parse_dotenv does not strip 'export ' prefix;
    // the key becomes "export KEY" and value becomes the rest
    int saved = nob_minimal_log_level;
    nob_minimal_log_level = NOB_ERROR;
    char *dir = write_temp_dotenv_dir("export KEY=value\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "export KEY");
    ASSERT_SV_EQ(vars.items[0].value, "value");
    free(dir);
    nob_minimal_log_level = saved;
}

static void test_dotenv_whitespace_trimmed_from_line(void)
{
    // sv_trim strips leading/trailing whitespace from the whole line
    char *dir = write_temp_dotenv_dir("  KEY=value   \n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "KEY");
    ASSERT_SV_EQ(vars.items[0].value, "value");
    free(dir);
}

static void test_dotenv_key_equals_empty_value_skipped(void)
{
    // "KEY=" has equals but empty value — treated same as missing value, skipped
    int saved = nob_minimal_log_level;
    nob_minimal_log_level = NOB_ERROR;
    char *dir = write_temp_dotenv_dir("KEY=\nA=1\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "A");
    free(dir);
    nob_minimal_log_level = saved;
}

static void test_dotenv_whitespace_only_line_skipped(void)
{
    // Line containing only spaces is trimmed to empty and skipped
    char *dir = write_temp_dotenv_dir("   \nA=1\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir)));
    ASSERT(vars.count == 1);
    ASSERT_SV_EQ(vars.items[0].key, "A");
    free(dir);
}

static void test_dotenv_accumulates_across_calls(void)
{
    // Two parse_dotenv calls into the same Variables accumulate entries
    char *dir1 = write_temp_dotenv_dir("A=1\n");
    char *dir2 = write_temp_dotenv_dir("B=2\n");
    Variables vars = {0};
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir1)));
    ASSERT(parse_dotenv(&vars, path_from_cstr(dir2)));
    ASSERT(vars.count == 2);
    ASSERT_SV_EQ(vars.items[0].key, "A");
    ASSERT_SV_EQ(vars.items[1].key, "B");
    free(dir1);
    free(dir2);
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
