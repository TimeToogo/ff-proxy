#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/unity.h"
#include "../../client/c/client_p.h"
#include "../../client/c/client.h"

void test_client_generate_request_id()
{
    uint64_t request_id = ff_client_generate_request_id();
    uint64_t request_id2 = ff_client_generate_request_id();

    TEST_ASSERT_NOT_EQUAL_MESSAGE(request_id, request_id2, "request ids must be random");
}

void test_client_send_request_no_packets()
{
    struct ff_client_packet packets[0];

    struct ff_client_config config =
        {
            .ip_address = {.s_addr = htonl(INADDR_LOOPBACK)},
            .port = 8088,
        };

    uint8_t res = ff_client_send_request(&config, packets, sizeof(packets) / sizeof(packets[0]));

    TEST_ASSERT_EQUAL_MESSAGE(0, res, "return value check failed");
}

void test_client_send_request_two_packets()
{
    uint8_t packet_1_value[] = {1, 2, 3, 4};
    uint8_t packet_2_value[] = {200, 201, 203, 204, 205};

    struct ff_client_packet packets[] = {
        {.length = sizeof(packet_1_value), .value = (uint8_t *)&packet_1_value},
        {.length = sizeof(packet_2_value), .value = (uint8_t *)&packet_2_value},
    };

    struct ff_client_config config =
        {
            .ip_address = {.s_addr = htonl(INADDR_LOOPBACK)},
            .port = 8088,
        };

    uint8_t res = ff_client_send_request(&config, packets, sizeof(packets) / sizeof(packets[0]));

    TEST_ASSERT_EQUAL_MESSAGE(0, res, "return value check failed");
}

void test_client_read_payload_from_file()
{
    char contents[] = "hello world";
    char *tmp_file = "/tmp/ff_test_file";
    FILE *fd = fopen(tmp_file, "w+");
    fwrite(contents, sizeof(contents[0]), sizeof(contents) - 1, fd);
    fseek(fd, 0, SEEK_SET);

    struct ff_request *request = ff_request_alloc();

    ff_client_read_payload_from_file(request, fd);

    TEST_ASSERT_EQUAL_MESSAGE(strlen(contents), request->payload_length, "payload_length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(contents), request->payload->length, "payload->length check failed");
    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(contents, request->payload->value, strlen(contents), "payload value check failed");

    ff_request_free(request);
    fclose(fd);
}