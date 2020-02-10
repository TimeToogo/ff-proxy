#include <stdlib.h>
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

    free(request);
}

void test_request_free()
{
    struct ff_request *request = ff_request_alloc();

    struct ff_request_option_node* option_a = ff_request_option_node_alloc();
    struct ff_request_option_node* option_b = ff_request_option_node_alloc();
    option_a->next = option_b;

    struct ff_request_payload_node* node_a = ff_request_payload_node_alloc();
    struct ff_request_payload_node* node_b = ff_request_payload_node_alloc();
    node_a->next = node_b;

    request->options = option_a;
    request->payload = node_a;

    ff_request_free(request);
}