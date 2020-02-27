#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/unity.h"
#include "../../src/http.h"
#include "../../src/http_p.h"
#include "../../src/alloc.h"

struct ff_request *mock_test_http_request(char *http_request, bool tls)
{
    struct ff_request *request = ff_request_alloc();
    request->payload = ff_request_payload_node_alloc();
    request->payload_length = strlen(http_request);
    request->payload->length = strlen(http_request);
    ff_request_payload_load_buff(request->payload, strlen(http_request), http_request);

    if (tls)
    {
        request->options_length++;
        request->options = (struct ff_request_option_node **)malloc(sizeof(struct ff_request_option_node *));
        request->options[0] = ff_request_option_node_alloc();
        request->options[0]->type = FF_REQUEST_OPTION_TYPE_HTTPS;
        request->options[0]->length = 1;
        request->options[0]->value = malloc(1);
        request->options[0]->value[0] = 1;
    }

    return request;
}

void test_http_get_host_empty_request()
{
    struct ff_request *request = mock_test_http_request("", false);

    char *host = ff_http_get_destination_host(request);

    TEST_ASSERT_EQUAL_MESSAGE(NULL, host, "host check failed");

    ff_request_free(request);
    free(host);
}

void test_http_get_host_valid_request()
{
    struct ff_request *request = mock_test_http_request("POST / HTTP/1.1\nHost: stackoverflow.com\n\nSome\nTest\nData", false);

    char *host = ff_http_get_destination_host(request);

    TEST_ASSERT_EQUAL_STRING_MESSAGE("stackoverflow.com", host, "host check failed");

    ff_request_free(request);
    free(host);
}

void test_http_get_host_valid_request_with_carriage()
{
    struct ff_request *request = mock_test_http_request("POST / HTTP/1.1\r\nHost: stackoverflow.com\r\n\r\nSome\r\nTest\r\nData", false);

    char *host = ff_http_get_destination_host(request);

    TEST_ASSERT_EQUAL_STRING_MESSAGE("stackoverflow.com", host, "host check failed");

    ff_request_free(request);
    free(host);
}

void test_http_get_host_no_host_header()
{
    struct ff_request *request = mock_test_http_request("POST / HTTP/1.1\nConnection: close\n\nSome\nTest\nData", false);

    char *host = ff_http_get_destination_host(request);

    TEST_ASSERT_EQUAL_MESSAGE(NULL, host, "host check failed");

    ff_request_free(request);
    free(host);
}

void test_http_get_host_host_in_body()
{
    struct ff_request *request = mock_test_http_request("POST / HTTP/1.1\nSome: header\n\nSome\nTest\nData\nHost: somehost.com", false);

    char *host = ff_http_get_destination_host(request);

    TEST_ASSERT_EQUAL_MESSAGE(NULL, host, "host check failed");

    ff_request_free(request);
    free(host);
}

void test_http_get_host_host_in_body_with_carriage()
{
    struct ff_request *request = mock_test_http_request("POST / HTTP/1.1\r\nSome: header\r\n\r\nSome\r\nTest\r\nData\r\nHost: somehost.com", false);

    char *host = ff_http_get_destination_host(request);

    TEST_ASSERT_EQUAL_MESSAGE(NULL, host, "host check failed");

    ff_request_free(request);
    free(host);
}

void test_http_get_host_multiple_headers()
{
    struct ff_request *request = mock_test_http_request("POST / HTTP/1.1\nSome: header\nOther: header\nHOST: google.com \n\n\nSome\nTest\nData\nHost: somehost.com", false);

    char *host = ff_http_get_destination_host(request);

    TEST_ASSERT_EQUAL_STRING_MESSAGE("google.com", host, "host check failed");

    ff_request_free(request);
    free(host);
}

void test_http_unencrypted_google()
{
    struct ff_request *request = mock_test_http_request("GET / HTTP/1.1\nConnection: close\nHost: google.com\n\n", false);

    ff_http_send_request(request);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_SENT, request->state, "state check failed");

    ff_request_free(request);
}

void test_http_unencrypted_google_connection_keep_alive()
{
    struct ff_request *request = mock_test_http_request("GET / HTTP/1.1\nConnection: keep-alivelose\nHost: google.com\n\n", false);

    ff_http_send_request(request);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_SENT, request->state, "state check failed");

    ff_request_free(request);
}

void test_http_unencrypted_invalid_host()
{
    struct ff_request *request = mock_test_http_request("GET / HTTP/1.1\nHost: somenonexistanthost555.co\n\n", false);

    ff_http_send_request(request);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_SENDING_FAILED, request->state, "state check failed");

    ff_request_free(request);
}

void test_http_tls_google()
{
    struct ff_request *request = mock_test_http_request("GET / HTTP/1.1\nConnection: close\nHost: www.google.com\n\n", true);

    ff_http_send_request(request);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_SENT, request->state, "state check failed");

    ff_request_free(request);
}

void test_http_tls_google_connection_keep_alive()
{
    struct ff_request *request = mock_test_http_request("GET / HTTP/1.1\nConnection: keep-alivelose\nHost: google.com\n\n", true);

    ff_http_send_request(request);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_SENT, request->state, "state check failed");

    ff_request_free(request);
}

void test_http_tls_invalid_host()
{
    struct ff_request *request = mock_test_http_request("GET / HTTP/1.1\nHost: somenonexistanthost555.co\n\n", true);

    ff_http_send_request(request);

    TEST_ASSERT_EQUAL_MESSAGE(FF_REQUEST_STATE_SENDING_FAILED, request->state, "state check failed");

    ff_request_free(request);
}