#include "include/unity.h"
#include "test_hello_world.c"

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_hello_world);
    return UNITY_END();
}