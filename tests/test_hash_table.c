#include <stdlib.h>
#include <string.h>
#include "include/unity.h"
#include "../src/hash_table.h"

void test_hash_table_init()
{
    struct ff_hash_table *hash_table = ff_hash_table_init(8);

    TEST_ASSERT_EQUAL_MESSAGE(8, hash_table->prefix_bit_length, "prefix_bit_length check failed");
    TEST_ASSERT_EQUAL_MESSAGE(1, hash_table->bucket_levels, "bucket_levels check failed");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, hash_table->buckets, "buckets check failed");
    TEST_ASSERT_EQUAL_MESSAGE(0, hash_table->length, "length check failed");

    ff_hash_table_free(hash_table);
}

void test_hash_table_free()
{
    struct ff_hash_table *hash_table = ff_hash_table_init(8);

    ff_hash_table_free(hash_table);

    // TODO: test better
}

void test_hash_table_get_item_null()
{
    struct ff_hash_table *hash_table = ff_hash_table_init(8);

    TEST_ASSERT_EQUAL_MESSAGE(NULL, ff_hash_table_get_item(hash_table, 123), "return check failed");

    ff_hash_table_free(hash_table);
}

void test_hash_table_put_item_1_level()
{
    uint64_t item_id = 0;
    int item_val = 123;

    struct ff_hash_table *hash_table = ff_hash_table_init(8);

    ff_hash_table_put_item(hash_table, item_id, &item_val);

    TEST_ASSERT_EQUAL_MESSAGE(1, hash_table->length, "length check failed");

    // level 1 hash = 0
    TEST_ASSERT_EQUAL_MESSAGE(item_id, hash_table->buckets[0].nodes->item_id, "item_id check failed");
    TEST_ASSERT_EQUAL_MESSAGE(&item_val, hash_table->buckets[0].nodes->value, "item_val check failed");

    ff_hash_table_free(hash_table);
}

void test_hash_table_put_item_2_levels()
{
    uint64_t item_id = 0x0201UL;
    int item_val = 123;

    struct ff_hash_table *hash_table = ff_hash_table_init(16);

    ff_hash_table_put_item(hash_table, item_id, &item_val);

    TEST_ASSERT_EQUAL_MESSAGE(1, hash_table->length, "length check failed");

    // level 1 hash = 1
    // level 2 hash = 2
    TEST_ASSERT_EQUAL_MESSAGE(item_id, hash_table->buckets[1].buckets[2].nodes->item_id, "item_id check failed");
    TEST_ASSERT_EQUAL_MESSAGE(&item_val, hash_table->buckets[1].buckets[2].nodes->value, "item_val check failed");

    ff_hash_table_free(hash_table);
}

void test_hash_table_put_then_get_item_1_level()
{
    uint64_t item_id = 5;
    int item_val = 123;
    int* returned_item;

    struct ff_hash_table *hash_table = ff_hash_table_init(8);

    ff_hash_table_put_item(hash_table, item_id, &item_val);
    returned_item = (int*)ff_hash_table_get_item(hash_table, item_id);

    TEST_ASSERT_EQUAL_MESSAGE(&item_val, returned_item, "get return check address failed");
    TEST_ASSERT_EQUAL_MESSAGE(item_val, *returned_item, "get return check value failed");

    ff_hash_table_free(hash_table);
}

void test_hash_table_put_then_get_item_2_levels()
{
    uint64_t item_id = 0x0201;
    int item_val = 123;
    int* returned_item;

    struct ff_hash_table *hash_table = ff_hash_table_init(16);

    ff_hash_table_put_item(hash_table, item_id, &item_val);
    returned_item = (int*)ff_hash_table_get_item(hash_table, item_id);

    TEST_ASSERT_EQUAL_MESSAGE(&item_val, returned_item, "get return check address failed");
    TEST_ASSERT_EQUAL_MESSAGE(item_val, *returned_item, "get return check value failed");

    ff_hash_table_free(hash_table);
}

