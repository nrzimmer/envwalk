#define _POSIX_C_SOURCE 200809L

#include <string.h>

#include "cli.h"
#include "path.h"
#include "framework.h"

static void test_parse_params_no_args(void)
{
    const char *argv[] = {"envwalk"};
    _cleanup_(params_cleanup) Params *p = parse_params(1, argv);
    ASSERT(p->action == RUN);
    ASSERT(p->text.count == 0);
}

static void test_parse_params_allow_no_path(void)
{
    const char *argv[] = {"envwalk", "allow"};
    _cleanup_(params_cleanup) Params *p = parse_params(2, argv);
    ASSERT(p->action == ALLOW);
    // cwd expansion for missing path happens in main(), not parse_params()
    ASSERT(p->text.count == 0);
}

static void test_parse_params_allow_with_path(void)
{
    const char *argv[] = {"envwalk", "allow", "/tmp"};
    _cleanup_(params_cleanup) Params *p = parse_params(3, argv);
    ASSERT(p->action == ALLOW);
    _cleanup_(sb_cleanup) String_Builder sb = sb_from_path(p->path, true);
    ASSERT_STR_EQ(sb.data, "/tmp/");
}

static void test_parse_params_deny_with_path(void)
{
    const char *argv[] = {"envwalk", "deny", "/tmp"};
    _cleanup_(params_cleanup) Params *p = parse_params(3, argv);
    ASSERT(p->action == DENY);
    _cleanup_(sb_cleanup) String_Builder sb = sb_from_path(p->path, true);
    ASSERT_STR_EQ(sb.data, "/tmp/");
}

static void test_parse_params_list(void)
{
    const char *argv[] = {"envwalk", "list"};
    _cleanup_(params_cleanup) Params *p = parse_params(2, argv);
    ASSERT(p->action == LIST);
}

static void test_parse_params_chpwd(void)
{
    const char *argv[] = {"envwalk", "cd", "/old/path"};
    _cleanup_(params_cleanup) Params *p = parse_params(3, argv);
    ASSERT(p->action == CHPWD);
    _cleanup_(sb_cleanup) String_Builder sb = sb_from_path(p->path, true);
    ASSERT_STR_EQ(sb.data, "/old/path/");
}

static void test_parse_params_hook(void)
{
    const char *argv[] = {"envwalk", "hook", "zsh"};
    _cleanup_(params_cleanup) Params *p = parse_params(3, argv);
    ASSERT(p->action == HOOK);
    // HOOK text is not path-expanded
    ASSERT_SV_EQ(p->text, "zsh");
}

static void test_parse_params_actions_case_insensitive(void)
{
    const char *argv_allow[] = {"envwalk", "ALLOW"};
    _cleanup_(params_cleanup) Params *p_allow = parse_params(2, argv_allow);
    ASSERT(p_allow->action == ALLOW);

    const char *argv_deny[] = {"envwalk", "Deny"};
    _cleanup_(params_cleanup) Params *p_deny = parse_params(2, argv_deny);
    ASSERT(p_deny->action == DENY);

    const char *argv_list[] = {"envwalk", "LIST"};
    _cleanup_(params_cleanup) Params *p_list = parse_params(2, argv_list);
    ASSERT(p_list->action == LIST);

    const char *argv_cd[] = {"envwalk", "CD"};
    _cleanup_(params_cleanup) Params *p_cd = parse_params(2, argv_cd);
    ASSERT(p_cd->action == CHPWD);

    const char *argv_hook[] = {"envwalk", "HOOK", "zsh"};
    _cleanup_(params_cleanup) Params *p_hook = parse_params(3, argv_hook);
    ASSERT(p_hook->action == HOOK);
}

static void test_parse_params_quoted_path(void)
{
    // Surrounding quotes are stripped before path expansion
    const char *argv[] = {"envwalk", "allow", "\"/tmp\""};
    _cleanup_(params_cleanup) Params *p = parse_params(3, argv);
    ASSERT(p->action == ALLOW);
    _cleanup_(sb_cleanup) String_Builder sb = sb_from_path(p->path, true);
    ASSERT_STR_EQ(sb.data, "/tmp/");
}

static void test_parse_params_hook_multi_arg(void)
{
    // Extra args are concatenated with a space for HOOK
    const char *argv[] = {"envwalk", "hook", "bash", "--norc"};
    _cleanup_(params_cleanup) Params *p = parse_params(4, argv);
    ASSERT(p->action == HOOK);
    ASSERT_SV_EQ(p->text, "bash --norc");
}

static void test_parse_params_deny_no_path(void)
{
    // deny with no path: action=DENY, text stays nullptr (main() fills cwd)
    const char *argv[] = {"envwalk", "deny"};
    _cleanup_(params_cleanup) Params *p = parse_params(2, argv);
    ASSERT(p->action == DENY);
    ASSERT(p->text.count == 0);
}

static void test_parse_params_chpwd_no_path(void)
{
    // cd with no path: action=CHPWD, text stays nullptr
    const char *argv[] = {"envwalk", "cd"};
    _cleanup_(params_cleanup) Params *p = parse_params(2, argv);
    ASSERT(p->action == CHPWD);
    ASSERT(p->text.count == 0);
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
