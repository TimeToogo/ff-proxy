#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <openssl/evp.h>
#include "crypto.h"
#include "constants.h"
#include "logging.h"
#include "alloc.h"

void ff_decrypt_request(struct ff_request *request, struct ff_encryption_key *key)
{
    request->state = FF_REQUEST_STATE_DECRYPTING;

    uint8_t encryption_mode = 0;
    uint8_t *iv = NULL;
    uint16_t iv_len = 0;
    uint8_t *tag = NULL;
    uint16_t tag_len = 0;

    for (uint8_t i = 0; i < request->options_length; i++)
    {
        switch (request->options[i]->type)
        {
        case FF_REQUEST_OPTION_TYPE_ENCRYPTION_MODE:
            if (request->options[i]->length == 1)
            {
                encryption_mode = (uint8_t)*request->options[i]->value;
            };
            break;

        case FF_REQUEST_OPTION_TYPE_ENCRYPTION_IV:
            iv_len = request->options[i]->length;
            iv = (uint8_t *)malloc(iv_len);
            memcpy(iv, request->options[i]->value, iv_len * sizeof(uint8_t));
            break;

        case FF_REQUEST_OPTION_TYPE_ENCRYPTION_TAG:
            tag_len = request->options[i]->length;
            tag = (uint8_t *)malloc(tag_len);
            memcpy(tag, request->options[i]->value, tag_len * sizeof(uint8_t));
            break;

        default:
            break;
        }
    }

    if (encryption_mode == 0)
    {
        goto done;
    }

    if (iv == NULL || iv_len == 0)
    {
        ff_log(FF_WARNING, "Encountered encrypted request with empty IV");
    }

    if (tag == NULL || tag_len == 0)
    {
        ff_log(FF_WARNING, "Encountered encrypted request with empty encryption tag");
    }

    switch (encryption_mode)
    {
    case FF_CRYPTO_MODE_AES_256_GCM:
        if (ff_decrypt_request_aes_256_gcm(request, key, iv, iv_len, tag, tag_len))
        {
            goto done;
        }
        else
        {
            goto error;
        }

    default:
        ff_log(FF_WARNING, "Encountered request with unknown encryption mode: %d", encryption_mode);
        goto error;
    }

error:
    request->state = FF_REQUEST_STATE_DECRYPTING_FAILED;
    goto cleanup;

done:
    request->state = FF_REQUEST_STATE_DECRYPTED;
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

    return;
}

bool ff_decrypt_request_aes_256_gcm(
    struct ff_request *request,
    struct ff_encryption_key *key,
    uint8_t *iv,
    uint16_t iv_len,
    uint8_t *tag,
    uint16_t tag_len)
{
    EVP_CIPHER_CTX *ctx;
    int len;

    uint8_t *plaintext_buff = (uint8_t *)malloc(request->payload_length * sizeof(uint8_t));
    int plaintext_len = 0;
    int ret;
    struct ff_request_payload_node *payload_chunk = request->payload;

    if (!(ctx = EVP_CIPHER_CTX_new()))
    {
        ff_log(FF_ERROR, "Failed to create new OpenSSL cipher");
        return false;
    }

    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
    {
        ff_log(FF_ERROR, "Failed to init OpenSSL cipher");
        return false;
    }

    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
    {
        ff_log(FF_ERROR, "Failed to update OpenSSL cipher - IV length");
        return false;
    }

    if (!EVP_DecryptInit_ex(ctx, NULL, NULL, key->key, iv))
    {
        ff_log(FF_ERROR, "Failed to init OpenSSL cipher with key and IV");
        return false;
    }

    do
    {
        if (!EVP_DecryptUpdate(ctx, plaintext_buff + plaintext_len, &len, payload_chunk->value, payload_chunk->length))
        {
            ff_log(FF_ERROR, "Failed decrypt request payload");
            return false;
        }

        plaintext_len += len;
    } while ((payload_chunk = payload_chunk->next) != NULL);

    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag_len, tag))
    {
        ff_log(FF_ERROR, "Failed to set cipher tag");
        return false;
    }

    ret = EVP_DecryptFinal_ex(ctx, plaintext_buff + plaintext_len, &len);

    EVP_CIPHER_CTX_free(ctx);

    if (ret > 0)
    {
        plaintext_len += len;
        request->payload_length = plaintext_len;

        payload_chunk = request->payload;
        struct ff_request_payload_node *tmp_payload;

        while (payload_chunk != NULL)
        {
            tmp_payload = payload_chunk;
            payload_chunk = payload_chunk->next;
            ff_request_payload_node_free(tmp_payload);
        }

        request->payload = ff_request_payload_node_alloc();
        request->payload->length = plaintext_len;
        request->payload->offset = 0;
        request->payload->next = NULL;
        request->payload->value = plaintext_buff;
        return true;
    }
    else
    {
        ff_log(FF_ERROR, "Failed decrypt and finalize request payload");
        return false;
    }
}