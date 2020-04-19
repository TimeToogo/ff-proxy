#include <stdio.h>
#include <stdbool.h>
#include "logging.h"
#include "crypto.h"
#include "version.h"

#ifndef FF_CONFIG_H
#define FF_CONFIG_H

struct ff_config
{
    char *port;
    char *ip_address;
    uint16_t timestamp_fudge_factor;
    struct ff_encryption_config encryption;
    enum ff_log_type logging_level;
    bool ipv6_v6only;
};

enum ff_action
{
    FF_ACTION_START_PROXY = 1,
    FF_ACTION_PRINT_VERSION = 2,
    FF_ACTION_PRINT_USAGE = 3,
    FF_ACTION_INVALID_ARGS = 4
};

enum ff_action ff_parse_arguments(struct ff_config *config, int argc, char **argv);

void ff_print_usage(FILE *fd);

void ff_print_version(FILE *fd);

#endif