void test_hash_table_put_item_overwrite()
{
    uint64_t item_id = 0x0201;
    int item1_val = 123;
    int item2_val = 456;

    struct ff_hash_table *hash_table = ff_hash_table_init(16);

    ff_hash_table_put_item(hash_table, item_id, &item1_val);
    ff_hash_table_put_item(hash_table, item_id, &item2_val);

    TEST_ASSERT_EQUAL_MESSAGE(1, hash_table->length, "length check failed");

    // level 1 hash = 1
    // level 2 hash = 2
    TEST_ASSERT_EQUAL_MESSAGE(item_id, hash_table->buckets[1].buckets[2].nodes->item_id, "item_id check failed");
    TEST_ASSERT_EQUAL_MESSAGE(&item2_val, hash_table->buckets[1].buckets[2].nodes->value, "item_val check failed");

    ff_hash_table_free(hash_table);
}

void test_hash_table_put_item_multiple_different_bucket()
{
    uint64_t item1_id = 0x0201;
    int item1_val = 123;
    uint64_t item2_id = 0x0301;
    int item2_val = 456;

    struct ff_hash_table *hash_table = ff_hash_table_init(16);

    ff_hash_table_put_item(hash_table, item1_id, &item1_val);
    ff_hash_table_put_item(hash_table, item2_id, &item2_val);

    TEST_ASSERT_EQUAL_MESSAGE(2, hash_table->length, "length check failed");

    // item 1: level 1 hash = 1
    // item 1: level 2 hash = 2
    TEST_ASSERT_EQUAL_MESSAGE(item1_id, hash_table->buckets[1].buckets[2].nodes->item_id, "item 1: item_id check failed");
    TEST_ASSERT_EQUAL_MESSAGE(&item1_val, hash_table->buckets[1].buckets[2].nodes->value, "item 1: item_val check failed");

    // item 2: level 1 hash = 1
    // item 2: level 2 hash = 3
    TEST_ASSERT_EQUAL_MESSAGE(item2_id, hash_table->buckets[1].buckets[3].nodes->item_id, "item 2: item_id check failed");
    TEST_ASSERT_EQUAL_MESSAGE(&item2_val, hash_table->buckets[1].buckets[3].nodes->value, "item 2: item_val check failed");

    ff_hash_table_free(hash_table);
}

void test_hash_table_put_item_multiple_same_bucket()
{
    uint64_t item1_id = 0x010201;
    int item1_val = 123;
    uint64_t item2_id = 0x020201;
    int item2_val = 456;

    struct ff_hash_table *hash_table = ff_hash_table_init(16);

    ff_hash_table_put_item(hash_table, item1_id, &item1_val);
    ff_hash_table_put_item(hash_table, item2_id, &item2_val);

    TEST_ASSERT_EQUAL_MESSAGE(2, hash_table->length, "length check failed");

    // item 1: level 1 hash = 1
    // item 1: level 2 hash = 2
    TEST_ASSERT_EQUAL_MESSAGE(item1_id, hash_table->buckets[1].buckets[2].nodes->item_id, "item 1: item_id check failed");
    TEST_ASSERT_EQUAL_MESSAGE(&item1_val, hash_table->buckets[1].buckets[2].nodes->value, "item 1: item_val check failed");

    // item 2: level 1 hash = 1
    // item 2: level 2 hash = 2
    TEST_ASSERT_EQUAL_MESSAGE(item2_id, hash_table->buckets[1].buckets[2].nodes->next->item_id, "item 2: item_id check failed");
    TEST_ASSERT_EQUAL_MESSAGE(&item2_val, hash_table->buckets[1].buckets[2].nodes->next->value, "item 2: item_val check failed");

    ff_hash_table_free(hash_table);
}

void test_hash_table_remove_item_non_existant()
{
    uint64_t item_id = 0x010201;
    int item_val = 123;

    struct ff_hash_table *hash_table = ff_hash_table_init(16);

    ff_hash_table_remove_item(hash_table, item_id);

    TEST_ASSERT_EQUAL_MESSAGE(0, hash_table->length, "length check failed");

    ff_hash_table_free(hash_table);
}

