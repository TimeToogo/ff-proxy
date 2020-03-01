#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include "server.h"
#include "server_p.h"
#include "parser.h"
#include "http.h"
#include "logging.h"
#include "alloc.h"

#define BUFF_SIZE 2000 // Based on typical path MTU of 1500

int ff_proxy_start(struct ff_config *config)
{
    int sockfd;
    struct sockaddr_in bind_address;

    char buffer[BUFF_SIZE];
    int recv_len;
    struct sockaddr_storage src_address;
    socklen_t src_address_length;

    char ip_string[INET6_ADDRSTRLEN + 1] = {0};

    struct ff_hash_table *requests = ff_hash_table_init(16);

    ff_set_logging_level(config->logging_level);

    ff_log(FF_DEBUG, "Initialising OpenSSL");
    ff_init_openssl();
    ff_log(FF_DEBUG, "Initialised OpenSSL");

    inet_ntop(AF_INET, &config->ip_address, ip_string, sizeof(ip_string));
    ff_log(FF_INFO, "Starting UDP proxy on %.16s:%d", ip_string, config->port);

    ff_log(FF_DEBUG, "Creating socket");
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == 0)
    {
        ff_log(FF_FATAL, "Failed to create socket");
        return EXIT_FAILURE;
    }

    ff_log(FF_DEBUG, "Created socket");

    ff_log(FF_DEBUG, "Binding to address");
    bind_address.sin_family = AF_INET;
    bind_address.sin_addr = config->ip_address;
    bind_address.sin_port = htons(config->port);

    int flag = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0)
    {
        ff_log(FF_WARNING, "Failed to set socket option (errno: %d)", errno);
    }

    if (bind(sockfd, (struct sockaddr *)&bind_address, sizeof(bind_address)))
    {
        ff_log(FF_FATAL, "Failed to bind to address");
        return EXIT_FAILURE;
    }

    ff_log(FF_DEBUG, "Bound to socket");

    // A single packet at a time
    while ((recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&src_address, &src_address_length)))
    {
        if (recv_len == -1)
        {
            ff_log(FF_FATAL, "Failed to read from socket");
            return EXIT_FAILURE;
        }

        memset(&ip_string, 0, sizeof(ip_string));
        getnameinfo((struct sockaddr *)&src_address, src_address_length, ip_string, sizeof(ip_string), NULL, 0, NI_NUMERICHOST);
        ff_log(FF_DEBUG, "Received packet of %d bytes from %s", recv_len, ip_string);

        ff_proxy_process_incoming_packet(config, requests, (struct sockaddr *)&src_address, buffer, recv_len);
    }

    ff_hash_table_free(requests);

    return 0;
}

void ff_proxy_process_incoming_packet(struct ff_config *config, struct ff_hash_table *requests, struct sockaddr *src_address, void *packet_buff, int buff_len)
{
    bool is_raw_http = ff_request_is_raw_http(buff_len, packet_buff);
    uint64_t request_id;
    struct ff_request *request;
    pthread_t thread;
    pthread_attr_t thread_attrs;
    struct ff_process_request_args *thread_args;

    if (is_raw_http)
    {
        ff_log(FF_DEBUG, "Incoming packet is raw HTTP request");
        request = ff_request_alloc();
        ff_request_parse_chunk(request, buff_len, packet_buff);
    }
    else
    {
        ff_log(FF_DEBUG, "Parsing incoming packet");
        request_id = ff_request_parse_id(buff_len, packet_buff);

        if (request_id == 0)
        {
            ff_log(FF_DEBUG, "Could not parse request ID from incoming packet");
            goto done;
        }

        request = (struct ff_request *)ff_hash_table_get_item(requests, request_id);

        if (request == NULL)
        {
            request = ff_request_alloc();
            memcpy(&request->source, src_address, sizeof(struct sockaddr));
            ff_hash_table_put_item(requests, request_id, (void *)request);
        }

        if (memcmp(&request->source, src_address, sizeof(struct sockaddr)) != 0)
        {
            ff_log(FF_WARNING, "Incoming packet IP address does not match original source IP address for request %s (will discard)", request->request_id);
            goto done;
        }

        ff_request_parse_chunk(request, buff_len, packet_buff);
    }

    switch (request->state)
    {
    case FF_REQUEST_STATE_RECEIVING:
        break;

    case FF_REQUEST_STATE_RECEIVING_FAIL:
        if (request_id != 0)
        {
            ff_hash_table_remove_item(requests, request_id);
        }
        ff_request_free(request);
        break;

    case FF_REQUEST_STATE_RECEIVED:
        ff_log(FF_DEBUG, "Finished receiving incoming request");

        thread_args = (struct ff_process_request_args *)malloc(sizeof(struct ff_process_request_args));
        thread_args->config = config;
        thread_args->request = request;
        thread_args->requests = requests;

        pthread_attr_init(&thread_attrs);
        pthread_attr_setdetachstate(&thread_attrs, PTHREAD_CREATE_DETACHED);

        pthread_create(&thread, &thread_attrs, (void *)ff_proxy_process_request, (void *)thread_args);

        pthread_attr_destroy(&thread_attrs);
        break;
    default:
        ff_log(FF_ERROR, "Encountered invalid request state: %d", request->state);
        break;
    }

done:
    return;
}

void ff_proxy_process_request(struct ff_process_request_args *args)
{
    struct ff_config *config = args->config;
    struct ff_request *request = args->request;
    struct ff_hash_table *requests = args->requests;

    ff_request_vectorise_payload(request);

    ff_decrypt_request(request, &config->encryption_key);

    if (request->state != FF_REQUEST_STATE_DECRYPTED)
    {
        goto error;
    }

    ff_http_send_request(request);

    if (request->state != FF_REQUEST_STATE_SENT)
    {
        goto error;
    }

    goto done;

error:
    ff_log(FF_DEBUG, "Failed to process request %lu", request->request_id);
    goto cleanup;

done:
    ff_log(FF_DEBUG, "Successfully processed request %lu", request->request_id);
    goto cleanup;

cleanup:
    if (request->request_id != 0)
    {
        ff_hash_table_remove_item(requests, request->request_id);
    }

    ff_request_free(request);
    FREE(args);
}