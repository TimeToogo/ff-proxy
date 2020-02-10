#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "alloc.h"

struct ff_request_option_node *ff_request_option_node_alloc()
{
    struct ff_request_option_node *option = (struct ff_request_option_node *)malloc(sizeof(struct ff_request_option_node));

    option->type = 0;
    option->length = 0;
    option->value = NULL;
    option->next = NULL;

    return option;
}

void ff_request_option_node_free(struct ff_request_option_node *option)
{
    if (option == NULL)
        return;

    FREE(option->value);

    FREE(option);
}

struct ff_request_payload_node *ff_request_payload_node_alloc()
{
    struct ff_request_payload_node *node = (struct ff_request_payload_node *)malloc(sizeof(struct ff_request_payload_node));

    node->length = 0;
    node->offset = 0;
    node->value = NULL;
    node->next = NULL;

    return node;
}

void ff_request_payload_node_free(struct ff_request_payload_node *node)
{
    if (node == NULL)
        return;

    FREE(node->value);

    FREE(node);
}

struct ff_request *ff_request_alloc()
{
    struct ff_request *request = (struct ff_request *)malloc(sizeof(struct ff_request));

    request->state = FF_REQUEST_STATE_RECEIVING;
    request->version = 0;
    request->request_id = 0;
    request->source_address_type = 0;
    request->options = NULL;
    request->payload_length = 0;
    request->payload = NULL;

    return request;
}

void ff_request_free(struct ff_request *request)
{
    if (request == NULL)
        return;

    struct ff_request_option_node *option_node = request->options;
    struct ff_request_option_node *option_prev;

    while (option_node != NULL)
    {
        option_prev = option_node;
        option_node = option_node->next;
        ff_request_option_node_free(option_prev);
    }

    struct ff_request_payload_node *payload_node = request->payload;
    struct ff_request_payload_node *payload_prev;

    while (payload_node != NULL)
    {
        payload_prev = payload_node;
        payload_node = payload_node->next;
        ff_request_payload_node_free(payload_prev);
    }

    FREE(request);
}