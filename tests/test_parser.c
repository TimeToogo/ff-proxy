#include <stdlib.h>
#include <string.h>
#include "include/unity.h"
#include "../src/parser.h"

void test_request_option_node_alloc()
{
    struct ff_request_option_node *option = ff_request_option_node_alloc();

    TEST_ASSERT(option->type == 0);
    TEST_ASSERT(option->length == 0);
    TEST_ASSERT(option->value == NULL);
    TEST_ASSERT(option->next == NULL);

    free(option);
}

void test_request_option_node_free()
{
    struct ff_request_option_node *option = ff_request_option_node_alloc();
    void *next_node;

    option->value = (char *)malloc(10);
    next_node = option->next = (void *)malloc(10);

    ff_request_option_node_free(option);

    TEST_ASSERT(option->value == NULL);
    TEST_ASSERT_MESSAGE(option->next == next_node, "Should not free next node");
    free(next_node);
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
    option_a->next = option_b;

    struct ff_request_payload_node *node_a = ff_request_payload_node_alloc();
    struct ff_request_payload_node *node_b = ff_request_payload_node_alloc();
    node_a->next = node_b;

    request->options = option_a;
    request->payload = node_a;

    ff_request_free(request);
}

void test_request_parse_raw_http()
{
    char *RAW_HTTP_REQUEST = "GET / HTTP/1.1\nHost: stackoverflow.com\n\n";

    struct ff_request *request = ff_request_alloc();
    ff_request_parse_chunk(request, strlen(RAW_HTTP_REQUEST), RAW_HTTP_REQUEST);

    TEST_ASSERT_EQUAL_MESSAGE(FF_VERSION_RAW, request->version, "Version check failed");
    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_RECEIVED, request->state, "State check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->request_id, "Request ID check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(RAW_HTTP_REQUEST), request->received_length, "Received length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(RAW_HTTP_REQUEST), request->payload_length, "Payload length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, request->payload->offset, "Payload node offset check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(RAW_HTTP_REQUEST), request->payload->length, "Payload node length check failed");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(RAW_HTTP_REQUEST, request->payload->value, "Payload node value check failed");

    ff_request_free(request);
}