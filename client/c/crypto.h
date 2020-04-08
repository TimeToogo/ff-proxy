#include <stdbool.h>
#include "../../src/crypto.h"
#include "request.h"

#ifndef FF_CLIENT_CRYPTO_H
#define FF_CLIENT_CRYPTO_H

bool ff_client_encrypt_request(struct ff_request *request, struct ff_encryption_config *config);

#endif