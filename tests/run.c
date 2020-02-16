#include "include/unity.h"
#include "test_hello_world.c"
#include "test_parser.c"
#include "test_hash_table.c"
#include "test_logging.c"

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_hello_world);
    
    RUN_TEST(test_request_option_node_alloc);
    RUN_TEST(test_request_option_node_free);
    RUN_TEST(test_request_payload_node_alloc);
    RUN_TEST(test_request_payload_node_free);
    RUN_TEST(test_request_alloc);
    RUN_TEST(test_request_free);
    RUN_TEST(test_request_parse_raw_http_get);
    RUN_TEST(test_request_parse_raw_http_post);
    RUN_TEST(test_request_parse_v1_single_chunk);
    RUN_TEST(test_request_parse_v1_multiple_chunks);
    RUN_TEST(test_request_parse_v1_single_chunk_with_options);

    RUN_TEST(test_hash_table_init);
    RUN_TEST(test_hash_table_free);
    RUN_TEST(test_hash_table_get_item_null);
    RUN_TEST(test_hash_table_put_item_1_level);
    RUN_TEST(test_hash_table_put_item_2_levels);
    RUN_TEST(test_hash_table_put_then_get_item_1_level);
    RUN_TEST(test_hash_table_put_then_get_item_2_levels);
    RUN_TEST(test_hash_table_put_item_overwrite);
    RUN_TEST(test_hash_table_put_item_multiple_different_bucket);
    RUN_TEST(test_hash_table_put_item_multiple_same_bucket);
    RUN_TEST(test_hash_table_remove_item_non_existant);
    RUN_TEST(test_hash_table_remove_single_item);
    RUN_TEST(test_hash_table_remove_item_last_in_bucket);
    RUN_TEST(test_hash_table_remove_item_first_in_bucket);
    RUN_TEST(test_hash_table_remove_item_different_buckets);

    RUN_TEST(test_log_debug);
    return UNITY_END();
}