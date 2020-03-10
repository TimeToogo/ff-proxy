#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include "crypto.h"
#include "crypto_p.h"
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

    if (key == NULL || key->key == NULL)
    {
        ff_log(FF_WARNING, "Encountered encrypted request and no pre-shared-key was set");
        goto error;
    }

    if (iv == NULL || iv_len == 0)
    {
        ff_log(FF_WARNING, "Encountered encrypted request with empty IV");
        goto error;
    }

    if (tag == NULL || tag_len == 0)
    {
        ff_log(FF_WARNING, "Encountered encrypted request with empty encryption tag");
        goto error;
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
    bool ret_val;

    uint8_t *plaintext_buff = (uint8_t *)malloc(request->payload_length * sizeof(uint8_t));
    int plaintext_len = 0;
    int ret;
    struct ff_request_payload_node *payload_chunk = request->payload;

    unsigned char padded_key[32] = {0};
    memcpy(padded_key, key->key, strlen((char *)key->key));

    if (!(ctx = EVP_CIPHER_CTX_new()))
    {
        ff_log(FF_ERROR, "Failed to create new OpenSSL cipher");
        goto error;
    }

    if (!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
    {
        ff_log(FF_ERROR, "Failed to init OpenSSL cipher");
        goto error;
    }

    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
    {
        ff_log(FF_ERROR, "Failed to update OpenSSL cipher - IV length");
        goto error;
    }

    if (!EVP_DecryptInit_ex(ctx, NULL, NULL, padded_key, iv))
    {
        ff_log(FF_ERROR, "Failed to init OpenSSL cipher with key and IV");
        goto error;
    }

    do
    {
        if (!EVP_DecryptUpdate(ctx, plaintext_buff + plaintext_len, &len, payload_chunk->value, payload_chunk->length))
        {
            ff_log(FF_ERROR, "Failed decrypt request payload");
            goto error;
        }

        plaintext_len += len;

    } while ((payload_chunk = payload_chunk->next) != NULL);

    if (!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, tag_len, tag))
    {
        ff_log(FF_ERROR, "Failed to set cipher tag");
        goto error;
    }

    ret = EVP_DecryptFinal_ex(ctx, plaintext_buff + plaintext_len, &len);

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
        goto done;
    }
    else
    {
        ff_log(FF_ERROR, "Failed decrypt and finalize request payload");
        goto error;
    }

error:
    FREE(plaintext_buff);
    ret_val = false;
    goto cleanup;

done:
    ret_val = true;
    goto cleanup;

cleanup:
    EVP_CIPHER_CTX_free(ctx);
    return ret_val;
}

void ff_init_openssl()
{
    (void)SSL_library_init();

    SSL_load_error_strings();

    OpenSSL_add_all_algorithms();
   
#if OPENSSL_VERSION_NUMBER < 0x10100000L // If OpenSSL < 1.1.0
    OPENSSL_config(NULL);
#endif
}
