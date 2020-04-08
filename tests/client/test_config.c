#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/unity.h"
#include "../../client/c/config.h"
#include "../../client/c/logging.h"
#include "../../client/c/alloc.h"

void test_client_parse_args_empty()
{
    struct ff_client_config config;
    enum ff_client_action action;
    char *args[] = {"ff_client"};

    action = ff_client_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_CLIENT_ACTION_INVALID_ARGS, action, "action check failed");
}

void test_client_parse_args_help()
{
    struct ff_client_config config;
    enum ff_client_action action;
    char *args[] = {"ff_client", "--help"};

    action = ff_client_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_CLIENT_ACTION_PRINT_USAGE, action, "action check failed");
}

void test_client_parse_args_version()
{
    struct ff_client_config config;
    enum ff_client_action action;
    char *args[] = {"ff_client", "--version"};

    action = ff_client_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_CLIENT_ACTION_PRINT_VERSION, action, "action check failed");
}

void test_client_parse_args_invalid_port()
{
    struct ff_client_config config;
    enum ff_client_action action;
    char *args[] = {"ff_client", "--port", "abc"};

    action = ff_client_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_CLIENT_ACTION_INVALID_ARGS, action, "action check failed");
}

void test_client_parse_args_invalid_ip()
{
    struct ff_client_config config;
    enum ff_client_action action;
    char *args[] = {"ff_client", "--ip-address", "12345678"};

    action = ff_client_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_CLIENT_ACTION_INVALID_ARGS, action, "action check failed");
}

void test_client_parse_args_make_request()
{
    struct ff_client_config config;
    enum ff_client_action action;
    char *args[] = {"ff_client", "--port", "8080", "--ip-address", "127.0.0.1"};

    action = ff_client_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_CLIENT_ACTION_MAKE_REQUEST, action, "action check failed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("8080", config.port, "port check failed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("127.0.0.1", config.ip_address, "ip address check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_ERROR, config.logging_level, "logging level check failed");
    TEST_ASSERT_EQUAL_MESSAGE(false, config.https, "https check failed");
}

void test_client_parse_args_make_request_warning()
{
    struct ff_client_config config;
    enum ff_client_action action;
    char *args[] = {"ff_client", "--port", "8080", "--ip-address", "127.0.0.1", "-v"};

    action = ff_client_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_CLIENT_ACTION_MAKE_REQUEST, action, "action check failed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("8080", config.port, "port check failed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("127.0.0.1", config.ip_address, "ip address check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_WARNING, config.logging_level, "logging level check failed");
}

void test_client_parse_args_make_request_info()
{
    struct ff_client_config config;
    enum ff_client_action action;
    char *args[] = {"ff_client", "--port", "8080", "--ip-address", "127.0.0.1", "-vv"};

    action = ff_client_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_CLIENT_ACTION_MAKE_REQUEST, action, "action check failed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("8080", config.port, "port check failed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("127.0.0.1", config.ip_address, "ip address check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_INFO, config.logging_level, "logging level check failed");
}

void test_client_parse_args_make_request_debug()
{
    struct ff_client_config config;
    enum ff_client_action action;
    char *args[] = {"ff_client", "--port", "8080", "--ip-address", "127.0.0.1", "-vvv"};

    action = ff_client_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_CLIENT_ACTION_MAKE_REQUEST, action, "action check failed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("8080", config.port, "port check failed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("127.0.0.1", config.ip_address, "ip address check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_DEBUG, config.logging_level, "logging level check failed");
    TEST_ASSERT_EQUAL_MESSAGE(NULL, config.encryption.key, "encryption key check failed");
    TEST_ASSERT_EQUAL_MESSAGE(1000, config.encryption.pbkdf2_iterations, "pbkdf2 iterations check failed");
}

void test_client_parse_args_make_request_psk()
{
    struct ff_client_config config;
    enum ff_client_action action;
    char *args[] = {"ff_client", "--port", "8080", "--ip-address", "127.0.0.1", "--pre-shared-key", "abc123", "--pbkdf2-iterations", "2000"};

    action = ff_client_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_CLIENT_ACTION_MAKE_REQUEST, action, "action check failed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("8080", config.port, "port check failed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("127.0.0.1", config.ip_address, "ip address check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_ERROR, config.logging_level, "logging level check failed");
    TEST_ASSERT_EQUAL_MESSAGE(args[6], config.encryption.key, "encryption key check failed");
    TEST_ASSERT_EQUAL_MESSAGE(2000, config.encryption.pbkdf2_iterations, "pbkdf2 iterations check failed");
}

void test_client_parse_args_make_request_https()
{
    struct ff_client_config config;
    enum ff_client_action action;
    char *args[] = {"ff_client", "--port", "8080", "--https"};

    action = ff_client_parse_arguments(&config, sizeof(args) / sizeof(args[0]), args);

    TEST_ASSERT_EQUAL_MESSAGE(FF_CLIENT_ACTION_MAKE_REQUEST, action, "action check failed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("8080", config.port, "port check failed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("127.0.0.1", config.ip_address, "ip address check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_ERROR, config.logging_level, "logging level check failed");
    TEST_ASSERT_EQUAL_MESSAGE(true, config.https, "https check failed");
}

void test_client_print_usage()
{
    ff_print_usage(stdout);
}

void test_client_print_version()
{
    ff_print_version(stdout);
}
