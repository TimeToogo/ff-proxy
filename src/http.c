#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <math.h>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include "alloc.h"
#include "http.h"
#include "http_p.h"
#include "logging.h"

void ff_http_send_request(struct ff_request *request)
{
    bool https = false;

    // Read options backwards to ensure the encrypted options take precedence
    uint8_t i = request->options_length;
    while (i-- > 0)
    {
        switch (request->options[i]->type)
        {
        case FF_REQUEST_OPTION_TYPE_HTTPS:
            https = request->options[i]->length == 1 && request->options[i]->value[0] == 1;
            break;

        default:
            break;
        }
    }

    char *host_name = ff_http_get_destination_host(request);
    bool success = false;

    if (host_name == NULL)
    {
        goto error;
    }

    if (https)
    {
        success = ff_http_send_request_tls(request, host_name);
    }
    else
    {
        success = ff_http_send_request_unencrypted(request, host_name);
    }

    if (success)
    {
        goto done;
    }
    else
    {
        goto error;
    }

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

    return;
}

bool ff_http_send_request_unencrypted(struct ff_request *request, char *host_name)
{
    bool ret;
    struct addrinfo hints;
    struct addrinfo *res = NULL;
    char formatted_address[INET6_ADDRSTRLEN] = {0};
    struct timeval timeout = {.tv_sec = FF_HTTP_RESPONSE_MAX_WAIT_SECS, .tv_usec = 0};
    int err;
    int sockfd = 0;
    ssize_t chunk = 0;
    uint32_t sent = 0;
    uint32_t received = 0;
    char response[FF_HTTP_RESPONSE_BUFF_SIZE] = {0};

    if (host_name == NULL)
    {
        goto error;
    }

    ff_log(FF_DEBUG, "Performing DNS lookup of %s", host_name);
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    err = getaddrinfo(host_name, "80", &hints, &res);
    if (err)
    {
        ff_log(FF_WARNING, "Failed to perform DNS lookup for host: %s", host_name);
        goto error;
    }

    // TODO: filter out private IP ranges

    sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (sockfd < 0)
    {
        ff_log(FF_ERROR, "Failed to open socket");
        goto error;
    }

    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, sizeof(timeout));

    if (res->ai_family == AF_INET)
    {
        inet_ntop(AF_INET, &((struct sockaddr_in *)res->ai_addr)->sin_addr, formatted_address, INET_ADDRSTRLEN);
    }
    else
    {
        inet_ntop(AF_INET6, &((struct sockaddr_in6 *)res->ai_addr)->sin6_addr, formatted_address, INET6_ADDRSTRLEN);
    }

    ff_log(FF_DEBUG, "Resolved host %s to ip address %s", host_name, formatted_address);

    err = connect(sockfd, res->ai_addr, res->ai_addrlen);
    if (err)
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

        sent += (uint32_t)chunk;
    } while (sent < request->payload_length);

    ff_log(FF_DEBUG, "Finished sending request to %s over HTTP (%d bytes sent)", host_name, sent);

    do
    {
        chunk = recv(sockfd, response + received, sizeof(response) - received - 1, 0);

        if (chunk < 0)
        {
            ff_log(FF_WARNING, "Failed to read response from socket for host: %s (%d bytes received)", host_name, received);
            goto error;
        }

        if (chunk == 0)
        {
            break;
        }

        received += (uint32_t)chunk;

        if (received > 0 && strncasecmp(response, "HTTP/", 5) == 0)
        {
            break;
        }
    } while (received < sizeof(response) - 1);

    ff_log(FF_DEBUG, "Finished receiving response from %s (%d bytes received)", host_name, received);
    size_t response_header_length = strchr((char *)response, '\n') - response;
    ff_log(FF_DEBUG, "Response: %.*s", response_header_length > 100l ? 100 : (int)response_header_length, response);
    goto done;

error:
    ret = false;
    goto cleanup;

done:
    ret = true;
    goto cleanup;

cleanup:
    if (res != NULL)
    {
        freeaddrinfo(res);
    }

    if (sockfd != 0)
    {
        close(sockfd);
    }

    return ret;
}