void test_hash_table_remove_single_item()
{
    uint64_t item_id = 0x010201;
    int item_val = 123;

    struct ff_hash_table *hash_table = ff_hash_table_init(16);

    ff_hash_table_put_item(hash_table, item_id, &item_val);
    ff_hash_table_remove_item(hash_table, item_id);

    TEST_ASSERT_EQUAL_MESSAGE(0, hash_table->length, "length check failed");

    // Should remove empty bucket
    // level 1 hash: 1
    TEST_ASSERT_EQUAL_MESSAGE(NULL, hash_table->buckets[1].buckets, "bucket remove check failed");

    ff_hash_table_free(hash_table);
}

void test_hash_table_remove_item_last_in_bucket()
{
    uint64_t item1_id = 0x010201;
    int item1_val = 123;
    uint64_t item2_id = 0x020201;
    int item2_val = 123;

    struct ff_hash_table *hash_table = ff_hash_table_init(16);

    ff_hash_table_put_item(hash_table, item1_id, &item1_val);
    ff_hash_table_put_item(hash_table, item2_id, &item2_val);

    ff_hash_table_remove_item(hash_table, item2_id);

    TEST_ASSERT_EQUAL_MESSAGE(1, hash_table->length, "length check failed");

    // level 1 hash: 1
    // level 2 hash: 2
    TEST_ASSERT_EQUAL_MESSAGE(item1_id, hash_table->buckets[1].buckets[2].nodes->item_id, "item 1 id check failed");

    ff_hash_table_free(hash_table);
}

void test_hash_table_remove_item_first_in_bucket()
{
    uint64_t item1_id = 0x010201;
    int item1_val = 123;
    uint64_t item2_id = 0x020201;
    int item2_val = 123;

    struct ff_hash_table *hash_table = ff_hash_table_init(16);

    ff_hash_table_put_item(hash_table, item1_id, &item1_val);
    ff_hash_table_put_item(hash_table, item2_id, &item2_val);

    ff_hash_table_remove_item(hash_table, item1_id);

    TEST_ASSERT_EQUAL_MESSAGE(1, hash_table->length, "length check failed");

    // level 1 hash: 1
    // level 2 hash: 2
    TEST_ASSERT_EQUAL_MESSAGE(item2_id, hash_table->buckets[1].buckets[2].nodes->item_id, "item 1 id check failed");

    ff_hash_table_free(hash_table);
}

void test_hash_table_remove_item_different_buckets()
{
    uint64_t item1_id = 0x010201;
    int item1_val = 123;
    uint64_t item2_id = 0x020301;
    int item2_val = 123;

    struct ff_hash_table *hash_table = ff_hash_table_init(16);

    ff_hash_table_put_item(hash_table, item1_id, &item1_val);
    ff_hash_table_put_item(hash_table, item2_id, &item2_val);

    ff_hash_table_remove_item(hash_table, item1_id);

    TEST_ASSERT_EQUAL_MESSAGE(1, hash_table->length, "length check failed");

    // item 2 level 1 hash: 1
    // item 2 level 2 hash: 3
    TEST_ASSERT_EQUAL_MESSAGE(item2_id, hash_table->buckets[1].buckets[3].nodes->item_id, "item 2 id check failed");
    TEST_ASSERT_EQUAL_MESSAGE(&item2_val, hash_table->buckets[1].buckets[3].nodes->value, "item 2 value check failed");
    
    // should remove empty bucket
    TEST_ASSERT_EQUAL_MESSAGE(NULL, hash_table->buckets[1].buckets[2].nodes, "remove empty bucket check failed");
    
    ff_hash_table_remove_item(hash_table, item2_id);

    TEST_ASSERT_EQUAL_MESSAGE(0, hash_table->length, "length check 2 failed");

    // should remove empty bucket
    TEST_ASSERT_EQUAL_MESSAGE(NULL, hash_table->buckets[1].buckets, "remove empty bucket 2 check failed");

    ff_hash_table_free(hash_table);
}
