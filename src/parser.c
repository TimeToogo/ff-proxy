#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "parser.h"
#include "parser_p.h"
#include "alloc.h"
#include "constants.h"
#include "logging.h"
#include "assert.h"
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
        if (buff_size < strlen(HTTP_METHODS[i]))
        {
            break;
        }

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
    size_t i = 0;

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

    if (buff_size < i + sizeof(struct __raw_ff_request_option_header))
    {
        ff_log(FF_WARNING, "Packet buffer ran out while looking for TLV options");
        request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
        return;
    }

    // Only first request can contain options
    if (header->chunk_offset == 0)
    {
        size_t options_i = ff_request_parse_options(request, buff_size - i, buff + i);

        if (options_i == 0)
        {
            request->state = FF_REQUEST_STATE_RECEIVING_FAIL;
            return;
        }

        i += options_i;
    }
    else
    {
        struct __raw_ff_request_option_header *option_header = (struct __raw_ff_request_option_header *)(buff + i);

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

size_t ff_request_parse_options(struct ff_request *request, uint32_t buff_size, void *buff)
{
    struct __raw_ff_request_option_header *option_header = NULL;
    struct ff_request_option_node *options[FF_REQUEST_MAX_OPTIONS];
    uint8_t options_i = 0;
    uint32_t i = 0;
    size_t current_options_size = request->options_length;

    // Parse TLV options
    while (1)
    {
        if (current_options_size + options_i >= FF_REQUEST_MAX_OPTIONS)
        {
            ff_log(FF_WARNING, "Encountered request with too many options");
            goto error;
        }

        if (buff_size < i + sizeof(struct __raw_ff_request_option_header))
        {
            ff_log(FF_WARNING, "Packet buffer ran out while processing TLV options");
            goto error;
        }

        option_header = (struct __raw_ff_request_option_header *)(buff + i);

        // Normalise endianess
        uint16_t option_length = ntohs(option_header->length);

        i += sizeof(struct __raw_ff_request_option_header);

        if (option_header->type == FF_REQUEST_OPTION_TYPE_BREAK)
        {
            if (option_length != 0)
            {
                ff_log(FF_WARNING, "Request option FF_REQUEST_OPTION_TYPE_BREAK must have length = 0");
                goto error;
            }

            request->payload_contains_options = true;
            break;
        }

        if (option_header->type == FF_REQUEST_OPTION_TYPE_EOL)
        {
            if (option_length != 0)
            {
                ff_log(FF_WARNING, "Request option FF_REQUEST_OPTION_TYPE_EOL must have length = 0");
                goto error;
            }

            request->payload_contains_options = false;
            break;
        }

        if (buff_size < i + option_length)
        {
            ff_log(FF_WARNING, "Packet buffer ran out while processing TLV options");
            goto error;
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

    if (options_i == 0)
    {
        return i;
    }

    request->options_length = current_options_size + options_i;

    if (request->options == NULL)
    {
        request->options = malloc(sizeof(struct ff_request_option_node *) * request->options_length);
    }
    else
    {
        request->options = realloc(request->options, sizeof(struct ff_request_option_node *) * request->options_length);
    }

    memcpy(
        request->options + current_options_size,
        options,
        sizeof(struct ff_request_option_node *) * options_i);

    goto done;

done:
    goto cleanup;

error:
    i = 0;

    for (uint8_t j = 0; j < options_i; j++) {
        ff_request_option_node_free(options[j]);
    }

    goto cleanup;

cleanup:
    return i;
}

void ff_request_parse_options_from_payload(struct ff_request *request)
{
    assert(request != NULL);
    assert(request->payload != NULL);
    assert(request->payload->next == NULL);

    struct ff_request_payload_node *payload = NULL;
    size_t options_length = 0;

    request->state = FF_REQUEST_STATE_PARSING_OPTIONS;

    if (!request->payload_contains_options)
    {
        goto done;
    }

    payload = request->payload;
    options_length = ff_request_parse_options(request, payload->length, payload->value);

    if (options_length == 0)
    {
        request->state = FF_REQUEST_STATE_PARSING_OPTIONS_FAILED;
        return;
    }

    if (request->payload->length <= options_length)
    {
        ff_log(FF_WARNING, "Request buffer ran out of buffer while parsing options within payload");
        goto error;
    }

    if (request->payload_contains_options)
    {
        ff_log(FF_WARNING, "Request cannot contain multiple FF_REQUEST_OPTION_TYPE_BREAK options");
        goto error;
    }

    struct ff_request_payload_node *new_payload = ff_request_payload_node_alloc();
    new_payload->length = payload->length - options_length;
    ff_request_payload_load_buff(new_payload, new_payload->length, payload->value + options_length);

    request->payload = new_payload;
    request->payload_length = new_payload->length;

    goto done;

done:
    request->state = FF_REQUEST_STATE_PARSED_OPTIONS;
    goto cleanup;

error:
    request->state = FF_REQUEST_STATE_PARSING_OPTIONS_FAILED;
    goto cleanup;

cleanup:
    ff_request_payload_node_free(payload);
}
