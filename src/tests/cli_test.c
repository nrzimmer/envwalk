#define _POSIX_C_SOURCE 200809L

#include <string.h>

#include "cli.h"
#include "framework.h"

static void test_parse_params_no_args(void)
{
    const char *argv[] = {"envwalk"};
    Params *p = parse_params(1, argv);
    ASSERT(p->action == RUN);
    ASSERT(p->text == nullptr);
}

static void test_parse_params_allow_no_path(void)
{
    const char *argv[] = {"envwalk", "allow"};
    Params *p = parse_params(2, argv);
    ASSERT(p->action == ALLOW);
    // cwd expansion for missing path happens in main(), not parse_params()
    ASSERT(p->text == nullptr);
}

static void test_parse_params_allow_with_path(void)
{
    const char *argv[] = {"envwalk", "allow", "/tmp"};
    Params *p = parse_params(3, argv);
    ASSERT(p->action == ALLOW);
    ASSERT_STR_EQ(p->text, "/tmp/");
}

static void test_parse_params_deny_with_path(void)
{
    const char *argv[] = {"envwalk", "deny", "/tmp"};
    Params *p = parse_params(3, argv);
    ASSERT(p->action == DENY);
    ASSERT_STR_EQ(p->text, "/tmp/");
}

static void test_parse_params_list(void)
{
    const char *argv[] = {"envwalk", "list"};
    Params *p = parse_params(2, argv);
    ASSERT(p->action == LIST);
}

static void test_parse_params_chpwd(void)
{
    const char *argv[] = {"envwalk", "cd", "/old/path"};
    Params *p = parse_params(3, argv);
    ASSERT(p->action == CHPWD);
    ASSERT_STR_EQ(p->text, "/old/path/");
}

static void test_parse_params_hook(void)
{
    const char *argv[] = {"envwalk", "hook", "zsh"};
    Params *p = parse_params(3, argv);
    ASSERT(p->action == HOOK);
    // HOOK text is not path-expanded
    ASSERT_STR_EQ(p->text, "zsh");
}

static void test_parse_params_actions_case_insensitive(void)
{
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

static void test_parse_params_quoted_path(void)
{
    // Surrounding quotes are stripped before path expansion
    const char *argv[] = {"envwalk", "allow", "\"/tmp\""};
    Params *p = parse_params(3, argv);
    ASSERT(p->action == ALLOW);
    ASSERT_STR_EQ(p->text, "/tmp/");
}

static void test_parse_params_hook_multi_arg(void)
{
    // Extra args are concatenated with a space for HOOK
    const char *argv[] = {"envwalk", "hook", "bash", "--norc"};
    Params *p = parse_params(4, argv);
    ASSERT(p->action == HOOK);
    ASSERT_STR_EQ(p->text, "bash --norc");
}

static void test_parse_shell_zsh(void)
{
    ASSERT(parse_shell("zsh") == ZSH);
    ASSERT(parse_shell("ZSH") == ZSH);
    ASSERT(parse_shell("Zsh") == ZSH);
}

static void test_parse_shell_bash(void)
{
    ASSERT(parse_shell("bash") == BASH);
    ASSERT(parse_shell("BASH") == BASH);
}

static void test_parse_shell_unknown(void)
{
    ASSERT(parse_shell("fish") == UNKNOWN);
    ASSERT(parse_shell("") == UNKNOWN);
}

void run_cli_tests(void)
{
    printf("parse_params:\n");
    SUITE("no args");
    test_parse_params_no_args();
    SUITE("allow no path");
    test_parse_params_allow_no_path();
    SUITE("allow with path");
    test_parse_params_allow_with_path();
    SUITE("deny with path");
    test_parse_params_deny_with_path();
    SUITE("list");
    test_parse_params_list();
    SUITE("chpwd");
    test_parse_params_chpwd();
    SUITE("hook");
    test_parse_params_hook();
    SUITE("case insensitive");
    test_parse_params_actions_case_insensitive();
    SUITE("quoted path");
    test_parse_params_quoted_path();
    SUITE("hook multi-arg");
    test_parse_params_hook_multi_arg();

    printf("cli:\n");
    SUITE("shell: zsh");
    test_parse_shell_zsh();
    SUITE("shell: bash");
    test_parse_shell_bash();
    SUITE("shell: unknown");
    test_parse_shell_unknown();
}
