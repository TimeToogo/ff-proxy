#include <stdbool.h>
#include "parser.h"
#include "request.h"

#ifndef FF_CRYPTO_H
#define FF_CRYPTO_H

enum ff_request_encryption_type
{
    FF_CRYPTO_MODE_AES_256_GCM = 1
};

enum ff_request_key_derivation_type
{
    FF_KEY_DERIVE_MODE_PBKDF2 = 1
};

struct ff_encryption_config
{
    // NULL-terminated key
    uint8_t *key;
    uint32_t pbkdf2_iterations;
};

struct ff_derived_key
{
    uint8_t *key;
    uint16_t length;
};

void ff_decrypt_request(struct ff_request *request, struct ff_encryption_config *config);

struct ff_derived_key *ff_derived_key_alloc(uint16_t length);

void ff_derived_key_free(struct ff_derived_key *);

bool ff_derive_key(
    struct ff_request *request,
    struct ff_encryption_config *config,
    struct ff_derived_key *out_key);

bool ff_derive_key_pbkdf2(
    struct ff_encryption_config *config,
    uint8_t *salt,
    uint16_t salt_length,
    struct ff_derived_key *out_key);
void ff_init_openssl();

#endif