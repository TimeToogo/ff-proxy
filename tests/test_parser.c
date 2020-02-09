#include <stdlib.h>
#include "include/unity.h"
#include "../src/parser.h"

void test_request_header_alloc()
{
    struct ff_request_header* header = ff_request_header_alloc();

    TEST_ASSERT(header->key == NULL);
    TEST_ASSERT(header->value == NULL);

    free(header);
}

void test_request_header_free()
{
    struct ff_request_header* header = ff_request_header_alloc();

    header->key = (char*)malloc(10);
    header->value = (char*)malloc(10);

    ff_request_header_free(header);

    TEST_ASSERT(header->key == NULL);
    TEST_ASSERT(header->value == NULL);
}

void test_request_alloc()
{
    struct ff_request* request = ff_request_alloc();

    TEST_ASSERT(request->state == FF_REQUEST_STATE_RECEIVING);
    TEST_ASSERT(request->method == NULL);
    TEST_ASSERT(request->path == NULL);
    TEST_ASSERT(request->headers_length == 0);
    TEST_ASSERT(request->headers == NULL);
    TEST_ASSERT(request->body == NULL);

    free(request);
}

void test_request_free()
{
    struct ff_request* request = ff_request_alloc();

    request->method = (char*)malloc(10);
    request->path = (char*)malloc(10);
    request->headers = (void*)malloc(10);
    request->body = (char*)malloc(10);

    ff_request_free(request);

    TEST_ASSERT(request->method == NULL);
    TEST_ASSERT(request->path == NULL);
    TEST_ASSERT(request->headers == NULL);
    TEST_ASSERT(request->body == NULL);
}