#include <stdlib.h>
#include "include/unity.h"
#include "../src/logging.h"

void test_log_debug()
{
    ff_log(FF_DEBUG, "test message %d", 123);
}
