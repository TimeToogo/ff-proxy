#ifndef FF_ALLOC_H
#define FF_ALLOC_H

#define FREE(x) do { \
    free((x));       \
    (x) = NULL;      \
} while (0)

#endif