/**
 * @file polycall_doctor.h
 * @brief Configuration validation and optimization for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This module provides configuration validation, verification, and optimization
 * capabilities to ensure configuration integrity and performance.
 */

#ifndef POLYCALL_DOCTOR_POLYCALL_DOCTOR_H_H
#define POLYCALL_DOCTOR_POLYCALL_DOCTOR_H_H

#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_config.h"
#include "polycall/core/polycall/polycall_error.h"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Maximum path length
 */
#define POLYCALL_DOCTOR_POLYCALL_DOCTOR_H_H

/**
 * @brief Maximum error message length
 */
#define POLYCALL_DOCTOR_POLYCALL_DOCTOR_H_H
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t
/* Forward declarations */
typedef struct polycall_core_context polycall_core_context_t;
typedef struct polycall_config_context polycall_config_context_t;
/**
 * @brief Validation severity levels
 */
typedef enum {
    POLYCALL_DOCTOR_SEVERITY_INFO = 0,    /**< Informational message */
    POLYCALL_DOCTOR_SEVERITY_WARNING,     /**< Warning, should be addressed */
    POLYCALL_DOCTOR_SEVERITY_ERROR,       /**< Error, must be fixed */
    POLYCALL_DOCTOR_SEVERITY_CRITICAL     /**< Critical issue, system may fail */
} polycall_doctor_severity_t;

/**
 * @brief Validation issue categories
 */
typedef enum {
    POLYCALL_DOCTOR_CATEGORY_SCHEMA = 0,  /**< Schema validation issues */
    POLYCALL_DOCTOR_CATEGORY_SECURITY,    /**< Security configuration issues */
    POLYCALL_DOCTOR_CATEGORY_PERFORMANCE, /**< Performance optimization issues */
    POLYCALL_DOCTOR_CATEGORY_CONSISTENCY, /**< Configuration consistency issues */
    POLYCALL_DOCTOR_CATEGORY_PORTABILITY, /**< Portability issues */
    POLYCALL_DOCTOR_CATEGORY_DEPENDENCY,  /**< Dependency issues */
    POLYCALL_DOCTOR_CATEGORY_DEPRECATION, /**< Deprecated configurations */
    POLYCALL_DOCTOR_CATEGORY_CUSTOM       /**< Custom validation issues */
} polycall_doctor_category_t;


// Doctor rule structure
struct polycall_doctor_rule {
    polycall_doctor_rule_fn rule_fn;
    polycall_doctor_category_t category;
    char path[POLYCALL_DOCTOR_MAX_PATH_LENGTH];
    void* user_data;
    uint32_t id;
    struct polycall_doctor_rule* next;
};

// Doctor context structure
struct polycall_doctor_context {
    polycall_core_context_t* core_ctx;
    polycall_doctor_config_t config;
    struct polycall_doctor_rule* rules;
    polycall_doctor_issue_t* issues;
    uint32_t issue_count;
    uint32_t issue_capacity;
    uint32_t next_rule_id;
    uint32_t fixed_count;
    time_t last_validation_time;
};

// Forward declarations for built-in validation rules
static bool validate_schema_rule(
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    const char* path,
    void* user_data,
    polycall_doctor_issue_t* issue
);

static bool validate_security_rule(
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    const char* path,
    void* user_data,
    polycall_doctor_issue_t* issue
);

static bool validate_performance_rule(
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    const char* path,
    void* user_data,
    polycall_doctor_issue_t* issue
);

static bool validate_consistency_rule(
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    const char* path,
    void* user_data,
    polycall_doctor_issue_t* issue
);

static bool validate_deprecated_rule(
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    const char* path,
    void* user_data,
    polycall_doctor_issue_t* issue
);

/**
 * @brief Validation issue structure
 */
typedef struct {
    polycall_doctor_severity_t severity;
    polycall_doctor_category_t category;
    char path[POLYCALL_DOCTOR_MAX_PATH_LENGTH];
    char message[POLYCALL_DOCTOR_MAX_ERROR_LENGTH];
    char suggestion[POLYCALL_DOCTOR_MAX_ERROR_LENGTH];
    bool auto_fixable;
} polycall_doctor_issue_t;

/**
 * @brief Validation rule structure
 */
typedef struct polycall_doctor_rule polycall_doctor_rule_t;

/**
 * @brief Doctor context opaque structure
 */
typedef struct polycall_doctor_context polycall_doctor_context_t;

/**
 * @brief Custom validation rule function type
 */
typedef bool (*polycall_doctor_rule_fn)(
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    const char* path,
    void* user_data,
    polycall_doctor_issue_t* issue
);

/**
 * @brief Doctor configuration structure
 */
