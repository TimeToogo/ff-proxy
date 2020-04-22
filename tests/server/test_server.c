#include <stdlib.h>
#include <string.h>
#include "../include/unity.h"
#include "../../src/server.h"
#include "../../src/server_p.h"

void test_validate_request_timestamp_valid()
{
    uint64_t now = (uint64_t)time(NULL);
    now = htonll(now);

    struct ff_request *request = ff_request_alloc();

    request->options_length++;
    request->options = (struct ff_request_option_node **)malloc(sizeof(struct ff_request_option_node *));
    request->options[0] = ff_request_option_node_alloc();
    request->options[0]->type = FF_REQUEST_OPTION_TYPE_TIMESTAMP;
    request->options[0]->length = 8;
    request->options[0]->value = malloc(8);
    memcpy(request->options[0]->value, &now, 8);

    struct ff_config config = {.timestamp_fudge_factor = 1};

    bool result = ff_proxy_validate_request_timestamp(request, &config);

    TEST_ASSERT_EQUAL_MESSAGE(true, result, "return value check failed");

    ff_request_free(request); 
}

void test_validate_request_timestamp_invalid()
{
    // Should fail since offset 5 is greater than fudge factor 1
    uint64_t now = (uint64_t)time(NULL) - 5;

    struct ff_request *request = ff_request_alloc();

    request->options_length++;
    request->options = (struct ff_request_option_node **)malloc(sizeof(struct ff_request_option_node *));
    request->options[0] = ff_request_option_node_alloc();
    request->options[0]->type = FF_REQUEST_OPTION_TYPE_TIMESTAMP;
    request->options[0]->length = 8;
    request->options[0]->value = malloc(8);
    memcpy(request->options[0]->value, &now, 8);

    struct ff_config config = {.timestamp_fudge_factor = 1};

    bool result = ff_proxy_validate_request_timestamp(request, &config);

    TEST_ASSERT_EQUAL_MESSAGE(false, result, "return value check failed");

    ff_request_free(request); 
}