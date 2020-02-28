#include <stdbool.h>
#include "request.h"

#ifndef FF_HTTP_H
#define FF_HTTP_H

void ff_http_send_request(struct ff_request *request);

#endif