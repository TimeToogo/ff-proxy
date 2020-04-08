#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include "main.h"
#include "config.h"

#define FF_CLIENT_PARSE_ARG_STATE_DEFAULT 0
#define FF_CLIENT_PARSE_ARG_PARSE_PORT 1
#define FF_CLIENT_PARSE_ARG_PARSE_IP 2
#define FF_CLIENT_PARSE_ARG_PARSE_PSK 3
#define FF_CLIENT_PARSE_ARG_PARSE_PBKDF2_ITERATIONS 4

static const char *default_ip_address = "127.0.0.1";

enum ff_client_action ff_client_parse_arguments(struct ff_client_config *config, int argc, char **argv)
{
    enum ff_client_action action = FF_CLIENT_ACTION_MAKE_REQUEST;
    int state = FF_CLIENT_PARSE_ARG_STATE_DEFAULT;
    enum ff_log_type logging_level = FF_ERROR;
    struct ff_encryption_config encryption_config = {.key = NULL, .pbkdf2_iterations = 1000};

    /*
     * Not strictly necessary as config is declared global static,
     * but the tests repeatedly call this function, so ensure it's
     * cleared each time for them.
     */
    memset(config, 0, sizeof(struct ff_config));

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
            else if (strcasecmp(arg, "--pbkdf2-iterations") == 0)
            {
                state = FF_CLIENT_PARSE_ARG_PARSE_PBKDF2_ITERATIONS;
            }
            else if (strcasecmp(arg, "--https") == 0)
            {
                config->https = true;
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
        {
            int port = atoi(arg);

            if (port <= 0 || port > UINT16_MAX)
            {
                fprintf(stderr, "Invalid --port argument: %s\n\n", arg);
                action = FF_CLIENT_ACTION_INVALID_ARGS;
                goto done;
            }

            config->port = arg;
            state = FF_CLIENT_PARSE_ARG_STATE_DEFAULT;
            break;
       }

        case FF_CLIENT_PARSE_ARG_PARSE_IP:
            config->ip_address = arg;
            state = FF_CLIENT_PARSE_ARG_STATE_DEFAULT;
            break;

        case FF_CLIENT_PARSE_ARG_PARSE_PSK:
            encryption_config.key = (uint8_t *)arg;
            state = FF_CLIENT_PARSE_ARG_STATE_DEFAULT;
            break;

        case FF_CLIENT_PARSE_ARG_PARSE_PBKDF2_ITERATIONS:
            sscanf(arg, "%u", &encryption_config.pbkdf2_iterations);

            if (encryption_config.pbkdf2_iterations == 0)
            {
                fprintf(stderr, "Invalid --pbkdf2-iterations argument: %s\n\n", arg);
                action = FF_CLIENT_ACTION_INVALID_ARGS;
                goto done;
            }

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
        if (!config->port)
        {
            fputs("--port is required\n\n", stderr);
            action = FF_CLIENT_ACTION_INVALID_ARGS;
            goto done;
        }

        if (!config->ip_address)
        {
            config->ip_address = default_ip_address;
        }

        config->encryption = encryption_config;
        config->logging_level = logging_level;
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
    [--ip-address dest_server] # format: IPv[46] address or hostname \n\
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
