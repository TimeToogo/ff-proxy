#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "request.h"
#include "alloc.h"
#include "constants.h"
#include "logging.h"

struct ff_request_option_node *ff_request_option_node_alloc()
{
    struct ff_request_option_node *option = malloc(sizeof(struct ff_request_option_node));

    option->type = 0;
    option->length = 0;
    option->value = NULL;

    return option;
}

void ff_request_option_load_buff(struct ff_request_option_node *node, uint32_t buff_size, void *buff)
{
    void *buff_copy = malloc(buff_size * sizeof(buff));

    memcpy(buff_copy, buff, buff_size);

    node->value = buff_copy;
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
    struct ff_request_payload_node *node = malloc(sizeof(struct ff_request_payload_node));

    node->length = 0;
    node->offset = 0;
    node->value = NULL;
    node->next = NULL;

    return node;
}

void ff_request_payload_load_buff(struct ff_request_payload_node *node, uint32_t buff_size, void *buff)
{
    void *buff_copy = malloc(buff_size * sizeof(buff));

    memcpy(buff_copy, buff, buff_size);

    node->value = buff_copy;
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
    struct ff_request *request = calloc(1, sizeof(struct ff_request));

    request->state = FF_REQUEST_STATE_RECEIVING;

    return request;
}

void ff_request_free(struct ff_request *request)
{
    if (request == NULL)
        return;

    for (int i = 0; i < request->options_length; i++)
    {
        ff_request_option_node_free(request->options[i]);
    }

    FREE(request->options);

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
