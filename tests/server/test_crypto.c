#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include "../include/unity.h"
#include "../../src/parser.h"
#include "../../src/alloc.h"
#include "../../src/crypto.h"
#include "../../client/c/crypto.h"

void test_request_decrypt_with_unencrypted_request_with_key()
{
    struct ff_request *request = ff_request_alloc();
    struct ff_encryption_config key = {.key = (uint8_t *)"testkey", .pbkdf2_iterations = 1000};

    ff_decrypt_request(request, &key);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_DECRYPTING_FAILED, request->state, "state check failed");

    ff_request_free(request);
}

void test_request_decrypt_without_key()
{
    struct ff_request *request = ff_request_alloc();

    request->options_length = 1;
    request->options = (struct ff_request_option_node **)malloc(sizeof(struct ff_request_option_node **));
    request->options[0] = ff_request_option_node_alloc();
    request->options[0]->type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_MODE;
    request->options[0]->length = 1;
    request->options[0]->value = (uint8_t *)malloc(1);
    *request->options[0]->value = 9;

    ff_decrypt_request(request, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_DECRYPTING_FAILED, request->state, "state check failed");

    ff_request_free(request);
}

void test_request_decrypt_with_unknown_encryption_mode()
{
    struct ff_request *request = ff_request_alloc();
    struct ff_encryption_config key = {.key = (uint8_t *)"testkey", .pbkdf2_iterations = 1000};

    request->options_length = 1;
    request->options = (struct ff_request_option_node **)malloc(sizeof(struct ff_request_option_node **));
    request->options[0] = ff_request_option_node_alloc();
    request->options[0]->type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_MODE;
    request->options[0]->length = 1;
    request->options[0]->value = (uint8_t *)malloc(1);
    *request->options[0]->value = 9;

    ff_decrypt_request(request, &key);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_DECRYPTING_FAILED, request->state, "state check failed");

    ff_request_free(request);
}

void test_request_decrypt_without_iv()
{
    struct ff_request *request = ff_request_alloc();
    struct ff_encryption_config key = {.key = (uint8_t *)"testkey", .pbkdf2_iterations = 1000};

    request->options_length = 2;
    request->options = (struct ff_request_option_node **)malloc(sizeof(struct ff_request_option_node **) * 2);
    request->options[0] = ff_request_option_node_alloc();
    request->options[0]->type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_MODE;
    request->options[0]->length = 1;
    request->options[0]->value = (uint8_t *)malloc(1);
    *request->options[0]->value = FF_CRYPTO_MODE_AES_256_GCM;

    request->options[1] = ff_request_option_node_alloc();
    request->options[1]->type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_TAG;
    request->options[1]->length = 1;
    request->options[1]->value = (uint8_t *)malloc(1);

    ff_decrypt_request(request, &key);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_DECRYPTING_FAILED, request->state, "state check failed");

    ff_request_free(request);
}

void test_request_decrypt_without_tag()
{
    struct ff_request *request = ff_request_alloc();
    struct ff_encryption_config key = {.key = (uint8_t *)"testkey", .pbkdf2_iterations = 1000};

    request->options_length = 2;
    request->options = (struct ff_request_option_node **)malloc(sizeof(struct ff_request_option_node **) * 2);
    request->options[0] = ff_request_option_node_alloc();
    request->options[0]->type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_MODE;
    request->options[0]->length = 1;
    request->options[0]->value = (uint8_t *)malloc(1);
    *request->options[0]->value = FF_CRYPTO_MODE_AES_256_GCM;

    request->options[1] = ff_request_option_node_alloc();
    request->options[1]->type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_IV;
    request->options[1]->length = 1;
    request->options[1]->value = (uint8_t *)malloc(1);

    ff_decrypt_request(request, &key);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_DECRYPTING_FAILED, request->state, "state check failed");

    ff_request_free(request);
}

