#include "include/unity.h"
#include "test_hello_world.c"
#include "test_request.c"
#include "test_parser.c"
#include "test_hash_table.c"
#include "test_crypto.c"
#include "test_http.c"
#include "test_logging.c"

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
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

    RUN_TEST(test_request_decrypt_with_unencrypted_request);
    RUN_TEST(test_request_decrypt_with_unknown_encryption_mode);
    RUN_TEST(test_request_decrypt_without_iv);
    RUN_TEST(test_request_decrypt_without_tag);
    RUN_TEST(test_request_decrypt_valid);
    RUN_TEST(test_request_decrypt_invalid);

    RUN_TEST(test_http_get_host_valid_reqest);
    RUN_TEST(test_http_get_host_empty_reqest);
    RUN_TEST(test_http_get_host_no_host_header);
    RUN_TEST(test_http_get_host_host_in_body);
    RUN_TEST(test_http_get_host_multiple_headers);
    RUN_TEST(test_http_unencrypted_google);

    RUN_TEST(test_log_debug);
    return UNITY_END();
}