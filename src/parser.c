#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "parser.h"
#include "parser_p.h"
#include "alloc.h"
#include "constants.h"
#include "logging.h"
#include "os/linux_endian.h"

void ff_request_parse_chunk(struct ff_request *request, uint32_t buff_size, void *buff)
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

uint64_t ff_request_parse_id(uint32_t buff_size, void *buff)
{
    bool is_raw_http = ff_request_is_raw_http(buff_size, buff);

    if (is_raw_http)
    {
        return 0;
    }
    else
    {
        struct __raw_ff_request_header *header = (struct __raw_ff_request_header *)buff;
        return ntohll(header->request_id);
    }
}

bool ff_request_is_raw_http(uint32_t buff_size, void *buff)
{
    if (buff_size == 0)
    {
        return false;
    }

    bool is_raw_http = false;

    for (int i = 0; i < (int)(sizeof(HTTP_METHODS) / sizeof(HTTP_METHODS[0])); i++)
    {
        is_raw_http |= strncmp(HTTP_METHODS[i], buff, strlen(HTTP_METHODS[i])) == 0;

        if (is_raw_http)
        {
            break;
        }
    }

    return is_raw_http;
}

void ff_request_parse_first_chunk(struct ff_request *request, uint32_t buff_size, void *buff)
{
    bool is_raw_http = ff_request_is_raw_http(buff_size, buff);

    if (is_raw_http)
    {
        ff_request_parse_raw_http(request, buff_size, buff);
        return;
    }

    if (buff_size < sizeof(struct __raw_ff_request_header))
    {
        ff_log(FF_WARNING, "Packet buffer too small to contain header (%d)", buff_size);
        request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
        return;
    }

    struct __raw_ff_request_header *header = (struct __raw_ff_request_header *)buff;
    request->version = ntohs(header->version);
    request->request_id = ntohll(header->request_id);
    request->payload_length = ntohl(header->total_length);
    ff_request_parse_data_chunk(request, buff_size, buff);
}

void ff_request_parse_raw_http(struct ff_request *request, uint32_t buff_size, void *buff)
{
    request->version = FF_VERSION_RAW;
    request->state = FF_REQUEST_STATE_RECEIVED;
    request->payload_length = buff_size;
    request->received_length = buff_size;
    request->payload = ff_request_payload_node_alloc();
    request->payload->length = buff_size;
    ff_request_payload_load_buff(request->payload, buff_size, buff);
}

