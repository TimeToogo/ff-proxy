#include <stdbool.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "constants.h"
#include "request.h"

#ifndef FF_PARSER_P_H
#define FF_PARSER_P_H

void ff_request_parse_first_chunk(struct ff_request *request, uint32_t buff_size, void *buff);

void ff_request_parse_raw_http(struct ff_request *request, uint32_t buff_size, void *buff);

void ff_request_parse_data_chunk(struct ff_request *request, uint32_t buff_size, void *buff);

size_t ff_request_parse_options(struct ff_request *request, uint32_t buff_size, void *buff);

#endif