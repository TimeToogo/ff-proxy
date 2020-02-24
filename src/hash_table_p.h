#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef FF_HASH_TABLE_P_H
#define FF_HASH_TABLE_P_H

union ff_hash_table_bucket {
    union ff_hash_table_bucket *buckets;
    struct ff_hash_table_node *nodes;
};

struct ff_hash_table_node
{
    uint64_t item_id;
    void *value;
    struct ff_hash_table_node *next;
};

union ff_hash_table_bucket *ff_hash_table_init_bucket();

union ff_hash_table_bucket *ff_hash_table_get_or_create_bucket(
    struct ff_hash_table *hash_table,
    uint64_t item_id,
    bool should_create,
    union ff_hash_table_bucket **buckets);

void ff_hash_table_free_bucket_level(uint8_t bucket_levels, union ff_hash_table_bucket *bucket);

#endif