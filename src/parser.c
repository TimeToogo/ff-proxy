#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "parser.h"
#include "alloc.h"
#include "constants.h"
#include "logging.h"

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

void ff_request_payload_load_buff(struct ff_request_payload_node *node, int buff_size, char *buff)
{
    char *buff_copy = (char *)malloc(buff_size * sizeof(buff));

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
    struct ff_request *request = (struct ff_request *)malloc(sizeof(struct ff_request));

    request->state = FF_REQUEST_STATE_RECEIVING;
    request->version = 0;
    request->request_id = 0;
    request->source_address_type = 0;
    request->options = NULL;
    request->payload_length = 0;
    request->received_length = 0;
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

void ff_request_parse_chunk(struct ff_request *request, uint32_t buff_size, char *buff)
{
    bool isFirstChunk = request->version == 0;

    if (isFirstChunk)
    {
        ff_request_parse_first_chunk(request, buff_size, buff);
    }
    else
    {
        ff_request_parse_data_chunk(request, buff_size, buff);
    }
}

void ff_request_parse_first_chunk(struct ff_request *request, uint32_t buff_size, char *buff)
{
    bool isRawHttpRequest = false;

    for (int i = 0; i < (sizeof(HTTP_METHODS) / sizeof(HTTP_METHODS[0])); i++)
    {
        isRawHttpRequest |= strncmp(HTTP_METHODS[i], buff, strlen(HTTP_METHODS[i])) == 0;

        if (isRawHttpRequest)
        {
            break;
        }
    }

    if (isRawHttpRequest)
    {
        ff_request_parse_raw_http(request, buff_size, buff);
    }
    else
    {
        struct __raw_ff_request_header *header = (struct __raw_ff_request_header *)buff;
        request->version = header->version;
        request->request_id = header->request_id;
        request->payload_length = header->total_length;
        ff_request_parse_data_chunk(request, buff_size, buff);
    }
}

void ff_request_parse_raw_http(struct ff_request *request, uint32_t buff_size, char *buff)
{
    request->version = FF_VERSION_RAW;
    request->state = FF_REQUEST_STATE_RECEIVED;
    request->payload_length = buff_size;
    request->received_length = buff_size;
    request->payload = ff_request_payload_node_alloc();
    request->payload->length = buff_size;
    ff_request_payload_load_buff(request->payload, buff_size, buff);
}

void ff_request_parse_data_chunk(struct ff_request *request, uint32_t buff_size, char *buff)
{
    struct __raw_ff_request_header *header = (struct __raw_ff_request_header *)buff;

    if (request->version != FF_VERSION_1)
    {
        ff_log(FF_WARNING, "Request received unknown version flag %d", request->version);
        request->state = FF_REQUEST_STATE_RECEIVED_FAIL;
        return;
    }

    if (header->version != request->version)
    {
        ff_log(FF_WARNING, "Mismatch between request version (%d) and chunk version (%d)", request->version, header->version);
        request->state = FF_REQUEST_STATE_RECEIVED_FAIL;
        return;
    }

    if (header->request_id != request->request_id)
    {
        ff_log(FF_WARNING, "Mismatch between request ID (%d) and chunk ID (%d)", request->request_id, header->request_id);
        request->state = FF_REQUEST_STATE_RECEIVED_FAIL;
        return;
    }

    if (header->total_length != request->payload_length)
    {
        ff_log(FF_WARNING, "Mismatch between request length (%d) and chunk length (%d)", request->payload_length, header->total_length);
        request->state = FF_REQUEST_STATE_RECEIVED_FAIL;
        return;
    }

    if (header->chunk_offset + header->chunk_length > request->payload_length)
    {
        ff_log(FF_WARNING, "Chunk offset and length too long");
        request->state = FF_REQUEST_STATE_RECEIVED_FAIL;
        return;
    }

    // TODO: Parse TLV header options

    struct ff_request_payload_node *node = ff_request_payload_node_alloc();
    node->offset = header->chunk_offset;
    node->length = header->chunk_length;
    ff_request_payload_load_buff(
        node,
        buff_size - sizeof(struct __raw_ff_request_header),
        buff + sizeof(struct __raw_ff_request_header));

    if (request->payload == NULL)
    {
        request->payload = node;
        request->received_length = node->length;
    }
    else
    {
        struct ff_request_payload_node *current_node = request->payload;

        while (current_node->next != NULL)
        {
            bool overlapsExistingNode = node->offset <= current_node->offset + current_node->length && current_node->offset <= node->offset + node->length;
            if (overlapsExistingNode)
            {
                ff_log(FF_WARNING, "Received chunk range overlaps existing data");
                request->state = FF_REQUEST_STATE_RECEIVED_FAIL;
                return;
            }

            current_node = current_node->next;
        }

        current_node->next = node;
        request->received_length += node->length;
    }

    if (request->received_length == request->payload_length) {
        request->state = FF_REQUEST_STATE_RECEIVED;
    }
}
