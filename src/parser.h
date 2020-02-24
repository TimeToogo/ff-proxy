#include <stdbool.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "constants.h"
#include "request.h"

#ifndef FF_PARSER_H
#define FF_PARSER_H

struct __raw_ff_request_header
{
    uint16_t version;
    uint64_t request_id;
    uint32_t total_length;
    uint16_t chunk_offset;
    uint16_t chunk_length;
} __attribute__((packed));

struct __raw_ff_request_option_header
{
    uint8_t type;
    uint16_t length;
} __attribute__((packed));

void ff_request_parse_chunk(struct ff_request *request, uint32_t buff_size, void *buff);

uint64_t ff_request_parse_id(uint32_t buff_size, void *buff);
 
bool ff_request_is_raw_http(uint32_t buff_size, void *buff);
 
void ff_request_parse_first_chunk(struct ff_request *request, uint32_t buff_size, void *buff);

void ff_request_parse_raw_http(struct ff_request *request, uint32_t buff_size, void *buff);

void ff_request_parse_data_chunk(struct ff_request *request, uint32_t buff_size, void *buff);

void ff_request_vectorise_payload(struct ff_request *request);

#endif