#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/rand.h>
#include <errno.h>
#include "client.h"
#include "client_p.h"
#include "config.h"
#include "request.h"
#include "crypto.h"
#include "alloc.h"
#include "crypto.h"
#include "../../src/os/linux_endian.h"

int ff_client_make_request(struct ff_client_config *config, FILE *fd)
{
    ff_log(FF_DEBUG, "Initialising OpenSSL");
    ff_init_openssl();
    ff_log(FF_DEBUG, "Initialised OpenSSL");

    int ret_val = 0;

    struct ff_request *request = ff_request_alloc();
    request->options = malloc(sizeof(struct ff_request_option_node *) * FF_REQUEST_MAX_OPTIONS);

    uint16_t packet_count = 0;
    struct ff_client_packet *packets = NULL;

    uint8_t options_in_payload = ff_client_create_payload_options(request, config);

    ff_client_read_payload_from_file(request, fd);

    ff_request_vectorise_payload(request);

    if (config->encryption.key != NULL)
    {
        if (!ff_client_encrypt_request(request, &config->encryption))
        {
            ff_log(FF_ERROR, "Failed to encrypt payload");
            goto error;
        }
        ff_log(FF_DEBUG, "Encrypted payload using pre-shared key (ciphertext length: %u)", request->payload_length);
    }

    request->options[request->options_length] = ff_request_option_node_alloc();
    request->options[request->options_length]->type = options_in_payload == 0 ? FF_REQUEST_OPTION_TYPE_EOL : FF_REQUEST_OPTION_TYPE_BREAK;
    request->options_length++;

    packets = ff_client_packetise_request(request, &packet_count);

    if (!ff_client_send_request(config, packets, packet_count))
    {
        goto done;
    }
    else
    {
        goto error;
    }

done:
    ret_val = 0;
    goto cleanup;

error:
    ret_val = 1;
    goto cleanup;

cleanup:
    ff_request_free(request);

    for (uint16_t i = 0; i < packet_count; i++)
    {
        FREE(packets[i].value);
    }

    FREE(packets);

    return ret_val;
}

uint8_t ff_client_create_payload_options(struct ff_request *request, struct ff_client_config *config)
{
    uint64_t now = (uint64_t)time(NULL);
    now = htonll(now);

    struct ff_request_payload_node *payload = NULL;
    struct ff_request_option_node **options = malloc(sizeof(struct ff_request_option_node *) * FF_REQUEST_MAX_OPTIONS);
    uint8_t option_i = 0;
    uint16_t length = 0;

    if (config->https)
    {
        options[option_i] = ff_request_option_node_alloc();
        options[option_i]->type = FF_REQUEST_OPTION_TYPE_HTTPS;
        options[option_i]->length = 1;
        options[option_i]->value = malloc(1);
        options[option_i]->value[0] = 1;
        length += sizeof(struct __raw_ff_request_option_header) + options[option_i]->length;
        option_i++;
    }

    options[option_i] = ff_request_option_node_alloc();
    options[option_i]->type = FF_REQUEST_OPTION_TYPE_TIMESTAMP;
    options[option_i]->length = 8;
    options[option_i]->value = malloc(8);
    memcpy(options[option_i]->value, &now, 8);
    length += sizeof(struct __raw_ff_request_option_header) + options[option_i]->length;
    option_i++;

    options[option_i] = ff_request_option_node_alloc();
    options[option_i]->type = FF_REQUEST_OPTION_TYPE_EOL;
    length += sizeof(struct __raw_ff_request_option_header) + options[option_i]->length;
    option_i++;

    payload = ff_request_payload_node_alloc();
    payload->value = malloc(length);
    payload->length = length;

    ff_client_write_options(payload->value, options, option_i);

    ff_client_request_add_payload(request, payload);

    for (uint8_t i = 0; i < option_i; i++)
    {
        ff_request_option_node_free(options[i]);
    }

    FREE(options);

    return option_i;
}

void ff_client_request_add_payload(struct ff_request *request, struct ff_request_payload_node *node)
{
    struct ff_request_payload_node *last_node = request->payload, *next_node = NULL;
    while (next_node != NULL)
    {
        last_node = next_node;
        next_node = node->next;
    }

    if (last_node == NULL)
    {
        request->payload = node;
    }
    else
    {
        last_node->next = node;
        node->offset = last_node->offset + last_node->length;
    }

    request->payload_length += node->length;
}

void ff_client_read_payload_from_file(struct ff_request *request, FILE *fd)
{
    uint8_t buffer[1024];
    uint16_t payload_length = 0;
    uint16_t chunk_length = 0;

    struct ff_request_payload_node *payload = ff_request_payload_node_alloc();
    payload->value = malloc(sizeof(buffer));

    while ((chunk_length = fread(buffer, 1, sizeof(buffer), fd)) != 0)
    {
        payload_length += chunk_length;
        payload->value = realloc(payload->value, payload_length);

        if (payload->value == NULL)
        {
            ff_log(FF_FATAL, "Could not reallocate payload buffer");
            exit(1);
        }

        memcpy(payload->value, buffer, chunk_length);
    }

    ff_log(FF_DEBUG, "Read %u bytes from STDIN", payload_length);

    payload->length = payload_length;
    ff_client_request_add_payload(request, payload);
}

struct ff_client_packet *ff_client_packetise_request(struct ff_request *request, uint16_t *packet_count)
{
    struct ff_client_packet *packets = calloc(1, sizeof(struct ff_client_packet *) * FF_CLIENT_MAX_PACKETS);
    uint64_t request_id = ff_client_generate_request_id();
    uint32_t chunk_offset = 0;
    uint16_t bytes_left = request->payload_length;

    *packet_count = 0;

