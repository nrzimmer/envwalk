#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include "framework.h"

extern const char _binary_hook_zsh_start[];
extern const char _binary_hook_zsh_end[];
extern const char _binary_hook_bash_start[];
extern const char _binary_hook_bash_end[];

static char *hook_to_str(const char *start, const char *end)
{
    return strndup(start, end - start);
}

static bool hook_contains(const char *start, const char *end, const char *needle)
{
    char *s = hook_to_str(start, end);
    bool found = strstr(s, needle) != NULL;
    free(s);
    return found;
}

static int hook_count(const char *start, const char *end, const char *needle)
{
    char *s = hook_to_str(start, end);
    size_t needle_len = strlen(needle);
    int count = 0;
    const char *p = s;
    while ((p = strstr(p, needle)) != NULL)
    {
        count++;
        p += needle_len;
    }
    free(s);
    return count;
}

static char *hook_format(const char *start, const char *end, const char *bin)
{
    char *format = hook_to_str(start, end);
    size_t buf_size = (end - start) + 2 * strlen(bin) + 1;
    char *buf = malloc(buf_size);
    snprintf(buf, buf_size, format, bin, bin);
    free(format);
    return buf;
}

// --- zsh hook tests ---

static void test_hook_zsh_defines_envwalk_exec(void)
{
    ASSERT(hook_contains(_binary_hook_zsh_start, _binary_hook_zsh_end, "envwalk_exec()"));
}

static void test_hook_zsh_defines_envwalk_chpwd(void)
{
    ASSERT(hook_contains(_binary_hook_zsh_start, _binary_hook_zsh_end, "envwalk_chpwd()"));
}

static void test_hook_zsh_skips_cd(void)
{
    ASSERT(hook_contains(_binary_hook_zsh_start, _binary_hook_zsh_end, "skip_cmds=("));
    ASSERT(hook_contains(_binary_hook_zsh_start, _binary_hook_zsh_end, " cd)"));
}

static void test_hook_zsh_skips_zoxide(void)
{
    ASSERT(hook_contains(_binary_hook_zsh_start, _binary_hook_zsh_end, "zoxide"));
}

static void test_hook_zsh_registers_preexec(void)
{
    ASSERT(hook_contains(_binary_hook_zsh_start, _binary_hook_zsh_end,
                         "add-zsh-hook preexec envwalk_exec"));
}

static void test_hook_zsh_registers_chpwd(void)
{
    ASSERT(hook_contains(_binary_hook_zsh_start, _binary_hook_zsh_end,
                         "add-zsh-hook chpwd envwalk_chpwd"));
}

static void test_hook_zsh_unregisters_before_registering(void)
{
    ASSERT(hook_contains(_binary_hook_zsh_start, _binary_hook_zsh_end,
                         "add-zsh-hook -d preexec envwalk_exec"));
    ASSERT(hook_contains(_binary_hook_zsh_start, _binary_hook_zsh_end,
                         "add-zsh-hook -d chpwd envwalk_chpwd"));
}

static void test_hook_zsh_inits_prev_pwd(void)
{
    ASSERT(hook_contains(_binary_hook_zsh_start, _binary_hook_zsh_end,
                         "ENVWALK_PREV_PWD=\"$PWD\""));
}

static void test_hook_zsh_has_two_format_specifiers(void)
{
    ASSERT(hook_count(_binary_hook_zsh_start, _binary_hook_zsh_end, "%s") == 2);
}

static void test_hook_zsh_format_replaces_path(void)
{
    char *out = hook_format(_binary_hook_zsh_start, _binary_hook_zsh_end,
                            "/usr/bin/envwalk");
    ASSERT(strstr(out, "/usr/bin/envwalk") != NULL);
    ASSERT(strstr(out, "%s") == NULL);
    free(out);
}

// --- bash hook tests ---

static void test_hook_bash_defines_preexec(void)
{
    ASSERT(hook_contains(_binary_hook_bash_start, _binary_hook_bash_end,
                         "_envwalk_preexec()"));
}

