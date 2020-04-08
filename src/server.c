#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <time.h>
#include "server.h"
#include "server_p.h"
#include "parser.h"
#include "http.h"
#include "logging.h"
#include "alloc.h"

#define FF_PROXY_BUFF_SIZE 2000 // Based on typical path MTU of 1500
#define FF_PROXY_CLEAN_INTERVAL_SECS 60
#define FF_PROXY_OLD_REQUEST_THRESHOLD_SECS 60

int ff_proxy_start(struct ff_config *config)
{
    ff_set_logging_level(config->logging_level);

    int err;
    int sockfd;
    int optval = 1;
    socklen_t optlen = sizeof(optval);
    struct addrinfo hints;
    struct addrinfo *res;
    char buffer[FF_PROXY_BUFF_SIZE];
    int recv_len;
    struct sockaddr_storage src_address;
    socklen_t src_address_length = sizeof(src_address);
    struct ff_hash_table *requests = ff_hash_table_init(16);

    pthread_t cleanup_thread;
    pthread_attr_t cleanup_thread_attrs;
    pthread_attr_init(&cleanup_thread_attrs);
    pthread_attr_setdetachstate(&cleanup_thread_attrs, PTHREAD_CREATE_DETACHED);
    pthread_create(&cleanup_thread, &cleanup_thread_attrs, (void *)ff_proxy_clean_up_old_requests_loop, (void *)requests);
    pthread_attr_destroy(&cleanup_thread_attrs);

    ff_log(FF_DEBUG, "Initialising OpenSSL");
    ff_init_openssl();
    ff_log(FF_DEBUG, "Initialised OpenSSL");

    ff_log(FF_INFO, "Starting UDP proxy on %s%s%s:%s",
           strchr(config->ip_address, ':') ? "[" : "", config->ip_address,
           strchr(config->ip_address, ':') ? "]" : "", config->port);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_NUMERICHOST | AI_NUMERICSERV | AI_PASSIVE;
    getaddrinfo(config->ip_address, config->port, &hints, &res);

    ff_log(FF_DEBUG, "Creating socket");
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd == -1)
    {
        ff_log(FF_FATAL, "Failed to create socket");
        return EXIT_FAILURE;
    }

    ff_log(FF_DEBUG, "Created socket");

    err = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, optlen);
    if (err)
    {
        ff_log(FF_WARNING, "Failed to set socket option SO_REUSEADDR (errno: %d)", errno);
    }

    if (res->ai_family == AF_INET6 && config->ipv6_v6only)
    {
        ff_log(FF_DEBUG, "Setting IPV6_V6ONLY socket option");
        err = setsockopt(sockfd, IPPROTO_IPV6, IPV6_V6ONLY, &optval, optlen);
        if (err)
        {
            ff_log(FF_WARNING, "Failed to set socket option IPV6_V6ONLY (errno: %d)", errno);
        }
    }

    ff_log(FF_DEBUG, "Binding to address");
    err = bind(sockfd, res->ai_addr, res->ai_addrlen);
    freeaddrinfo(res);
    
    if (err)
    {
        ff_log(FF_FATAL, "Failed to bind to address");
        return EXIT_FAILURE;
    }

    ff_log(FF_DEBUG, "Bound to socket");

    // A single packet at a time
    while ((recv_len = recvfrom(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&src_address, &src_address_length)))
    {
        char ip_string[INET6_ADDRSTRLEN];

        if (recv_len == -1)
        {
            ff_log(FF_FATAL, "Failed to read from socket");
            return EXIT_FAILURE;
        }

        getnameinfo((struct sockaddr *)&src_address, src_address_length, ip_string, sizeof(ip_string), NULL, 0, NI_NUMERICHOST);
        ff_log(FF_DEBUG, "Received packet of %d bytes from %s", recv_len, ip_string);

        ff_proxy_process_incoming_packet(config, requests, (struct sockaddr *)&src_address, buffer, recv_len);

        /* need to reset for subsequent recvfrom()'s */
        src_address_length = sizeof(src_address);
    }

    ff_hash_table_free(requests);

    return 0;
}

void ff_proxy_process_incoming_packet(struct ff_config *config, struct ff_hash_table *requests, struct sockaddr *src_address, void *packet_buff, int buff_len)
{
    bool is_raw_http = ff_request_is_raw_http(buff_len, packet_buff);
    uint64_t request_id = 0;
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
            time(&request->received_at);
            memcpy(&request->source, src_address, sizeof(struct sockaddr));
            ff_hash_table_put_item(requests, request_id, (void *)request);
        }

        if (memcmp(&request->source, src_address, sizeof(struct sockaddr)) != 0)
        {
            ff_log(FF_WARNING, "Incoming packet IP address does not match original source IP address/port for request %lu (will discard)", request->request_id);
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

        thread_args = malloc(sizeof(struct ff_process_request_args));
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

    ff_decrypt_request(request, &config->encryption);

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

void ff_proxy_clean_up_old_requests_loop(struct ff_hash_table *requests)
{
    while (1)
    {
        sleep(FF_PROXY_CLEAN_INTERVAL_SECS);

        ff_log(FF_DEBUG, "Cleaning up partially received requests");

        time_t now;
        time(&now);

        struct ff_hash_table_iterator *iterator = ff_hash_table_iterator_init(requests);
        struct ff_request *request = NULL;
        struct ff_request **requests_to_remove = calloc(1, requests->length * sizeof(struct ff_request *));
        uint32_t count = 0;

        while ((request = ff_hash_table_iterator_next(iterator)) != NULL)
        {
            bool has_expired = request->state == FF_REQUEST_STATE_RECEIVING &&
                               difftime(now, request->received_at) >= FF_PROXY_OLD_REQUEST_THRESHOLD_SECS;

            if (has_expired)
            {
                requests_to_remove[count++] = request;
            }
        }

        ff_hash_table_iterator_free(iterator);

        for (uint32_t i = 0; i < count; i++)
        {
            ff_hash_table_remove_item(requests, requests_to_remove[i]->request_id);
        }

        ff_log(count == 0 ? FF_DEBUG : FF_WARNING, "Cleaned up %u expired partial requests", count);
    }
}
