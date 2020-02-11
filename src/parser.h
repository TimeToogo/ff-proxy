#include <netinet/in.h>

#ifndef FF_PARSER_H
#define FF_PARSER_H

enum ff_request_state {
    FF_REQUEST_STATE_RECEIVING = 0,
    FF_REQUEST_STATE_RECEIVED = 1,
    FF_REQUEST_STATE_RECEIVED_FAIL = 2,
    FF_REQUEST_STATE_DECRYPTING = 3,
    FF_REQUEST_STATE_DECRYPTED = 4,
    FF_REQUEST_STATE_VERIFYING = 5,
    FF_REQUEST_STATE_VERIFIED = 6,
    FF_REQUEST_STATE_SENDING = 7,
    FF_REQUEST_STATE_SENT = 8
};

enum ff_request_version {
    // Support for proxying a RAW HTTP request
    FF_VERSION_RAW = -1,
    // Ensure that version numbers do not overlap 'A' (65) to 'Z' (90)
    // to prevent collisions with determining if the payload contains a raw HTTP request
    // identified by the request method.
    FF_VERSION_1 = 1
};

enum ff_request_option_type {
    FF_REQUEST_OPTION_TYPE_EOL = 0,
    FF_REQUEST_OPTION_TYPE_CHECKSUM = 1,
    FF_REQUEST_OPTION_TYPE_ENCRYPTED = 2
};

struct ff_request_option_node {
    enum ff_request_option_type type;
    uint16_t length;
    char* value;
    struct ff_request_option_node* next;
};

union ff_source_address {
    struct in_addr ipv4;
    struct in6_addr ipv6;
};

struct ff_request_payload_node {
    uint16_t offset;
    uint16_t length;
    char* value;
    struct ff_request_payload_node* next;
};

struct ff_request {
    enum ff_request_state state;
    enum ff_request_version version;
    uint16_t source_address_type;
    uint64_t request_id;
    union ff_source_address source_address;
    struct ff_request_option_node* options;
    uint64_t payload_length;
    uint64_t received_length;
    struct ff_request_payload_node* payload;
};

struct __raw_ff_request_header {
    uint16_t version;
    uint64_t request_id;
    uint32_t total_length;
    uint16_t chunk_offset;
    uint16_t chunk_length;
};

struct __raw_ff_request_option_header {
    uint8_t type;
    uint16_t length;
};

struct ff_request_option_node* ff_request_option_node_alloc(void);

void ff_request_option_node_free(struct ff_request_option_node*);

struct ff_request_payload_node* ff_request_payload_node_alloc(void);

void ff_request_payload_load_buff(struct ff_request_payload_node* node, int buff_size, char* buff);

void ff_request_payload_node_free(struct ff_request_payload_node*);

struct ff_request* ff_request_alloc(void);

void ff_request_free(struct ff_request*);

void ff_request_parse_chunk(struct ff_request* request, uint32_t buff_size, char* buff);

void ff_request_parse_first_chunk(struct ff_request* request, uint32_t buff_size, char* buff);

void ff_request_parse_raw_http(struct ff_request* request, uint32_t buff_size, char* buff);

void ff_request_parse_data_chunk(struct ff_request* request, uint32_t buff_size, char* buff);

#endif