    while (bytes_left > 0)
    {
        uint8_t *buffer = calloc(1, FF_CLIENT_MAX_PACKET_LENGTH);
        uint16_t packet_length = 0;
        struct __raw_ff_request_header *header = (struct __raw_ff_request_header *)buffer;

        header->version = htons(FF_VERSION_1);
        header->request_id = htonll(request_id);
        header->total_length = htonl(request->payload_length);
        header->chunk_offset = htonl(chunk_offset);
        packet_length += sizeof(struct __raw_ff_request_header);

        if (*packet_count == 0)
        {
            // If first packet, send options
            packet_length += ff_client_write_options(buffer + packet_length, request->options, request->options_length);
        }
        else
        {
            // If subsequent packet provide empty option list
            struct __raw_ff_request_option_header *option_header = (struct __raw_ff_request_option_header *)(buffer + packet_length);
            option_header->type = FF_REQUEST_OPTION_TYPE_EOL;
            option_header->length = 0;
            packet_length += sizeof(struct __raw_ff_request_option_header);
        }

        uint16_t bytes_left_in_packet = FF_CLIENT_MAX_PACKET_LENGTH - packet_length;
        uint16_t chunk_length = bytes_left > bytes_left_in_packet ? bytes_left_in_packet : bytes_left;

        header->chunk_length = htons(chunk_length);

        memcpy(buffer + packet_length, request->payload->value + chunk_offset, chunk_length);
        packet_length += chunk_length;
        bytes_left -= chunk_length;
        chunk_offset += chunk_length;

        packets[*packet_count].length = packet_length;
        packets[*packet_count].value = buffer;
        (*packet_count)++;
    }

    ff_log(FF_DEBUG, "Packetised payload into %u packets", *packet_count);

    return packets;
}

uint32_t ff_client_write_options(void *buffer, struct ff_request_option_node **options, uint8_t amount)
{
    uint32_t buff_i = 0;

    for (uint8_t i = 0; i < amount; i++)
    {
        struct __raw_ff_request_option_header *option_header = (struct __raw_ff_request_option_header *)(buffer + buff_i);
        option_header->type = options[i]->type;
        option_header->length = htons(options[i]->length);
        buff_i += sizeof(struct __raw_ff_request_option_header);
        memcpy(buffer + buff_i, options[i]->value, options[i]->length);
        buff_i += options[i]->length;
    }

    return buff_i;
}

uint32_t ff_client_calculate_request_size(struct ff_request *request)
{
    uint32_t length = 0;

    length += sizeof(struct __raw_ff_request_header);

    for (int i = 0; i < request->options_length; i++)
    {
        length += sizeof(struct __raw_ff_request_option_header);
        length += request->options[i]->length;
    }

    length += request->payload_length;

    return length;
}

uint64_t ff_client_generate_request_id()
{
    uint8_t buffer[8];

    if (!RAND_bytes(buffer, sizeof(buffer)))
    {
        ff_log(FF_FATAL, "Failed to generate request ID");
        exit(EXIT_FAILURE);
    }

    return (uint64_t)(
        (uint64_t)buffer[0] | ((uint64_t)buffer[1] << 8) | ((uint64_t)buffer[2] << 16) | ((uint64_t)buffer[3] << 24) | ((uint64_t)buffer[4] << 32) | ((uint64_t)buffer[5] << 40) | ((uint64_t)buffer[6] << 48) | ((uint64_t)buffer[7] << 56));
}

uint8_t ff_client_send_request(struct ff_client_config *config, struct ff_client_packet *packets, uint32_t packets_count)
{
    uint8_t ret;
    int err;
    int sockfd = 0;
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    int sent_length = 0;
    int chunk_length = 0;
    char ip_string[INET6_ADDRSTRLEN + 1] = {0};

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    err = getaddrinfo(config->ip_address, config->port, &hints, &res);
    if (err)
    {
        ff_log(FF_FATAL, "Failed to perform DNS lookup for host: %s", config->ip_address);
        goto error;
    }

    ff_log(FF_DEBUG, "Creating socket");
    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

    if (sockfd < 0)
    {
        ff_log(FF_FATAL, "Failed to create socket");
        goto error;
    }

    if (res->ai_family == AF_INET)
    {
        inet_ntop(AF_INET, &((struct sockaddr_in *)res->ai_addr)->sin_addr, ip_string, INET_ADDRSTRLEN);
    }
    else
    {
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr, ip_string, INET6_ADDRSTRLEN);
    }
    ff_log(FF_INFO, "Sending request to %s%s%s:%s",
           strchr(ip_string, ':') ? "[" : "", ip_string, strchr(ip_string, ':') ? "]" : "", config->port);

    int flag = true;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)) < 0)
    {
        ff_log(FF_WARNING, "Failed to set socket option (errno: %d)", errno);
    }


    for (uint16_t i = 0; i < packets_count; i++)
    {
        do
        {
            ff_log(FF_INFO, "test: %d", packets_count);
            chunk_length = sendto(sockfd, packets[i].value + sent_length, packets[i].length, 0, res->ai_addr, res->ai_addrlen);

            if (chunk_length <= 0)
            {
                printf("errno: %d\n", errno);
                ff_log(FF_FATAL, "Failed to send UDP datagrams during byte range %d - %hu", sent_length, packets[i].length);
                goto error;
            }

            sent_length += chunk_length;
        } while (sent_length < packets[i].length);
    }

    ff_log(FF_DEBUG, "Finished sending %d bytes", sent_length);
    goto done;

done:
    ret = 0;
    goto cleanup;

error:
    ret = EXIT_FAILURE;
    goto cleanup;

cleanup:
    if (res != NULL)
    {
        freeaddrinfo(res);
    }

    if (close(sockfd))
    {
        ff_log(FF_WARNING, "Failed to close socket");
    }

    return ret;
}
