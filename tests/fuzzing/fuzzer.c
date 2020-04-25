#include <stdlib.h>
#include "../../src/logging.h"
#include "../../src/request.h"
#include "../../src/parser.h"

extern int LLVMFuzzerTestOneInput(const uint8_t *data, size_t length)
{
    ff_set_logging_level(FF_ERROR);

    struct ff_request *request = ff_request_alloc();

    ff_request_parse_chunk(request, (uint32_t)length, (void*)data);

    if (request->state == FF_REQUEST_STATE_RECEIVED) {
        ff_request_parse_options_from_payload(request);
    }

    ff_request_free(request);

    return 0;
}