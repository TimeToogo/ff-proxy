#include <stdlib.h>
#include <string.h>
#include "../include/unity.h"
#include "../../src/request.h"
#include "../../src/alloc.h"

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

    option->value = (uint8_t *)malloc(10);

    ff_request_option_node_free(option);
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

    node->value = (uint8_t *)malloc(10);
    next_node = node->next = (void *)malloc(10);

    ff_request_payload_node_free(node);

    free(next_node);
}

void test_request_alloc()
{
    struct ff_request *request = ff_request_alloc();

    TEST_ASSERT(request->state == FF_REQUEST_STATE_RECEIVING);
    TEST_ASSERT(request->version == 0);
    TEST_ASSERT(request->request_id == 0);
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

    request->options = malloc(sizeof(struct ff_request_option_node *) * 2);
    request->options_length = 2;
    request->options[0] = option_a;
    request->options[1] = option_b;
    request->payload = node_a;

    ff_request_free(request);
}
