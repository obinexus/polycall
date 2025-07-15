/**
 * @file polycall_logger.h
 * @brief Logging system for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This header defines the logging system for LibPolyCall,
 * providing configurable logging functionality across the library.
 */

#ifndef POLYCALL_POLYCALL_POLYCALL_LOGGER_H_H
#define POLYCALL_POLYCALL_POLYCALL_LOGGER_H_H

#include "polycall/core/polycall/polycall_core.h"
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include "polycall/core/polycall/polycall_error.h"
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stddef.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdarg.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdbool.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
#include <stdint.h>
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;

#ifdef __cplusplus
extern "C" {
#endif

/* Logging levels */
typedef enum {
  POLYCALL_LOG_DEBUG = 0,
  POLYCALL_LOG_INFO,
  POLYCALL_LOG_WARNING,
  POLYCALL_LOG_ERROR,
  POLYCALL_LOG_FATAL
} polycall_log_level_t;

/* Log destination types */
typedef enum {
  POLYCALL_LOG_DEST_CONSOLE = 0, /**< Log to console/terminal */
  POLYCALL_LOG_DEST_FILE,        /**< Log to file */
  POLYCALL_LOG_DEST_CUSTOM,      /**< Log to custom handler */
  POLYCALL_LOG_DEST_SYSLOG       /**< Log to system logger */
} polycall_log_destination_t;

/* Log format flags */
typedef enum {
  POLYCALL_LOG_FLAG_NONE = 0,
  POLYCALL_LOG_FLAG_DATE = (1 << 0),      /**< Include date in log */
  POLYCALL_LOG_FLAG_TIME = (1 << 1),      /**< Include time in log */
  POLYCALL_LOG_FLAG_LEVEL = (1 << 2),     /**< Include level in log */
  POLYCALL_LOG_FLAG_LOCATION = (1 << 3),  /**< Include file/line in log */
  POLYCALL_LOG_FLAG_THREAD_ID = (1 << 4), /**< Include thread ID in log */
  POLYCALL_LOG_FLAG_COLOR = (1 << 5), /**< Use ANSI colors in console output */
  POLYCALL_LOG_FLAG_DEFAULT = (POLYCALL_LOG_FLAG_DATE | POLYCALL_LOG_FLAG_TIME |
                               POLYCALL_LOG_FLAG_LEVEL)
} polycall_log_flags_t;

/* Logger rotation policy */
typedef enum {
  POLYCALL_LOG_ROTATE_NONE = 0, /**< No log rotation */
  POLYCALL_LOG_ROTATE_SIZE,     /**< Rotate based on size */
  POLYCALL_LOG_ROTATE_TIME      /**< Rotate based on time */
} polycall_log_rotation_policy_t;

/* Forward declarations */
typedef struct polycall_logger polycall_logger_t;

/**
 * @brief Custom log handler callback
 *
 * @param level Log level
 * @param message Log message
 * @param user_data User data pointer passed during initialization
 */
typedef void (*polycall_log_handler_t)(polycall_log_level_t level,
                                       const char *message, void *user_data);

/**
 * @brief Logger configuration
 */
typedef struct polycall_logger_config {
  polycall_log_level_t min_level;         /**< Minimum log level to record */
  polycall_log_destination_t destination; /**< Log destination */
  polycall_log_flags_t flags;             /**< Log format flags */

  /* File logging options (when destination is POLYCALL_LOG_DEST_FILE) */
  const char *log_file_path;                      /**< Log file path */
  polycall_log_rotation_policy_t rotation_policy; /**< Log rotation policy */
  size_t max_file_size; /**< Maximum log file size in bytes */
  int max_files;        /**< Maximum number of log files to keep */

  /* Custom handler (when destination is POLYCALL_LOG_DEST_CUSTOM) */
  polycall_log_handler_t custom_handler; /**< Custom log handler function */
  void *user_data;                       /**< User data for custom handler */
} polycall_logger_config_t;

/**
 * @brief Initialize the logger
 *
 * @param core_ctx Core context
 * @param logger Pointer to receive the logger instance
 * @param config Logger configuration
 * @return Error code
 */
polycall_core_error_t
polycall_logger_init(polycall_core_context_t *core_ctx,
                     polycall_logger_t **logger,
                     const polycall_logger_config_t *config);

/**
 * @brief Set the minimum log level
 *
 * @param logger Logger instance
 * @param level Minimum log level
 * @return Error code
 */
polycall_core_error_t polycall_logger_set_level(polycall_logger_t *logger,
                                                polycall_log_level_t level);

/**
 * @brief Log a message
 *
 * @param logger Logger instance
 * @param level Log level
 * @param file Source file
 * @param line Source line
 * @param format Format string
 * @param ... Additional arguments for format string
 * @return Error code
 */
polycall_core_error_t polycall_logger_log(polycall_logger_t *logger,
                                          polycall_log_level_t level,
                                          const char *file, int line,
                                          const char *format, ...);

/**
 * @brief Log a message with va_list
 *
 * @param logger Logger instance
 * @param level Log level
 * @param file Source file
 * @param line Source line
 * @param format Format string
 * @param args Variable argument list
 * @return Error code
 */
polycall_core_error_t polycall_logger_logv(polycall_logger_t *logger,
                                           polycall_log_level_t level,
                                           const char *file, int line,
                                           const char *format, va_list args);

/**
 * @brief Flush log buffer
 *
 * @param logger Logger instance
 * @return Error code
 */
polycall_core_error_t polycall_logger_flush(polycall_logger_t *logger);

/**
 * @brief Clean up logger resources
 *
 * @param logger Logger instance
 * @return Error code
 */
polycall_core_error_t polycall_logger_destroy(polycall_logger_t *logger);

/**
 * @brief Get logger default configuration
 *
 * @param config Configuration structure to populate
 * @return Error code
 */
polycall_core_error_t
polycall_logger_get_default_config(polycall_logger_config_t *config);

/**
 * @brief Convert log level to string
 *
 * @param level Log level
 * @return String representation of log level
 */
const char *polycall_logger_level_to_string(polycall_log_level_t level);

/* Convenience macros for logging */
#define POLYCALL_POLYCALL_POLYCALL_LOGGER_H_H
polycall_logger_log(logger, POLYCALL_LOG_DEBUG, __FILE__, __LINE__, format,
                    ##__VA_ARGS__)

#define POLYCALL_POLYCALL_POLYCALL_LOGGER_H_H
    polycall_logger_log(logger, POLYCALL_LOG_INFO, __FILE__, __LINE__, format,
                        ##__VA_ARGS__)

#define POLYCALL_POLYCALL_POLYCALL_LOGGER_H_H
        polycall_logger_log(logger, POLYCALL_LOG_WARNING, __FILE__, __LINE__,
                            format, ##__VA_ARGS__)

#define POLYCALL_POLYCALL_POLYCALL_LOGGER_H_H
            polycall_logger_log(logger, POLYCALL_LOG_ERROR, __FILE__, __LINE__,
                                format, ##__VA_ARGS__)

#define POLYCALL_POLYCALL_POLYCALL_LOGGER_H_H
                polycall_logger_log(logger, POLYCALL_LOG_FATAL, __FILE__,
                                    __LINE__, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_POLYCALL_POLYCALL_LOGGER_H_H */