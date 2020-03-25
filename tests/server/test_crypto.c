#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include "../include/unity.h"
#include "../../src/parser.h"
#include "../../src/alloc.h"
#include "../../src/crypto.h"

void test_request_decrypt_with_unencrypted_request_with_key()
{
    struct ff_request *request = ff_request_alloc();
    struct ff_encryption_key key = {.key = (uint8_t *)"testkey"};

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
    struct ff_encryption_key key = {.key = (uint8_t *)"testkey"};

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
    struct ff_encryption_key key = {.key = (uint8_t *)"testkey"};

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
    struct ff_encryption_key key = {.key = (uint8_t *)"testkey"};

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

    struct ff_encryption_key key = {.key = (uint8_t *)"testkey123456789"};

    // Ciphertext for plaintext: "plaintext"
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

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_DECRYPTED, request->state, "state check failed");
    TEST_ASSERT_EQUAL_MESSAGE(9, request->payload_length, "payload length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->payload->offset, "payload node offset check failed");
    TEST_ASSERT_EQUAL_MESSAGE(9, request->payload->length, "payload node length check failed");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("plaintext", request->payload->value, 9, "payload check failed");

    ff_request_free(request);
}

void test_request_decrypt_invalid()
{
    struct ff_request *request = ff_request_alloc();

    struct ff_encryption_key key = {.key = (uint8_t *)"wrongkey"};

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
