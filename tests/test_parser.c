#include <stdlib.h>
#include <string.h>
#include "include/unity.h"
#include "../src/parser.h"
#include "../src/alloc.h"

void test_request_option_node_alloc()
{
    struct ff_request_option_node *option = ff_request_option_node_alloc();

    TEST_ASSERT(option->type == 0);
    TEST_ASSERT(option->length == 0);
    TEST_ASSERT(option->value == NULL);

    free(option);
}

void test_request_option_node_free()
{
    struct ff_request_option_node *option = ff_request_option_node_alloc();

    option->value = (char *)malloc(10);

    ff_request_option_node_free(option);

    TEST_ASSERT(option->value == NULL);
}

void test_request_payload_node_alloc()
{
    struct ff_request_payload_node *node = ff_request_payload_node_alloc();

    TEST_ASSERT(node->offset == 0);
    TEST_ASSERT(node->length == 0);
    TEST_ASSERT(node->value == NULL);
    TEST_ASSERT(node->next == NULL);

    free(node);
}

void test_request_payload_node_free()
{
    struct ff_request_payload_node *node = ff_request_payload_node_alloc();
    void *next_node;

    node->value = (char *)malloc(10);
    next_node = node->next = (void *)malloc(10);

    ff_request_payload_node_free(node);

    TEST_ASSERT(node->value == NULL);
    TEST_ASSERT_MESSAGE(node->next == next_node, "Should not free next node");
    free(next_node);
}

void test_request_alloc()
{
    struct ff_request *request = ff_request_alloc();

    TEST_ASSERT(request->state == FF_REQUEST_STATE_RECEIVING);
    TEST_ASSERT(request->version == 0);
    TEST_ASSERT(request->request_id == 0);
    TEST_ASSERT(request->source_address_type == 0);
    TEST_ASSERT(request->options == NULL);
    TEST_ASSERT(request->payload == NULL);
    TEST_ASSERT(request->payload_length == 0);
    TEST_ASSERT(request->received_length == 0);

    free(request);
}

void test_request_free()
{
    struct ff_request *request = ff_request_alloc();

    struct ff_request_option_node *option_a = ff_request_option_node_alloc();
    struct ff_request_option_node *option_b = ff_request_option_node_alloc();

    struct ff_request_payload_node *node_a = ff_request_payload_node_alloc();
    struct ff_request_payload_node *node_b = ff_request_payload_node_alloc();
    node_a->next = node_b;

    request->options = malloc(sizeof(struct ff_request_option_node*) * 2);
    request->options_length = 2;
    request->options[0] = option_a;
    request->options[1] = option_b;
    request->payload = node_a;

    ff_request_free(request);
}

