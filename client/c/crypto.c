#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include "crypto.h"
#include "crypto_p.h"
#include "logging.h"
#include "alloc.h"

bool ff_client_encrypt_request(struct ff_request *request, struct ff_encryption_key *key)
{
    uint8_t encryption_mode = FF_CRYPTO_MODE_AES_256_GCM;
    bool ret_val;
    uint8_t *iv = NULL;
    uint16_t iv_len = 0;
    uint8_t *tag = NULL;
    uint16_t tag_len = 0;

    if (key == NULL)
    {
        ff_log(FF_WARNING, "Encountered encrypted request and no pre-shared-key was set");
        goto error;
    }

    if (!ff_client_encrypt_request_aes_256_gcm(request, key, &iv, &iv_len, &tag, &tag_len))
    {
        goto error;
    }

    request->options[request->options_length] = ff_request_option_node_alloc();
    request->options[request->options_length]->type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_MODE;
    request->options[request->options_length]->length = 1;
    ff_request_option_load_buff(request->options[request->options_length], 1, &encryption_mode);
    request->options_length++;

    request->options[request->options_length] = ff_request_option_node_alloc();
    request->options[request->options_length]->type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_IV;
    request->options[request->options_length]->length = iv_len;
    ff_request_option_load_buff(request->options[request->options_length], iv_len, iv);
    request->options_length++;

    request->options[request->options_length] = ff_request_option_node_alloc();
    request->options[request->options_length]->type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_TAG;
    request->options[request->options_length]->length = tag_len;
    ff_request_option_load_buff(request->options[request->options_length], tag_len, tag);
    request->options_length++;

    goto done;

error:
    ret_val = false;
    goto cleanup;

done:
    ret_val = true;
    goto cleanup;

cleanup:
    if (iv != NULL)
    {
        FREE(iv);
    }

    if (tag != NULL)
    {
        FREE(tag);
    }

    return ret_val;
}

bool ff_client_encrypt_request_aes_256_gcm(
    struct ff_request *request,
    struct ff_encryption_key *key,
    uint8_t **iv,
    uint16_t *iv_len,
    uint8_t **tag,
    uint16_t *tag_len)
{
    EVP_CIPHER_CTX *ctx = NULL;
    int len;
    bool ret_val;

    uint8_t *ciphertext_buff = malloc(request->payload_length * sizeof(uint8_t));
    int ciphertext_len = 0;
    struct ff_request_payload_node *payload_chunk = request->payload;

    unsigned char padded_key[32] = {0};
    memcpy(padded_key, key->key, strlen((char *)key->key));

    *iv_len = 12;
    *iv = calloc(1, *iv_len);

    if (!RAND_bytes(*iv, *iv_len))
    {
        ff_log(FF_ERROR, "Failed to generate IV while encrypting request");
        goto error;
    }

    if (!(ctx = EVP_CIPHER_CTX_new()))
    {
        ff_log(FF_ERROR, "Failed to create new OpenSSL cipher");
        goto error;
    }

    if (!EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
    {
        ff_log(FF_ERROR, "Failed to init OpenSSL cipher");
        goto error;
    }

    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, *iv_len, NULL))
    {
        ff_log(FF_ERROR, "Failed to update OpenSSL cipher - IV length");
        goto error;
    }

    if (!EVP_EncryptInit_ex(ctx, NULL, NULL, padded_key, *iv))
    {
        ff_log(FF_ERROR, "Failed to init OpenSSL cipher with key and IV");
        goto error;
    }

    do
    {
        if (!EVP_EncryptUpdate(ctx, ciphertext_buff + ciphertext_len, &len, payload_chunk->value, payload_chunk->length))
        {
            ff_log(FF_ERROR, "Failed encrypt request payload");
            goto error;
        }

        ciphertext_len += len;

    } while ((payload_chunk = payload_chunk->next) != NULL);

    if (!EVP_EncryptFinal_ex(ctx, ciphertext_buff + ciphertext_len, &len))
    {
        ff_log(FF_ERROR, "Failed encrypt and finalize request payload");
        goto error;
    }

    ciphertext_len += len;
    *tag_len = 16;
    *tag = calloc(1, 16);

    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, *tag_len, *tag))
    {
        ff_log(FF_ERROR, "Failed to get cipher tag");
        goto error;
    }

    payload_chunk = request->payload;
    struct ff_request_payload_node *tmp_payload;

    while (payload_chunk != NULL)
    {
        tmp_payload = payload_chunk;
        payload_chunk = payload_chunk->next;
        ff_request_payload_node_free(tmp_payload);
    }

    request->payload = ff_request_payload_node_alloc();
    request->payload->length = ciphertext_len;
    request->payload->offset = 0;
    request->payload->next = NULL;
    request->payload->value = ciphertext_buff;
    request->payload_length = ciphertext_len;
    goto done;

error:
    FREE(ciphertext_buff);
    ret_val = false;
    goto cleanup;

done:
    ret_val = true;
    goto cleanup;

cleanup:
    EVP_CIPHER_CTX_free(ctx);
    return ret_val;
}
