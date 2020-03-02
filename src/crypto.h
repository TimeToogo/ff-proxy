#include <stdbool.h>
#include "parser.h"
#include "request.h"

#ifndef FF_CRYPTO_H
#define FF_CRYPTO_H

enum ff_request_encryption_type
{
    FF_CRYPTO_MODE_AES_256_GCM = 1
};

struct ff_encryption_key
{
    uint8_t *key;
};

void ff_decrypt_request(struct ff_request *request, struct ff_encryption_key *key);

void ff_init_openssl();

#endif