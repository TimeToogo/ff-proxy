#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include "main.h"
#include "config.h"

#define FF_PARSE_ARG_STATE_DEFAULT 0
#define FF_PARSE_ARG_PARSE_PORT 1
#define FF_PARSE_ARG_PARSE_IP 2

enum ff_action ff_parse_arguments(struct ff_config *config, int argc, char **argv)
{
    enum ff_action action = FF_ACTION_START_PROXY;

    int state = FF_PARSE_ARG_STATE_DEFAULT;

    uint16_t port = 0;
    struct in_addr ip_address;
    bool parsed_port = false;
    bool parsed_ip = false;

    for (int i = 0; i < argc; i++)
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
            break;

        case FF_PARSE_ARG_PARSE_PORT:
            sscanf(arg, "%hu", &port);

            if (port == 0)
            {
                fprintf(stderr, "Invalid --port argument: %s\n\n", arg);
                action = FF_ACTION_INVALID_ARGS;
                goto done;
            }

            config->port = port;
            parsed_port = true;
            state = FF_PARSE_ARG_STATE_DEFAULT;
            break;

        case FF_PARSE_ARG_PARSE_IP:
            if (inet_pton(AF_INET, arg, &ip_address) != 1)
            {
                fprintf(stderr, "Invalid --ip-address argument: %s\n\n", arg);
                action = FF_ACTION_INVALID_ARGS;
                goto done;
            }

            config->ip_address = ip_address;
            parsed_ip = true;
            state = FF_PARSE_ARG_STATE_DEFAULT;
            break;

        default:
            fputs("Unkown parse arg state\n\n", stderr);
            action = FF_ACTION_INVALID_ARGS;
            goto done;
        }
    }

    if (action == FF_ACTION_START_PROXY && !parsed_port)
    {
        fputs("--port is required\n\n", stderr);
        action = FF_ACTION_INVALID_ARGS;
    }

    if (action == FF_ACTION_START_PROXY && !parsed_ip)
    {
        fputs("--ip-address is required\n\n", stderr);
        action = FF_ACTION_INVALID_ARGS;
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
    --ip-address bind_ip_address # format: 0.0.0.0 \n\
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
