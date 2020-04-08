#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/unity.h"
#include "../../client/c/client_p.h"
#include "../../client/c/client.h"
#include "../../src/os/linux_endian.h"

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
            .ip_address = "127.0.0.1",
            .port = "8088",
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
            .ip_address = "127.0.0.1",
            .port = "8088",
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

void test_client_packetise_request_empty_request()
{
    struct ff_request *request = ff_request_alloc();
    uint16_t packet_count;

    struct ff_client_packet *packets = ff_client_packetise_request(request, &packet_count);

    TEST_ASSERT_EQUAL_MESSAGE(0, packet_count, "packet count check failed");

    ff_request_free(request);
    FREE(packets);
}

void test_client_packetise_request_single_packet()
{
    char payload[] = "hello world";

    struct ff_request *request = ff_request_alloc();

    request->options = (struct ff_request_option_node **)malloc(sizeof(struct ff_request_option_node *) * 1);
    request->options[0] = ff_request_option_node_alloc();
    request->options[0]->type = FF_REQUEST_OPTION_TYPE_EOL;
    request->options[0]->length = 0;
    request->options_length++;

    request->payload = ff_request_payload_node_alloc();
    ff_request_payload_load_buff(request->payload, strlen(payload), payload);
    request->payload->length = strlen(payload);
    request->payload_length = strlen(payload);

    uint16_t packet_count;

    struct ff_client_packet *packets = ff_client_packetise_request(request, &packet_count);

    TEST_ASSERT_EQUAL_MESSAGE(1, packet_count, "packet count check failed");
    TEST_ASSERT_EQUAL_MESSAGE(
        sizeof(struct __raw_ff_request_header) + request->options_length * sizeof(struct __raw_ff_request_option_header) + strlen(payload),
        packets[0].length,
        "payload length check failed");

    struct __raw_ff_request_header *packet_header = (struct __raw_ff_request_header *)packets[0].value;

    TEST_ASSERT_EQUAL_MESSAGE(FF_VERSION_1, ntohs(packet_header->version), "packet version check failed");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, ntohll(packet_header->request_id), "packet request id check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, ntohl(packet_header->chunk_offset), "packet chunk offset check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(payload), ntohs(packet_header->chunk_length), "packet chunk length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(strlen(payload), ntohl(packet_header->total_length), "packet total length check failed");

    struct __raw_ff_request_option_header *option_1 = (struct __raw_ff_request_option_header *)((void *)packet_header + sizeof(struct __raw_ff_request_header));

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_OPTION_TYPE_EOL, option_1->type, "option (1) type check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, ntohs(option_1->length), "option (1) type check failed");

    uint8_t *packet_payload = (uint8_t *)((void *)option_1 + sizeof(struct __raw_ff_request_option_header));

    TEST_ASSERT_EQUAL_STRING_LEN_MESSAGE(payload, (char *)packet_payload, strlen(payload), "packet payload check failed");

    ff_request_free(request);
    FREE(packets[0].value);
    FREE(packets);
}