bool ff_http_send_request_tls(struct ff_request *request, char *host_name)
{
    bool ret;
    long res = 1;

    SSL_CTX *ctx = NULL;
    BIO *web = NULL;
    SSL *ssl = NULL;
    char error_string[256] = {0};

    int chunk = 0;
    int sent = 0;
    int received = 0;
    char response[FF_HTTP_RESPONSE_BUFF_SIZE] = {0};

    const SSL_METHOD *method = SSLv23_method();
    if (method == NULL)
    {
        ff_log(FF_ERROR, "Failed to initialise OpenSSL method");
        goto error;
    }

    ctx = SSL_CTX_new(method);
    if (ctx == NULL)
    {
        ff_log(FF_ERROR, "Failed to initialise OpenSSL context");
        goto error;
    }

    SSL_CTX_set_default_verify_paths(ctx);

    web = BIO_new_ssl_connect(ctx);

    if (web == NULL)
    {
        ff_log(FF_ERROR, "Failed to initialise OpenSSL connection");
        goto error;
    }

    res = BIO_set_conn_hostname(web, host_name);
    res = BIO_set_conn_port(web, "443");
    if (res != 1)
    {
        ff_log(FF_ERROR, "Failed to set OpenSSL connection host name");
        goto error;
    }

    BIO_get_ssl(web, &ssl);
    if (ssl == NULL)
    {
        ff_log(FF_ERROR, "Failed to set OpenSSL ssl");
        goto error;
    }

    const char *const PREFERRED_CIPHERS = "HIGH:!aNULL:!kRSA:!PSK:!SRP:!MD5:!RC4";
    res = SSL_set_cipher_list(ssl, PREFERRED_CIPHERS);
    if (res != 1)
    {
        ff_log(FF_ERROR, "Failed to set OpenSSL preferred ciphers");
        goto error;
    }

    res = SSL_set_tlsext_host_name(ssl, host_name);
    if (res != 1)
    {
        ff_log(FF_ERROR, "Failed to set OpenSSL TLS host name");
        goto error;
    }

#ifndef OPENSSL_SKIP_HOST_VALIDATION
    {
        X509_VERIFY_PARAM *param = SSL_get0_param(ssl);

        res = X509_VERIFY_PARAM_set1_host(param, host_name, strlen(host_name));
        if (res != 1)
        {
            ff_log(FF_ERROR, "Failed to set OpenSSL param host name");
            goto error;
        }
    }
#endif

    SSL_set_verify(ssl, SSL_VERIFY_PEER, NULL);

    res = BIO_do_connect(web);
    if (res != 1)
    {
        ERR_error_string(ERR_get_error(), error_string);
        ff_log(FF_WARNING, "Failed to perform OpenSSL request connection: %s", error_string);
        goto error;
    }

    res = BIO_do_handshake(web);
    if (res != 1)
    {
        ERR_error_string(ERR_get_error(), error_string);
        ff_log(FF_WARNING, "Failed to perform OpenSSL request handshake: %s", error_string);
        goto error;
    }

    X509 *cert = SSL_get_peer_certificate(ssl);
    if (cert)
    {
        X509_free(cert);
    }
    if (cert == NULL)
    {
        ff_log(FF_WARNING, "Server did not present a certificate");
        goto error;
    }

    res = SSL_get_verify_result(ssl);
    if (res != X509_V_OK)
    {
        ERR_error_string(ERR_get_error(), error_string);
        ff_log(FF_WARNING, "Server certificate could not be validated against CA: %s", error_string);
        goto error;
    }

    do
    {
        chunk = BIO_write(web, request->payload->value + sent, request->payload_length - sent);
        sent += chunk;
    } while (chunk > 0);

    ff_log(FF_DEBUG, "Finished sending request to %s over HTTPS (%d bytes sent)", host_name, sent);

    do
    {
        chunk = BIO_read(web, response + received, sizeof(response) - received);

        received += chunk;

        if (received > 5 && strncasecmp(response, "http/", 5) == 0)
        {
            break;
        }
    } while (chunk > 0 || BIO_should_retry(web));

    ff_log(FF_DEBUG, "Finished receiving response from %s (%d bytes received)", host_name, received);
    size_t response_header_length = strchr((char *)response, '\n') - response;
    ff_log(FF_DEBUG, "Response: %.*s", response_header_length > 100l ? 100 : response_header_length, response);
    goto done;

error:
    ret = false;
    goto cleanup;

done:
    ret = true;
    goto cleanup;

cleanup:
    if (web != NULL)
    {
        BIO_free_all(web);
    }

    if (ctx != NULL)
    {
        SSL_CTX_free(ctx);
    }

    return ret;
}

char *ff_http_get_destination_host(struct ff_request *request)
{
    // @see https://stackoverflow.com/questions/8724954/what-is-the-maximum-number-of-characters-for-a-host-name-in-unix
    char *host_name = malloc(_POSIX_HOST_NAME_MAX + 1);
    char *http_request = (char *)request->payload->value;
    char *line = http_request;
    uint64_t payload_len = request->payload_length;
    uint64_t i = 0;

#define FF_HTTP_DESTINATION_CONSUME_CHAR(x)                                                          \
    line += (x);                                                                                     \
    i += (x);                                                                                        \
    if (i > payload_len || *line == '\0')                                                            \
    {                                                                                                \
        ff_log(FF_WARNING, "Encountered end of request payload before finding Host header");         \
        goto error;                                                                                  \
    }                                                                                                \
    if (i >= FF_HTTP_HOST_HEADER_MAX_SEARCH_LENGTH)                                                  \
    {                                                                                                \
        ff_log(FF_WARNING, "Reached char limit of request payload while searching for host header"); \
        goto error;                                                                                  \
    }

    if (payload_len == 0)
    {
        FF_HTTP_DESTINATION_CONSUME_CHAR(1); // Force error
    }

    while (1)
    {
        if (payload_len - i > 2 && line != http_request && (*line == '\n' || (*line == '\r' && *(line + 1) == '\n')))
        {
            // Subsequent new lines indicate request headers have finished
            ff_log(FF_WARNING, "Encountered end of request headers before finding Host header");
            goto error;
        }

        if (payload_len - i > 5 && strncasecmp(line, "host:", 5) == 0)
        {
            FF_HTTP_DESTINATION_CONSUME_CHAR(5);

            while (*line == ' ')
            {
                FF_HTTP_DESTINATION_CONSUME_CHAR(1);
            }

            int host_len = 0;

            // http://man7.org/linux/man-pages/man7/hostname.7.html
            while (i <= payload_len && (*line == '.' || *line == '-' || isalnum(*line)) && host_len <= 255)
            {
                *(host_name + host_len) = *line;
                line++;
                i++;
                host_len++;
            }

            *(host_name + host_len) = '\0';
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
