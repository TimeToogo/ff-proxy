#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include "client.h"
#include "config.h"
#include "request.h"

int ff_client_make_request(struct ff_client_config *config, FILE *fp)
{
    struct ff_request *request = ff_request_alloc();
    request->options = malloc(sizeof(struct ff_request_option_node *) * FF_REQUEST_MAX_OPTIONS);
}