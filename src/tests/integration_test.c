#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "framework.h"

// Path to binary; set by main via ENVWALK_BIN env var or defaults to ./envwalk
static const char *envwalk_bin = "./envwalk";

// Run envwalk with given args in given cwd, with HOME set to fake_home.
// Returns heap-allocated stdout output. Caller frees.
static char *run_envwalk(const char *cwd, const char *fake_home, const char *args)
{
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "cd '%s' && HOME='%s' %s %s 2>/dev/null",
             cwd, fake_home, envwalk_bin, args);

    FILE *f = popen(cmd, "r");
    if (!f) return strdup("");

    char *buf = malloc(4096);
    size_t n = fread(buf, 1, 4095, f);
    buf[n] = '\0';
    pclose(f);
    return buf;
}

// Returns exit code of envwalk
static int run_envwalk_exit(const char *cwd, const char *fake_home, const char *args)
{
    char cmd[4096];
    snprintf(cmd, sizeof(cmd), "cd '%s' && HOME='%s' %s %s >/dev/null 2>&1",
             cwd, fake_home, envwalk_bin, args);
    return system(cmd);
}

static char *integration_setup(const char *config_content)
{
    char *home = strdup("/tmp/envwalk_int_XXXXXX");
    mkdtemp(home);
    char config_dir[4096];
    snprintf(config_dir, sizeof(config_dir), "%s/.config", home);
    mkdir(config_dir, 0755);

    if (config_content != NULL)
    {
        char config_path[4096];
        snprintf(config_path, sizeof(config_path), "%s/.config/envwalk", home);
        int fd = open(config_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        write(fd, config_content, strlen(config_content));
        close(fd);
    }
    return home;
}

static void write_file(const char *path, const char *content)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(fd, content, strlen(content));
    close(fd);
}

// --- run() tests ---

static void test_run_no_allowed_paths(void)
{
    char *home = integration_setup(NULL);
    // config has no allowed paths — run produces no export lines
    char *out = run_envwalk("/tmp", home, "");
    ASSERT(strstr(out, "export") == NULL);
    free(out);
    free(home);
}

static void test_run_exports_allowed_dir(void)
{
    char *home = integration_setup(NULL);

    // Create a temp dir, write a .env, allow it in config
    char dir[] = "/tmp/envwalk_itest_XXXXXX";
    mkdtemp(dir);

    char env_path[4096];
    snprintf(env_path, sizeof(env_path), "%s/.env", dir);
    write_file(env_path, "MYKEY=myvalue\n");

    char config_content[4096];
    snprintf(config_content, sizeof(config_content), "allowed=%s/\n", dir);
    char config_path[4096];
    snprintf(config_path, sizeof(config_path), "%s/.config/envwalk", home);
    write_file(config_path, config_content);

    char *out = run_envwalk(dir, home, "");
    ASSERT(strstr(out, "export MYKEY=\"myvalue\"") != NULL);

    free(out);
    rmdir(dir);
    free(home);
}

static void test_run_tilde_value_expanded(void)
{
    char *home = integration_setup(NULL);

    char dir[] = "/tmp/envwalk_itest_XXXXXX";
    mkdtemp(dir);

    char env_path[4096];
    snprintf(env_path, sizeof(env_path), "%s/.env", dir);
    write_file(env_path, "MYPATH=~/projects\n");

    char config_content[4096];
    snprintf(config_content, sizeof(config_content), "allowed=%s/\n", dir);
    char config_path[4096];
    snprintf(config_path, sizeof(config_path), "%s/.config/envwalk", home);
    write_file(config_path, config_content);

    char *out = run_envwalk(dir, home, "");
    // tilde should be expanded to $HOME/projects
    char expected[4096];
    snprintf(expected, sizeof(expected), "%s/projects", home);
    ASSERT(strstr(out, expected) != NULL);
    ASSERT(strstr(out, "~/projects") == NULL);

    free(out);
    rmdir(dir);
    free(home);
}

static void test_run_parent_dir_vars_loaded(void)
{
    // .env in parent dir is loaded when cwd is a subdirectory
    char *home = integration_setup(NULL);

    char parent[] = "/tmp/envwalk_itest_XXXXXX";
    mkdtemp(parent);

    char child[4096];
    snprintf(child, sizeof(child), "%s/sub", parent);
    mkdir(child, 0755);

    char env_path[4096];
    snprintf(env_path, sizeof(env_path), "%s/.env", parent);
    write_file(env_path, "PARENT_KEY=parentval\n");

    char config_content[4096];
    snprintf(config_content, sizeof(config_content), "allowed=%s/\n", parent);
    char config_path[4096];
    snprintf(config_path, sizeof(config_path), "%s/.config/envwalk", home);
    write_file(config_path, config_content);

    char *out = run_envwalk(child, home, "");
    ASSERT(strstr(out, "export PARENT_KEY=\"parentval\"") != NULL);

    free(out);
    rmdir(child);
    rmdir(parent);
    free(home);
}

