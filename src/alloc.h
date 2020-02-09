#ifndef ALLOC_H
#define ALLOC_H

#define FREE(x) do { \
    free((x));       \
    (x) = NULL;      \
} while (0)

#endif