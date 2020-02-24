#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include "logging.h"

static enum ff_log_type ff_log_level = FF_DEBUG;

void ff_set_logging_level(enum ff_log_type level)
{
    ff_log_level = level;
}

void ff_log(enum ff_log_type type, char *message, ...)
{
    if (type < ff_log_level)
    {
        return;
    }

    char time_str[20];
    char *prefix;
    char full_message[255];

    time_t now;
    time(&now);
    struct tm *nowinfo = localtime(&now);
    strftime(time_str, sizeof(time_str) / sizeof(time_str[0]), "%Y-%m-%d %H:%I:%S", nowinfo);

    switch (type)
    {
    case FF_DEBUG:
        prefix = "DEBUG";
        break;
    case FF_INFO:
        prefix = "INFO";
        break;
    case FF_WARNING:
        prefix = "WARNING";
        break;
    case FF_ERROR:
        prefix = "ERROR";
        break;
    case FF_FATAL:
        prefix = "FATAL";
        break;
    default:
        prefix = "UNKNOWN";
        break;
    }

    va_list args;
    va_start(args, message);

    vsprintf(full_message, message, args);
    printf("[%s] %s %s\n", time_str, prefix, full_message);

    va_end(args);
}
