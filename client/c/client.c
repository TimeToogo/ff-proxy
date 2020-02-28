#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
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

#define FF_CLIENT_READ_PAYLOAD_BUFFER_SIZE 1024
#define FF_CLIENT_MAX_PACKETS 1024
#define FF_CLIENT_MAX_PACKET_LENGTH 1300 // Based on typical PMTU of 1500

int ff_client_make_request(struct ff_client_config *config, FILE *fd)
{
    ff_log(FF_DEBUG, "Initialising OpenSSL");
    ff_init_openssl();
    ff_log(FF_DEBUG, "Initialised OpenSSL");

    struct ff_request *request = ff_request_alloc();
    request->options = malloc(sizeof(struct ff_request_option_node *) * FF_REQUEST_MAX_OPTIONS);
    request->payload = ff_request_payload_node_alloc();

    ff_client_read_payload_from_file(request, fd);

    if (config->encryption_key.key != NULL)
    {
        ff_client_encrypt_request(request, &config->encryption_key);
        ff_log(FF_DEBUG, "Encrypted payload using pre-shared key");
    }

    request->options[request->options_length] = ff_request_option_node_alloc();
    request->options[request->options_length]->type = FF_REQUEST_OPTION_TYPE_EOL;
    request->options_length++;

    uint16_t packet_count;
    struct ff_client_packet *packets = ff_client_serialize_request(request, &packet_count);

    int ret_val = ff_client_send_request(config, packets, packet_count);

    ff_request_free(request);
    FREE(packets);

    return ret_val;
}

void ff_client_read_payload_from_file(struct ff_request *request, FILE *fd)
{
    uint8_t buffer[1024];
    uint16_t payload_length = 0;
    uint16_t chunk_length = 0;

    struct ff_request_payload_node *node, *temp_node;
    node = request->payload;
    while (node != NULL)
    {
        temp_node = node;
        node = node->next;
        ff_request_payload_node_free(node);
    }

    request->payload = ff_request_payload_node_alloc();
    request->payload->value = (uint8_t *)malloc(sizeof(buffer));

    while ((chunk_length = fread(buffer, 1, sizeof(buffer), fd)) != 0)
    {
        payload_length += chunk_length;
        request->payload->value = realloc(request->payload->value, payload_length);

        if (request->payload->value == NULL)
        {
            ff_log(FF_FATAL, "Could not reallocate payload buffer");
            exit(1);
        }

        memcpy(request->payload->value, buffer, chunk_length);
    }

    ff_log(FF_DEBUG, "Read %u bytes from STDIN", payload_length);
    request->payload_length = payload_length;
    request->payload->offset = 0;
    request->payload->length = payload_length;
}

struct ff_client_packet *ff_client_serialize_request(struct ff_request *request, uint16_t *packet_count)
{
    struct ff_client_packet *packets = (struct ff_client_packet *)malloc(sizeof(struct ff_client_packet *) * FF_CLIENT_MAX_PACKETS);
    uint64_t request_id = ff_client_generate_request_id();
    uint32_t chunk_offset = 0;

    *packet_count = 0;

    while (1)
    {
        uint8_t *buffer = (uint8_t *)calloc(1, FF_CLIENT_MAX_PACKET_LENGTH);
        uint16_t packet_length = 0;
        struct __raw_ff_request_header *header = (struct __raw_ff_request_header *)buffer;

        header->version = FF_VERSION_1;
        header->request_id = request_id;
        header->total_length = request->payload_length;
        header->total_length = request->payload_length;
        header->chunk_offset = chunk_offset;
        packet_length += sizeof(struct __raw_ff_request_header);

        if (*packet_count == 0)
        {
            // If first packet, send options
            for (uint8_t i = 0; i < request->options_length; i++)
            {
                struct __raw_ff_request_option_header *option_header = (struct __raw_ff_request_option_header *)(buffer + packet_length);
                option_header->type = request->options[i]->type;
                option_header->length = request->options[i]->length;
                packet_length += sizeof(struct __raw_ff_request_option_header);
                memcpy(buffer + packet_length, request->options[i]->value, request->options[i]->length);
                packet_length += request->options[i]->length;
            }
        }
        else
        {
            // If subsequent packet provide empty option list
            struct __raw_ff_request_option_header *option_header = (struct __raw_ff_request_option_header *)(buffer + packet_length);
            option_header->type = FF_REQUEST_OPTION_TYPE_EOL;
            option_header->length = 0;
            packet_length += sizeof(struct __raw_ff_request_option_header);
        }

        uint16_t bytes_left = request->payload_length - chunk_offset;
        uint16_t bytes_left_in_packet = FF_CLIENT_MAX_PACKET_LENGTH - packet_length;

        header->chunk_length = bytes_left > bytes_left_in_packet ? bytes_left_in_packet : bytes_left;

        memcpy(buffer + packet_length, request->payload->value + header->chunk_offset, header->chunk_length);
        packet_length += header->chunk_length;
        (*packet_count)++;
    }

    ff_log(FF_DEBUG, "Serialized payload in %u packets", *packet_count);

    return packets;
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
    int sockfd;
    struct sockaddr_in bind_address;

    int sent_length = 0;
    int chunk_length = 0;

    char ip_string[INET6_ADDRSTRLEN + 1] = {0};

    ff_log(FF_DEBUG, "Creating socket");
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    if (sockfd == 0)
    {
        ff_log(FF_FATAL, "Failed to create socket");
        goto error;
    }

    inet_ntop(AF_INET, &config->ip_address, ip_string, sizeof(ip_string));
    ff_log(FF_INFO, "Sending request to %.16s:%d", ip_string, config->port);

    ff_log(FF_DEBUG, "Binding to address");
    bind_address.sin_family = AF_INET;
    bind_address.sin_addr = config->ip_address;
    bind_address.sin_port = htons(config->port);

    bool flag = true;
    if (!setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag)))
    {
        ff_log(FF_WARNING, "Failed to set socket option");
    }

    if (bind(sockfd, (struct sockaddr *)&bind_address, sizeof(bind_address)))
    {
        ff_log(FF_FATAL, "Failed to bind to address");
        goto error;
    }

    for (uint16_t i = 0; i < packets_count; i++)
    {
        do
        {
            chunk_length = sendto(sockfd, packets[i].value + sent_length, packets[i].length, 0, (struct sockaddr *)&bind_address, sizeof(bind_address));

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
    if (close(sockfd))
    {
        ff_log(FF_WARNING, "Failed to close socket");
    }

    return ret;
}