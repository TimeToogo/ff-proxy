#include <stdlib.h>
#include <string.h>
#include "../include/unity.h"
#include "../../src/parser.h"
#include "../../src/parser_p.h"
#include "../../src/alloc.h"
#include "../../src/os/linux_endian.h"

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
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(raw_http_request, request->payload->value, request->payload->length, "Payload node value check failed");

    ff_request_free(request);
}

void test_request_parse_single_char()
{
    char *payload = "D";

    struct ff_request *request = ff_request_alloc();
    ff_request_parse_chunk(request, strlen(payload), payload);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_RECEIVING_FAIL, request->state, "State check failed");

    ff_request_free(request);
}

void test_request_parse_fuzz1()
{
    uint8_t payload[] = "\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\xF9\x00\x00\x00\xE6";

    struct ff_request *request = ff_request_alloc();
    ff_request_parse_chunk(request, sizeof(payload), &payload);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_RECEIVING_FAIL, request->state, "State check failed");

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
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(raw_http_request, request->payload->value, request->payload->length, "Payload node value check failed");

    ff_request_free(request);
}

void test_request_parse_v1_single_chunk()
{
    char *http_request = "POST / HTTP/1.1\nHost: stackoverflow.com\n\nSome\nTest\nData";

    struct __raw_ff_request_header header = {
        .version = htons(FF_VERSION_1),
        .request_id = htonll(1234568ULL),
        .total_length = htonl(strlen(http_request)),
        .chunk_offset = htonl(0),
        .chunk_length = htons(strlen(http_request))};

    struct __raw_ff_request_option_header eol_option = {
        .type = FF_REQUEST_OPTION_TYPE_EOL,
        .length = htons(0)};

    int chunk_length = sizeof(header) + sizeof(eol_option) + strlen(http_request);

    char *raw_chunk = (char *)malloc(chunk_length);
    memcpy(raw_chunk, &header, (int)sizeof(header));
    memcpy(raw_chunk + sizeof(header), &eol_option, (int)sizeof(eol_option));
    memcpy(raw_chunk + sizeof(eol_option) + sizeof(header), http_request, strlen(http_request));

    struct ff_request *request = ff_request_alloc();
    ff_request_parse_chunk(request, chunk_length, raw_chunk);

    TEST_ASSERT_EQUAL_MESSAGE(FF_VERSION_1, request->version, "Version check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_RECEIVED, request->state, "State check failed");
    TEST_ASSERT_EQUAL_MESSAGE(1234568, request->request_id, "Request ID check failed");
    TEST_ASSERT_EQUAL_MESSAGE(false, request->payload_contains_options, "Payload contains options check failed");
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

    char *full_http_request = (char *)calloc(1, strlen(chunk_1_http_request) + strlen(chunk_2_http_request) + 1);
    strcat(full_http_request, chunk_1_http_request);
    strcat(full_http_request, chunk_2_http_request);

    struct __raw_ff_request_header chunk_1_header = {
        .version = htons(FF_VERSION_1),
        .request_id = htonll(1234568ULL),
        .total_length = htonl(strlen(chunk_1_http_request) + strlen(chunk_2_http_request)),
        .chunk_offset = htonl(0),
        .chunk_length = htons(strlen(chunk_1_http_request))};

    struct __raw_ff_request_header chunk_2_header = {
        .version = htons(FF_VERSION_1),
        .request_id = htonll(1234568ULL),
        .total_length = htonl(strlen(chunk_1_http_request) + strlen(chunk_2_http_request)),
        .chunk_offset = htonl(strlen(chunk_1_http_request)),
        .chunk_length = htons(strlen(chunk_2_http_request))};

    struct __raw_ff_request_option_header eol_option = {
        .type = FF_REQUEST_OPTION_TYPE_EOL,
        .length = htons(0)};

    int chunk_1_length = sizeof(chunk_1_header) + sizeof(eol_option) + strlen(chunk_1_http_request);
    int chunk_2_length = sizeof(chunk_2_header) + sizeof(eol_option) + strlen(chunk_2_http_request);

    char *raw_chunk_1 = (char *)malloc(chunk_1_length);
    memcpy(raw_chunk_1, &chunk_1_header, (int)sizeof(chunk_1_header));
    memcpy(raw_chunk_1 + sizeof(chunk_1_header), &eol_option, (int)sizeof(eol_option));
    memcpy(raw_chunk_1 + sizeof(chunk_1_header) + sizeof(eol_option), chunk_1_http_request, strlen(chunk_1_http_request));

    char *raw_chunk_2 = (char *)malloc(chunk_2_length);
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
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(chunk_1_http_request, request->payload->value, strlen(chunk_1_http_request), "Payload node (1) value check failed");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, request->payload->next, "Payload node (1) next check failed");

    TEST_ASSERT_EQUAL_MESSAGE(strlen(chunk_2_http_request), request->payload->next->length, "Payload node (2) length check failed");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(chunk_2_http_request, request->payload->next->value, strlen(chunk_2_http_request), "Payload node (2) value check failed");

    ff_request_free(request);

    FREE(raw_chunk_1);
    FREE(raw_chunk_2);
    FREE(full_http_request);
}


