#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "alloc.h"

struct ff_request_header* ff_request_header_alloc()
{
    struct ff_request_header* header = (struct ff_request_header*)malloc(sizeof(struct ff_request_header));

    header->key = NULL;
    header->value = NULL;

    return header;
}

void ff_request_header_free(struct ff_request_header* header)
{
    if (header == NULL) return;

    FREE(header->key);
    FREE(header->value);

    FREE(header);
}

struct ff_request* ff_request_alloc()
{
    struct ff_request* request = (struct ff_request*)malloc(sizeof(struct ff_request));

    request->state = FF_REQUEST_STATE_RECEIVING;
    request->method = NULL;
    request->path = NULL;
    request->headers_length = 0;
    request->headers = NULL;
    request->body = NULL;

    return request;
}

void ff_request_free(struct ff_request* request)
{
    if (request == NULL) return;

    FREE(request->method);
    FREE(request->path);
    FREE(request->headers);
    FREE(request->body);

    FREE(request);
}