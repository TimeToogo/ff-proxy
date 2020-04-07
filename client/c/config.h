#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "../../src/config.h"
#include "logging.h"
#include "crypto.h"
#include "version.h"

#ifndef FF_CLIENT_CONFIG_H
#define FF_CLIENT_CONFIG_H

struct ff_client_config
{
    uint16_t port;
    struct in_addr ip_address;
    bool https;
    struct ff_encryption_config encryption;
    enum ff_log_type logging_level;
};

enum ff_client_action
{
    FF_CLIENT_ACTION_MAKE_REQUEST = 1,
    FF_CLIENT_ACTION_PRINT_VERSION = 2,
    FF_CLIENT_ACTION_PRINT_USAGE = 3,
    FF_CLIENT_ACTION_INVALID_ARGS = 4
};

enum ff_client_action ff_client_parse_arguments(struct ff_client_config *config, int argc, char **argv);

void ff_client_print_usage(FILE *fd);

#endif