#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <math.h>
#include "alloc.h"
#include "http.h"
#include "logging.h"

void ff_http_send_request(struct ff_request *request)
{
    bool https = false;

    for (int i = 0; i < request->options_length; i++)
    {
        switch (request->options[i]->type)
        {
        case FF_REQUEST_OPTION_TYPE_HTTPS:
            https = request->options[i]->length == 1 && request->options[i]->value[0] == '1';
            break;

        default:
            break;
        }
    }

    if (https)
    {
        ff_http_send_request_tls(request);
    }
    else
    {
        ff_http_send_request_unencrypted(request);
    }
}

void ff_http_send_request_unencrypted(struct ff_request *request)
{
    char *host_name = ff_http_get_destination_host(request);
    struct hostent *host_entry = NULL;
    struct sockaddr_in host_address;
    char formatted_address[INET6_ADDRSTRLEN] = {0};
    struct timeval timeout = {.tv_sec = 1, .tv_usec = 0};
    int sockfd = 0;
    int chunk = 0;
    int sent = 0;
    int received = 0;
    char response[FF_HTTP_RESPONSE_BUFF_SIZE] = {0};

    if (host_name == NULL)
    {
        goto error;
    }

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    if (sockfd < 0)
    {
        ff_log(FF_ERROR, "Failed to open socket");
        goto error;
    }
    
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout));

    host_entry = gethostbyname(host_name);

    if (host_entry == NULL)
    {
        ff_log(FF_WARNING, "Failed to perform DNS lookup for host: %s", host_name);
        goto error;
    }

    memset(&host_address, 0, sizeof(host_address));
    host_address.sin_family = AF_INET;
    host_address.sin_port = htons(80);
    memcpy(&host_address.sin_addr.s_addr, host_entry->h_addr_list[0], host_entry->h_length);

    inet_ntop(AF_INET, &host_address.sin_addr, formatted_address, INET_ADDRSTRLEN);
    ff_log(FF_DEBUG, "Resolved host %s to ip address %s", host_name, formatted_address);

    if (connect(sockfd, (struct sockaddr *)&host_address, sizeof(host_address)))
    {
        ff_log(FF_WARNING, "Failed to connect to host: %s", host_name);
        goto error;
    }

    do
    {
        chunk = write(sockfd, request->payload->value + sent, request->payload_length - sent);

        if (chunk < 0)
        {
            ff_log(FF_WARNING, "Failed to write to socket: %s (%d bytes remaining)", host_name, request->payload_length - sent);
            goto error;
        }

        if (chunk == 0)
        {
            break;
        }

        sent += chunk;
    } while (sent < request->payload_length);

    ff_log(FF_DEBUG, "Finished sending request to %s (%d bytes sent)", host_name, sent);

    do
    {
        chunk = read(sockfd, response + received, sizeof(response) - received - 1);

        if (chunk < 0)
        {
            ff_log(FF_WARNING, "Failed to read response from socket for host: %d (%d bytes received)", host_name, received);
            goto error;
        }

        if (chunk == 0)
        {
            break;
        }

        received += chunk;
    } while (received < sizeof(response) - 1);

    ff_log(FF_DEBUG, "Finished receiving response from %s (%d bytes received)", host_name, received);
    ff_log(FF_DEBUG, "Response: %.*s", (int)fminl(strchr((char *)response, '\n') - response, 100l), response);
    goto done;

error:
    request->state = FF_REQUEST_STATE_SENDING_FAILED;
    goto cleanup;

done:
    request->state = FF_REQUEST_STATE_SENT;
    goto cleanup;

cleanup:
    if (host_name != NULL)
    {
        FREE(host_name);
    }

    if (sockfd != 0) {
        close(sockfd);
    }

    return;
}

void ff_http_send_request_tls(struct ff_request *request)
{
}

char *ff_http_get_destination_host(struct ff_request *request)
{
    // @see https://stackoverflow.com/questions/8724954/what-is-the-maximum-number-of-characters-for-a-host-name-in-unix
    char *host_name = (char *)malloc(_POSIX_HOST_NAME_MAX + 1);
    char *http_request = (char *)request->payload->value;
    char *line = http_request;
    int i = 0;

#define FF_HTTP_DESTINATION_CONSUME_CHAR(x)                                                          \
    line += (x);                                                                                     \
    i += (x);                                                                                        \
    if (*line == '\0')                                                                               \
    {                                                                                                \
        ff_log(FF_WARNING, "Encountered end of request payload before finding Host header");         \
        goto error;                                                                                  \
    }                                                                                                \
    if (i >= FF_HTTP_HOST_HEADER_MAX_SEARCH_LENGTH)                                                  \
    {                                                                                                \
        ff_log(FF_WARNING, "Reached char limit of request payload while searching for host header"); \
        goto error;                                                                                  \
    }

    while (1)
    {
        if (line != http_request && *line == '\n')
        {
            // Subsequent new lines indicate request headers have finished
            ff_log(FF_WARNING, "Encountered end of request headers before finding Host header");
            goto error;
        }

        if (strncasecmp(line, "host:", 5) == 0)
        {
            FF_HTTP_DESTINATION_CONSUME_CHAR(5);

            while (*line == ' ')
            {
                FF_HTTP_DESTINATION_CONSUME_CHAR(1);
            }

            int host_len = 0;

            // http://man7.org/linux/man-pages/man7/hostname.7.html
            while ((*line == '.' || *line == '-' || isalnum(*line)) && host_len <= 255)
            {
                *(host_name + host_len) = *line;
                line++;
                host_len++;
            }

            *(host_name + host_len + 1) = '\0';
            goto done;
        }

        // Advance to next line
        while (*line != '\n')
        {
            FF_HTTP_DESTINATION_CONSUME_CHAR(1);
        }

        FF_HTTP_DESTINATION_CONSUME_CHAR(1);
    }

error:
    FREE(host_name);
    return NULL;

done:
    ff_log(FF_DEBUG, "Found destination host: %s", host_name);
    return host_name;
}
