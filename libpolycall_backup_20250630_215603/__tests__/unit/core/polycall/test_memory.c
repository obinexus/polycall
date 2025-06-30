/**
 * @file test_memory.c
 * @brief Unit tests for the memory management functionality in LibPolyCall
 * @author Nnamdi Okpala (OBINexusComputing)
 */

 #include "unit_test_framework.h"
 #include "polycall/core/polycall/polycall_memory.h"
 #include "polycall/core/polycall/polycall_core.h"
 #include <stdio.h>
 #include <string.h>
 
 // Global variables for test
 static polycall_core_context_t* g_core_ctx = NULL;
 static polycall_memory_pool_t* g_test_pool = NULL;
 
 // Custom allocator tracking for testing
 static int g_custom_malloc_called = 0;
 static int g_custom_free_called = 0;
 static void* g_last_allocated_ptr = NULL;
 static size_t g_last_allocation_size = 0;
 
 // Custom malloc function for testing
 static void* test_custom_malloc(size_t size, void* user_data) {
     g_custom_malloc_called++;
     g_last_allocation_size = size;
     g_last_allocated_ptr = malloc(size);
     return g_last_allocated_ptr;
 }
 
 // Custom free function for testing
 static void test_custom_free(void* ptr, void* user_data) {
     g_custom_free_called++;
     if (ptr == g_last_allocated_ptr) {
         g_last_allocated_ptr = NULL;
     }
     free(ptr);
 }
 
 // Initialize test resources
 static void setup() {
     // Initialize the core context
     polycall_core_config_t config = {
         .flags = POLYCALL_CORE_FLAG_NONE,
         .memory_pool_size = 1024 * 1024,  // 1MB for testing
         .user_data = NULL,
         .error_callback = NULL
     };
     
     polycall_core_init(&g_core_ctx, &config);
     
     // Reset custom allocator tracking
     g_custom_malloc_called = 0;
     g_custom_free_called = 0;
     g_last_allocated_ptr = NULL;
     g_last_allocation_size = 0;
 }
 
 // Clean up test resources
 static void teardown() {
     // Destroy memory pool if it exists
     if (g_test_pool) {
         polycall_memory_destroy_pool(g_core_ctx, g_test_pool);
         g_test_pool = NULL;
     }
     
     // Clean up the core context
     if (g_core_ctx) {
         polycall_core_cleanup(g_core_ctx);
         g_core_ctx = NULL;
     }
 }
 
 // Test memory pool creation
 static int test_memory_pool_creation() {
     // Create a memory pool
     polycall_core_error_t result = polycall_memory_create_pool(
         g_core_ctx,
         &g_test_pool,
         1024 * 1024  // 1MB pool
     );
     
     // Verify creation succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     ASSERT_NOT_NULL(g_test_pool);
     
     return 0;
 }
 
 // Test memory allocation and free
 static int test_memory_alloc_free() {
     // Create a memory pool
     polycall_memory_create_pool(g_core_ctx, &g_test_pool, 1024 * 1024);
     
     // Allocate memory from the pool
     size_t alloc_size = 1024;
     void* ptr = polycall_memory_alloc(
         g_core_ctx,
         g_test_pool,
         alloc_size,
         POLYCALL_MEMORY_FLAG_NONE
     );
     
     // Verify allocation succeeded
     ASSERT_NOT_NULL(ptr);
     
     // Write to the memory to ensure it's usable
     memset(ptr, 0xAA, alloc_size);
     
     // Free the memory
     polycall_memory_free(g_core_ctx, g_test_pool, ptr);
     
     // Pool statistics should be updated (implementation specific, but we can verify basics)
     polycall_memory_stats_t stats;
     polycall_memory_get_stats(g_core_ctx, g_test_pool, &stats);
     
     // Verify allocation count matches free count
     ASSERT_EQUAL_INT(stats.allocation_count, stats.free_count);
     
     return 0;
 }
 
 // Test memory flags (zero init)
 static int test_memory_zero_init() {
     // Create a memory pool
     polycall_memory_create_pool(g_core_ctx, &g_test_pool, 1024 * 1024);
     
     // Allocate memory with zero init flag
     size_t alloc_size = 1024;
     void* ptr = polycall_memory_alloc(
         g_core_ctx,
         g_test_pool,
         alloc_size,
         POLYCALL_MEMORY_FLAG_ZERO_INIT
     );
     
     // Verify allocation succeeded
     ASSERT_NOT_NULL(ptr);
     
     // Verify memory is zeroed
     unsigned char* bytes = (unsigned char*)ptr;
     for (size_t i = 0; i < alloc_size; i++) {
         ASSERT_EQUAL_INT(0, bytes[i]);
     }
     
     // Free the memory
     polycall_memory_free(g_core_ctx, g_test_pool, ptr);
     
     return 0;
 }
 
 // Test memory reallocation
 static int test_memory_realloc() {
     // Create a memory pool
     polycall_memory_create_pool(g_core_ctx, &g_test_pool, 1024 * 1024);
     
     // Allocate initial memory
     size_t initial_size = 128;
     void* ptr = polycall_memory_alloc(
         g_core_ctx,
         g_test_pool,
         initial_size,
         POLYCALL_MEMORY_FLAG_NONE
     );
     
     // Verify allocation succeeded
     ASSERT_NOT_NULL(ptr);
     
     // Fill memory with a pattern
     memset(ptr, 0xBB, initial_size);
     
     // Reallocate to a larger size
     size_t new_size = 256;
     void* new_ptr = polycall_memory_realloc(
         g_core_ctx,
         g_test_pool,
         ptr,
         new_size
     );
     
     // Verify reallocation succeeded
     ASSERT_NOT_NULL(new_ptr);
     
     // Verify original data is preserved
     unsigned char* bytes = (unsigned char*)new_ptr;
     for (size_t i = 0; i < initial_size; i++) {
         ASSERT_EQUAL_INT(0xBB, bytes[i]);
     }
     
     // Free the reallocated memory
     polycall_memory_free(g_core_ctx, g_test_pool, new_ptr);
     
     return 0;
 }
 
 // Test memory region creation
 static int test_memory_region() {
     // Create a memory pool
     polycall_memory_create_pool(g_core_ctx, &g_test_pool, 1024 * 1024);
     
     // Create a memory region
     polycall_memory_region_t* region = polycall_memory_create_region(
         g_core_ctx,
         g_test_pool,
         1024,
         POLYCALL_MEMORY_PERM_READ | POLYCALL_MEMORY_PERM_WRITE,
         POLYCALL_MEMORY_FLAG_NONE,
         "TestOwner"
     );
     
     // Verify region creation succeeded
     ASSERT_NOT_NULL(region);
     ASSERT_NOT_NULL(region->base);
     ASSERT_EQUAL_INT(1024, region->size);
     
     // Verify permissions
     ASSERT_TRUE(polycall_memory_verify_permissions(
         g_core_ctx,
         region,
         "TestOwner",
         POLYCALL_MEMORY_PERM_READ | POLYCALL_MEMORY_PERM_WRITE
     ));
     
     // Verify permissions for unauthorized component
     ASSERT_FALSE(polycall_memory_verify_permissions(
         g_core_ctx,
         region,
         "UnauthorizedComponent",
         POLYCALL_MEMORY_PERM_READ
     ));
     
     // Share the region
     polycall_core_error_t result = polycall_memory_share_region(
         g_core_ctx,
         region,
         "SharedComponent"
     );
     
     // Verify sharing succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Verify permissions for shared component
     ASSERT_TRUE(polycall_memory_verify_permissions(
         g_core_ctx,
         region,
         "SharedComponent",
         POLYCALL_MEMORY_PERM_READ | POLYCALL_MEMORY_PERM_WRITE
     ));
     
     // Unshare the region
     result = polycall_memory_unshare_region(g_core_ctx, region);
     
     // Verify unsharing succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Verify permissions for formerly shared component
     ASSERT_FALSE(polycall_memory_verify_permissions(
         g_core_ctx,
         region,
         "SharedComponent",
         POLYCALL_MEMORY_PERM_READ
     ));
     
     // Destroy the region
     polycall_memory_destroy_region(g_core_ctx, g_test_pool, region);
     
     return 0;
 }
 
 // Test memory statistics
 static int test_memory_stats() {
     // Create a memory pool
     polycall_memory_create_pool(g_core_ctx, &g_test_pool, 1024 * 1024);
     
     // Get initial stats
     polycall_memory_stats_t initial_stats;
     polycall_memory_get_stats(g_core_ctx, g_test_pool, &initial_stats);
     
     // Allocate some memory
     void* ptr1 = polycall_memory_alloc(g_core_ctx, g_test_pool, 1024, POLYCALL_MEMORY_FLAG_NONE);
     void* ptr2 = polycall_memory_alloc(g_core_ctx, g_test_pool, 2048, POLYCALL_MEMORY_FLAG_NONE);
     
     // Get updated stats
     polycall_memory_stats_t mid_stats;
     polycall_memory_get_stats(g_core_ctx, g_test_pool, &mid_stats);
     
     // Verify allocation stats increased
     ASSERT_TRUE(mid_stats.allocation_count > initial_stats.allocation_count);
     ASSERT_TRUE(mid_stats.current_usage > initial_stats.current_usage);
     
     // Free one allocation
     polycall_memory_free(g_core_ctx, g_test_pool, ptr1);
     
     // Get updated stats
     polycall_memory_stats_t after_free_stats;
     polycall_memory_get_stats(g_core_ctx, g_test_pool, &after_free_stats);
     
     // Verify free stats increased
     ASSERT_TRUE(after_free_stats.free_count > mid_stats.free_count);
     ASSERT_TRUE(after_free_stats.current_usage < mid_stats.current_usage);
     
     // Free remaining allocation
     polycall_memory_free(g_core_ctx, g_test_pool, ptr2);
     
     // Get final stats
     polycall_memory_stats_t final_stats;
     polycall_memory_get_stats(g_core_ctx, g_test_pool, &final_stats);
     
     // Verify all memory is freed
     ASSERT_EQUAL_INT(final_stats.allocation_count, final_stats.free_count);
     
     return 0;
 }
 
 // Test memory pool reset
 static int test_memory_reset() {
     // Create a memory pool
     polycall_memory_create_pool(g_core_ctx, &g_test_pool, 1024 * 1024);
     
     // Allocate some memory
     void* ptr = polycall_memory_alloc(g_core_ctx, g_test_pool, 1024, POLYCALL_MEMORY_FLAG_NONE);
     
     // Reset the pool (note: this invalidates all allocations)
     polycall_core_error_t result = polycall_memory_reset_pool(g_core_ctx, g_test_pool);
     
     // Verify reset succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Get stats after reset
     polycall_memory_stats_t stats;
     polycall_memory_get_stats(g_core_ctx, g_test_pool, &stats);
     
     // Verify all counters are reset
     ASSERT_EQUAL_INT(0, stats.current_usage);
     ASSERT_EQUAL_INT(0, stats.allocation_count);
     ASSERT_EQUAL_INT(0, stats.free_count);
     
     // Allocate memory again to verify pool is still usable
     void* new_ptr = polycall_memory_alloc(g_core_ctx, g_test_pool, 1024, POLYCALL_MEMORY_FLAG_NONE);
     
     // Verify allocation succeeded
     ASSERT_NOT_NULL(new_ptr);
     
     // Free memory
     polycall_memory_free(g_core_ctx, g_test_pool, new_ptr);
     
     return 0;
 }
 
 // Test custom allocator
 static int test_custom_allocator() {
     // This test is a bit tricky as we can't directly modify the pool's custom allocator
     // Instead, we'll test the core memory functions which can use custom allocators
     
     // Set custom memory functions on the core context
     polycall_core_error_t result = polycall_core_set_memory_functions(
         g_core_ctx,
         test_custom_malloc,
         test_custom_free,
         NULL
     );
     
     // Verify setting functions succeeded
     ASSERT_EQUAL_INT(POLYCALL_CORE_SUCCESS, result);
     
     // Use core malloc function
     void* ptr = polycall_core_malloc(g_core_ctx, 1024);
     
     // Verify allocation succeeded
     ASSERT_NOT_NULL(ptr);
     
     // Verify custom malloc was called
     ASSERT_TRUE(g_custom_malloc_called > 0);
     ASSERT_EQUAL_INT(1024, g_last_allocation_size);
     
     // Free memory
     polycall_core_free(g_core_ctx, ptr);
     
     // Verify custom free was called
     ASSERT_TRUE(g_custom_free_called > 0);
     
     return 0;
 }
 
 // Run all memory tests
 int run_memory_tests() {
     RESET_TESTS();
     
     setup();
     
     RUN_TEST(test_memory_pool_creation);
     RUN_TEST(test_memory_alloc_free);
     RUN_TEST(test_memory_zero_init);
     RUN_TEST(test_memory_realloc);
     RUN_TEST(test_memory_region);
     RUN_TEST(test_memory_stats);
     RUN_TEST(test_memory_reset);
     RUN_TEST(test_custom_allocator);
     
     teardown();
     
     return g_tests_failed > 0 ? 1 : 0;
 }
