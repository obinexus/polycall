/**
 * @file test_protocol_state_machine.c
 * @brief Unit tests for protocol state machine functionality
 */

 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 
 #ifdef USE_CHECK_FRAMEWORK
 #include <check.h>
 #else
 #include "unit_tests_framwork.h"
 #endif
 
 #include "polycall/core/protocol/protocol_state_machine.h"
 #include "polycall/core/polycall/polycall_core.h"
 #include "mock_protocol_context.h"
 
 // Test fixture
 static polycall_core_context_t* test_ctx = NULL;
 static polycall_state_machine_t* test_sm = NULL;
 
 // Setup function - runs before each test
 static void setup(void) {
     // Initialize test context
     test_ctx = polycall_core_create();
     
     // Create state machine
     polycall_sm_create(test_ctx, &test_sm);
 }
 
 // Teardown function - runs after each test
 static void teardown(void) {
     // Free resources
     if (test_sm) {
         polycall_sm_destroy(test_sm);
         test_sm = NULL;
     }
     
     if (test_ctx) {
         polycall_core_destroy(test_ctx);
         test_ctx = NULL;
     }
 }
 
 // Custom guard function for testing
 static bool test_guard_func(polycall_core_context_t* ctx, void* user_data) {
     int* guard_data = (int*)user_data;
     return *guard_data > 0;
 }
 
 // Custom state callback for testing
 static void test_state_callback(polycall_core_context_t* ctx, void* user_data) {
     int* callback_data = (int*)user_data;
     if (callback_data) {
         (*callback_data)++;
     }
 }
 
 // Test state machine creation
 static int test_sm_create(void) {
     ASSERT_NOT_NULL(test_sm);
     
     // Test with integrity checking
     polycall_state_machine_t* sm_with_integrity = NULL;
     int integrity_data = 42;
     
     polycall_sm_status_t status = polycall_sm_create_with_integrity(
         test_ctx, &sm_with_integrity, &integrity_data);
     
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     ASSERT_NOT_NULL(sm_with_integrity);
     
     if (sm_with_integrity) {
         polycall_sm_destroy(sm_with_integrity);
     }
     
     return 0;
 }
 
 // Test adding states
 static int test_add_states(void) {
     polycall_sm_status_t status;
     
     // Add states
     status = polycall_sm_add_state(test_sm, "init", NULL, NULL, false);
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     status = polycall_sm_add_state(test_sm, "handshake", NULL, NULL, false);
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     status = polycall_sm_add_state(test_sm, "auth", NULL, NULL, false);
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     status = polycall_sm_add_state(test_sm, "ready", NULL, NULL, false);
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     // Add state with callbacks
     int enter_counter = 0;
     int exit_counter = 0;
     
     status = polycall_sm_add_state(
         test_sm, 
         "active", 
         test_state_callback, 
         test_state_callback, 
         false
     );
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     // Add locked state
     status = polycall_sm_add_state(test_sm, "secure", NULL, NULL, true);
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     // Try to add duplicate state - should fail
     status = polycall_sm_add_state(test_sm, "init", NULL, NULL, false);
     ASSERT_EQUAL_INT(POLYCALL_SM_ERROR_INVALID_PARAMETERS, status);
     
     return 0;
 }
 
 // Test adding transitions
 static int test_add_transitions(void) {
     polycall_sm_status_t status;
     
     // Add states first
     polycall_sm_add_state(test_sm, "init", NULL, NULL, false);
     polycall_sm_add_state(test_sm, "handshake", NULL, NULL, false);
     polycall_sm_add_state(test_sm, "auth", NULL, NULL, false);
     polycall_sm_add_state(test_sm, "ready", NULL, NULL, false);
     
     // Add transitions
     status = polycall_sm_add_transition(
         test_sm, "to_handshake", "init", "handshake", NULL, NULL);
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     status = polycall_sm_add_transition(
         test_sm, "to_auth", "handshake", "auth", NULL, NULL);
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     status = polycall_sm_add_transition(
         test_sm, "to_ready", "auth", "ready", NULL, NULL);
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     // Add transition with guard function
     int guard_value = 1;
     status = polycall_sm_add_transition(
         test_sm, "back_to_init", "ready", "init", test_guard_func, &guard_value);
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     // Try to add transition between non-existent states - should fail
     status = polycall_sm_add_transition(
         test_sm, "invalid", "nonexistent", "ready", NULL, NULL);
     ASSERT_EQUAL_INT(POLYCALL_SM_ERROR_STATE_NOT_FOUND, status);
     
     // Try to add duplicate transition - should fail
     status = polycall_sm_add_transition(
         test_sm, "to_handshake", "init", "handshake", NULL, NULL);
     ASSERT_EQUAL_INT(POLYCALL_SM_ERROR_INVALID_PARAMETERS, status);
     
     return 0;
 }
 
 // Test executing transitions
 static int test_execute_transitions(void) {
     polycall_sm_status_t status;
     
     // Add states
     polycall_sm_add_state(test_sm, "init", NULL, NULL, false);
     polycall_sm_add_state(test_sm, "handshake", NULL, NULL, false);
     polycall_sm_add_state(test_sm, "auth", NULL, NULL, false);
     polycall_sm_add_state(test_sm, "ready", NULL, NULL, false);
     polycall_sm_add_state(test_sm, "locked", NULL, NULL, true);
     
     // Add transitions
     polycall_sm_add_transition(test_sm, "to_handshake", "init", "handshake", NULL, NULL);
     polycall_sm_add_transition(test_sm, "to_auth", "handshake", "auth", NULL, NULL);
     polycall_sm_add_transition(test_sm, "to_ready", "auth", "ready", NULL, NULL);
     polycall_sm_add_transition(test_sm, "to_locked", "ready", "locked", NULL, NULL);
     
     int guard_value = 1;
     polycall_sm_add_transition(
         test_sm, "back_to_init", "ready", "init", test_guard_func, &guard_value);
     
     // Execute transitions
     status = polycall_sm_execute_transition(test_sm, "to_handshake");
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     status = polycall_sm_execute_transition(test_sm, "to_auth");
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     status = polycall_sm_execute_transition(test_sm, "to_ready");
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     // Try invalid transition (not from current state)
     status = polycall_sm_execute_transition(test_sm, "to_auth");
     ASSERT_EQUAL_INT(POLYCALL_SM_ERROR_INVALID_TRANSITION, status);
     
     // Try transition to locked state
     status = polycall_sm_execute_transition(test_sm, "to_locked");
     ASSERT_EQUAL_INT(POLYCALL_SM_ERROR_STATE_LOCKED, status);
     
     // Try transition with guard function
     status = polycall_sm_execute_transition(test_sm, "back_to_init");
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     // Make guard function fail
     guard_value = 0;
     
     // Go back to ready state first
     polycall_sm_execute_transition(test_sm, "to_handshake");
     polycall_sm_execute_transition(test_sm, "to_auth");
     polycall_sm_execute_transition(test_sm, "to_ready");
     
     // Now try the guarded transition
     status = polycall_sm_execute_transition(test_sm, "back_to_init");
     ASSERT_EQUAL_INT(POLYCALL_SM_ERROR_INVALID_TRANSITION, status);
     
     return 0;
 }
 
 // Test state machine state querying
 static int test_get_state(void) {
     char state_name[64] = {0};
     
     // Add states
     polycall_sm_add_state(test_sm, "init", NULL, NULL, false);
     polycall_sm_add_state(test_sm, "handshake", NULL, NULL, false);
     
     // Add transition
     polycall_sm_add_transition(test_sm, "to_handshake", "init", "handshake", NULL, NULL);
     
     // Initial state is init (index 0)
     ASSERT_EQUAL_INT(0, polycall_sm_get_current_state_index(test_sm));
     
     // Get state name
     polycall_sm_status_t status = polycall_sm_get_current_state(test_sm, state_name, sizeof(state_name));
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     ASSERT_EQUAL_STR("init", state_name);
     
     // Transition to handshake
     polycall_sm_execute_transition(test_sm, "to_handshake");
     
     // Now state should be handshake (index 1)
     ASSERT_EQUAL_INT(1, polycall_sm_get_current_state_index(test_sm));
     
     // Get state name again
     status = polycall_sm_get_current_state(test_sm, state_name, sizeof(state_name));
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     ASSERT_EQUAL_STR("handshake", state_name);
     
     return 0;
 }
 
 // Test state machine snapshots
 static int test_snapshots(void) {
     polycall_sm_snapshot_t snapshot;
     
     // Add states
     polycall_sm_add_state(test_sm, "init", NULL, NULL, false);
     polycall_sm_add_state(test_sm, "handshake", NULL, NULL, false);
     polycall_sm_add_state(test_sm, "auth", NULL, NULL, false);
     
     // Add transitions
     polycall_sm_add_transition(test_sm, "to_handshake", "init", "handshake", NULL, NULL);
     polycall_sm_add_transition(test_sm, "to_auth", "handshake", "auth", NULL, NULL);
     polycall_sm_add_transition(test_sm, "to_init", "auth", "init", NULL, NULL);
     
     // Initial state is init
     ASSERT_EQUAL_INT(0, polycall_sm_get_current_state_index(test_sm));
     
     // Create snapshot
     polycall_sm_status_t status = polycall_sm_create_snapshot(test_sm, &snapshot);
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     ASSERT_EQUAL_INT(0, snapshot.state_index);
     
     // Transition to handshake
     polycall_sm_execute_transition(test_sm, "to_handshake");
     ASSERT_EQUAL_INT(1, polycall_sm_get_current_state_index(test_sm));
     
     // Transition to auth
     polycall_sm_execute_transition(test_sm, "to_auth");
     ASSERT_EQUAL_INT(2, polycall_sm_get_current_state_index(test_sm));
     
     // Restore snapshot (should go back to init)
     status = polycall_sm_restore_snapshot(test_sm, &snapshot);
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     ASSERT_EQUAL_INT(0, polycall_sm_get_current_state_index(test_sm));
     
     return 0;
 }
 
 // Test finding transitions
 static int test_find_transitions(void) {
     // Add states
     polycall_sm_add_state(test_sm, "init", NULL, NULL, false);
     polycall_sm_add_state(test_sm, "handshake", NULL, NULL, false);
     polycall_sm_add_state(test_sm, "auth", NULL, NULL, false);
     
     // Add transitions
     polycall_sm_add_transition(test_sm, "to_handshake", "init", "handshake", NULL, NULL);
     polycall_sm_add_transition(test_sm, "to_auth", "handshake", "auth", NULL, NULL);
     
     // Find transition from init to handshake
     char transition_name[64] = {0};
     polycall_sm_status_t status = polycall_sm_get_transition(
         test_sm, "init", "handshake", transition_name, sizeof(transition_name));
     
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     ASSERT_EQUAL_STR("to_handshake", transition_name);
     
     // Find transition index
     int index = polycall_sm_find_transition(test_sm, "to_handshake");
     ASSERT_TRUE(index >= 0);
     
     // Find nonexistent transition
     index = polycall_sm_find_transition(test_sm, "nonexistent");
     ASSERT_EQUAL_INT(-1, index);
     
     return 0;
 }
 
 // Test locking and unlocking states
 static int test_lock_unlock_states(void) {
     // Add states
     polycall_sm_add_state(test_sm, "init", NULL, NULL, false);
     polycall_sm_add_state(test_sm, "secure", NULL, NULL, false);
     
     // Add transition
     polycall_sm_add_transition(test_sm, "to_secure", "init", "secure", NULL, NULL);
     
     // Lock secure state
     polycall_sm_status_t status = polycall_sm_lock_state(test_sm, "secure");
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     // Try to transition to locked state
     polycall_sm_execute_transition(test_sm, "to_secure");
     ASSERT_EQUAL_INT(0, polycall_sm_get_current_state_index(test_sm));
     
     // Unlock secure state
     status = polycall_sm_unlock_state(test_sm, "secure");
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     
     // Now transition should succeed
     status = polycall_sm_execute_transition(test_sm, "to_secure");
     ASSERT_EQUAL_INT(POLYCALL_SM_SUCCESS, status);
     ASSERT_EQUAL_INT(1, polycall_sm_get_current_state_index(test_sm));
     
     return 0;
 }
 
 // Main function to run all tests
 int main(void) {
     RESET_TESTS();
     
     // Run tests
     setup();
     RUN_TEST(test_sm_create);
     teardown();
     
     setup();
     RUN_TEST(test_add_states);
     teardown();
     
     setup();
     RUN_TEST(test_add_transitions);
     teardown();
     
     setup();
     RUN_TEST(test_execute_transitions);
     teardown();
     
     setup();
     RUN_TEST(test_get_state);
     teardown();
     
     setup();
     RUN_TEST(test_snapshots);
     teardown();
     
     setup();
     RUN_TEST(test_find_transitions);
     teardown();
     
     setup();
     RUN_TEST(test_lock_unlock_states);
     teardown();
     
     return TEST_REPORT();
 }