// --- chpwd() tests ---

static void test_chpwd_unsets_old_dir_vars(void)
{
    char *home = integration_setup(NULL);

    char dir_a[] = "/tmp/envwalk_itest_XXXXXX";
    mkdtemp(dir_a);
    char dir_b[] = "/tmp/envwalk_itest_XXXXXX";
    mkdtemp(dir_b);

    char env_path_a[4096];
    snprintf(env_path_a, sizeof(env_path_a), "%s/.env", dir_a);
    write_file(env_path_a, "OLD_VAR=oldval\n");

    char config_content[4096];
    snprintf(config_content, sizeof(config_content), "allowed=%s/\n", dir_a);
    char config_path[4096];
    snprintf(config_path, sizeof(config_path), "%s/.config/envwalk", home);
    write_file(config_path, config_content);

    // cd from dir_a to dir_b — OLD_VAR should be unset
    char args[4096];
    snprintf(args, sizeof(args), "cd '%s'", dir_a);
    char *out = run_envwalk(dir_b, home, args);
    ASSERT(strstr(out, "unset OLD_VAR") != NULL);

    free(out);
    rmdir(dir_a);
    rmdir(dir_b);
    free(home);
}

static void test_chpwd_no_unset_when_var_in_new_dir(void)
{
    // If a var exists in both old and new dir's .env, it should NOT be unset
    char *home = integration_setup(NULL);

    char dir_a[] = "/tmp/envwalk_itest_XXXXXX";
    mkdtemp(dir_a);
    char dir_b[] = "/tmp/envwalk_itest_XXXXXX";
    mkdtemp(dir_b);

    char env_path_a[4096];
    snprintf(env_path_a, sizeof(env_path_a), "%s/.env", dir_a);
    write_file(env_path_a, "SHARED_VAR=aval\n");

    char env_path_b[4096];
    snprintf(env_path_b, sizeof(env_path_b), "%s/.env", dir_b);
    write_file(env_path_b, "SHARED_VAR=bval\n");

    char config_content[4096];
    snprintf(config_content, sizeof(config_content), "allowed=%s/\nallowed=%s/\n",
             dir_a, dir_b);
    char config_path[4096];
    snprintf(config_path, sizeof(config_path), "%s/.config/envwalk", home);
    write_file(config_path, config_content);

    char args[4096];
    snprintf(args, sizeof(args), "cd '%s'", dir_a);
    char *out = run_envwalk(dir_b, home, args);
    ASSERT(strstr(out, "unset SHARED_VAR") == NULL);
    ASSERT(strstr(out, "export SHARED_VAR=\"bval\"") != NULL);

    free(out);
    rmdir(dir_a);
    rmdir(dir_b);
    free(home);
}

// --- hook subcommand tests ---

static void test_hook_zsh_exit_ok(void)
{
    char *home = integration_setup(NULL);
    int ret = run_envwalk_exit("/tmp", home, "hook zsh");
    ASSERT(ret == 0);
    free(home);
}

static void test_hook_bash_exit_ok(void)
{
    char *home = integration_setup(NULL);
    int ret = run_envwalk_exit("/tmp", home, "hook bash");
    ASSERT(ret == 0);
    free(home);
}

static void test_hook_unknown_shell_exit_nonzero(void)
{
    char *home = integration_setup(NULL);
    int ret = run_envwalk_exit("/tmp", home, "hook fish");
    ASSERT(ret != 0);
    free(home);
}

static void test_hook_zsh_output_contains_envwalk(void)
{
    char *home = integration_setup(NULL);
    char *out = run_envwalk("/tmp", home, "hook zsh");
    ASSERT(strstr(out, "envwalk") != NULL);
    free(out);
    free(home);
}

void run_integration_tests(void)
{
    const char *bin = getenv("ENVWALK_BIN");
    if (bin != NULL)
        envwalk_bin = bin;

    printf("integration (run):\n");
    SUITE("no allowed paths → no output");
    test_run_no_allowed_paths();
    SUITE("allowed dir exports .env");
    test_run_exports_allowed_dir();
    SUITE("tilde value expanded");
    test_run_tilde_value_expanded();
    SUITE("parent dir vars loaded");
    test_run_parent_dir_vars_loaded();

    printf("integration (chpwd):\n");
    SUITE("unsets vars from old dir");
    test_chpwd_unsets_old_dir_vars();
    SUITE("shared var not unset");
    test_chpwd_no_unset_when_var_in_new_dir();

    printf("integration (hook subcommand):\n");
    SUITE("hook zsh exits 0");
    test_hook_zsh_exit_ok();
    SUITE("hook bash exits 0");
    test_hook_bash_exit_ok();
    SUITE("hook unknown exits nonzero");
    test_hook_unknown_shell_exit_nonzero();
    SUITE("hook zsh output contains envwalk");
    test_hook_zsh_output_contains_envwalk();
}
