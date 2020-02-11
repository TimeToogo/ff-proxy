#ifndef FF_LOGGING_H
#define FF_LOGGING_H

enum ff_log_type {
    FF_DEBUG,
    FF_INFO,
    FF_WARNING,
    FF_ERROR,
    FF_FATAL
};

void ff_log(enum ff_log_type, char *message, ...);

#endif