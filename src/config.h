#include <stdio.h>
#include <stdlib.h>
#include "netinet/in.h"

#ifndef FF_CONFIG_H
#define FF_CONFIG_H

struct ff_config
{
    uint16_t port;
    struct in_addr ip_address;
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