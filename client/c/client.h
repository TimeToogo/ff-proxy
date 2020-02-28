#include "config.h"

#ifndef FF_CLIENT_H
#define FF_CLIENT_H

int ff_client_make_request(struct ff_client_config *config, FILE *fd);

#endif