void ff_request_parse_data_chunk(struct ff_request *request, uint32_t buff_size, void *buff)
{
    uint32_t i = 0;

    if (buff_size < sizeof(struct __raw_ff_request_header))
    {
        ff_log(FF_WARNING, "Packet buffer to small to contain header (%d)", buff_size);
        request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
        return;
    }

    struct __raw_ff_request_header *header = (struct __raw_ff_request_header *)buff;
    i += sizeof(struct __raw_ff_request_header);

    // Normalise endianess
    uint16_t version = ntohs(header->version);
    uint64_t request_id = ntohll(header->request_id);
    uint32_t total_length = ntohl(header->total_length);
    uint32_t chunk_offset = ntohl(header->chunk_offset);
    uint16_t chunk_length = ntohs(header->chunk_length);

    if (request->version != FF_VERSION_1)
    {
        ff_log(FF_WARNING, "Request received with unknown version flag %d", request->version);
        request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
        return;
    }

    if (version != request->version)
    {
        ff_log(FF_WARNING, "Mismatch between request version (%d) and chunk version (%d)", request->version, version);
        request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
        return;
    }

    if (request_id != request->request_id)
    {
        ff_log(FF_WARNING, "Mismatch between request ID (%d) and chunk ID (%d)", request->request_id, request_id);
        request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
        return;
    }

    if (total_length != request->payload_length)
    {
        ff_log(FF_WARNING, "Mismatch between request length (%d) and chunk length (%d)", request->payload_length, total_length);
        request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
        return;
    }

    if (chunk_offset + chunk_length > request->payload_length)
    {
        ff_log(FF_WARNING, "Chunk offset and length too long");
        request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
        return;
    }

    struct __raw_ff_request_option_header *option_header = NULL;

    if (buff_size < i + sizeof(struct __raw_ff_request_option_header))
    {
        ff_log(FF_WARNING, "Packet buffer ran out while looking for TLV options");
        request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
        return;
    }

    // Only first request can contain options
    if (header->chunk_offset == 0)
    {
        // Parse TLV options
        struct ff_request_option_node *options[FF_REQUEST_MAX_OPTIONS];
        int options_i = 0;

        while (1)
        {
            if (options_i >= FF_REQUEST_MAX_OPTIONS)
            {
                ff_log(FF_WARNING, "Encountered request with too many options");
                request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
                return;
            }

            if (buff_size < i + sizeof(struct __raw_ff_request_option_header))
            {
                ff_log(FF_WARNING, "Packet buffer ran out while processing TLV options");
                request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
                return;
            }

            option_header = (struct __raw_ff_request_option_header *)(buff + i);

            // Normalise endianess
            uint16_t option_length = ntohs(option_header->length);

            i += sizeof(struct __raw_ff_request_option_header);

            if (option_header->type == FF_REQUEST_OPTION_TYPE_EOL)
            {
                break;
            }

            if (buff_size < i + option_length)
            {
                ff_log(FF_WARNING, "Packet buffer ran out while processing TLV options");
                request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
                return;
            }

            options[options_i] = ff_request_option_node_alloc();
            options[options_i]->type = option_header->type;
            options[options_i]->length = option_length;
            ff_request_option_load_buff(
                options[options_i],
                option_length,
                buff + i);

            i += option_length;
            options_i++;
        }

        request->options_length = options_i;
        if (options_i != 0)
        {
            request->options = malloc(sizeof(struct ff_request_option_node *) * options_i);
            memcpy(request->options, options, sizeof(struct ff_request_option_node *) * options_i);
        }
    }
    else
    {
        option_header = (struct __raw_ff_request_option_header *)(buff + i);

        i += sizeof(struct __raw_ff_request_option_header);

        if (option_header->type != FF_REQUEST_OPTION_TYPE_EOL)
        {
            ff_log(FF_WARNING, "Noninitial packets cannot contain request options");
            request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
            return;
        }
    }

    if (buff_size < i + chunk_length)
    {
        ff_log(FF_WARNING, "Packet buffer ran out while receiving payload. Expected %hu bytes, %u bytes remain", chunk_length, buff_size - i);
        request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
        return;
    }

    struct ff_request_payload_node *node = ff_request_payload_node_alloc();
    node->offset = chunk_offset;
    node->length = chunk_length;

    ff_request_payload_load_buff(
        node,
        buff_size - i,
        buff + i);

    if (request->payload == NULL)
    {
        request->payload = node;
        request->received_length = node->length;
    }
    else
    {
        struct ff_request_payload_node *current_node = request->payload;

        while (1)
        {

            bool overlaps_existing_node = node->offset < current_node->offset + current_node->length && current_node->offset <= node->offset + node->length;
            if (overlaps_existing_node)
            {
                ff_log(FF_WARNING,
                       "Received chunk range overlaps existing data. Received byte range [%u, %u] intersects existing range [%u, %u]",
                       node->offset, node->offset + node->length,
                       current_node->offset, current_node->offset + current_node->length);
                request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
                return;
            }

            if (current_node == NULL || current_node->next == NULL)
            {
                break;
            }

            current_node = current_node->next;
        }

        current_node->next = node;
        request->received_length += node->length;
    }

    if (request->received_length == request->payload_length)
    {
        ff_log(FF_DEBUG, "Request %lu successfully received", request->request_id);
        request->state = FF_REQUEST_STATE_RECEIVED;
    }
    else
    {
        ff_log(FF_DEBUG, "Finished parsing request %lu partial packet, %lu bytes remain", request->request_id, request->payload_length - request->received_length);
    }
}

void ff_request_vectorise_payload(struct ff_request *request)
{
    if (request->payload->next == NULL)
    {
        return;
    }

    struct ff_request_payload_node *payload = ff_request_payload_node_alloc();
    payload->length = request->payload_length;
    payload->offset = 0;
    payload->next = NULL;
    payload->value = malloc(request->payload_length * sizeof(uint8_t));

    struct ff_request_payload_node *node = request->payload;
    struct ff_request_payload_node *tmp_node = NULL;

    do
    {
        memcpy(payload->value + node->offset, node->value, node->length * sizeof(uint8_t));

        tmp_node = node->next;
        ff_request_payload_node_free(node);
        node = tmp_node;
    } while (node != NULL);

    request->payload = payload;
}
