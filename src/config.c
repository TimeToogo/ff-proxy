#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <arpa/inet.h>
#include "main.h"
#include "config.h"

#define FF_PARSE_ARG_STATE_DEFAULT 0
#define FF_PARSE_ARG_PARSE_PORT 1
#define FF_PARSE_ARG_PARSE_IP 2
#define FF_PARSE_ARG_PARSE_PSK 3

static const char *default_listen_address = "0.0.0.0";

enum ff_action ff_parse_arguments(struct ff_config *config, int argc, char **argv)
{
    enum ff_action action = FF_ACTION_START_PROXY;
    int state = FF_PARSE_ARG_STATE_DEFAULT;
    enum ff_log_type logging_level = FF_ERROR;
    struct ff_encryption_key encryption_key = {.key = NULL};

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
        case FF_PARSE_ARG_STATE_DEFAULT:
            if (strcasecmp(arg, "--help") == 0)
            {
                action = FF_ACTION_PRINT_USAGE;
                goto done;
            }
            else if (strcasecmp(arg, "--version") == 0)
            {
                action = FF_ACTION_PRINT_VERSION;
                goto done;
            }
            else if (strcasecmp(arg, "--port") == 0)
            {
                state = FF_PARSE_ARG_PARSE_PORT;
            }
            else if (strcasecmp(arg, "--ip-address") == 0)
            {
                state = FF_PARSE_ARG_PARSE_IP;
            }
            else if (strcasecmp(arg, "--ipv6-v6only") == 0)
            {
                config->ipv6_v6only = true;
            }
            else if (strcasecmp(arg, "--pre-shared-key") == 0)
            {
                state = FF_PARSE_ARG_PARSE_PSK;
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
                action = FF_ACTION_INVALID_ARGS;
                goto done;
            }
            break;

        case FF_PARSE_ARG_PARSE_PORT:
        {
            int port = atoi(arg);

            if (port <= 0 || port > UINT16_MAX)
            {
                fprintf(stderr, "Invalid --port argument: %s\n\n", arg);
                action = FF_ACTION_INVALID_ARGS;
                goto done;
            }

            config->port = arg;
            state = FF_PARSE_ARG_STATE_DEFAULT;
            break;
        }

        case FF_PARSE_ARG_PARSE_IP:
        {
            int ret;
            unsigned char buf[sizeof(struct in6_addr)];

            if (strchr(arg, ':'))
            {
                ret = inet_pton(AF_INET6, arg, buf);
            }
            else
            {
                ret = inet_pton(AF_INET, arg, buf);
            }

            if (ret < 1)
            {
                fprintf(stderr, "Invalid --ip-address argument: %s\n\n", arg);
                action = FF_ACTION_INVALID_ARGS;
                goto done;
            }

            config->ip_address = arg;
            state = FF_PARSE_ARG_STATE_DEFAULT;
            break;
        }

        case FF_PARSE_ARG_PARSE_PSK:
            encryption_key.key = (uint8_t *)arg;
            state = FF_PARSE_ARG_STATE_DEFAULT;
            break;

        default:
            fputs("Unkown parse arg state\n\n", stderr);
            action = FF_ACTION_INVALID_ARGS;
            goto done;
        }
    }

    if (action == FF_ACTION_START_PROXY)
    {
        if (!config->port)
        {
            fputs("--port is required\n\n", stderr);
            action = FF_ACTION_INVALID_ARGS;
            goto done;
        }

        if (!config->ip_address)
        {
            config->ip_address = default_listen_address;
        }
        config->encryption_key = encryption_key;
        config->logging_level = logging_level;
    }

done:
    return action;
}

void ff_print_usage(FILE *fd)
{
    const char message[] = "\
ff version " FF_VERSION "\n\n\
start proxy: ff\n\
    --port bind_port_num\n\
    [--ip-address bind_ip_address] # format: 0.0.0.0 or 2001:db8::1\n\
    [--ipv6-v6only] # don't accept IPv4 connections on an IPv6 socket\n\
    [--pre-shared-key pre_shared_key]\n\
    -v[vv] \n\
\n\
show version: ff --version\n\
show usage: ff --help\n\
";

    fwrite(message, sizeof(char), sizeof(message), fd);
}

void ff_print_version(FILE *fd)
{
    const char message[] = "\
ff version " FF_VERSION "\n\
";

    fwrite(message, sizeof(char), sizeof(message), fd);
}
