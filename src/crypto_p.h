#include <stdbool.h>
#include "parser.h"
#include "request.h"

#ifndef FF_CRYPTO_P_H
#define FF_CRYPTO_P_H

bool ff_decrypt_request_aes_256_gcm(
    struct ff_request *request,
    struct ff_encryption_config *config,
    uint8_t *iv,
    uint16_t iv_len,
    uint8_t *tag,
    uint16_t tag_len);

#endif