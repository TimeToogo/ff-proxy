#include <stdlib.h>
#include <stdbool.h>

#ifndef FF_HASH_TABLE_H
#define FF_HASH_TABLE_H

struct ff_hash_table
{
    const uint8_t prefix_bit_length;
    const uint8_t bucket_levels;
    uint32_t length;
    union ff_hash_table_bucket *buckets;
};

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

struct ff_hash_table *ff_hash_table_init(uint8_t prefix_bit_length);

union ff_hash_table_bucket *ff_hash_table_init_bucket();

union ff_hash_table_bucket *ff_hash_table_get_or_create_bucket(
    struct ff_hash_table *hash_table,
    uint64_t item_id,
    bool should_create,
    union ff_hash_table_bucket **buckets);

void *ff_hash_table_get_item(struct ff_hash_table *, uint64_t item_id);

void ff_hash_table_put_item(struct ff_hash_table *, uint64_t item_id, void *item);

void ff_hash_table_remove_item(struct ff_hash_table *, uint64_t item_id);

void ff_hash_table_free(struct ff_hash_table *);

void ff_hash_table_free_bucket_level(uint8_t bucket_levels, union ff_hash_table_bucket *bucket);

#endif