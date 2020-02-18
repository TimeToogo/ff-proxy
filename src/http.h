#include "request.h"

#ifndef FF_HTTP_H
#define FF_HTTP_H

#define FF_HTTP_HOST_HEADER_MAX_SEARCH_LENGTH 8096
#define FF_HTTP_RESPONSE_BUFF_SIZE 4096

void ff_http_send_request(struct ff_request *request);

void ff_http_send_request_unencrypted(struct ff_request *request);

void ff_http_send_request_tls(struct ff_request *request);

char *ff_http_get_destination_host(struct ff_request *request);

#endif