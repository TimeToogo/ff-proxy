#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include "alloc.h"
#include "hash_table.h"

#define FF_BUCKET_POOL_BITS 8
#define FF_BUCKET_POOL_LENGTH 1 << FF_BUCKET_POOL_BITS

#define FF_HASH_GET_FOR_LEVEL(item_id, level) (uint8_t)(((item_id) & (0xFFFFFFFFFFFFFFFFULL >> (64 - FF_BUCKET_POOL_BITS * ((level) + 1)))) >> FF_BUCKET_POOL_BITS * (level))

struct ff_hash_table *ff_hash_table_init(uint8_t prefix_bit_length)
{
    assert(prefix_bit_length > 0);

    struct ff_hash_table *hash_table = (struct ff_hash_table *)malloc(sizeof(struct ff_hash_table));

    hash_table->length = 0;
    *(uint8_t *)&hash_table->prefix_bit_length = prefix_bit_length;
    *(uint8_t *)&hash_table->bucket_levels = (int)ceil(hash_table->prefix_bit_length / FF_BUCKET_POOL_BITS);

    hash_table->buckets = ff_hash_table_init_bucket();

    return hash_table;
}

union ff_hash_table_bucket *ff_hash_table_init_bucket()
{
    union ff_hash_table_bucket *buckets = (union ff_hash_table_bucket *)calloc(1, sizeof(union ff_hash_table_bucket) * FF_BUCKET_POOL_LENGTH);

    return buckets;
}

union ff_hash_table_bucket *ff_hash_table_get_or_create_bucket(
    struct ff_hash_table *hash_table,
    uint64_t item_id,
    bool should_create,
    union ff_hash_table_bucket **buckets_list)
{
    union ff_hash_table_bucket *buckets = hash_table->buckets;
    uint8_t hash;

    for (uint8_t level = 0; level < hash_table->bucket_levels - 1; level++)
    {
        hash = FF_HASH_GET_FOR_LEVEL(item_id, level);

        if (buckets[hash].buckets == NULL)
        {
            if (!should_create)
            {
                return NULL;
            }

            // Init new layer
            buckets[hash].buckets = ff_hash_table_init_bucket();
        }

        buckets = buckets[hash].buckets;

        if (buckets_list != NULL)
        {
            buckets_list[level] = buckets;
        }
    }

    hash = FF_HASH_GET_FOR_LEVEL(item_id, hash_table->bucket_levels - 1);
    buckets = &buckets[hash];

    return buckets;
}

void *ff_hash_table_get_item(struct ff_hash_table *hash_table, uint64_t item_id)
{
    union ff_hash_table_bucket *buckets = ff_hash_table_get_or_create_bucket(hash_table, item_id, false, NULL);

    if (buckets == NULL || buckets->nodes == NULL)
    {
        return NULL;
    }

    struct ff_hash_table_node *node = buckets->nodes;

    do
    {
        if (node->item_id == item_id && node->value != NULL)
        {
            return node->value;
        }
    } while ((node = node->next) != NULL);

    return NULL;
}

void ff_hash_table_put_item(struct ff_hash_table *hash_table, uint64_t item_id, void *item)
{
    union ff_hash_table_bucket *buckets = ff_hash_table_get_or_create_bucket(hash_table, item_id, true, NULL);

    // If no nodes in bucket add first node
    if (buckets->nodes == NULL)
    {

        buckets->nodes = (struct ff_hash_table_node *)malloc(sizeof(struct ff_hash_table_node));
        buckets->nodes->next = NULL;
        buckets->nodes->item_id = item_id;
        buckets->nodes->value = item;
        hash_table->length++;
        return;
    }

    // Find last node or matching in chain
    struct ff_hash_table_node *node = buckets->nodes;

    if (node->item_id == item_id)
    {
        node->value = item;
        return;
    }

    while (node->next != NULL)
    {
        node = node->next;

        if (node->item_id == item_id)
        {
            node->value = item;
            return;
        }
    }

    struct ff_hash_table_node *new_node = (struct ff_hash_table_node *)malloc(sizeof(struct ff_hash_table_node));
    new_node->next = NULL;
    new_node->item_id = item_id;
    new_node->value = item;
    node->next = new_node;
    hash_table->length++;
}

void ff_hash_table_remove_item(struct ff_hash_table *hash_table, uint64_t item_id)
{
    union ff_hash_table_bucket **bucket_list = (union ff_hash_table_bucket **)calloc(1, sizeof(union ff_hash_table_bucket *) * hash_table->bucket_levels);
    union ff_hash_table_bucket *buckets = ff_hash_table_get_or_create_bucket(hash_table, item_id, true, bucket_list);

    if (buckets == NULL || buckets->nodes == NULL)
    {
        goto cleanup;
    }

    // Find last node or matching in chain
    struct ff_hash_table_node *prev_node = NULL;
    struct ff_hash_table_node *node = buckets->nodes;

    while (node != NULL)
    {
        if (node->item_id == item_id)
        {
            if (prev_node != NULL)
            {
                prev_node->next = node->next;
            }
            else
            {
                buckets->nodes = node->next;
            }

            hash_table->length--;
            FREE(node);
            break;
        }

        prev_node = node;
        node = node->next;
    }

    if (buckets->nodes == NULL)
    {
        // Walk back up tree clearing unused buckets
        for (uint8_t level = hash_table->bucket_levels - 2; level >= 0; level--)
        {
            bool should_free = true;
            union ff_hash_table_bucket *bucket_level = bucket_list[level];
            uint8_t hash = FF_HASH_GET_FOR_LEVEL(item_id, level);

            for (int i = 0; i <= FF_BUCKET_POOL_LENGTH; i++)
            {
                if (bucket_level[i].buckets != NULL)
                {
                    should_free = false;
                    break;
                }
            }

            if (!should_free)
            {
                break;
            }

            FREE(bucket_level);
            if (level == 0)
            {
                hash_table->buckets[hash].buckets = NULL;
                break;
            }
            else
            {
                bucket_list[level - 1][hash].buckets = NULL;
            }
        }
    }

cleanup:
    FREE(bucket_list);
}

void ff_hash_table_free(struct ff_hash_table *hash_table)
{
    ff_hash_table_free_bucket_level(hash_table->bucket_levels, hash_table->buckets);

    FREE(hash_table);
}

void ff_hash_table_free_bucket_level(uint8_t bucket_levels, union ff_hash_table_bucket *bucket)
{
    assert(bucket_levels > 0);

    if (bucket_levels > 1)
    {
        for (int i = 0; i < FF_BUCKET_POOL_LENGTH; i++)
        {
            if (bucket[i].buckets != NULL)
            {
                ff_hash_table_free_bucket_level(bucket_levels - 1, bucket[i].buckets);
            }
        }

        FREE(bucket);
    }
    else if (bucket_levels == 1)
    {
        for (int i = 0; i < FF_BUCKET_POOL_LENGTH; i++)
        {
            if (bucket[i].nodes != NULL)
            {
                struct ff_hash_table_node *node = bucket[i].nodes;
                struct ff_hash_table_node *tmp_node = NULL;

                while (node != NULL)
                {
                    tmp_node = node;
                    node = node->next;
                    FREE(tmp_node);
                }
            }
        }

        FREE(bucket);
    }
}
