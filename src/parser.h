#include <stdbool.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "constants.h"
#include "request.h"

#ifndef FF_PARSER_H
#define FF_PARSER_H

uint64_t ff_request_parse_id(uint32_t buff_size, void *buff);
 
bool ff_request_is_raw_http(uint32_t buff_size, void *buff);

void ff_request_parse_chunk(struct ff_request *request, uint32_t buff_size, void *buff);

void ff_request_parse_options_from_payload(struct ff_request *request);

#endif