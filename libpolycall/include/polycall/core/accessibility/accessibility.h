/**
 * @file accessibility.h
 * @brief Main Accessibility Module Umbrella Header
 * @author OBINexus Computing - LibPolyCall Framework
 * 
 * This umbrella header provides unified access to the accessibility subsystem
 * following IoC container patterns and zero-trust security architecture.
 */

#ifndef POLYCALL_ACCESSIBILITY_H
#define POLYCALL_ACCESSIBILITY_H

#ifdef __cplusplus
extern "C" {
#endif

/* Core Dependencies - Order Matters for Forward Declarations */
#include <polycall/core/polycall/polycall_core.h>
#include <polycall/core/polycall/polycall_error.h>
#include <polycall/core/polycall/polycall_context.h>

/* Accessibility Module Components */
#include <polycall/core/accessibility/accessibility_error.h>
#include <polycall/core/accessibility/accessibility_config.h>
#include <polycall/core/accessibility/accessibility_audio.h>
#include <polycall/core/accessibility/accessibility_colors.h>
#include <polycall/core/accessibility/accessibility_container.h>
#include <polycall/core/accessibility/accessibility_interface.h>
#include <polycall/core/accessibility/accessibility_registry.h>
#include <polycall/core/accessibility/polycall_accessibility_error.h>

/**
 * @brief Accessibility Context Structure
 * 
 * IoC container for accessibility module state management
 * following zero-trust security principles.
 */
typedef struct polycall_accessibility_context {
    polycall_core_context_t*        core_ctx;           /**< Core context reference */
    polycall_context_ref_t*         context_ref;        /**< Context registry reference */
    void*                           audio_ctx;          /**< Audio accessibility context */
    void*                           visual_ctx;         /**< Visual accessibility context */
    void*                           config_ctx;         /**< Configuration context */
    polycall_context_flags_t        flags;              /**< Module flags */
    polycall_memory_pool_t*         memory_pool;        /**< Dedicated memory pool */
    bool                            initialized;        /**< Initialization state */
} polycall_accessibility_context_t;

/**
 * @brief Accessibility Configuration Parameters
 */
typedef struct polycall_accessibility_config {
    bool                            audio_enabled;      /**< Audio notifications enabled */
    bool                            visual_enabled;     /**< Visual enhancements enabled */
    bool                            high_contrast;      /**< High contrast mode */
    float                           audio_volume;       /**< Audio volume level (0.0-1.0) */
    uint32_t                        notification_tone;  /**< Notification tone frequency */
    polycall_memory_flags_t         memory_flags;       /**< Memory allocation flags */
} polycall_accessibility_config_t;

/* =================================================================
 * IoC Container Interface - Primary API
 * ================================================================= */

/**
 * @brief Create accessibility context from Polycallfile configuration
 * 
 * IoC factory method that initializes the accessibility subsystem
 * using configuration parameters from config.Polycallfile.
 * 
 * @param core_ctx Core context (must be initialized)
 * @return Accessibility context or NULL on failure
 */
polycall_accessibility_context_t* polycall_accessibility_context_create(
    polycall_core_context_t* core_ctx
);

/**
 * @brief Create accessibility context with explicit configuration
 * 
 * @param core_ctx Core context (must be initialized)
 * @param config Accessibility configuration parameters
 * @return Accessibility context or NULL on failure
 */
polycall_accessibility_context_t* polycall_accessibility_context_create_with_config(
    polycall_core_context_t* core_ctx,
    const polycall_accessibility_config_t* config
);

/**
 * @brief Initialize accessibility subsystem
 * 
 * Registers accessibility components with IoC container and
 * initializes audio/visual subsystems.
 * 
 * @param access_ctx Accessibility context
 * @return Error code
 */
polycall_core_error_t polycall_accessibility_init(
    polycall_accessibility_context_t* access_ctx
);

/**
 * @brief Clean up accessibility subsystem
 * 
 * Unregisters components from IoC container and releases resources.
 * 
 * @param access_ctx Accessibility context
 */
void polycall_accessibility_cleanup(
    polycall_accessibility_context_t* access_ctx
);

/**
 * @brief Get accessibility configuration from context
 * 
 * @param access_ctx Accessibility context
 * @param config Buffer to receive configuration
 * @return Error code
 */
polycall_core_error_t polycall_accessibility_get_config(
    polycall_accessibility_context_t* access_ctx,
    polycall_accessibility_config_t* config
);

/**
 * @brief Update accessibility configuration
 * 
 * @param access_ctx Accessibility context
 * @param config New configuration parameters
 * @return Error code
 */
polycall_core_error_t polycall_accessibility_set_config(
    polycall_accessibility_context_t* access_ctx,
    const polycall_accessibility_config_t* config
);

/* =================================================================
 * Component Access Methods - IoC Service Locator Pattern
 * ================================================================= */

/**
 * @brief Get audio accessibility interface
 * 
 * @param access_ctx Accessibility context
 * @return Audio interface or NULL if not initialized
 */
void* polycall_accessibility_get_audio_interface(
    polycall_accessibility_context_t* access_ctx
);

/**
 * @brief Get visual accessibility interface
 * 
 * @param access_ctx Accessibility context  
 * @return Visual interface or NULL if not initialized
 */
void* polycall_accessibility_get_visual_interface(
    polycall_accessibility_context_t* access_ctx
);

/**
 * @brief Get configuration interface
 * 
 * @param access_ctx Accessibility context
 * @return Configuration interface or NULL if not initialized
 */
void* polycall_accessibility_get_config_interface(
    polycall_accessibility_context_t* access_ctx
);

/* =================================================================
 * High-Level Convenience API
 * ================================================================= */

/**
 * @brief Play accessibility notification
 * 
 * @param access_ctx Accessibility context
 * @param notification_type Type of notification
 * @return Error code
 */
polycall_core_error_t polycall_accessibility_notify(
    polycall_accessibility_context_t* access_ctx,
    int notification_type
);

/**
 * @brief Check if accessibility features are enabled
 * 
 * @param access_ctx Accessibility context
 * @param feature_enabled Pointer to receive status
 * @return Error code
 */
polycall_core_error_t polycall_accessibility_is_enabled(
    polycall_accessibility_context_t* access_ctx,
    bool* feature_enabled
);

/**
 * @brief Get accessibility status information
 * 
 * @param access_ctx Accessibility context
 * @param status_buffer Buffer to receive status string
 * @param buffer_size Size of status buffer
 * @return Error code
 */
polycall_core_error_t polycall_accessibility_get_status(
    polycall_accessibility_context_t* access_ctx,
    char* status_buffer,
    size_t buffer_size
);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_ACCESSIBILITY_H */