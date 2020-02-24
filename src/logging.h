#ifndef FF_LOGGING_H
#define FF_LOGGING_H

enum ff_log_type {
    FF_DEBUG = 1,
    FF_INFO = 2,
    FF_WARNING = 3,
    FF_ERROR = 4,
    FF_FATAL = 5
};

void ff_set_logging_level(enum ff_log_type);

void ff_log(enum ff_log_type, char *message, ...);

#endif