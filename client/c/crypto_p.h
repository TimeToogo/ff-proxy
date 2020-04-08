#include <stdio.h>
#include <stdbool.h>
#include "../../src/request.h"

#ifndef FF_CLIENT_CRYPTO_P_H
#define FF_CLIENT_CRYPTO_P_H

bool ff_client_encrypt_request_aes_256_gcm_pbkdf2(
    struct ff_request *request,
    struct ff_encryption_config *config,
    uint8_t **iv,
    uint16_t *iv_len,
    uint8_t **tag,
    uint16_t *tag_len,
    uint8_t **salt,
    uint16_t *salt_len);

#endif