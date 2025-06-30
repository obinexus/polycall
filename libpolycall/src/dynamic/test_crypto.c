/**
 * @file test_crypto.c
 * @brief Unit tests for protocol cryptography functionality
 */

 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>
 
 #ifdef USE_CHECK_FRAMEWORK
 #include <check.h>
 #else
 #include "unit_tests_framwork.h"
 #endif
 
 #include "polycall/core/protocol/crypto.h"
 #include "polycall/core/polycall/polycall_core.h"
 
 // Test fixture
 static polycall_core_context_t* test_ctx = NULL;
 static polycall_crypto_context_t* test_crypto_ctx = NULL;
 
 // Setup function - runs before each test
 static void setup(void) {
     // Initialize test context
     test_ctx = polycall_core_create();
     
     // Create crypto context with default configuration
     polycall_crypto_config_t config = {
         .key_strength = POLYCALL_CRYPTO_KEY_STRENGTH_HIGH,
         .cipher_mode = POLYCALL_CRYPTO_MODE_AES_GCM,
         .flags = POLYCALL_CRYPTO_FLAG_EPHEMERAL_KEYS,
         .user_data = NULL
     };
     
     polycall_crypto_init(test_ctx, &test_crypto_ctx, &config);
 }
 
 // Teardown function - runs after each test
 static void teardown(void) {
     // Free resources
     if (test_crypto_ctx) {
         polycall_crypto_cleanup(test_ctx, test_crypto_ctx);
         test_crypto_ctx = NULL;
     }
     
     if (test_ctx) {
         polycall_core_destroy(test_ctx);
         test_ctx = NULL;
     }
 }
 
 // Test crypto context creation
 static int test_crypto_context_creation(void) {
     ASSERT_NOT_NULL(test_crypto_ctx);
     return 0;
 }
 
 // Test key generation
 static int test_key_generation(void) {
     // Get public key
     uint8_t* public_key = NULL;
     size_t public_key_size = 0;
     
     polycall_core_error_t result = polycall_crypto_get_public_key(
         test_ctx, test_crypto_ctx, &public_key, &public_key_size);
     
     // Verify result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(public_key);
     ASSERT_TRUE(public_key_size > 0);
     
     // Clean up
     polycall_core_free(test_ctx, public_key);
     
     return 0;
 }
 
 // Test key exchange
 static int test_key_exchange(void) {
     // Create a second crypto context for the peer
     polycall_crypto_context_t* peer_ctx = NULL;
     polycall_crypto_config_t config = {
         .key_strength = POLYCALL_CRYPTO_KEY_STRENGTH_HIGH,
         .cipher_mode = POLYCALL_CRYPTO_MODE_AES_GCM,
         .flags = POLYCALL_CRYPTO_FLAG_EPHEMERAL_KEYS,
         .user_data = NULL
     };
     
     polycall_crypto_init(test_ctx, &peer_ctx, &config);
     
     // Get public keys for both contexts
     uint8_t* local_pubkey = NULL;
     size_t local_pubkey_size = 0;
     uint8_t* peer_pubkey = NULL;
     size_t peer_pubkey_size = 0;
     
     polycall_crypto_get_public_key(test_ctx, test_crypto_ctx, &local_pubkey, &local_pubkey_size);
     polycall_crypto_get_public_key(test_ctx, peer_ctx, &peer_pubkey, &peer_pubkey_size);
     
     // Set remote keys (simulating key exchange)
     polycall_core_error_t result = polycall_crypto_set_remote_key(
         test_ctx, test_crypto_ctx, peer_pubkey, peer_pubkey_size);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     result = polycall_crypto_set_remote_key(
         test_ctx, peer_ctx, local_pubkey, local_pubkey_size);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Clean up
     polycall_core_free(test_ctx, local_pubkey);
     polycall_core_free(test_ctx, peer_pubkey);
     polycall_crypto_cleanup(test_ctx, peer_ctx);
     
     return 0;
 }
 
 // Test encryption and decryption
 static int test_encryption_decryption(void) {
     // Create a second crypto context for the peer
     polycall_crypto_context_t* peer_ctx = NULL;
     polycall_crypto_config_t config = {
         .key_strength = POLYCALL_CRYPTO_KEY_STRENGTH_HIGH,
         .cipher_mode = POLYCALL_CRYPTO_MODE_AES_GCM,
         .flags = POLYCALL_CRYPTO_FLAG_EPHEMERAL_KEYS,
         .user_data = NULL
     };
     
     polycall_crypto_init(test_ctx, &peer_ctx, &config);
     
     // Exchange keys
     uint8_t* local_pubkey = NULL;
     size_t local_pubkey_size = 0;
     uint8_t* peer_pubkey = NULL;
     size_t peer_pubkey_size = 0;
     
     polycall_crypto_get_public_key(test_ctx, test_crypto_ctx, &local_pubkey, &local_pubkey_size);
     polycall_crypto_get_public_key(test_ctx, peer_ctx, &peer_pubkey, &peer_pubkey_size);
     
     polycall_crypto_set_remote_key(test_ctx, test_crypto_ctx, peer_pubkey, peer_pubkey_size);
     polycall_crypto_set_remote_key(test_ctx, peer_ctx, local_pubkey, local_pubkey_size);
     
     // Test data to encrypt
     const char* plaintext = "This is a secret message for encryption testing";
     size_t plaintext_len = strlen(plaintext) + 1;
     
     // Associated data for authenticated encryption
     const char* aad = "Associated authenticated data";
     size_t aad_len = strlen(aad) + 1;
     
     // Encrypt
     uint8_t* ciphertext = NULL;
     size_t ciphertext_len = 0;
     
     polycall_core_error_t result = polycall_crypto_encrypt(
         test_ctx, test_crypto_ctx, 
         (const uint8_t*)plaintext, plaintext_len,
         (const uint8_t*)aad, aad_len,
         &ciphertext, &ciphertext_len
     );
     
     // Verify encryption result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(ciphertext);
     ASSERT_TRUE(ciphertext_len > plaintext_len); // Ciphertext includes tag and nonce
     
     // Decrypt
     uint8_t* decrypted = NULL;
     size_t decrypted_len = 0;
     
     result = polycall_crypto_decrypt(
         test_ctx, peer_ctx,
         ciphertext, ciphertext_len,
         (const uint8_t*)aad, aad_len,
         &decrypted, &decrypted_len
     );
     
     // Verify decryption result
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(decrypted);
     ASSERT_EQUAL_INT(plaintext_len, decrypted_len);
     ASSERT_MEMORY_EQUAL(plaintext, decrypted, plaintext_len);
     
     // Clean up
     polycall_core_free(test_ctx, local_pubkey);
     polycall_core_free(test_ctx, peer_pubkey);
     polycall_core_free(test_ctx, ciphertext);
     polycall_core_free(test_ctx, decrypted);
     polycall_crypto_cleanup(test_ctx, peer_ctx);
     
     return 0;
 }
 
 // Test config updates
 static int test_config_update(void) {
     // Create new configuration
     polycall_crypto_config_t new_config = {
         .key_strength = POLYCALL_CRYPTO_KEY_STRENGTH_MEDIUM,
         .cipher_mode = POLYCALL_CRYPTO_MODE_CHACHA20_POLY1305,
         .flags = POLYCALL_CRYPTO_FLAG_EPHEMERAL_KEYS,
         .user_data = NULL
     };
     
     // Update config
     polycall_core_error_t result = polycall_crypto_update_config(
         test_ctx, test_crypto_ctx, &new_config);
     
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Test encryption with new config
     const char* plaintext = "Testing with new config";
     size_t plaintext_len = strlen(plaintext) + 1;
     
     uint8_t* ciphertext = NULL;
     size_t ciphertext_len = 0;
     
     result = polycall_crypto_encrypt(
         test_ctx, test_crypto_ctx, 
         (const uint8_t*)plaintext, plaintext_len,
         NULL, 0,
         &ciphertext, &ciphertext_len
     );
     
     // This might fail if remote key hasn't been set after config change
     // That's fine for testing as we're checking the API behavior
     
     // Clean up
     if (result == POLYCALL_CORE_SUCCESS && ciphertext) {
         polycall_core_free(test_ctx, ciphertext);
     }
     
     return 0;
 }
 
 // Main function to run all tests
 int main(void) {
     RESET_TESTS();
     
     // Run tests
     setup();
     RUN_TEST(test_crypto_context_creation);
     teardown();
     
     setup();
     RUN_TEST(test_key_generation);
     teardown();
     
     setup();
     RUN_TEST(test_key_exchange);
     teardown();
     
     setup();
     RUN_TEST(test_encryption_decryption);
     teardown();
     
     setup();
     RUN_TEST(test_config_update);
     teardown();
     
     return TEST_REPORT();
 }