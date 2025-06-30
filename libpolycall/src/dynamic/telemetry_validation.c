/**
 * @file telemetry_validation.c
 * @brief System-wide telemetry validation test
 * @author OBINexus LibPolyCall Testing Framework
 */

#include "polycall_test_utils.h"
#include <polycall/core/telemetry/polycall_telemetry.h>

int main(void) {
    printf("Starting telemetry system validation...\n");
    
    polycall_test_context_t* test_ctx = NULL;
    polycall_core_error_t result = polycall_test_init_context(&test_ctx);
    POLYCALL_TEST_ASSERT_SUCCESS(result, "test context initialization");
    
    // Validate telemetry is properly initialized
    POLYCALL_TEST_ASSERT(test_ctx->telemetry_ctx != NULL, "telemetry context initialized");
    
    // Generate some telemetry events
    polycall_telemetry_record_operation(test_ctx->core_ctx, test_ctx->telemetry_ctx, 
                                       "test_operation", 100);
    polycall_telemetry_record_operation(test_ctx->core_ctx, test_ctx->telemetry_ctx, 
                                       "test_operation", 150);
    
    // Validate telemetry data
    result = polycall_test_validate_telemetry(test_ctx, "test_operation", 2);
    POLYCALL_TEST_ASSERT_SUCCESS(result, "telemetry validation");
    
    // Cleanup
    polycall_test_cleanup_context(test_ctx);
    
    printf("✅ Telemetry system validation passed\n");
    return 0;
}
