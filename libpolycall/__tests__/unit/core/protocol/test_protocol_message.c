/**
 * @file test_protocol_message.c
 * @brief Unit tests for protocol message functionality
 */

 #include "polycall/protocol/message.h"
 #include "unit_test_framework.h"
 #include "mock_protocol_context.h"
 
 /* Test fixtures */
 static polycall_core_context_t *core_ctx = NULL;
 static polycall_protocol_context_t *protocol_ctx = NULL;
 
 static void setup(void) {
     /* Arrange: Initialize test environment */
     core_ctx = create_mock_core_context();
     protocol_ctx = create_mock_protocol_context(core_ctx);
 }
 
 static void teardown(void) {
     /* Clean up test environment */
     destroy_mock_protocol_context(protocol_ctx);
     destroy_mock_core_context(core_ctx);
     protocol_ctx = NULL;
     core_ctx = NULL;
 }
 
 /* Test cases */
 START_TEST(test_protocol_message_create) {
     /* Arrange */
     const char *test_data = "test_message";
     polycall_protocol_message_t *message = NULL;
     
     /* Act */
     polycall_core_error_t result = polycall_protocol_create_message(
         core_ctx, protocol_ctx, test_data, strlen(test_data), &message);
     
     /* Assert */
     assert_int_equal(result, POLYCALL_CORE_SUCCESS);
     assert_non_null(message);
     
     /* Clean up */
     polycall_protocol_destroy_message(message);
 }
 END_TEST
 
 START_TEST(test_protocol_message_get_content) {
     /* Arrange */
     const char *test_data = "test_message_content";
     polycall_protocol_message_t *message = NULL;
     polycall_protocol_create_message(
         core_ctx, protocol_ctx, test_data, strlen(test_data), &message);
     assert_non_null(message);
     
     /* Act */
     const void *content;
     size_t content_size;
     polycall_core_error_t result = polycall_protocol_get_message_content(
         message, &content, &content_size);
     
     /* Assert */
     assert_int_equal(result, POLYCALL_CORE_SUCCESS);
     assert_int_equal(content_size, strlen(test_data));
     assert_memory_equal(content, test_data, content_size);
     
     /* Clean up */
     polycall_protocol_destroy_message(message);
 }
 END_TEST
 
 /* Test suite */
 static void create_test_suite(void) {
     Suite *s = suite_create("Protocol_Message");
     
     /* Core test case */
     TCase *tc_core = tcase_create("Core");
     tcase_add_checked_fixture(tc_core, setup, teardown);
     tcase_add_test(tc_core, test_protocol_message_create);
     tcase_add_test(tc_core, test_protocol_message_get_content);
     
     suite_add_tcase(s, tc_core);
     
     /* Run the tests */
     run_test_suite(s);
 }
 
 int main(void) {
     create_test_suite();
     return 0;
 }