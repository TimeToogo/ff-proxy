#include "config.h"

#ifndef FF_CLIENT_P_H
#define FF_CLIENT_P_H

struct ff_client_packet
{
    uint8_t *value;
    uint16_t length;
};

void ff_client_read_payload_from_file(struct ff_request *request, FILE *fd);

struct ff_client_packet *ff_client_serialize_request(struct ff_request *request, uint16_t *packet_count);

uint64_t ff_client_generate_request_id();

uint8_t ff_client_send_request(struct ff_client_config *config, struct ff_client_packet *packets, uint32_t packets_count);

#endif