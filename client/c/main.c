#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "main.h"
#include "config.h"
#include "client.h"
#include "logging.h"

static struct ff_client_config ff_global_config;

int main(int argc, char **argv)
{
    enum ff_client_action action = ff_client_parse_arguments(&ff_global_config, argc, argv);
    int ret = EXIT_SUCCESS;

    switch (action)
    {
    case FF_CLIENT_ACTION_MAKE_REQUEST:
        ret = ff_client_make_request(&ff_global_config, stdin);
        break;

    case FF_CLIENT_ACTION_PRINT_VERSION:
        ff_print_version(stdout);
        ret = 0;
        break;

    case FF_CLIENT_ACTION_PRINT_USAGE:
        ff_client_print_usage(stdout);
        ret = 0;
        break;

    case FF_CLIENT_ACTION_INVALID_ARGS:
        ff_client_print_usage(stderr);
        ret = EXIT_FAILURE;
        break;
    }

    ff_log(FF_INFO, "Exiting...");
    return ret;
}