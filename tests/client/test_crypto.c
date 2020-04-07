#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include "../include/unity.h"
#include "../../src/parser.h"
#include "../../src/alloc.h"
#include "../../src/crypto.h"

void test_client_request_encrypt()
{
    struct ff_request *request = ff_request_alloc();
    request->options = malloc(sizeof(struct ff_request_option_node *) * FF_REQUEST_MAX_OPTIONS);
    struct ff_encryption_config config = {.key = (uint8_t *)"testkey", .pbkdf2_iterations = 1000};

    char *payload = "hello world";

    request->payload_length = strlen(payload);
    request->payload = ff_request_payload_node_alloc();
    request->payload->length = strlen(payload);
    ff_request_payload_load_buff(request->payload, strlen(payload), payload);

    bool result = ff_client_encrypt_request(request, &config);

    TEST_ASSERT_EQUAL_MESSAGE(true, result, "return check failed");

    TEST_ASSERT_EQUAL_MESSAGE(5, request->options_length, "options length check failed");

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_OPTION_TYPE_ENCRYPTION_MODE, request->options[0]->type, "option node (1) type check failed");
    TEST_ASSERT_EQUAL_MESSAGE(1, request->options[0]->length, "option node (1) length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_CRYPTO_MODE_AES_256_GCM, request->options[0]->value[0], "option node (1) value check failed");

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_OPTION_TYPE_ENCRYPTION_IV, request->options[1]->type, "option node (2) type check failed");
    TEST_ASSERT_EQUAL_MESSAGE(12, request->options[1]->length, "option node (2) length check failed");

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_OPTION_TYPE_ENCRYPTION_TAG, request->options[2]->type, "option node (3) type check failed");
    TEST_ASSERT_EQUAL_MESSAGE(16, request->options[2]->length, "option node (3) length check failed");

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_OPTION_TYPE_KEY_DERIVE_MODE, request->options[3]->type, "option node (4) type check failed");
    TEST_ASSERT_EQUAL_MESSAGE(1, request->options[3]->length, "option node (4) length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_KEY_DERIVE_MODE_PBKDF2, request->options[3]->value[0], "option node (4) value check failed");

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_OPTION_TYPE_KEY_DERIVE_SALT, request->options[4]->type, "option node (5) type check failed");
    TEST_ASSERT_EQUAL_MESSAGE(16, request->options[4]->length, "option node (5) length check failed");

    ff_request_free(request);
}

void test_client_request_encrypt_without_key()
{
    struct ff_request *request = ff_request_alloc();
    request->options = malloc(sizeof(struct ff_request_option_node *) * FF_REQUEST_MAX_OPTIONS);

    bool result = ff_client_encrypt_request(request, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(false, result, "return check failed");

    ff_request_free(request);
}

void test_client_request_encrypt_and_decrypt_returns_original_payload()
{
    struct ff_request *request = ff_request_alloc();
    request->options = malloc(sizeof(struct ff_request_option_node *) * FF_REQUEST_MAX_OPTIONS);

    struct ff_encryption_config key = {.key = (uint8_t *)"testkey", .pbkdf2_iterations = 1000};

    char *payload = "hello world";

    request->payload_length = strlen(payload);
    request->payload = ff_request_payload_node_alloc();
    request->payload->length = strlen(payload);
    ff_request_payload_load_buff(request->payload, strlen(payload), payload);

    bool result = ff_client_encrypt_request(request, &key);

    TEST_ASSERT_EQUAL_MESSAGE(true, result, "return check failed");

    ff_decrypt_request(request, &key);

    TEST_ASSERT_EQUAL_MESSAGE(strlen(payload), request->payload_length, "payload length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(payload), request->payload->length, "payload node length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->payload->offset, "payload node offset check failed");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(payload, request->payload->value, strlen(payload), "payload node value check failed");

    ff_request_free(request);
}