#include <pthread.h>
#include "config.h"
#include "request.h"
#include "hash_table.h"
#include "parser.h"
#include "crypto.h"
#include "http.h"

#ifndef FF_SERVER_P_H
#define FF_SERVER_P_H

struct ff_process_request_args
{
    struct ff_config *config;
    struct ff_request *request;
    struct ff_hash_table *requests;
};

void ff_proxy_process_incoming_packet(
    struct ff_config *config,
    struct ff_hash_table *requests,
    struct sockaddr *src_address,
    void *packet_buff,
    int buff_len);

void ff_proxy_process_request(struct ff_process_request_args *args);

bool ff_proxy_validate_request_timestamp(struct ff_request *request, struct ff_config *config);

void ff_proxy_clean_up_old_requests_loop(struct ff_hash_table *requests);

#endif