void test_request_parse_v1_break_option()
{
    char *payload = "payload";

    struct __raw_ff_request_header header = {
        .version = htons(FF_VERSION_1),
        .request_id = htonll(1234568ULL),
        .total_length = htonl(strlen(payload)),
        .chunk_offset = htonl(0),
        .chunk_length = htons(strlen(payload))};

    struct __raw_ff_request_option_header break_option = {
        .type = FF_REQUEST_OPTION_TYPE_BREAK,
        .length = htons(0)};

    int chunk_length = sizeof(header) + sizeof(break_option) + strlen(payload);

    char *raw_chunk = (char *)malloc(chunk_length);
    memcpy(raw_chunk, &header, (int)sizeof(header));
    memcpy(raw_chunk + sizeof(header), &break_option, (int)sizeof(break_option));
    memcpy(raw_chunk + sizeof(break_option) + sizeof(header), payload, strlen(payload));

    struct ff_request *request = ff_request_alloc();
    ff_request_parse_chunk(request, chunk_length, raw_chunk);

    TEST_ASSERT_EQUAL_MESSAGE(FF_VERSION_1, request->version, "Version check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_RECEIVED, request->state, "State check failed");
    TEST_ASSERT_EQUAL_MESSAGE(1234568, request->request_id, "Request ID check failed");
    TEST_ASSERT_EQUAL_MESSAGE(true, request->payload_contains_options, "Payload contains options check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(payload), request->received_length, "Received length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(payload), request->payload_length, "Payload length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->options_length, "Options length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(NULL, request->options, "Options check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->payload->offset, "Payload node offset check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(payload), request->payload->length, "Payload node length check failed");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(payload, request->payload->value, request->payload->length, "Payload node value check failed");

    ff_request_free(request);

    FREE(raw_chunk);
}

void test_request_vectorise_payload()
{
    char *chunk_1 = "chunk1";
    char *chunk_2 = "chunk2";

    char *full_payload = (char *)calloc(1, strlen(chunk_1) + strlen(chunk_2) + 1);
    strcat(full_payload, chunk_1);
    strcat(full_payload, chunk_2);

    struct ff_request *request = ff_request_alloc();
    request->payload_length = strlen(full_payload);
    request->payload = ff_request_payload_node_alloc();
    ff_request_payload_load_buff(request->payload, strlen(chunk_1), chunk_1);
    request->payload->length = strlen(chunk_1);

    request->payload->next = ff_request_payload_node_alloc();
    ff_request_payload_load_buff(request->payload->next, strlen(chunk_2), chunk_2);
    request->payload->next->offset = strlen(chunk_1);
    request->payload->next->length = strlen(chunk_2);

    ff_request_vectorise_payload(request);

    TEST_ASSERT_EQUAL_MESSAGE(strlen(full_payload), request->payload->length, "Payload node length check failed");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(full_payload, request->payload->value, strlen(full_payload), "Payload node value check failed");
    TEST_ASSERT_EQUAL_MESSAGE(NULL, request->payload->next, "Payload node next check failed");

    ff_request_free(request);

    FREE(full_payload);
}

void test_ff_request_parse_options_from_payload()
{
    // One option in request header (TIMESTAMP)
    // Followed by two options in payload (HTTPS, EOL)

    struct __raw_ff_request_option_header https_option = {
        .type = FF_REQUEST_OPTION_TYPE_HTTPS,
        .length = htons(1)};

    struct __raw_ff_request_option_header eol_option = {
        .type = FF_REQUEST_OPTION_TYPE_EOL,
        .length = htons(0)};

    char *payload = "payload";

    size_t payload_length = sizeof(https_option) + 1 + sizeof(eol_option) + strlen(payload);
    uint8_t *payload_with_options = calloc(1, payload_length);
    uint8_t *ptr = payload_with_options;
    memcpy(ptr, &https_option, sizeof(https_option));
    ptr += sizeof(https_option);
    *ptr++ = 1;
    memcpy(ptr, &eol_option, sizeof(eol_option));
    ptr += sizeof(eol_option);
    memcpy(ptr, payload, strlen(payload));
    ptr += strlen(payload);

    struct ff_request *request = ff_request_alloc();

    request->options_length = 1;
    request->options = calloc(1, sizeof(struct ff_request_option_node *) * 1);
    request->options[0] = ff_request_option_node_alloc();
    request->options[0]->type = FF_REQUEST_OPTION_TYPE_TIMESTAMP;
    request->payload_contains_options = true;

    request->payload_length = payload_length;
    request->payload = ff_request_payload_node_alloc();
    ff_request_payload_load_buff(request->payload, payload_length, payload_with_options);
    request->payload->length = payload_length;

    ff_request_parse_options_from_payload(request);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_PARSED_OPTIONS, request->state, "request state check failed");
    TEST_ASSERT_EQUAL_MESSAGE(false, request->payload_contains_options, "request contains options flag check failed");

    TEST_ASSERT_EQUAL_MESSAGE(2, request->options_length, "request options length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_OPTION_TYPE_TIMESTAMP, request->options[0]->type, "request option (1) type check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_OPTION_TYPE_HTTPS, request->options[1]->type, "request option (2) type check failed");

    TEST_ASSERT_EQUAL_MESSAGE(strlen(payload), request->payload->length, "Payload node length check failed");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(payload, request->payload->value, strlen(payload), "Payload node value check failed");
    TEST_ASSERT_EQUAL_MESSAGE(NULL, request->payload->next, "Payload node next check failed");

    ff_request_free(request);

    FREE(payload_with_options);
}

void test_request_parse_v1_single_chunk_with_options()
{
    char *http_request = "POST / HTTP/1.1\nHost: stackoverflow.com\n\nSome\nTest\nData";

    struct __raw_ff_request_header header = {
        .version = htons(FF_VERSION_1),
        .request_id = htonll(1234568ULL),
        .total_length = htonl(strlen(http_request)),
        .chunk_offset = htonl(0),
        .chunk_length = htons(strlen(http_request))};

    struct __raw_ff_request_option_header checksum_option = {
        .type = FF_REQUEST_OPTION_TYPE_ENCRYPTION_IV,
        .length = htons(3)};

    struct __raw_ff_request_option_header eol_option = {
        .type = FF_REQUEST_OPTION_TYPE_EOL,
        .length = htons(0)};

    int chunk_length = sizeof(header) + sizeof(checksum_option) + 3 + sizeof(eol_option) + strlen(http_request);

    void *raw_chunk = (char *)malloc(chunk_length);
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
    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_OPTION_TYPE_ENCRYPTION_IV, request->options[0]->type, "Option (1) type check failed");
    TEST_ASSERT_EQUAL_MESSAGE(3, request->options[0]->length, "Option (1) length check failed");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE("abc", request->options[0]->value, request->options[0]->length, "Option value check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->payload->offset, "Payload node offset check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(http_request), request->payload->length, "Payload node length check failed");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(http_request, request->payload->value, request->payload->length, "Payload node value check failed");

    ff_request_free(request);

    FREE(raw_chunk);
}

void test_ff_request_parse_id_raw_http()
{
    char *raw_http_request = "GET / HTTP/1.1\nHost: stackoverflow.com\n\n";

    uint64_t request_id = ff_request_parse_id(strlen(raw_http_request), raw_http_request);

    TEST_ASSERT_EQUAL_MESSAGE(0, request_id, "Request ID check failed");
}

void test_ff_request_parse_id()
{
    struct __raw_ff_request_header header = {
        .version = htons(FF_VERSION_1),
        .request_id = htonll(1234568ULL),
        .total_length = 0,
        .chunk_offset = 0,
        .chunk_length = 0};

    void *raw_chunk = (char *)malloc(sizeof(struct __raw_ff_request_header));
    void *chunk_ptr = raw_chunk;

    memcpy(chunk_ptr, &header, (int)sizeof(header));
    chunk_ptr += sizeof(header);

    uint64_t request_id = ff_request_parse_id(sizeof(struct __raw_ff_request_header), raw_chunk);

    TEST_ASSERT_EQUAL_MESSAGE(1234568, request_id, "Request ID check failed");

    FREE(raw_chunk);
}