#ifndef PARSER_H
#define PARSER_H

enum ff_request_state {
    FF_REQUEST_STATE_RECEIVING,
    FF_REQUEST_STATE_QUEUED,
    FF_REQUEST_STATE_SENDING,
    FF_REQUEST_STATE_SENT
};

struct ff_request_header {
    char* key;
    char* value;
};

struct ff_request {
    enum ff_request_state state;
    char* method;
    char* path;
    int headers_length;
    struct ff_request_header* headers;
    // TODO: support larger bodies via fd
    char* body;
};

struct ff_request_header* ff_request_header_alloc(void);

void ff_request_header_free(struct ff_request_header*);

struct ff_request* ff_request_alloc(void);

void ff_request_free(struct ff_request*);

#endif