static void test_hook_bash_defines_chpwd(void)
{
    ASSERT(hook_contains(_binary_hook_bash_start, _binary_hook_bash_end,
                         "_envwalk_chpwd()"));
}

static void test_hook_bash_skips_cd(void)
{
    ASSERT(hook_contains(_binary_hook_bash_start, _binary_hook_bash_end, "cd)"));
}

static void test_hook_bash_skips_zoxide(void)
{
    ASSERT(hook_contains(_binary_hook_bash_start, _binary_hook_bash_end, "zoxide"));
}

static void test_hook_bash_uses_debug_trap(void)
{
    ASSERT(hook_contains(_binary_hook_bash_start, _binary_hook_bash_end,
                         "trap '__envwalk_hook_preexec' DEBUG"));
}

static void test_hook_bash_uses_prompt_command(void)
{
    ASSERT(hook_contains(_binary_hook_bash_start, _binary_hook_bash_end,
                         "PROMPT_COMMAND="));
}

static void test_hook_bash_has_reentrancy_guard(void)
{
    ASSERT(hook_contains(_binary_hook_bash_start, _binary_hook_bash_end,
                         "__envwalk_preexec_running"));
}

static void test_hook_bash_detects_pwd_change(void)
{
    ASSERT(hook_contains(_binary_hook_bash_start, _binary_hook_bash_end,
                         "\"$PWD\" != \"$ENVWALK_PREV_PWD\""));
}

static void test_hook_bash_inits_prev_pwd(void)
{
    ASSERT(hook_contains(_binary_hook_bash_start, _binary_hook_bash_end,
                         "ENVWALK_PREV_PWD=\"$PWD\""));
}

static void test_hook_bash_has_two_format_specifiers(void)
{
    ASSERT(hook_count(_binary_hook_bash_start, _binary_hook_bash_end, "%s") == 2);
}

static void test_hook_bash_format_replaces_path(void)
{
    char *out = hook_format(_binary_hook_bash_start, _binary_hook_bash_end,
                            "/usr/bin/envwalk");
    ASSERT(strstr(out, "/usr/bin/envwalk") != NULL);
    ASSERT(strstr(out, "%s") == NULL);
    free(out);
}

void run_hooks_tests(void)
{
    printf("hook.zsh:\n");
    SUITE("defines envwalk_exec");
    test_hook_zsh_defines_envwalk_exec();
    SUITE("defines envwalk_chpwd");
    test_hook_zsh_defines_envwalk_chpwd();
    SUITE("skips cd");
    test_hook_zsh_skips_cd();
    SUITE("skips zoxide");
    test_hook_zsh_skips_zoxide();
    SUITE("registers preexec");
    test_hook_zsh_registers_preexec();
    SUITE("registers chpwd");
    test_hook_zsh_registers_chpwd();
    SUITE("unregisters before register");
    test_hook_zsh_unregisters_before_registering();
    SUITE("inits ENVWALK_PREV_PWD");
    test_hook_zsh_inits_prev_pwd();
    SUITE("has two %%s specifiers");
    test_hook_zsh_has_two_format_specifiers();
    SUITE("format replaces path");
    test_hook_zsh_format_replaces_path();

    printf("hook.bash:\n");
    SUITE("defines _envwalk_preexec");
    test_hook_bash_defines_preexec();
    SUITE("defines _envwalk_chpwd");
    test_hook_bash_defines_chpwd();
    SUITE("skips cd");
    test_hook_bash_skips_cd();
    SUITE("skips zoxide");
    test_hook_bash_skips_zoxide();
    SUITE("uses DEBUG trap");
    test_hook_bash_uses_debug_trap();
    SUITE("uses PROMPT_COMMAND");
    test_hook_bash_uses_prompt_command();
    SUITE("has re-entrancy guard");
    test_hook_bash_has_reentrancy_guard();
    SUITE("detects PWD change");
    test_hook_bash_detects_pwd_change();
    SUITE("inits ENVWALK_PREV_PWD");
    test_hook_bash_inits_prev_pwd();
    SUITE("has two %%s specifiers");
    test_hook_bash_has_two_format_specifiers();
    SUITE("format replaces path");
    test_hook_bash_format_replaces_path();
}
