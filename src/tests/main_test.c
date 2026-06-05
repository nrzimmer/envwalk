#define _POSIX_C_SOURCE 200809L
#define _XOPEN_SOURCE 700

#include <stdio.h>
#include <stdlib.h>

#define NOB_IMPLEMENTATION
#include "nob.h"
#undef NOB_IMPLEMENTATION

#include "framework.h"

int tests_run = 0;
int tests_passed = 0;
const char *current_suite = "";

void run_dotenv_tests(void);
void run_path_tests(void);
void run_types_tests(void);
void run_cli_tests(void);
void run_config_tests(void);
void run_hooks_tests(void);
void run_integration_tests(void);

int main(void)
{
    nob_minimal_log_level = NOB_ERROR;

    run_dotenv_tests();
    run_path_tests();
    run_types_tests();
    run_cli_tests();
    run_config_tests();
    run_hooks_tests();
    run_integration_tests();

    printf("\n%d/%d passed\n", tests_passed, tests_run);
    return tests_passed == tests_run ? 0 : 1;
}