void test_client_packetise_request_multiple_packets_with_option()
{
    uint8_t payload[2000] = {0};
    for (size_t i = 0; i < sizeof(payload); i++)
    {
        payload[i] = (uint8_t)(i % 255);
    }

    struct ff_request *request = ff_request_alloc();

    request->options = (struct ff_request_option_node **)malloc(sizeof(struct ff_request_option_node *) * 2);
    request->options[0] = ff_request_option_node_alloc();
    request->options[0]->type = FF_REQUEST_OPTION_TYPE_HTTPS;
    request->options[0]->length = 1;
    request->options[0]->value = malloc(1);
    request->options[0]->value[0] = 1;
    request->options_length++;

    request->options[1] = ff_request_option_node_alloc();
    request->options[1]->type = FF_REQUEST_OPTION_TYPE_EOL;
    request->options[1]->length = 0;
    request->options_length++;

    request->payload = ff_request_payload_node_alloc();
    ff_request_payload_load_buff(request->payload, sizeof(payload), payload);
    request->payload->length = sizeof(payload);
    request->payload_length = sizeof(payload);

    uint16_t packet_count;

    struct ff_client_packet *packets = ff_client_packetise_request(request, &packet_count);

    TEST_ASSERT_EQUAL_MESSAGE(2, packet_count, "packet count check failed");

    // Test packet 1
    TEST_ASSERT_EQUAL_MESSAGE(
        FF_CLIENT_MAX_PACKET_LENGTH,
        packets[0].length,
        "packet (1) payload length check failed");

    struct __raw_ff_request_header *p1_header = (struct __raw_ff_request_header *)packets[0].value;

    TEST_ASSERT_EQUAL_MESSAGE(FF_VERSION_1, ntohs(p1_header->version), "packet (1) version check failed");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, ntohll(p1_header->request_id), "packet (1) request id check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, ntohl(p1_header->chunk_offset), "packet (1) chunk offset check failed");
    TEST_ASSERT_EQUAL_MESSAGE(
        FF_CLIENT_MAX_PACKET_LENGTH - sizeof(struct __raw_ff_request_header) - 2 * sizeof(struct __raw_ff_request_option_header) - 1,
        ntohs(p1_header->chunk_length),
        "packet (1) chunk length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(sizeof(payload), ntohl(p1_header->total_length), "packet (1) total length check failed");

    struct __raw_ff_request_option_header *p1_option_1 = (struct __raw_ff_request_option_header *)((void *)p1_header + sizeof(struct __raw_ff_request_header));

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_OPTION_TYPE_HTTPS, p1_option_1->type, "packet (1) option (1) type check failed");
    TEST_ASSERT_EQUAL_MESSAGE(1, ntohs(p1_option_1->length), "packet (1) option (1) type check failed");
    TEST_ASSERT_EQUAL_MESSAGE(1, *((uint8_t *)p1_option_1 + sizeof(struct __raw_ff_request_option_header)), "packet (1) option (1) value check failed");

    struct __raw_ff_request_option_header *p1_option_2 = (struct __raw_ff_request_option_header *)((void *)p1_option_1 + sizeof(struct __raw_ff_request_option_header) + 1);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_OPTION_TYPE_EOL, p1_option_2->type, "packet (1) option (2) type check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, ntohs(p1_option_2->length), "packet (1) option (2) type check failed");

    uint8_t *p1_payload = (uint8_t *)((void *)p1_option_2 + sizeof(struct __raw_ff_request_option_header));

    TEST_ASSERT_EQUAL_CHAR_ARRAY_MESSAGE(payload, p1_payload, ntohs(p1_header->chunk_length), "packet (1) payload check failed");

    // Test packet 2
    TEST_ASSERT_EQUAL_MESSAGE(
        sizeof(struct __raw_ff_request_header) + sizeof(struct __raw_ff_request_option_header) + sizeof(payload) - ntohs(p1_header->chunk_length),
        packets[1].length,
        "packet (2) payload length check failed");

    struct __raw_ff_request_header *p2_header = (struct __raw_ff_request_header *)packets[1].value;

    TEST_ASSERT_EQUAL_MESSAGE(FF_VERSION_1, ntohs(p2_header->version), "packet (2) version check failed");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, ntohll(p2_header->request_id), "packet (2) request id check failed");
    TEST_ASSERT_EQUAL_MESSAGE(ntohs(p1_header->chunk_length), ntohl(p2_header->chunk_offset), "packet (2) chunk offset check failed");
    TEST_ASSERT_EQUAL_MESSAGE(
        sizeof(payload) - ntohs(p1_header->chunk_length),
        ntohs(p2_header->chunk_length),
        "packet (2) chunk length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(sizeof(payload), ntohl(p2_header->total_length), "packet (2) total length check failed");

    struct __raw_ff_request_option_header *p2_option_1 = (struct __raw_ff_request_option_header *)((void *)p2_header + sizeof(struct __raw_ff_request_header));

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_OPTION_TYPE_EOL, p2_option_1->type, "packet (2) option (1) type check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, ntohs(p2_option_1->length), "packet (2) option (1) type check failed");

    uint8_t *p2_payload = (uint8_t *)((void *)p2_option_1 + sizeof(struct __raw_ff_request_option_header));

    TEST_ASSERT_EQUAL_CHAR_ARRAY_MESSAGE(payload + ntohs(p1_header->chunk_length), p2_payload, ntohs(p2_header->chunk_length), "packet (2) payload check failed");

    TEST_ASSERT_EQUAL_MESSAGE(ntohll(p1_header->request_id), ntohll(p2_header->request_id), "request ids must match");

    ff_request_free(request);
    FREE(packets[0].value);
    FREE(packets[1].value);
    FREE(packets);
}

void test_client_make_request_http_and_encrypted()
{
    char contents[] = "hello world";
    char *tmp_file = "/tmp/ff_test_file";
    FILE *fd = fopen(tmp_file, "w+");
    fwrite(contents, sizeof(contents[0]), sizeof(contents) - 1, fd);
    fseek(fd, 0, SEEK_SET);

    struct ff_client_config *config = malloc(sizeof(struct ff_client_config));
    config->https = true;
    config->encryption.key = (uint8_t *)"test key";
    config->encryption.pbkdf2_iterations = 1000;
    config->ip_address = "127.0.0.1";
    config->port = "12345";
    config->logging_level = FF_DEBUG;

    int res = ff_client_make_request(config, fd);

    TEST_ASSERT_EQUAL_MESSAGE(0, res, "return value check failed");

    FREE(config);
    fclose(fd);
}
