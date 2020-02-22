#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "include/unity.h"
#include "../src/config.h"
#include "../src/alloc.h"

void test_parse_args_empty()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_INVALID_ARGS, action, "action check failed");
}

void test_parse_args_help()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"--help"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_PRINT_USAGE, action, "action check failed");
}

void test_parse_args_version()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"--version"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_PRINT_VERSION, action, "action check failed");
}

void test_parse_args_invalid_port()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"--port", "abc"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_INVALID_ARGS, action, "action check failed");
}

void test_parse_args_invalid_ip()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"--ip-address", "12345678"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_INVALID_ARGS, action, "action check failed");
}

void test_parse_args_start_proxy()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"--port", "8080", "--ip-address", "127.0.0.1"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_START_PROXY, action, "action check failed");
    TEST_ASSERT_EQUAL_MESSAGE(8080, config.port, "port check failed");
    TEST_ASSERT_EQUAL_MESSAGE(htonl(INADDR_LOOPBACK), config.ip_address.s_addr, "ip address check failed");
}

void test_print_usage()
{
    ff_print_usage(stdout);
}

void test_print_version()
{
    ff_print_version(stdout);
}
