#ifndef ENVWALK_TEST_FRAMEWORK_H
#define ENVWALK_TEST_FRAMEWORK_H

#ifdef _WIN32
#error "envwalk does not support Windows"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "nob.h"

extern int tests_run;
extern int tests_passed;
extern const char *current_suite;

#define ASSERT(expr)                                                            \
    do                                                                          \
    {                                                                           \
        tests_run++;                                                            \
        if (expr)                                                               \
        {                                                                       \
            tests_passed++;                                                     \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            fprintf(stderr, "  FAIL [%s:%d]: %s\n", __FILE__, __LINE__, #expr); \
        }                                                                       \
    } while (0)

#define ASSERT_STR_EQ(a, b)                                                  \
    do                                                                       \
    {                                                                        \
        tests_run++;                                                         \
        const char *_a = (a);                                                \
        const char *_b = (b);                                                \
        if (strcmp(_a, _b) == 0)                                             \
        {                                                                    \
            tests_passed++;                                                  \
        }                                                                    \
        else                                                                 \
        {                                                                    \
            fprintf(stderr, "  FAIL [%s:%d]: expected \"%s\", got \"%s\"\n", \
                    __FILE__, __LINE__, _b, _a);                             \
        }                                                                    \
    } while (0)

#define ASSERT_SV_EQ(sv, cstr)                                                  \
    do                                                                          \
    {                                                                           \
        tests_run++;                                                            \
        String_View _sv = (sv);                                                 \
        const char *_cs = (cstr);                                               \
        if (_sv.count == strlen(_cs) && strncmp(_sv.data, _cs, _sv.count) == 0) \
        {                                                                       \
            tests_passed++;                                                     \
        }                                                                       \
        else                                                                    \
        {                                                                       \
            fprintf(stderr, "  FAIL [%s:%d]: expected \"%s\", got \"%.*s\"\n",  \
                    __FILE__, __LINE__, _cs, (int)_sv.count, _sv.data);         \
        }                                                                       \
    } while (0)

#define SUITE(name)             \
    do                          \
    {                           \
        current_suite = name;   \
        printf("  %s\n", name); \
    } while (0)

static inline char *write_temp_file(const char *content)
{
    char *path = strdup("/tmp/envwalk_test_XXXXXX");
    int fd = mkstemp(path);
    write(fd, content, strlen(content));
    close(fd);
    return path;
}

// Creates a temp dir containing a .env file with content. Returns heap-alloc'd dir path (with trailing slash).
static inline char *write_temp_dotenv_dir(const char *content)
{
    char *dir = strdup("/tmp/envwalk_dotenv_XXXXXX");
    mkdtemp(dir);
    char dotenv_path[4096];
    snprintf(dotenv_path, sizeof(dotenv_path), "%s/.env", dir);
    int fd = open(dotenv_path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    write(fd, content, strlen(content));
    close(fd);
    // return with trailing slash so path_from_cstr resolves as dir
    char *result = malloc(strlen(dir) + 2);
    sprintf(result, "%s/", dir);
    free(dir);
    return result;
}

#endif
