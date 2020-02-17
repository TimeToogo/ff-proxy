#include <stdbool.h>
#include "parser.h"

#ifndef FF_CRYPTO_H
#define FF_CRYPTO_H

#define FF_CRYPTO_MODE_AES_256_GCM 1

struct ff_encryption_key
{
    uint8_t *key;
};

void ff_decrypt_request(struct ff_request *request, struct ff_encryption_key *key);

bool ff_decrypt_request_aes_256_gcm(
    struct ff_request *request,
    struct ff_encryption_key *key,
    uint8_t *iv,
    uint16_t iv_len,
    uint8_t *tag,
    uint16_t tag_len);

#endif