#include "include/unity.h"
#include "test_hello_world.c"
#include "test_parser.c"

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_hello_world);
    RUN_TEST(test_request_header_alloc);
    RUN_TEST(test_request_header_free);
    RUN_TEST(test_request_alloc);
    RUN_TEST(test_request_free);
    return UNITY_END();
}