#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <arpa/inet.h>
#include "main.h"
#include "config.h"

#define FF_PARSE_ARG_STATE_DEFAULT 0
#define FF_PARSE_ARG_PARSE_PORT 1
#define FF_PARSE_ARG_PARSE_IP 2
#define FF_PARSE_ARG_PARSE_PSK 3
#define FF_PARSE_ARG_PARSE_PBKDF2_ITERATIONS 4
#define FF_PARSE_ARG_PARSE_TIMESTAMP_FUDGE_FACTOR 5

static char *default_listen_address = "0.0.0.0";

enum ff_action ff_parse_arguments(struct ff_config *config, int argc, char **argv)
{
    // Default values
    char *port = NULL;
    char *listen_address = default_listen_address;
    enum ff_action action = FF_ACTION_START_PROXY;
    int state = FF_PARSE_ARG_STATE_DEFAULT;
    enum ff_log_type logging_level = FF_ERROR;
    struct ff_encryption_config encryption_config = {
        .key = NULL,
        .pbkdf2_iterations = 1000};
    uint16_t timestamp_fudge_factor = 30;

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
            else if (strcasecmp(arg, "--pbkdf2-iterations") == 0)
            {
                state = FF_PARSE_ARG_PARSE_PBKDF2_ITERATIONS;
            }
            else if (strcasecmp(arg, "--timestamp-fudge-factor") == 0)
            {
                state = FF_PARSE_ARG_PARSE_TIMESTAMP_FUDGE_FACTOR;
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
            int parsed_port = atoi(arg);

            if (parsed_port <= 0 || parsed_port > UINT16_MAX)
            {
                fprintf(stderr, "Invalid --port argument: %s\n\n", arg);
                action = FF_ACTION_INVALID_ARGS;
                goto done;
            }

            port = arg;
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

            listen_address = arg;
            state = FF_PARSE_ARG_STATE_DEFAULT;
            break;
        }

        case FF_PARSE_ARG_PARSE_PSK:
            encryption_config.key = (uint8_t *)arg;
            state = FF_PARSE_ARG_STATE_DEFAULT;
            break;

        case FF_PARSE_ARG_PARSE_PBKDF2_ITERATIONS:
            encryption_config.pbkdf2_iterations = atoi(arg);

            if (encryption_config.pbkdf2_iterations <= 0)
            {
                fprintf(stderr, "Invalid --pbkdf2-iterations argument: %s\n\n", arg);
                action = FF_ACTION_INVALID_ARGS;
                goto done;
            }

            state = FF_PARSE_ARG_STATE_DEFAULT;
            break;

        case FF_PARSE_ARG_PARSE_TIMESTAMP_FUDGE_FACTOR:
        {
            int parsed = atoi(arg);

            if (parsed <= 0 || parsed > UINT16_MAX)
            {
                fprintf(stderr, "Invalid --timestamp-fudge-factor argument: %s\n\n", arg);
                action = FF_ACTION_INVALID_ARGS;
                goto done;
            }

            timestamp_fudge_factor = (uint16_t)parsed;

            state = FF_PARSE_ARG_STATE_DEFAULT;
            break;
        }

        default:
            fputs("Unkown parse arg state\n\n", stderr);
            action = FF_ACTION_INVALID_ARGS;
            goto done;
        }
    }

    if (action == FF_ACTION_START_PROXY)
    {
        if (!port)
        {
            fputs("--port is required\n\n", stderr);
            action = FF_ACTION_INVALID_ARGS;
            goto done;
        }

        config->ip_address = listen_address;
        config->port = port;
        config->encryption = encryption_config;
        config->logging_level = logging_level;
        config->timestamp_fudge_factor = timestamp_fudge_factor;
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
    [--pbkdf2-iterations num] # hashing iterations used to derive encryption keys \n\
    [--timestamp-fudge-factor num] # amount of seconds away from the hosts time to tolerate for incoming requests \n\
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