void test_request_decrypt_valid()
{
    struct ff_request *request = ff_request_alloc();

    struct ff_encryption_config config = {.key = (uint8_t *)"testkey123456789", .pbkdf2_iterations = 0};

    // Ciphertext for plaintext: "plaintext" (with null byte)
    uint8_t ciphertext[] = {124, 30, 146, 130, 140, 105, 211, 155, 242, 28};
    uint8_t tag[] = {26, 77, 224, 83, 38, 78, 160, 226, 3, 47, 34, 41, 136, 183, 244, 213};
    uint8_t iv[] = "test12345678";
    uint8_t salt[] = "test123456789012";

    request->options_length = 5;
    request->options = (struct ff_request_option_node **)malloc(sizeof(struct ff_request_option_node **) * request->options_length);
    request->options[0] = ff_request_option_node_alloc();
    request->options[0]->type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_MODE;
    request->options[0]->length = 1;
    request->options[0]->value = (uint8_t *)malloc(1);
    *request->options[0]->value = FF_CRYPTO_MODE_AES_256_GCM;

    request->options[1] = ff_request_option_node_alloc();
    request->options[1]->type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_IV;
    request->options[1]->length = 12;
    request->options[1]->value = (uint8_t *)malloc(12);
    memcpy(request->options[1]->value, iv, 12);

    request->options[2] = ff_request_option_node_alloc();
    request->options[2]->type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_TAG;
    request->options[2]->length = 16;
    request->options[2]->value = (uint8_t *)malloc(16);
    memcpy(request->options[2]->value, tag, 16);

    request->options[3] = ff_request_option_node_alloc();
    request->options[3]->type = FF_REQUEST_OPTION_TYPE_KEY_DERIVE_MODE;
    request->options[3]->length = 1;
    request->options[3]->value = (uint8_t *)malloc(1);
    *request->options[3]->value = FF_KEY_DERIVE_MODE_PBKDF2;

    request->options[4] = ff_request_option_node_alloc();
    request->options[4]->type = FF_REQUEST_OPTION_TYPE_KEY_DERIVE_SALT;
    request->options[4]->length = 16;
    request->options[4]->value = (uint8_t *)malloc(16);
    memcpy(request->options[4]->value, salt, 16);

    request->payload_length = sizeof(ciphertext) / sizeof(ciphertext[0]);
    request->payload = ff_request_payload_node_alloc();
    request->payload->length = sizeof(ciphertext) / sizeof(ciphertext[0]);
    request->payload->value = (uint8_t *)malloc(sizeof(ciphertext) / sizeof(ciphertext[0]));
    memcpy(request->payload->value, ciphertext, sizeof(ciphertext) / sizeof(ciphertext[0]));

    ff_decrypt_request(request, &config);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_DECRYPTED, request->state, "state check failed");
    TEST_ASSERT_EQUAL_MESSAGE(10, request->payload_length, "payload length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->payload->offset, "payload node offset check failed");
    TEST_ASSERT_EQUAL_MESSAGE(10, request->payload->length, "payload node length check failed");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("plaintext", request->payload->value, 10, "payload check failed");

    ff_request_free(request);
}

void test_request_decrypt_invalid()
{
    struct ff_request *request = ff_request_alloc();

    struct ff_encryption_config key = {.key = (uint8_t *)"wrongkey", .pbkdf2_iterations = 1000};

    uint8_t ciphertext[] = {43, 97, 119, 68, 163, 127, 145, 239, 110};
    uint8_t tag[] = {92, 174, 9, 6, 224, 156, 40, 64, 186, 192, 160, 218, 192, 139, 27, 3};
    uint8_t iv[] = "test12345678";

    request->options_length = 3;
    request->options = (struct ff_request_option_node **)malloc(sizeof(struct ff_request_option_node **) * 3);
    request->options[0] = ff_request_option_node_alloc();
    request->options[0]->type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_MODE;
    request->options[0]->length = 1;
    request->options[0]->value = (uint8_t *)malloc(1);
    *request->options[0]->value = FF_CRYPTO_MODE_AES_256_GCM;

    request->options[1] = ff_request_option_node_alloc();
    request->options[1]->type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_IV;
    request->options[1]->length = 12;
    request->options[1]->value = (uint8_t *)malloc(12);
    memcpy(request->options[1]->value, iv, 12);

    request->options[2] = ff_request_option_node_alloc();
    request->options[2]->type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_TAG;
    request->options[2]->length = 16;
    request->options[2]->value = (uint8_t *)malloc(16);
    memcpy(request->options[2]->value, tag, 16);

    request->payload_length = sizeof(ciphertext) / sizeof(ciphertext[0]);
    request->payload = ff_request_payload_node_alloc();
    request->payload->length = sizeof(ciphertext) / sizeof(ciphertext[0]);
    request->payload->value = (uint8_t *)malloc(sizeof(ciphertext) / sizeof(ciphertext[0]));
    memcpy(request->payload->value, ciphertext, sizeof(ciphertext) / sizeof(ciphertext[0]));

    ff_decrypt_request(request, &key);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_DECRYPTING_FAILED, request->state, "state check failed");

    ff_request_free(request);
}
