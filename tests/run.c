#include "include/unity.h"
#include "server/test_hello_world.c"
#include "server/test_request.c"
#include "server/test_parser.c"
#include "server/test_hash_table.c"
#include "server/test_crypto.c"
#include "server/test_http.c"
#include "server/test_config.c"
#include "server/test_logging.c"
#include "server/test_server.c"
#include "client/test_config.c"
#include "client/test_crypto.c"
#include "client/test_client.c"

void setUp(void)
{
}

void tearDown(void)
{
}

int main(void)
{
    ff_init_openssl();

    UNITY_BEGIN();
    RUN_TEST(test_hello_world);

    RUN_TEST(test_request_option_node_alloc);
    RUN_TEST(test_request_payload_node_alloc);
#ifndef FF_OPTIMIZE
    RUN_TEST(test_request_option_node_free);
    RUN_TEST(test_request_payload_node_free);
#endif
    RUN_TEST(test_request_alloc);
    RUN_TEST(test_request_free);

    RUN_TEST(test_request_parse_raw_http_get);
    RUN_TEST(test_request_parse_raw_http_post);
    RUN_TEST(test_request_parse_v1_single_chunk);
    RUN_TEST(test_request_parse_single_char);
    RUN_TEST(test_request_parse_fuzz1);
    RUN_TEST(test_request_parse_v1_multiple_chunks);
    RUN_TEST(test_request_parse_v1_break_option);
    RUN_TEST(test_request_vectorise_payload);
    RUN_TEST(test_ff_request_parse_options_from_payload);
    RUN_TEST(test_request_parse_v1_single_chunk_with_options);
    RUN_TEST(test_ff_request_parse_id_raw_http);
    RUN_TEST(test_ff_request_parse_id);

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
    RUN_TEST(test_hash_table_iterator_init_empty);
    RUN_TEST(test_hash_table_iterator_init_with_item);
    RUN_TEST(test_hash_table_iterator_next);

    RUN_TEST(test_request_decrypt_with_unencrypted_request_with_key);
    RUN_TEST(test_request_decrypt_without_key);
    RUN_TEST(test_request_decrypt_with_unknown_encryption_mode);
    RUN_TEST(test_request_decrypt_without_iv);
    RUN_TEST(test_request_decrypt_without_tag);
    RUN_TEST(test_request_decrypt_valid);
    RUN_TEST(test_request_decrypt_invalid);

    RUN_TEST(test_http_get_host_valid_request);
    RUN_TEST(test_http_get_host_valid_request_with_carriage);
    RUN_TEST(test_http_get_host_empty_request);
    RUN_TEST(test_http_get_host_no_host_header);
    RUN_TEST(test_http_get_host_host_in_body);
    RUN_TEST(test_http_get_host_host_in_body_with_carriage);
    RUN_TEST(test_http_get_host_multiple_headers);
    RUN_TEST(test_http_unencrypted_google);
    RUN_TEST(test_http_unencrypted_google_connection_keep_alive);
    RUN_TEST(test_http_unencrypted_invalid_host);
    RUN_TEST(test_http_tls_google);
    RUN_TEST(test_http_tls_google_connection_keep_alive);
    RUN_TEST(test_http_tls_invalid_host);

    RUN_TEST(test_parse_args_empty);
    RUN_TEST(test_parse_args_help);
    RUN_TEST(test_parse_args_version);
    RUN_TEST(test_parse_args_invalid_port);
    RUN_TEST(test_parse_args_invalid_ip);
    RUN_TEST(test_parse_args_start_proxy);
    RUN_TEST(test_parse_args_start_proxy_warning);
    RUN_TEST(test_parse_args_start_proxy_info);
    RUN_TEST(test_parse_args_start_proxy_debug);
    RUN_TEST(test_parse_args_start_proxy_psk);
    RUN_TEST(test_parse_args_start_proxy_psk_pbkdf2_iterations);
    RUN_TEST(test_parse_args_start_proxy_timestamp_fudge_factor);
    RUN_TEST(test_print_usage);
    RUN_TEST(test_print_version);

    RUN_TEST(test_client_parse_args_empty);
    RUN_TEST(test_client_parse_args_help);
    RUN_TEST(test_client_parse_args_version);
    RUN_TEST(test_client_parse_args_invalid_port);
    RUN_TEST(test_client_parse_args_invalid_ip);
    RUN_TEST(test_client_parse_args_make_request);
    RUN_TEST(test_client_parse_args_make_request_warning);
    RUN_TEST(test_client_parse_args_make_request_info);
    RUN_TEST(test_client_parse_args_make_request_debug);
    RUN_TEST(test_client_parse_args_make_request_psk);
    RUN_TEST(test_client_parse_args_make_request_https);
    RUN_TEST(test_client_print_usage);
    RUN_TEST(test_client_print_version);

    RUN_TEST(test_client_request_encrypt);
    RUN_TEST(test_client_request_encrypt_without_key);
    RUN_TEST(test_client_request_encrypt_and_decrypt_returns_original_payload);

    RUN_TEST(test_client_generate_request_id);
    RUN_TEST(test_client_send_request_no_packets);
    RUN_TEST(test_client_send_request_two_packets);
    RUN_TEST(test_client_create_payload_options);
    RUN_TEST(test_client_read_payload_from_file);
    RUN_TEST(test_client_packetise_request_empty_request);
    RUN_TEST(test_client_packetise_request_single_packet);
    RUN_TEST(test_client_packetise_request_multiple_packets_with_option);
    RUN_TEST(test_client_make_request_http_and_encrypted);

    RUN_TEST(test_validate_request_timestamp_valid);
    RUN_TEST(test_validate_request_timestamp_invalid);

    RUN_TEST(test_log_debug);
    return UNITY_END();
}