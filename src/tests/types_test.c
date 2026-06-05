#define _POSIX_C_SOURCE 200809L

#include "string_utils.h"
#include "framework.h"

static void test_sb_from_string_list_empty(void)
{
    StringList list = {0};
    _cleanup_(sb_cleanup) String_Builder sb = sb_from_string_list(&list);
    // empty list → just the leading "/"
    ASSERT(sb.count == 1);
    ASSERT(sb.items[0] == '/');
}

static void test_sb_from_string_list_single(void)
{
    StringList list = {0};
    da_append(&list, "foo");
    _cleanup_(sb_cleanup) String_Builder sb = sb_from_string_list(&list);
    sb_append_null(&sb);
    ASSERT_STR_EQ(sb.items, "/foo/");
    da_free(list);
}

static void test_sb_from_string_list_multiple(void)
{
    StringList list = {0};
    da_append(&list, "a");
    da_append(&list, "b");
    da_append(&list, "c");
    _cleanup_(sb_cleanup) String_Builder sb = sb_from_string_list(&list);
    sb_append_null(&sb);
    ASSERT_STR_EQ(sb.items, "/a/b/c/");
    da_free(list);
}

void run_types_tests(void)
{
    printf("types:\n");
    SUITE("sb_from_list: empty");
    test_sb_from_string_list_empty();
    SUITE("sb_from_list: single");
    test_sb_from_string_list_single();
    SUITE("sb_from_list: multiple");
    test_sb_from_string_list_multiple();
}
