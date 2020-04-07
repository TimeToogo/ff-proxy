#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/unity.h"
#include "../../src/config.h"
#include "../../src/logging.h"
#include "../../src/alloc.h"

void test_parse_args_empty()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"ff"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_INVALID_ARGS, action, "action check failed");
}

void test_parse_args_help()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"ff", "--help"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_PRINT_USAGE, action, "action check failed");
}

void test_parse_args_version()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"ff", "--version"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_PRINT_VERSION, action, "action check failed");
}

void test_parse_args_invalid_port()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"ff", "--port", "abc"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_INVALID_ARGS, action, "action check failed");
}

void test_parse_args_invalid_ip()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"ff", "--ip-address", "12345678"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_INVALID_ARGS, action, "action check failed");
}

void test_parse_args_start_proxy()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"ff", "--port", "8080", "--ip-address", "127.0.0.1"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_START_PROXY, action, "action check failed");
    TEST_ASSERT_EQUAL_MESSAGE("8080", config.port, "port check failed");
    TEST_ASSERT_EQUAL_MESSAGE("127.0.0.1", config.ip_address, "ip address check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_ERROR, config.logging_level, "logging level check failed");
}

void test_parse_args_start_proxy_warning()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"ff", "--port", "8080", "--ip-address", "127.0.0.1", "-v"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_START_PROXY, action, "action check failed");
    TEST_ASSERT_EQUAL_MESSAGE("8080", config.port, "port check failed");
    TEST_ASSERT_EQUAL_MESSAGE("127.0.0.1", config.ip_address, "ip address check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_WARNING, config.logging_level, "logging level check failed");
}

void test_parse_args_start_proxy_info()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"ff", "--port", "8080", "--ip-address", "127.0.0.1", "-vv"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_START_PROXY, action, "action check failed");
    TEST_ASSERT_EQUAL_MESSAGE("8080", config.port, "port check failed");
    TEST_ASSERT_EQUAL_MESSAGE("127.0.0.1", config.ip_address, "ip address check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_INFO, config.logging_level, "logging level check failed");
}

void test_parse_args_start_proxy_debug()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"ff", "--port", "8080", "--ip-address", "127.0.0.1", "-vvv"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_START_PROXY, action, "action check failed");
    TEST_ASSERT_EQUAL_MESSAGE("8080", config.port, "port check failed");
    TEST_ASSERT_EQUAL_MESSAGE("127.0.0.1", config.ip_address, "ip address check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_DEBUG, config.logging_level, "logging level check failed");
    TEST_ASSERT_EQUAL_MESSAGE(NULL, config.encryption.key, "encryption key check failed");
}

void test_parse_args_start_proxy_psk()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"ff", "--port", "8080", "--ip-address", "127.0.0.1", "--pre-shared-key", "abc123"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_START_PROXY, action, "action check failed");
    TEST_ASSERT_EQUAL_MESSAGE("8080", config.port, "port check failed");
    TEST_ASSERT_EQUAL_MESSAGE("127.0.0.1", config.ip_address, "ip address check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_ERROR, config.logging_level, "logging level check failed");
    TEST_ASSERT_EQUAL_MESSAGE(args[6], config.encryption.key, "encryption key check failed");
    TEST_ASSERT_EQUAL_MESSAGE(1000, config.encryption.pbkdf2_iterations, "encryption key check failed");
}

void test_parse_args_start_proxy_psk_pbkdf2_iterations()
{
    struct ff_config config;
    enum ff_action action;
    char *args[] = {"ff", "--port", "8080", "--ip-address", "127.0.0.1", "--pkbdf2-iterations", "5000"};

    action = ff_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_ACTION_START_PROXY, action, "action check failed");
    TEST_ASSERT_EQUAL_MESSAGE(5000, config.encryption.pbkdf2_iterations, "encryption key check failed");
}

void test_print_usage()
{
    ff_print_usage(stdout);
}

void test_print_version()
{
    ff_print_version(stdout);
}
