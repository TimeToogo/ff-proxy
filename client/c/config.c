#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "main.h"
#include "config.h"

#define FF_CLIENT_PARSE_ARG_STATE_DEFAULT 0
#define FF_CLIENT_PARSE_ARG_PARSE_PORT 1
#define FF_CLIENT_PARSE_ARG_PARSE_IP 2
#define FF_CLIENT_PARSE_ARG_PARSE_PSK 3

enum ff_client_action ff_client_parse_arguments(struct ff_client_config *config, int argc, char **argv)
{
    enum ff_client_action action = FF_CLIENT_ACTION_MAKE_REQUEST;

    int state = FF_CLIENT_PARSE_ARG_STATE_DEFAULT;

    uint16_t port = 0;
    struct in_addr ip_address = {.s_addr = htonl(INADDR_LOOPBACK)};
    enum ff_log_type logging_level = FF_ERROR;
    struct ff_encryption_key encryption_key = {.key = NULL};
    bool https = false;
    bool parsed_port = false;

    for (int i = 1; i < argc; i++)
    {
        char *arg = argv[i];

        switch (state)
        {
        case FF_CLIENT_PARSE_ARG_STATE_DEFAULT:
            if (strcasecmp(arg, "--help") == 0)
            {
                action = FF_CLIENT_ACTION_PRINT_USAGE;
                goto done;
            }

            else if (strcasecmp(arg, "--version") == 0)
            {
                action = FF_CLIENT_ACTION_PRINT_VERSION;
                goto done;
            }
            else if (strcasecmp(arg, "--port") == 0)
            {
                state = FF_CLIENT_PARSE_ARG_PARSE_PORT;
            }
            else if (strcasecmp(arg, "--ip-address") == 0)
            {
                state = FF_CLIENT_PARSE_ARG_PARSE_IP;
            }
            else if (strcasecmp(arg, "--pre-shared-key") == 0)
            {
                state = FF_CLIENT_PARSE_ARG_PARSE_PSK;
            }
            else if (strcasecmp(arg, "--https") == 0)
            {
                https = true;
            }
            else if (strcasecmp(arg, "-vvv") == 0)
            {
                logging_level = FF_DEBUG;
            }
            else if (strcasecmp(arg, "-vv") == 0)
            {
                logging_level = FF_INFO;
            }
            else if (strcasecmp(arg, "-v") == 0)
            {
                logging_level = FF_WARNING;
            }
            else
            {
                fprintf(stderr, "Unkown argument %s\n\n", arg);
                action = FF_CLIENT_ACTION_INVALID_ARGS;
                goto done;
            }
            break;

        case FF_CLIENT_PARSE_ARG_PARSE_PORT:
            sscanf(arg, "%hu", &port);

            if (port == 0)
            {
                fprintf(stderr, "Invalid --port argument: %s\n\n", arg);
                action = FF_CLIENT_ACTION_INVALID_ARGS;
                goto done;
            }

            parsed_port = true;
            state = FF_CLIENT_PARSE_ARG_STATE_DEFAULT;
            break;

        case FF_CLIENT_PARSE_ARG_PARSE_IP:
            if (inet_pton(AF_INET, arg, &ip_address) != 1)
            {
                fprintf(stderr, "Invalid --ip-address argument: %s\n\n", arg);
                action = FF_CLIENT_ACTION_INVALID_ARGS;
                goto done;
            }

            state = FF_CLIENT_PARSE_ARG_STATE_DEFAULT;
            break;

        case FF_CLIENT_PARSE_ARG_PARSE_PSK:
            encryption_key.key = (uint8_t *)arg;
            state = FF_CLIENT_PARSE_ARG_STATE_DEFAULT;
            break;

        default:
            fputs("Unkown parse arg state\n\n", stderr);
            action = FF_CLIENT_ACTION_INVALID_ARGS;
            goto done;
        }
    }

    if (action == FF_CLIENT_ACTION_MAKE_REQUEST)
    {
        if (!parsed_port)
        {
            fputs("--port is required\n\n", stderr);
            action = FF_CLIENT_ACTION_INVALID_ARGS;
        }

        config->port = port;
        config->ip_address = ip_address;
        config->encryption_key = encryption_key;
        config->logging_level = logging_level;
        config->https = https;
    }

done:
    return action;
}

void ff_client_print_usage(FILE *fd)
{
    const char message[] = "\
ff version " FF_VERSION "\n\n\
make request: ff_client \n\
    --port dest_port_num\n\
    [--ip-address dest_ip_address] # format: 0.0.0.0 \n\
    [--pre-shared-key pre_shared_key] # encrypt the payload \n\
    [--https]\n\
    -v[vv] \n\
    # Request body is read from STDIN\n\
\n\
show version : ff_client --version\n\
show usage : ff_client --help\n\
";

    fwrite(message, sizeof(char), sizeof(message), fd);
}