typedef struct {
    bool auto_fix;                        /**< Automatically fix issues when possible */
    polycall_doctor_severity_t min_severity; /**< Minimum severity to report */
    const char* rules_path;               /**< Path to custom rules directory */
    bool validate_schema;                 /**< Validate against schema */
    bool validate_security;               /**< Validate security settings */
    bool validate_performance;            /**< Validate performance settings */
    bool validate_consistency;            /**< Validate configuration consistency */
    bool validate_dependencies;           /**< Validate dependencies */
    uint32_t timeout_ms;                  /**< Validation timeout in milliseconds */
} polycall_doctor_config_t;

/**
 * @brief Initialize doctor
 *
 * @param core_ctx Core context
 * @param doctor_ctx Pointer to receive doctor context
 * @param config Doctor configuration
 * @return Error code
 */
polycall_core_error_t polycall_doctor_init(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t** doctor_ctx,
    const polycall_doctor_config_t* config
);

/**
 * @brief Clean up doctor
 *
 * @param core_ctx Core context
 * @param doctor_ctx Doctor context
 */
void polycall_doctor_cleanup(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx
);

/**
 * @brief Validate configuration
 *
 * @param core_ctx Core context
 * @param doctor_ctx Doctor context
 * @param config_ctx Configuration context to validate
 * @return Error code
 */
polycall_core_error_t polycall_doctor_validate(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx
);

/**
 * @brief Register a custom validation rule
 *
 * @param core_ctx Core context
 * @param doctor_ctx Doctor context
 * @param rule Rule function
 * @param category Issue category
 * @param path Configuration path to validate (NULL for all)
 * @param user_data User data for rule function
 * @param rule_id Pointer to receive rule ID
 * @return Error code
 */
polycall_core_error_t polycall_doctor_register_rule(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    polycall_doctor_rule_fn rule,
    polycall_doctor_category_t category,
    const char* path,
    void* user_data,
    uint32_t* rule_id
);

/**
 * @brief Unregister a custom validation rule
 *
 * @param core_ctx Core context
 * @param doctor_ctx Doctor context
 * @param rule_id Rule ID
 * @return Error code
 */
polycall_core_error_t polycall_doctor_unregister_rule(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    uint32_t rule_id
);

/**
 * @brief Get validation issues
 *
 * @param core_ctx Core context
 * @param doctor_ctx Doctor context
 * @param issues Array to receive issues
 * @param max_issues Maximum number of issues to retrieve
 * @param issue_count Pointer to receive issue count
 * @return Error code
 */
polycall_core_error_t polycall_doctor_get_issues(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    polycall_doctor_issue_t* issues,
    uint32_t max_issues,
    uint32_t* issue_count
);

/**
 * @brief Fix validation issues automatically
 *
 * @param core_ctx Core context
 * @param doctor_ctx Doctor context
 * @param config_ctx Configuration context to fix
 * @param fixed_count Pointer to receive count of fixed issues
 * @return Error code
 */
polycall_core_error_t polycall_doctor_fix_issues(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    uint32_t* fixed_count
);

/**
 * @brief Generate optimization suggestions
 *
 * @param core_ctx Core context
 * @param doctor_ctx Doctor context
 * @param config_ctx Configuration context to optimize
 * @param suggestions Array to receive suggestions
 * @param max_suggestions Maximum number of suggestions to retrieve
 * @param suggestion_count Pointer to receive suggestion count
 * @return Error code
 */
polycall_core_error_t polycall_doctor_optimize(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    polycall_doctor_issue_t* suggestions,
    uint32_t max_suggestions,
    uint32_t* suggestion_count
);

/**
 * @brief Verify configuration portability
 *
 * @param core_ctx Core context
 * @param doctor_ctx Doctor context
 * @param config_ctx Configuration context to verify
 * @param portability_score Pointer to receive portability score (0-100)
 * @return Error code
 */
polycall_core_error_t polycall_doctor_verify_portability(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    uint32_t* portability_score
);

/**
 * @brief Generate configuration report
 *
 * @param core_ctx Core context
 * @param doctor_ctx Doctor context
 * @param config_ctx Configuration context to report on
 * @param report_path Path to output report file
 * @param report_format Report format (text, json, html)
 * @return Error code
 */
polycall_core_error_t polycall_doctor_generate_report(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    const char* report_path,
    const char* report_format
);

/**
 * @brief Create default doctor configuration
 *
 * @return Default doctor configuration
 */
polycall_doctor_config_t polycall_doctor_default_config(void);

#ifdef __cplusplus
}
#endif

#endif /* POLYCALL_DOCTOR_POLYCALL_DOCTOR_H_H */
