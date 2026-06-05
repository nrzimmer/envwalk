#define _POSIX_C_SOURCE 200809L

#include <string.h>

#include "cli.h"
#include "path.h"
#include "framework.h"

static void test_parse_params_no_args(void)
{
    const char *argv[] = {"envwalk"};
    Params *p = parse_params(1, argv);
    ASSERT(p->action == RUN);
    ASSERT(p->text.count == 0);
    params_free(p);
}

static void test_parse_params_allow_no_path(void)
{
    const char *argv[] = {"envwalk", "allow"};
    Params *p = parse_params(2, argv);
    ASSERT(p->action == ALLOW);
    // cwd expansion for missing path happens in main(), not parse_params()
    ASSERT(p->text.count == 0);
    params_free(p);
}

static void test_parse_params_allow_with_path(void)
{
    const char *argv[] = {"envwalk", "allow", "/tmp"};
    Params *p = parse_params(3, argv);
    ASSERT(p->action == ALLOW);
    String_Builder sb = sb_from_path(p->path, true);
    ASSERT_STR_EQ(sb.data, "/tmp/");
    sb_free(sb);
    params_free(p);
}

static void test_parse_params_deny_with_path(void)
{
    const char *argv[] = {"envwalk", "deny", "/tmp"};
    Params *p = parse_params(3, argv);
    ASSERT(p->action == DENY);
    String_Builder sb = sb_from_path(p->path, true);
    ASSERT_STR_EQ(sb.data, "/tmp/");
    sb_free(sb);
    params_free(p);
}

static void test_parse_params_list(void)
{
    const char *argv[] = {"envwalk", "list"};
    Params *p = parse_params(2, argv);
    ASSERT(p->action == LIST);
    params_free(p);
}

static void test_parse_params_chpwd(void)
{
    const char *argv[] = {"envwalk", "cd", "/old/path"};
    Params *p = parse_params(3, argv);
    ASSERT(p->action == CHPWD);
    String_Builder sb = sb_from_path(p->path, true);
    ASSERT_STR_EQ(sb.data, "/old/path/");
    sb_free(sb);
    params_free(p);
}

static void test_parse_params_hook(void)
{
    const char *argv[] = {"envwalk", "hook", "zsh"};
    Params *p = parse_params(3, argv);
    ASSERT(p->action == HOOK);
    // HOOK text is not path-expanded
    ASSERT_SV_EQ(p->text, "zsh");
    params_free(p);
}

static void test_parse_params_actions_case_insensitive(void)
{
    const char *argv_allow[] = {"envwalk", "ALLOW"};
    Params *p_allow = parse_params(2, argv_allow);
    ASSERT(p_allow->action == ALLOW);
    params_free(p_allow);

    const char *argv_deny[] = {"envwalk", "Deny"};
    Params *p_deny = parse_params(2, argv_deny);
    ASSERT(p_deny->action == DENY);
    params_free(p_deny);

    const char *argv_list[] = {"envwalk", "LIST"};
    Params *p_list = parse_params(2, argv_list);
    ASSERT(p_list->action == LIST);
    params_free(p_list);

    const char *argv_cd[] = {"envwalk", "CD"};
    Params *p_cd = parse_params(2, argv_cd);
    ASSERT(p_cd->action == CHPWD);
    params_free(p_cd);

    const char *argv_hook[] = {"envwalk", "HOOK", "zsh"};
    Params *p_hook = parse_params(3, argv_hook);
    ASSERT(p_hook->action == HOOK);
    params_free(p_hook);
}

static void test_parse_params_quoted_path(void)
{
    // Surrounding quotes are stripped before path expansion
    const char *argv[] = {"envwalk", "allow", "\"/tmp\""};
    Params *p = parse_params(3, argv);
    ASSERT(p->action == ALLOW);
    String_Builder sb = sb_from_path(p->path, true);
    ASSERT_STR_EQ(sb.data, "/tmp/");
    sb_free(sb);
    params_free(p);
}

static void test_parse_params_hook_multi_arg(void)
{
    // Extra args are concatenated with a space for HOOK
    const char *argv[] = {"envwalk", "hook", "bash", "--norc"};
    Params *p = parse_params(4, argv);
    ASSERT(p->action == HOOK);
    ASSERT_SV_EQ(p->text, "bash --norc");
    params_free(p);
}

static void test_parse_params_deny_no_path(void)
{
    // deny with no path: action=DENY, text stays nullptr (main() fills cwd)
    const char *argv[] = {"envwalk", "deny"};
    Params *p = parse_params(2, argv);
    ASSERT(p->action == DENY);
    ASSERT(p->text.count == 0);
    params_free(p);
}

static void test_parse_params_chpwd_no_path(void)
{
    // cd with no path: action=CHPWD, text stays nullptr
    const char *argv[] = {"envwalk", "cd"};
    Params *p = parse_params(2, argv);
    ASSERT(p->action == CHPWD);
    ASSERT(p->text.count == 0);
    params_free(p);
}

static void test_parse_shell_zsh(void)
{
    ASSERT(parse_shell(sv_from_cstr("zsh")) == ZSH);
    ASSERT(parse_shell(sv_from_cstr("ZSH")) == ZSH);
    ASSERT(parse_shell(sv_from_cstr("Zsh")) == ZSH);
}

static void test_parse_shell_bash(void)
{
    ASSERT(parse_shell(sv_from_cstr("bash")) == BASH);
    ASSERT(parse_shell(sv_from_cstr("BASH")) == BASH);
}

static void test_parse_shell_unknown(void)
{
    ASSERT(parse_shell(sv_from_cstr("fish")) == UNKNOWN);
    ASSERT(parse_shell(sv_from_cstr("")) == UNKNOWN);
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
    SUITE("deny no path");
    test_parse_params_deny_no_path();
    SUITE("chpwd no path");
    test_parse_params_chpwd_no_path();

    printf("cli:\n");
    SUITE("shell: zsh");
    test_parse_shell_zsh();
    SUITE("shell: bash");
    test_parse_shell_bash();
    SUITE("shell: unknown");
    test_parse_shell_unknown();
}
