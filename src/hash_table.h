#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef FF_HASH_TABLE_H
#define FF_HASH_TABLE_H

struct ff_hash_table
{
    const uint8_t prefix_bit_length;
    const uint8_t bucket_levels;
    uint32_t length;
    union ff_hash_table_bucket *buckets;
    struct ff_hash_table_node *linked_list;
    struct ff_hash_table_node *linked_list_last;
};

struct ff_hash_table_iterator
{
    struct ff_hash_table *hash_table;
    struct ff_hash_table_node *current_node;
};

struct ff_hash_table *ff_hash_table_init(uint8_t prefix_bit_length);

void *ff_hash_table_get_item(struct ff_hash_table *, uint64_t item_id);

void ff_hash_table_put_item(struct ff_hash_table *, uint64_t item_id, void *item);

void ff_hash_table_remove_item(struct ff_hash_table *, uint64_t item_id);

struct ff_hash_table_iterator *ff_hash_table_iterator_init(struct ff_hash_table *);

void *ff_hash_table_iterator_next(struct ff_hash_table_iterator *);

void ff_hash_table_iterator_free(struct ff_hash_table_iterator *);

void ff_hash_table_free(struct ff_hash_table *);

#endif