void test_request_parse_raw_http_get()
{
    char *raw_http_request = "GET / HTTP/1.1\nHost: stackoverflow.com\n\n";

    struct ff_request *request = ff_request_alloc();
    ff_request_parse_chunk(request, strlen(raw_http_request), raw_http_request);

    TEST_ASSERT_EQUAL_MESSAGE(FF_VERSION_RAW, request->version, "Version check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_RECEIVED, request->state, "State check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->request_id, "Request ID check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(raw_http_request), request->received_length, "Received length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(raw_http_request), request->payload_length, "Payload length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->payload->offset, "Payload node offset check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(raw_http_request), request->payload->length, "Payload node length check failed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(raw_http_request, request->payload->value, "Payload node value check failed");

    ff_request_free(request);
}

void test_request_parse_raw_http_post()
{
    char *raw_http_request = "POST / HTTP/1.1\nHost: stackoverflow.com\n\nSome\nTest\nData";

    struct ff_request *request = ff_request_alloc();
    ff_request_parse_chunk(request, strlen(raw_http_request), raw_http_request);

    TEST_ASSERT_EQUAL_MESSAGE(FF_VERSION_RAW, request->version, "Version check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_RECEIVED, request->state, "State check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->request_id, "Request ID check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(raw_http_request), request->received_length, "Received length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(raw_http_request), request->payload_length, "Payload length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->payload->offset, "Payload node offset check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(raw_http_request), request->payload->length, "Payload node length check failed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(raw_http_request, request->payload->value, "Payload node value check failed");

    ff_request_free(request);
}

void test_request_parse_v1_single_chunk()
{
    char *http_request = "POST / HTTP/1.1\nHost: stackoverflow.com\n\nSome\nTest\nData";

    struct __raw_ff_request_header header = {
        .version = FF_VERSION_1,
        .request_id = 1234568,
        .total_length = strlen(http_request),
        .chunk_offset = 0,
        .chunk_length = strlen(http_request)
    };
    
    struct __raw_ff_request_option_header eol_option = {
        .type = FF_REQUEST_OPTION_TYPE_EOL,
        .length = 0
    };

    int chunk_length = sizeof(header) + sizeof(eol_option) + strlen(http_request);

    char *raw_chunk = (char*)malloc(chunk_length);
    memcpy(raw_chunk, &header, (int)sizeof(header));
    memcpy(raw_chunk + sizeof(header), &eol_option, (int)sizeof(eol_option));
    memcpy(raw_chunk + sizeof(eol_option) + sizeof(header), http_request, strlen(http_request));

    struct ff_request *request = ff_request_alloc();
    ff_request_parse_chunk(request, chunk_length, raw_chunk);

    TEST_ASSERT_EQUAL_MESSAGE(FF_VERSION_1, request->version, "Version check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_RECEIVED, request->state, "State check failed");
    TEST_ASSERT_EQUAL_MESSAGE(1234568, request->request_id, "Request ID check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(http_request), request->received_length, "Received length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(http_request), request->payload_length, "Payload length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->options_length, "Options length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(NULL, request->options, "Options check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->payload->offset, "Payload node offset check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(http_request), request->payload->length, "Payload node length check failed");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(http_request, request->payload->value, request->payload->length, "Payload node value check failed");

    ff_request_free(request);

    FREE(raw_chunk);
}

void test_request_parse_v1_multiple_chunks()
{
    char *chunk_1_http_request = "POST / HTTP/1.1\nHost: stackoverflow.com\n\n";
    char *chunk_2_http_request = "Some\nBody\nData";

    struct __raw_ff_request_header chunk_1_header = {
        .version = FF_VERSION_1,
        .request_id = 1234568,
        .total_length = strlen(chunk_1_http_request) + strlen(chunk_2_http_request),
        .chunk_offset = 0,
        .chunk_length = strlen(chunk_1_http_request)
    };

    struct __raw_ff_request_header chunk_2_header = {
        .version = FF_VERSION_1,
        .request_id = 1234568,
        .total_length = strlen(chunk_1_http_request) + strlen(chunk_2_http_request),
        .chunk_offset = strlen(chunk_1_http_request),
        .chunk_length = strlen(chunk_2_http_request)
    };
    
    struct __raw_ff_request_option_header eol_option = {
        .type = FF_REQUEST_OPTION_TYPE_EOL,
        .length = 0
    };

    int chunk_1_length = sizeof(chunk_1_header) + sizeof(eol_option) + strlen(chunk_1_http_request);
    int chunk_2_length = sizeof(chunk_2_header) + sizeof(eol_option) + strlen(chunk_2_http_request);

    char *raw_chunk_1 = (char*)malloc(chunk_1_length);
    memcpy(raw_chunk_1, &chunk_1_header, (int)sizeof(chunk_1_header));
    memcpy(raw_chunk_1 + sizeof(chunk_1_header), &eol_option, (int)sizeof(eol_option));
    memcpy(raw_chunk_1 + sizeof(chunk_1_header) + sizeof(eol_option), chunk_1_http_request, strlen(chunk_1_http_request));

    char *raw_chunk_2 = (char*)malloc(chunk_2_length);
    memcpy(raw_chunk_2, &chunk_2_header, (int)sizeof(chunk_2_header));
    memcpy(raw_chunk_2 + sizeof(chunk_2_header), &eol_option, (int)sizeof(eol_option));
    memcpy(raw_chunk_2 + sizeof(chunk_2_header) + sizeof(eol_option), chunk_2_http_request, strlen(chunk_2_http_request));

    struct ff_request *request = ff_request_alloc();
    ff_request_parse_chunk(request, chunk_1_length, raw_chunk_1);
    ff_request_parse_chunk(request, chunk_2_length, raw_chunk_2);

    size_t total_length = strlen(chunk_1_http_request) + strlen(chunk_2_http_request);

    TEST_ASSERT_EQUAL_MESSAGE(FF_VERSION_1, request->version, "Version check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_RECEIVED, request->state, "State check failed");
    TEST_ASSERT_EQUAL_MESSAGE(1234568, request->request_id, "Request ID check failed");
    TEST_ASSERT_EQUAL_MESSAGE(total_length, request->received_length, "Received length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(total_length, request->payload_length, "Payload length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->options_length, "Options length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(NULL, request->options, "Options check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->payload->offset, "Payload node (1) offset check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(chunk_1_http_request), request->payload->length, "Payload node (1) length check failed");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(chunk_1_http_request, request->payload->value, request->payload->length, "Payload node (1) value check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(chunk_1_http_request), request->payload->next->offset, "Payload node (2) offset check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(chunk_2_http_request), request->payload->next->length, "Payload node (2) length check failed");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(chunk_2_http_request, request->payload->next->value, request->payload->next->length, "Payload node (2) value check failed");

    ff_request_free(request);

    FREE(raw_chunk_1);
    FREE(raw_chunk_2);
}


void test_request_parse_v1_single_chunk_with_options()
{
    char *http_request = "POST / HTTP/1.1\nHost: stackoverflow.com\n\nSome\nTest\nData";

    struct __raw_ff_request_header header = {
        .version = FF_VERSION_1,
        .request_id = 1234568,
        .total_length = strlen(http_request),
        .chunk_offset = 0,
        .chunk_length = strlen(http_request)
    };
    
    struct __raw_ff_request_option_header checksum_option = {
        .type = FF_REQUEST_OPTION_TYPE_CHECKSUM,
        .length = 3
    };
    
    struct __raw_ff_request_option_header eol_option = {
        .type = FF_REQUEST_OPTION_TYPE_EOL,
        .length = 0
    };

    int chunk_length = sizeof(header) + sizeof(checksum_option) + 3 + sizeof(eol_option) + strlen(http_request);

    void *raw_chunk = (char*)malloc(chunk_length);
    void *chunk_ptr = raw_chunk;

    memcpy(chunk_ptr, &header, (int)sizeof(header));
    chunk_ptr += sizeof(header);
    memcpy(chunk_ptr, &checksum_option, (int)sizeof(checksum_option));
    chunk_ptr += sizeof(checksum_option);
    memcpy(chunk_ptr, &"abc", 3);
    chunk_ptr += 3;
    memcpy(chunk_ptr, &eol_option, (int)sizeof(eol_option));
    chunk_ptr += sizeof(eol_option);
    memcpy(chunk_ptr, http_request, strlen(http_request));
    chunk_ptr += strlen(http_request);

    struct ff_request *request = ff_request_alloc();
    ff_request_parse_chunk(request, chunk_length, raw_chunk);

    TEST_ASSERT_EQUAL_MESSAGE(FF_VERSION_1, request->version, "Version check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_RECEIVED, request->state, "State check failed");
    TEST_ASSERT_EQUAL_MESSAGE(1234568, request->request_id, "Request ID check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(http_request), request->received_length, "Received length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(http_request), request->payload_length, "Payload length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(1, request->options_length, "Options length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_OPTION_TYPE_CHECKSUM, request->options[0]->type, "Option (1) type check failed");
    TEST_ASSERT_EQUAL_MESSAGE(3, request->options[0]->length, "Option (1) length check failed");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("abc", request->options[0]->value, request->options[0]->length, "Option value check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->payload->offset, "Payload node offset check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(http_request), request->payload->length, "Payload node length check failed");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(http_request, request->payload->value, request->payload->length, "Payload node value check failed");

    ff_request_free(request);

    FREE(raw_chunk);
}