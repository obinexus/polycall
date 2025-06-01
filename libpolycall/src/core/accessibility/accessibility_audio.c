/**
 * @file accessibility_audio.c
 * @brief Implementation of audio notification utilities for LibPolyCall CLI
 *
 * @copyright OBINexus Computing, 2025
 */

#include "polycall/core/accessibility/accessibility_audio.h"
#include "polycall/core/polycall/polycall_memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <windows.h>
#endif

/**
 * @brief Audio context structure
 */
typedef struct {
    bool enabled;
    int volume;  // 0-100
} polycall_audio_context_t;

// Audio context instance
static polycall_audio_context_t* g_audio_ctx = NULL;

/**
 * @brief Initialize audio notification system
 */
polycall_core_error_t polycall_audio_init(polycall_core_context_t* core_ctx) {
    if (!core_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    if (g_audio_ctx) {
        return POLYCALL_CORE_ERROR_ALREADY_INITIALIZED;
    }
    
    // Allocate audio context
    g_audio_ctx = (polycall_audio_context_t*)malloc(sizeof(polycall_audio_context_t));
    if (!g_audio_ctx) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize with default values
    g_audio_ctx->enabled = true;  // Enable by default
    g_audio_ctx->volume = 80;     // 80% volume by default
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Play terminal bell with specific frequency and duration
 * 
 * This is a platform-specific implementation to provide different tones
 */
static void play_bell_with_tone(int frequency, int duration_ms) {
#ifdef _WIN32
    // Windows implementation using Beep API
    Beep(frequency, duration_ms);
#else
    // POSIX implementation using terminal escape sequences
    // \a is the ASCII bell character
    // We use multiple bells with usleep to create different tones
    for (int i = 0; i < duration_ms / 100; i++) {
        putchar('\a');
        fflush(stdout);
        usleep(100000);  // 100ms sleep
    }
#endif
}

/**
 * @brief Play audio notification
 */
polycall_core_error_t polycall_audio_play_notification(
    polycall_core_context_t* core_ctx,
    polycall_audio_notification_t notification_type
) {
    if (!core_ctx || !g_audio_ctx) {
        return POLYCALL_CORE_ERROR_NOT_INITIALIZED;
    }
    
    // Skip if audio is disabled
    if (!g_audio_ctx->enabled) {
        return POLYCALL_CORE_SUCCESS;
    }
    
    // Configure frequency and duration based on notification type
    int frequency = 0;
    int duration_ms = 0;
    
    switch (notification_type) {
        case POLYCALL_AUDIO_NOTIFICATION_INFO:
            frequency = 800;
            duration_ms = 100;
            break;
            
        case POLYCALL_AUDIO_NOTIFICATION_SUCCESS:
            frequency = 1200;
            duration_ms = 150;
            break;
            
        case POLYCALL_AUDIO_NOTIFICATION_WARNING:
            frequency = 600;
            duration_ms = 200;
            break;
            
        case POLYCALL_AUDIO_NOTIFICATION_ERROR:
            frequency = 400;
            duration_ms = 300;
            break;
            
        case POLYCALL_AUDIO_NOTIFICATION_PROMPT:
            frequency = 1000;
            duration_ms = 100;
            break;
            
        case POLYCALL_AUDIO_NOTIFICATION_COMPLETION:
            frequency = 1500;
            duration_ms = 200;
            break;
            
        case POLYCALL_AUDIO_NOTIFICATION_PROGRESS:
            frequency = 900;
            duration_ms = 80;
            break;
            
        case POLYCALL_AUDIO_NOTIFICATION_NONE:
        default:
            return POLYCALL_CORE_SUCCESS;  // No notification to play
    }
    
    // Play the configured bell tone
    play_bell_with_tone(frequency, duration_ms);
    
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Enable or disable audio notifications
 */
polycall_core_error_t polycall_audio_enable(
    polycall_core_context_t* core_ctx,
    bool enable
) {
    if (!core_ctx || !g_audio_ctx) {
        return POLYCALL_CORE_ERROR_NOT_INITIALIZED;
    }
    
    g_audio_ctx->enabled = enable;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Check if audio notifications are enabled
 */
polycall_core_error_t polycall_audio_is_enabled(
    polycall_core_context_t* core_ctx,
    bool* enabled
) {
    if (!core_ctx || !g_audio_ctx || !enabled) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    *enabled = g_audio_ctx->enabled;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Configure audio notification volume
 */
polycall_core_error_t polycall_audio_set_volume(
    polycall_core_context_t* core_ctx,
    int volume
) {
    if (!core_ctx || !g_audio_ctx) {
        return POLYCALL_CORE_ERROR_NOT_INITIALIZED;
    }
    
    // Validate volume range
    if (volume < 0 || volume > 100) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    g_audio_ctx->volume = volume;
    return POLYCALL_CORE_SUCCESS;
}

/**
 * @brief Cleanup audio notification system
 */
void polycall_audio_cleanup(polycall_core_context_t* core_ctx) {
    if (g_audio_ctx) {
        free(g_audio_ctx);
        g_audio_ctx = NULL;
    }
}