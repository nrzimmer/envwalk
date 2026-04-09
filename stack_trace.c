#include "stack_trace.h"

#define _GNU_SOURCE
#include <execinfo.h>
#include <signal.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>

#define MAX_FRAMES 64

static void crash_handler(int sig, siginfo_t *info, void *ucontext) {
    (void)ucontext;

    void *buffer[MAX_FRAMES];
    int n = backtrace(buffer, MAX_FRAMES);

    pid_t pid = fork();

    if (pid == 0) {
        // ---- CHILD PROCESS ----
        // Now it's (relatively) safe to use stdio, popen, etc.

        fprintf(stderr, "\n=== Crash (signal %d) ===\n", sig);
        fprintf(stderr, "Fault address: %p\n\n", info->si_addr);

        // Build addr2line command
        char cmd[4096];
        int offset = snprintf(cmd, sizeof(cmd),
            "addr2line -e /proc/%d/exe -f -p",
            getppid());  // parent binary

        for (int i = 0; i < n; i++) {
            offset += snprintf(cmd + offset, sizeof(cmd) - offset,
                               " %p", buffer[i]);
        }

        FILE *fp = popen(cmd, "r");
        if (fp) {
            char line[512];
            while (fgets(line, sizeof(line), fp)) {
                fputs(line, stderr);
            }
            pclose(fp);
        } else {
            perror("popen");
        }

        _exit(1);
    }

    // ---- PARENT PROCESS ----
    // Exit immediately (don’t try to continue after crash)
    _exit(1);
}

void setup_handler(void) {
    struct sigaction sa;

    sa.sa_sigaction = crash_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESTART;

    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
    sigaction(SIGFPE,  &sa, NULL);
    sigaction(SIGILL,  &sa, NULL);
}