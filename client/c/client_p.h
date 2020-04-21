#include "config.h"

#ifndef FF_CLIENT_P_H
#define FF_CLIENT_P_H

#define FF_CLIENT_READ_PAYLOAD_BUFFER_SIZE 1024
#define FF_CLIENT_MAX_PACKETS 1024
#define FF_CLIENT_MAX_PACKET_LENGTH 1300 // Based on typical PMTU of 1500

struct ff_client_packet
{
    uint8_t *value;
    uint16_t length;
};

uint8_t ff_client_create_payload_options(struct ff_request *request, struct ff_client_config *config);

void ff_client_request_add_payload(struct ff_request *request, struct ff_request_payload_node *node);

void ff_client_read_payload_from_file(struct ff_request *request, FILE *fd);

uint32_t ff_client_write_options(void *buffer, struct ff_request_option_node** options, uint8_t amount);

struct ff_client_packet *ff_client_packetise_request(struct ff_request *request, uint16_t *packet_count);

uint64_t ff_client_generate_request_id();

uint8_t ff_client_send_request(struct ff_client_config *config, struct ff_client_packet *packets, uint32_t packets_count);

#endif