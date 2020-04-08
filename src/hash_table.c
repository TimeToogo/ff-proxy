#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <assert.h>
#include <pthread.h>
#include "alloc.h"
#include "hash_table.h"
#include "hash_table_p.h"

#define FF_BUCKET_POOL_BITS 8
#define FF_BUCKET_POOL_LENGTH 1 << FF_BUCKET_POOL_BITS

#define FF_HASH_GET_FOR_LEVEL(item_id, level) (uint8_t)(((item_id) & (0xFFFFFFFFFFFFFFFFULL >> (64 - FF_BUCKET_POOL_BITS * ((level) + 1)))) >> FF_BUCKET_POOL_BITS * (level))

pthread_mutex_t ff_hash_table_data_mutex = PTHREAD_MUTEX_INITIALIZER;

struct ff_hash_table *ff_hash_table_init(uint8_t prefix_bit_length)
{
    assert(prefix_bit_length > 0);

    struct ff_hash_table *hash_table = malloc(sizeof(struct ff_hash_table));

    hash_table->length = 0;
    *(uint8_t *)&hash_table->prefix_bit_length = prefix_bit_length;
    *(uint8_t *)&hash_table->bucket_levels = (int)ceil(hash_table->prefix_bit_length / FF_BUCKET_POOL_BITS);

    hash_table->buckets = ff_hash_table_init_bucket();
    hash_table->linked_list = NULL;
    hash_table->linked_list_last = NULL;

    return hash_table;
}

union ff_hash_table_bucket *ff_hash_table_init_bucket()
{
    union ff_hash_table_bucket *buckets = calloc(1, sizeof(union ff_hash_table_bucket) * FF_BUCKET_POOL_LENGTH);

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
    pthread_mutex_lock(&ff_hash_table_data_mutex);

    union ff_hash_table_bucket *buckets = ff_hash_table_get_or_create_bucket(hash_table, item_id, false, NULL);
    void *ret = NULL;

    if (buckets == NULL || buckets->nodes == NULL)
    {
        ret = NULL;
        goto cleanup;
    }

    struct ff_hash_table_node *node = buckets->nodes;

    do
    {
        if (node->item_id == item_id && node->value != NULL)
        {
            ret = node->value;
            goto cleanup;
        }
    } while ((node = node->next) != NULL);

cleanup:
    pthread_mutex_unlock(&ff_hash_table_data_mutex);
    return ret;
}

void ff_hash_table_put_item(struct ff_hash_table *hash_table, uint64_t item_id, void *item)
{
    pthread_mutex_lock(&ff_hash_table_data_mutex);

    union ff_hash_table_bucket *buckets = ff_hash_table_get_or_create_bucket(hash_table, item_id, true, NULL);

    // Find last node or matching in chain
    struct ff_hash_table_node *node = buckets->nodes;
    struct ff_hash_table_node *last_node = node;

    while (node != NULL)
    {
        // Item already exists, update in hash table
        if (node->item_id == item_id)
        {
            node->value = item;
            goto cleanup;
        }

        last_node = node;
        node = node->next;
    }

    // Store new item in hash table
    struct ff_hash_table_node *new_node = calloc(1, sizeof(struct ff_hash_table_node));
    new_node->item_id = item_id;
    new_node->value = item;

    if (last_node == NULL)
    {
        buckets->nodes = new_node;
    }
    else
    {
        last_node->next = new_node;
    }

    // Store new item in global linked list
    if (hash_table->linked_list == NULL)
    {
        hash_table->linked_list = new_node;
        hash_table->linked_list_last = new_node;
    }
    else
    {
        new_node->prev_in_list = hash_table->linked_list_last;
        hash_table->linked_list_last->next_in_list = new_node;
        hash_table->linked_list_last = new_node;
    }

    hash_table->length++;

cleanup:
    pthread_mutex_unlock(&ff_hash_table_data_mutex);
}

void ff_hash_table_remove_item(struct ff_hash_table *hash_table, uint64_t item_id)
{
    pthread_mutex_lock(&ff_hash_table_data_mutex);

    union ff_hash_table_bucket **bucket_list = calloc(1, sizeof(union ff_hash_table_bucket *) * hash_table->bucket_levels);
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
            // Remove from hash node linked list
            if (prev_node != NULL)
            {
                prev_node->next = node->next;
            }
            else
            {
                buckets->nodes = node->next;
            }

            // Remove from global linked list
            if (node->prev_in_list != NULL)
            {
                node->prev_in_list->next_in_list = node->next_in_list;
            }

            if (node->next_in_list != NULL)
            {
                node->next_in_list->prev_in_list = node->prev_in_list;
            }

            if (hash_table->linked_list == node)
            {
                hash_table->linked_list = node->next_in_list;
            }

            if (hash_table->linked_list_last == node)
            {
                hash_table->linked_list_last = node->prev_in_list;
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
        for (uint8_t level = hash_table->bucket_levels - 2; ; level--)
        {
            bool should_free = true;
            union ff_hash_table_bucket *bucket_level = bucket_list[level];
            uint8_t hash = FF_HASH_GET_FOR_LEVEL(item_id, level);

            for (int i = 0; i < FF_BUCKET_POOL_LENGTH; i++)
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
    pthread_mutex_unlock(&ff_hash_table_data_mutex);
    FREE(bucket_list);
}

void ff_hash_table_free(struct ff_hash_table *hash_table)
{
    ff_hash_table_free_bucket_level(hash_table->bucket_levels, hash_table->buckets);

    FREE(hash_table);
}

struct ff_hash_table_iterator *ff_hash_table_iterator_init(struct ff_hash_table *hash_table)
{
    pthread_mutex_lock(&ff_hash_table_data_mutex);

    struct ff_hash_table_iterator *iterator = calloc(1, sizeof(struct ff_hash_table_iterator));
    iterator->hash_table = hash_table;
    iterator->current_node = NULL;
    iterator->started = false;

    return iterator;
}

void *ff_hash_table_iterator_next(struct ff_hash_table_iterator *iterator)
{
    if (!iterator->started)
    {
        iterator->current_node = iterator->hash_table->linked_list;
        iterator->started = true;
    }
    else if (iterator->current_node != NULL)
    {
        iterator->current_node = iterator->current_node->next_in_list;
    }

    return iterator->current_node == NULL ? NULL : iterator->current_node->value;
}

void ff_hash_table_iterator_free(struct ff_hash_table_iterator *iterator)
{
    FREE(iterator);
    pthread_mutex_unlock(&ff_hash_table_data_mutex);
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
