/**
#include "polycall/core/protocol/crypto.h"

 * @file crypto.c
 * @brief Cryptographic module implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the cryptographic functionality for LibPolyCall,
 * providing secure communication capabilities with various encryption
 * algorithms, key management, and integrity protection mechanisms.
 */

 #include "polycall/core/protocol/crypto.h"
 #include "polycall/core/polycall/polycall_error.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include <string.h>
 #include <stdlib.h>
 #include <time.h>
 
 // Define default cryptographic parameters
 #define POLYCALL_CRYPTO_AES_KEY_SIZE_128 16
 #define POLYCALL_CRYPTO_AES_KEY_SIZE_256 32
 #define POLYCALL_CRYPTO_IV_SIZE 16
 #define POLYCALL_CRYPTO_HMAC_SIZE 32
 #define POLYCALL_CRYPTO_NONCE_SIZE 8
 #define POLYCALL_CRYPTO_TAG_SIZE 16
 #define POLYCALL_CRYPTO_SALT_SIZE 16
 #define POLYCALL_CRYPTO_DEFAULT_ITERATIONS 10000
 
 // Symmetric cipher definitions
 typedef enum {
     POLYCALL_CRYPTO_CIPHER_NONE = 0,
     POLYCALL_CRYPTO_CIPHER_AES_128_GCM,
     POLYCALL_CRYPTO_CIPHER_AES_256_GCM,
     POLYCALL_CRYPTO_CIPHER_CHACHA20_POLY1305
 } polycall_crypto_cipher_t;
 
 // Key exchange algorithm definitions
 typedef enum {
     POLYCALL_CRYPTO_KEX_NONE = 0,
     POLYCALL_CRYPTO_KEX_DH,
     POLYCALL_CRYPTO_KEX_ECDH
 } polycall_crypto_kex_t;
 
 // Internal crypto context structure
 struct polycall_crypto_context {
     polycall_core_context_t* core_ctx;
     polycall_crypto_config_t config;
     polycall_crypto_cipher_t active_cipher;
     polycall_crypto_kex_t key_exchange;
     
     // Key material
     uint8_t* session_key;
     size_t session_key_size;
     uint8_t* shared_secret;
     size_t shared_secret_size;
     uint8_t* public_key;
     size_t public_key_size;
     uint8_t* private_key;
     size_t private_key_size;
     
     // Session state
     uint64_t counter_send;
     uint64_t counter_recv;
     bool initialized;
     bool has_remote_key;
     
     // Memory pool for crypto operations
     polycall_memory_pool_t* memory_pool;
     
     // User data
     void* user_data;
 };
 
 // Forward declarations of internal functions
 static polycall_core_error_t initialize_cipher(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx
 );
 
 static polycall_core_error_t generate_keypair(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx
 );
 
 static polycall_core_error_t derive_session_key(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx,
     const uint8_t* remote_public_key,
     size_t remote_public_key_size
 );
 
 static polycall_core_error_t encrypt_data_aes_gcm(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx,
     const uint8_t* plaintext,
     size_t plaintext_size,
     const uint8_t* associated_data,
     size_t associated_data_size,
     uint8_t* ciphertext,
     size_t* ciphertext_size,
     uint8_t* tag,
     uint8_t* nonce
 );
 
 static polycall_core_error_t decrypt_data_aes_gcm(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx,
     const uint8_t* ciphertext,
     size_t ciphertext_size,
     const uint8_t* associated_data,
     size_t associated_data_size,
     const uint8_t* tag,
     const uint8_t* nonce,
     uint8_t* plaintext,
     size_t* plaintext_size
 );
 
 // Utility functions for crypto operations
 static void generate_random_bytes(uint8_t* buffer, size_t size) {
     // A simple PRNG implementation - in production, use a secure RNG
     // This is a placeholder for demonstration purposes
     srand((unsigned int)time(NULL));
     for (size_t i = 0; i < size; i++) {
         buffer[i] = (uint8_t)(rand() & 0xFF);
     }
 }
 
 static polycall_core_error_t alloc_secure_buffer(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx,
     uint8_t** buffer,
     size_t size
 ) {
     if (!ctx || !crypto_ctx || !buffer || size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     *buffer = polycall_memory_alloc(
         ctx, 
         crypto_ctx->memory_pool, 
         size, 
         POLYCALL_MEMORY_FLAG_SECURE | POLYCALL_MEMORY_FLAG_ZERO_INIT
     );
     
     if (!*buffer) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 static void secure_free_buffer(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx,
     uint8_t** buffer
 ) {
     if (!ctx || !crypto_ctx || !buffer || !*buffer) {
         return;
     }
     
     polycall_memory_free(ctx, crypto_ctx->memory_pool, *buffer);
     *buffer = NULL;
 }
 
 // Implementation of internal functions
 
 static polycall_core_error_t initialize_cipher(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx
 ) {
     if (!ctx || !crypto_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Determine key size based on cipher
     size_t key_size = 0;
     switch (crypto_ctx->active_cipher) {
         case POLYCALL_CRYPTO_CIPHER_AES_128_GCM:
             key_size = POLYCALL_CRYPTO_AES_KEY_SIZE_128;
             break;
             
         case POLYCALL_CRYPTO_CIPHER_AES_256_GCM:
             key_size = POLYCALL_CRYPTO_AES_KEY_SIZE_256;
             break;
             
         case POLYCALL_CRYPTO_CIPHER_CHACHA20_POLY1305:
             key_size = 32; // ChaCha20 uses 256-bit keys
             break;
             
         default:
             // For no cipher, we don't need to initialize anything
             return POLYCALL_CORE_SUCCESS;
     }
     
     // Allocate session key buffer
     polycall_core_error_t result = alloc_secure_buffer(
         ctx, crypto_ctx, &crypto_ctx->session_key, key_size
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate session key buffer");
         return result;
     }
     
     crypto_ctx->session_key_size = key_size;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t generate_keypair(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx
 ) {
     if (!ctx || !crypto_ctx) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Skip key generation if no key exchange method is selected
     if (crypto_ctx->key_exchange == POLYCALL_CRYPTO_KEX_NONE) {
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Determine key sizes based on key exchange algorithm
     size_t private_key_size = 0;
     size_t public_key_size = 0;
     
     switch (crypto_ctx->key_exchange) {
         case POLYCALL_CRYPTO_KEX_DH:
             private_key_size = 32; // 256-bit private key for DH
             public_key_size = 128; // DH public key size
             break;
             
         case POLYCALL_CRYPTO_KEX_ECDH:
             private_key_size = 32; // 256-bit private key for ECDH
             public_key_size = 64;  // ECDH public key size (X and Y coordinates)
             break;
             
         default:
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate key buffers
     polycall_core_error_t result;
     
     // Private key
     result = alloc_secure_buffer(ctx, crypto_ctx, &crypto_ctx->private_key, private_key_size);
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate private key buffer");
         return result;
     }
     crypto_ctx->private_key_size = private_key_size;
     
     // Public key
     result = alloc_secure_buffer(ctx, crypto_ctx, &crypto_ctx->public_key, public_key_size);
     if (result != POLYCALL_CORE_SUCCESS) {
         secure_free_buffer(ctx, crypto_ctx, &crypto_ctx->private_key);
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate public key buffer");
         return result;
     }
     crypto_ctx->public_key_size = public_key_size;
     
     // Generate random private key
     generate_random_bytes(crypto_ctx->private_key, private_key_size);
     
     // Simulate public key derivation
     // In a real implementation, this would compute the public key from the private key
     // using the appropriate algorithm (DH or ECDH)
     generate_random_bytes(crypto_ctx->public_key, public_key_size);
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t derive_session_key(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx,
     const uint8_t* remote_public_key,
     size_t remote_public_key_size
 ) {
     if (!ctx || !crypto_ctx || !remote_public_key || remote_public_key_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Validate remote key size against expected size
     if (remote_public_key_size != crypto_ctx->public_key_size) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Invalid remote public key size");
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Skip if we don't have a private key
     if (!crypto_ctx->private_key || crypto_ctx->private_key_size == 0) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "No private key available");
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Determine shared secret size based on key exchange algorithm
     size_t shared_secret_size = 0;
     
     switch (crypto_ctx->key_exchange) {
         case POLYCALL_CRYPTO_KEX_DH:
             shared_secret_size = 32; // 256-bit shared secret for DH
             break;
             
         case POLYCALL_CRYPTO_KEX_ECDH:
             shared_secret_size = 32; // 256-bit shared secret for ECDH
             break;
             
         default:
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate shared secret buffer
     polycall_core_error_t result;
     if (crypto_ctx->shared_secret) {
         secure_free_buffer(ctx, crypto_ctx, &crypto_ctx->shared_secret);
     }
     
     result = alloc_secure_buffer(ctx, crypto_ctx, &crypto_ctx->shared_secret, shared_secret_size);
     if (result != POLYCALL_CORE_SUCCESS) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           result,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Failed to allocate shared secret buffer");
         return result;
     }
     crypto_ctx->shared_secret_size = shared_secret_size;
     
     // Simulate shared secret derivation
     // In a real implementation, this would compute the shared secret using the
     // private key and remote public key with the appropriate algorithm
     
     // For simplicity, we'll just XOR the private key with the remote public key
     // This is NOT secure and is just for demonstration purposes
     for (size_t i = 0; i < shared_secret_size; i++) {
         if (i < crypto_ctx->private_key_size && i < remote_public_key_size) {
             crypto_ctx->shared_secret[i] = crypto_ctx->private_key[i] ^ remote_public_key[i];
         } else {
             crypto_ctx->shared_secret[i] = 0;
         }
     }
     
     // Derive session key from shared secret
     // In a real implementation, this would use a KDF like HKDF
     // For simplicity, we'll just copy the shared secret to the session key
     memcpy(crypto_ctx->session_key, crypto_ctx->shared_secret, 
            crypto_ctx->session_key_size < shared_secret_size ? 
            crypto_ctx->session_key_size : shared_secret_size);
     
     // Mark that we have a remote key
     crypto_ctx->has_remote_key = true;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t encrypt_data_aes_gcm(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx,
     const uint8_t* plaintext,
     size_t plaintext_size,
     const uint8_t* associated_data,
     size_t associated_data_size,
     uint8_t* ciphertext,
     size_t* ciphertext_size,
     uint8_t* tag,
     uint8_t* nonce
 ) {
     if (!ctx || !crypto_ctx || !plaintext || plaintext_size == 0 || 
         !ciphertext || !ciphertext_size || !tag || !nonce) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Verify we have a session key
     if (!crypto_ctx->session_key || crypto_ctx->session_key_size == 0) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "No session key available");
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Check output buffer size
     if (*ciphertext_size < plaintext_size) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Ciphertext buffer too small");
         *ciphertext_size = plaintext_size;
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Generate random nonce
     generate_random_bytes(nonce, POLYCALL_CRYPTO_NONCE_SIZE);
     
     // Increment counter
     crypto_ctx->counter_send++;
     
     // In a real implementation, this would use a cryptographic library like OpenSSL
     // to perform AES-GCM encryption
     
     // For this implementation, we'll just XOR the plaintext with the session key
     // This is NOT secure and is just for demonstration purposes
     for (size_t i = 0; i < plaintext_size; i++) {
         ciphertext[i] = plaintext[i] ^ crypto_ctx->session_key[i % crypto_ctx->session_key_size];
     }
     
     // Generate a simulated authentication tag
     // In a real implementation, this would be the GCM authentication tag
     uint32_t tag_seed = 0;
     
     // Include counter in tag calculation
     tag_seed ^= (uint32_t)crypto_ctx->counter_send;
     
     // Include nonce in tag calculation
     for (size_t i = 0; i < POLYCALL_CRYPTO_NONCE_SIZE; i++) {
         tag_seed = (tag_seed << 3) ^ nonce[i];
     }
     
     // Include ciphertext in tag calculation
     for (size_t i = 0; i < plaintext_size; i++) {
         tag_seed = (tag_seed << 1) ^ ciphertext[i];
     }
     
     // Include associated data in tag calculation if provided
     if (associated_data && associated_data_size > 0) {
         for (size_t i = 0; i < associated_data_size; i++) {
             tag_seed = (tag_seed << 2) ^ associated_data[i];
         }
     }
     
     // Generate tag from seed
     for (size_t i = 0; i < POLYCALL_CRYPTO_TAG_SIZE; i++) {
         tag[i] = (tag_seed >> ((i % 4) * 8)) & 0xFF;
     }
     
     *ciphertext_size = plaintext_size;
     return POLYCALL_CORE_SUCCESS;
 }
 
 static polycall_core_error_t decrypt_data_aes_gcm(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx,
     const uint8_t* ciphertext,
     size_t ciphertext_size,
     const uint8_t* associated_data,
     size_t associated_data_size,
     const uint8_t* tag,
     const uint8_t* nonce,
     uint8_t* plaintext,
     size_t* plaintext_size
 ) {
     if (!ctx || !crypto_ctx || !ciphertext || ciphertext_size == 0 || 
         !plaintext || !plaintext_size || !tag || !nonce) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Verify we have a session key
     if (!crypto_ctx->session_key || crypto_ctx->session_key_size == 0) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "No session key available");
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Check output buffer size
     if (*plaintext_size < ciphertext_size) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Plaintext buffer too small");
         *plaintext_size = ciphertext_size;
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Increment counter
     crypto_ctx->counter_recv++;
     
     // Verify tag
     uint8_t calculated_tag[POLYCALL_CRYPTO_TAG_SIZE];
     uint32_t tag_seed = 0;
     
     // Include counter in tag calculation
     tag_seed ^= (uint32_t)crypto_ctx->counter_recv;
     
     // Include nonce in tag calculation
     for (size_t i = 0; i < POLYCALL_CRYPTO_NONCE_SIZE; i++) {
         tag_seed = (tag_seed << 3) ^ nonce[i];
     }
     
     // Include ciphertext in tag calculation
     for (size_t i = 0; i < ciphertext_size; i++) {
         tag_seed = (tag_seed << 1) ^ ciphertext[i];
     }
     
     // Include associated data in tag calculation if provided
     if (associated_data && associated_data_size > 0) {
         for (size_t i = 0; i < associated_data_size; i++) {
             tag_seed = (tag_seed << 2) ^ associated_data[i];
         }
     }
     
     // Generate tag from seed
     for (size_t i = 0; i < POLYCALL_CRYPTO_TAG_SIZE; i++) {
         calculated_tag[i] = (tag_seed >> ((i % 4) * 8)) & 0xFF;
     }
     
     // Compare calculated tag with provided tag
     if (memcmp(calculated_tag, tag, POLYCALL_CRYPTO_TAG_SIZE) != 0) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Authentication tag verification failed");
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // In a real implementation, this would use a cryptographic library like OpenSSL
     // to perform AES-GCM decryption
     
     // For this implementation, we'll just XOR the ciphertext with the session key
     // This is NOT secure and is just for demonstration purposes
     for (size_t i = 0; i < ciphertext_size; i++) {
         plaintext[i] = ciphertext[i] ^ crypto_ctx->session_key[i % crypto_ctx->session_key_size];
     }
     
     *plaintext_size = ciphertext_size;
     return POLYCALL_CORE_SUCCESS;
 }
 
 // Implementation of public crypto functions
 
 polycall_core_error_t polycall_crypto_init(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t** crypto_ctx,
     const polycall_crypto_config_t* config
 ) {
     if (!ctx || !crypto_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Create memory pool for crypto operations
     polycall_memory_pool_t* memory_pool = NULL;
     polycall_core_error_t result = polycall_memory_create_pool(
         ctx, &memory_pool, 65536  // 64KB pool, adjust as needed
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // Allocate crypto context
     polycall_crypto_context_t* new_ctx = polycall_memory_alloc(
         ctx, memory_pool, sizeof(struct polycall_crypto_context),
         POLYCALL_MEMORY_FLAG_ZERO_INIT
     );
     
     if (!new_ctx) {
         polycall_memory_destroy_pool(ctx, memory_pool);
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize context
     new_ctx->core_ctx = ctx;
     new_ctx->memory_pool = memory_pool;
     new_ctx->user_data = config->user_data;
     
     // Copy configuration
     memcpy(&new_ctx->config, config, sizeof(polycall_crypto_config_t));
     
     // Set default cipher based on configuration
     switch (config->cipher_mode) {
         case POLYCALL_CRYPTO_MODE_AES_GCM:
             if (config->key_strength == POLYCALL_CRYPTO_KEY_STRENGTH_HIGH) {
                 new_ctx->active_cipher = POLYCALL_CRYPTO_CIPHER_AES_256_GCM;
             } else {
                 new_ctx->active_cipher = POLYCALL_CRYPTO_CIPHER_AES_128_GCM;
             }
             break;
             
         case POLYCALL_CRYPTO_MODE_CHACHA20_POLY1305:
             new_ctx->active_cipher = POLYCALL_CRYPTO_CIPHER_CHACHA20_POLY1305;
             break;
             
         default:
             new_ctx->active_cipher = POLYCALL_CRYPTO_CIPHER_NONE;
             break;
     }
     
     // Set key exchange algorithm (default to ECDH for high security)
     new_ctx->key_exchange = (config->key_strength >= POLYCALL_CRYPTO_KEY_STRENGTH_MEDIUM) ?
                            POLYCALL_CRYPTO_KEX_ECDH : POLYCALL_CRYPTO_KEX_DH;
     
     // Initialize cipher and generate keypair
     result = initialize_cipher(ctx, new_ctx);
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_crypto_cleanup(ctx, new_ctx);
         return result;
     }
     
     result = generate_keypair(ctx, new_ctx);
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_crypto_cleanup(ctx, new_ctx);
         return result;
     }
     
     // Initialize counters
     new_ctx->counter_send = 0;
     new_ctx->counter_recv = 0;
     
     // Mark as initialized
     new_ctx->initialized = true;
     new_ctx->has_remote_key = false;
     
     *crypto_ctx = new_ctx;
     return POLYCALL_CORE_SUCCESS;
 }
 
void polycall_crypto_cleanup(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx
 ) {
     if (!ctx || !crypto_ctx) {
         return;
     }
     
     // Free key material
     secure_free_buffer(ctx, crypto_ctx, &crypto_ctx->session_key);
     secure_free_buffer(ctx, crypto_ctx, &crypto_ctx->shared_secret);
     secure_free_buffer(ctx, crypto_ctx, &crypto_ctx->public_key);
     secure_free_buffer(ctx, crypto_ctx, &crypto_ctx->private_key);
     
     // Free context and memory pool
     polycall_memory_pool_t* pool = crypto_ctx->memory_pool;
     polycall_memory_free(ctx, pool, crypto_ctx);
     polycall_memory_destroy_pool(ctx, pool);
 }

polycall_core_error_t polycall_crypto_encrypt(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx,
     const uint8_t* plaintext,
     size_t plaintext_size,
     const uint8_t* associated_data,
     size_t associated_data_size,
     uint8_t** ciphertext,
     size_t* ciphertext_size
 ) {
     if (!ctx || !crypto_ctx || !plaintext || plaintext_size == 0 ||
         !ciphertext || !ciphertext_size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     if (!crypto_ctx->initialized || !crypto_ctx->has_remote_key) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Crypto context not ready for encryption");
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Calculate output size: ciphertext + nonce + tag
     size_t output_size = plaintext_size + POLYCALL_CRYPTO_NONCE_SIZE + POLYCALL_CRYPTO_TAG_SIZE;
     
     // Allocate output buffer
     uint8_t* output = polycall_memory_alloc(
         ctx, crypto_ctx->memory_pool, output_size, POLYCALL_MEMORY_FLAG_ZERO_INIT
     );
     
     if (!output) {
polycall_core_error_t polycall_crypto_set_remote_key(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx,
     const uint8_t* remote_key,
     size_t remote_key_size
 ) {
     if (!ctx || !crypto_ctx || !remote_key || remote_key_size == 0) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     if (!crypto_ctx->initialized) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Crypto context not initialized");
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Derive session key using remote public key
     return derive_session_key(ctx, crypto_ctx, remote_key, remote_key_size);
}
         ctx, crypto_ctx, plaintext, plaintext_size,
         associated_data, associated_data_size,
         ciphertext_ptr, &actual_ciphertext_size,
         tag_ptr, nonce_ptr
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_memory_free(ctx, crypto_ctx->memory_pool, output);
         *ciphertext_size = 0;
         *ciphertext = NULL;
         return result;
     }
     
     *ciphertext = output;
     *ciphertext_size = output_size;
     
     return POLYCALL_CORE_SUCCESS;
 }

polycall_core_error_t polycall_crypto_decrypt(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx,
     const uint8_t* ciphertext,
     size_t ciphertext_size,
     const uint8_t* associated_data,
     size_t associated_data_size,
     uint8_t** plaintext,
     size_t* plaintext_size
 ) {
     if (!ctx || !crypto_ctx || !ciphertext || ciphertext_size == 0 ||
         !plaintext || !plaintext_size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     if (!crypto_ctx->initialized || !crypto_ctx->has_remote_key) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Crypto context not ready for decryption");
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Check if ciphertext is large enough to contain nonce and tag
     if (ciphertext_size < POLYCALL_CRYPTO_NONCE_SIZE + POLYCALL_CRYPTO_TAG_SIZE) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_PARAMETERS,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "Ciphertext too small to contain nonce and tag");
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Extract components: [nonce][actual_ciphertext][tag]
     const uint8_t* nonce_ptr = ciphertext;
     const uint8_t* actual_ciphertext = ciphertext + POLYCALL_CRYPTO_NONCE_SIZE;
     size_t actual_ciphertext_size = ciphertext_size - POLYCALL_CRYPTO_NONCE_SIZE - POLYCALL_CRYPTO_TAG_SIZE;
     const uint8_t* tag_ptr = ciphertext + POLYCALL_CRYPTO_NONCE_SIZE + actual_ciphertext_size;
     
     // Allocate plaintext buffer
     uint8_t* output = polycall_memory_alloc(
         ctx, crypto_ctx->memory_pool, actual_ciphertext_size, POLYCALL_MEMORY_FLAG_ZERO_INIT
     );
     
     if (!output) {
         *plaintext_size = 0;
         *plaintext = NULL;
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     size_t output_size = actual_ciphertext_size;
     
     // Perform decryption
     polycall_core_error_t result = decrypt_data_aes_gcm(
         ctx, crypto_ctx, actual_ciphertext, actual_ciphertext_size,
         associated_data, associated_data_size,
         tag_ptr, nonce_ptr, output, &output_size
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_memory_free(ctx, crypto_ctx->memory_pool, output);
         *plaintext_size = 0;
         *plaintext = NULL;
         return result;
     }
     
     *plaintext = output;
     *plaintext_size = output_size;
     
     return POLYCALL_CORE_SUCCESS;
 }

polycall_core_error_t polycall_crypto_update_config(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx,
     const polycall_crypto_config_t* config
 ) {
     if (!ctx || !crypto_ctx || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Copy new configuration
     memcpy(&crypto_ctx->config, config, sizeof(polycall_crypto_config_t));
     
     // Update user data
     crypto_ctx->user_data = config->user_data;
     
     // Update cipher mode if necessary
     polycall_crypto_cipher_t new_cipher;
     
     switch (config->cipher_mode) {
         case POLYCALL_CRYPTO_MODE_AES_GCM:
             if (config->key_strength == POLYCALL_CRYPTO_KEY_STRENGTH_HIGH) {
                 new_cipher = POLYCALL_CRYPTO_CIPHER_AES_256_GCM;
             } else {
                 new_cipher = POLYCALL_CRYPTO_CIPHER_AES_128_GCM;
             }
             break;
             
         case POLYCALL_CRYPTO_MODE_CHACHA20_POLY1305:
             new_cipher = POLYCALL_CRYPTO_CIPHER_CHACHA20_POLY1305;
             break;
             
         default:
             new_cipher = POLYCALL_CRYPTO_CIPHER_NONE;
             break;
     }
     
     // Only regenerate session key if cipher changes
     if (new_cipher != crypto_ctx->active_cipher) {
         // Free old session key
         secure_free_buffer(ctx, crypto_ctx, &crypto_ctx->session_key);
         
         // Set new cipher and initialize
         crypto_ctx->active_cipher = new_cipher;
         polycall_core_error_t result = initialize_cipher(ctx, crypto_ctx);
         
         if (result != POLYCALL_CORE_SUCCESS) {
             return result;
         }
         
         // Regenerate session key if we have a remote key
         if (crypto_ctx->has_remote_key && crypto_ctx->shared_secret) {
             // Re-derive session key from shared secret
             size_t key_size = crypto_ctx->session_key_size;
             size_t copy_size = key_size < crypto_ctx->shared_secret_size ? 
                               key_size : crypto_ctx->shared_secret_size;
             
             memcpy(crypto_ctx->session_key, crypto_ctx->shared_secret, copy_size);
         }
     }
     
     // Update key exchange method if necessary
     polycall_crypto_kex_t new_kex = (config->key_strength >= POLYCALL_CRYPTO_KEY_STRENGTH_MEDIUM) ?
                                    POLYCALL_CRYPTO_KEX_ECDH : POLYCALL_CRYPTO_KEX_DH;
     
     if (new_kex != crypto_ctx->key_exchange) {
         // Free old keys
         secure_free_buffer(ctx, crypto_ctx, &crypto_ctx->public_key);
         secure_free_buffer(ctx, crypto_ctx, &crypto_ctx->private_key);
         secure_free_buffer(ctx, crypto_ctx, &crypto_ctx->shared_secret);
         
         // Set new key exchange method
         crypto_ctx->key_exchange = new_kex;
         crypto_ctx->has_remote_key = false;
         
         // Generate new keys
         polycall_core_error_t result = generate_keypair(ctx, crypto_ctx);
         if (result != POLYCALL_CORE_SUCCESS) {
             return result;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 polycall_core_error_t polycall_crypto_get_public_key(
     polycall_core_context_t* ctx,
     polycall_crypto_context_t* crypto_ctx,
     uint8_t** public_key,
     size_t* public_key_size
 ) {
     if (!ctx || !crypto_ctx || !public_key || !public_key_size) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     if (!crypto_ctx->initialized || !crypto_ctx->public_key || crypto_ctx->public_key_size == 0) {
         POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                           POLYCALL_CORE_ERROR_INVALID_STATE,
                           POLYCALL_ERROR_SEVERITY_ERROR, 
                           "No public key available");
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Allocate buffer for public key copy
     *public_key = polycall_core_malloc(ctx, crypto_ctx->public_key_size);
     if (!*public_key) {
         *public_key_size = 0;
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Copy public key
     memcpy(*public_key, crypto_ctx->public_key, crypto_ctx->public_key_size);
     *public_key_size = crypto_ctx->public_key_size;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
    polycall_core_error_t polycall_crypto_get_private_key(
        polycall_core_context_t* ctx,
        polycall_crypto_context_t* crypto_ctx,
        uint8_t** private_key,
        size_t* private_key_size
    ) {
        if (!ctx || !crypto_ctx || !private_key || !private_key_size) {
            return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
        }
        
        if (!crypto_ctx->initialized || !crypto_ctx->private_key || crypto_ctx->private_key_size == 0) {
            POLYCALL_ERROR_SET(ctx, POLYCALL_ERROR_SOURCE_PROTOCOL, 
                            POLYCALL_CORE_ERROR_INVALID_STATE,
                            POLYCALL_ERROR_SEVERITY_ERROR, 
                            "No private key available");
            return POLYCALL_CORE_ERROR_INVALID_STATE;
        }
        
        // Allocate buffer for private key copy
        *private_key = polycall_core_malloc(ctx, crypto_ctx->private_key_size);
        if (!*private_key) {
            *private_key_size = 0;
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        // Copy private key
        memcpy(*private_key, crypto_ctx->private_key, crypto_ctx->private_key_size);
        *private_key_size = crypto_ctx->private_key_size;
        
        return POLYCALL_CORE_SUCCESS;
    }