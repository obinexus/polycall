/**
 * @file accessibility_audio.h
 * @brief Audio notification utilities for LibPolyCall CLI
 *
 * This file defines the audio notification system for providing
 * auditory feedback for different CLI events.
 *
 * @copyright OBINexus Computing, 2025
 */

#ifndef POLYCALL_ACCESSIBILITY_AUDIO_H
#define POLYCALL_ACCESSIBILITY_AUDIO_H

#include <stdbool.h>
#include "polycall/core/polycall/polycall_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Audio notification type enumeration
 */
typedef enum {
    POLYCALL_AUDIO_NOTIFICATION_NONE,       /**< No notification */
    POLYCALL_AUDIO_NOTIFICATION_INFO,       /**< Informational notification */
    POLYCALL_AUDIO_NOTIFICATION_SUCCESS,    /**< Success notification */
    POLYCALL_AUDIO_NOTIFICATION_WARNING,    /**< Warning notification */
    POLYCALL_AUDIO_NOTIFICATION_ERROR,      /**< Error notification */
    POLYCALL_AUDIO_NOTIFICATION_PROMPT,     /**< Command prompt notification */
    POLYCALL_AUDIO_NOTIFICATION_COMPLETION, /**< Command completion notification */
    POLYCALL_AUDIO_NOTIFICATION_PROGRESS    /**< Progress update notification */
} polycall_audio_notification_t;

/**
 * @brief Initialize audio notification system
 *
 * @param core_ctx Core context
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_audio_init(polycall_core_context_t* core_ctx);

/**
 * @brief Play audio notification
 *
 * @param core_ctx Core context
 * @param notification_type Type of notification
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_audio_play_notification(
    polycall_core_context_t* core_ctx,
    polycall_audio_notification_t notification_type
);

/**
 * @brief Enable or disable audio notifications
 *
 * @param core_ctx Core context
 * @param enable True to enable, false to disable
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_audio_enable(
    polycall_core_context_t* core_ctx,
    bool enable
);

/**
 * @brief Check if audio notifications are enabled
 *
 * @param core_ctx Core context
 * @param enabled Pointer to receive enabled status
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_audio_is_enabled(
    polycall_core_context_t* core_ctx,
    bool* enabled
);

/**
 * @brief Configure audio notification volume
 *
 * @param core_ctx Core context
 * @param volume Volume level (0-100)
 * @return polycall_core_error_t Error code
 */
polycall_core_error_t polycall_audio_set_volume(
    polycall_core_context_t* core_ctx,
    int volume
);

/**
 * @brief Cleanup audio notification system
 *
 * @param core_ctx Core context
 */
void polycall_audio_cleanup(polycall_core_context_t* core_ctx);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_ACCESSIBILITY_AUDIO_H */