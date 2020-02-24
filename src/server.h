#include "config.h"
#include "request.h"
#include "hash_table.h"
#include "parser.h"
#include "crypto.h"
#include "http.h"

#ifndef FF_SERVER_H
#define FF_SERVER_H

int ff_proxy_start(struct ff_config *config);

void ff_proxy_process_incoming_packet(struct ff_config *config, struct ff_hash_table *requests, struct sockaddr *src_address, void *packet_buff, int buff_len);

void ff_proxy_process_request(struct ff_config *config, struct ff_request *request, struct ff_hash_table *requests);

#endif