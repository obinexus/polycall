/* Standard library includes */
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>

/* Core types */
#include "polycall/core/types.h"

/**
 * @file polycall_doctor.c
 * @brief Configuration validation and optimization implementation for LibPolyCall
 * @author Implementation based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * This file implements the configuration validator and optimizer for the
 * LibPolyCall system, providing comprehensive validation, diagnostic, and
 * suggestion capabilities to ensure configuration integrity and performance.
 */

#include "polycall/core/polycall/polycall_core.h"
#include "polycall/core/polycall/polycall_memory.h"
#include "polycall/core/polycall/polycall_logger.h"
#include <string.h>
#include <time.h>
// Helper functions for issue management
static polycall_core_error_t add_issue(
    polycall_doctor_context_t* doctor_ctx,
    const polycall_doctor_issue_t* issue
) {
    if (!doctor_ctx || !issue) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Check minimum severity threshold
    if (issue->severity < doctor_ctx->config.min_severity) {
        return POLYCALL_CORE_SUCCESS; // Ignore issues below threshold
    }
    
    // Check if we need to resize the issues array
    if (doctor_ctx->issue_count >= doctor_ctx->issue_capacity) {
        uint32_t new_capacity = doctor_ctx->issue_capacity == 0 ? 16 : doctor_ctx->issue_capacity * 2;
        polycall_doctor_issue_t* new_issues = polycall_core_malloc(
            doctor_ctx->core_ctx,
            new_capacity * sizeof(polycall_doctor_issue_t)
        );
        
        if (!new_issues) {
            return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
        }
        
        // Copy existing issues to new array
        if (doctor_ctx->issues) {
            memcpy(new_issues, doctor_ctx->issues, doctor_ctx->issue_count * sizeof(polycall_doctor_issue_t));
            polycall_core_free(doctor_ctx->core_ctx, doctor_ctx->issues);
        }
        
        doctor_ctx->issues = new_issues;
        doctor_ctx->issue_capacity = new_capacity;
    }
    
    // Add new issue
    memcpy(&doctor_ctx->issues[doctor_ctx->issue_count], issue, sizeof(polycall_doctor_issue_t));
    doctor_ctx->issue_count++;
    
    return POLYCALL_CORE_SUCCESS;
}

static void clear_issues(polycall_doctor_context_t* doctor_ctx) {
    if (!doctor_ctx) {
        return;
    }
    
    doctor_ctx->issue_count = 0;
    doctor_ctx->fixed_count = 0;
}

// Initialize doctor
polycall_core_error_t polycall_doctor_init(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t** doctor_ctx,
    const polycall_doctor_config_t* config
) {
    if (!core_ctx || !doctor_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Allocate context
    polycall_doctor_context_t* new_ctx = polycall_core_malloc(core_ctx, sizeof(polycall_doctor_context_t));
    if (!new_ctx) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize context
    memset(new_ctx, 0, sizeof(polycall_doctor_context_t));
    new_ctx->core_ctx = core_ctx;
    new_ctx->next_rule_id = 1;
    
    // Set configuration
    if (config) {
        memcpy(&new_ctx->config, config, sizeof(polycall_doctor_config_t));
    } else {
        new_ctx->config = polycall_doctor_default_config();
    }
    
    // Register built-in validation rules
    uint32_t rule_id;
    
    if (new_ctx->config.validate_schema) {
        polycall_doctor_register_rule(core_ctx, new_ctx, validate_schema_rule,
            POLYCALL_DOCTOR_CATEGORY_SCHEMA, NULL, NULL, &rule_id);
    }
    
    if (new_ctx->config.validate_security) {
        polycall_doctor_register_rule(core_ctx, new_ctx, validate_security_rule,
            POLYCALL_DOCTOR_CATEGORY_SECURITY, NULL, NULL, &rule_id);
    }
    
    if (new_ctx->config.validate_performance) {
        polycall_doctor_register_rule(core_ctx, new_ctx, validate_performance_rule,
            POLYCALL_DOCTOR_CATEGORY_PERFORMANCE, NULL, NULL, &rule_id);
    }
    
    if (new_ctx->config.validate_consistency) {
        polycall_doctor_register_rule(core_ctx, new_ctx, validate_consistency_rule,
            POLYCALL_DOCTOR_CATEGORY_CONSISTENCY, NULL, NULL, &rule_id);
    }
    
    // Always register deprecation check
    polycall_doctor_register_rule(core_ctx, new_ctx, validate_deprecated_rule,
        POLYCALL_DOCTOR_CATEGORY_DEPRECATION, NULL, NULL, &rule_id);
    
    // Initialize issue storage
    new_ctx->issue_capacity = 16;
    new_ctx->issues = polycall_core_malloc(core_ctx, new_ctx->issue_capacity * sizeof(polycall_doctor_issue_t));
    if (!new_ctx->issues) {
        polycall_core_free(core_ctx, new_ctx);
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    *doctor_ctx = new_ctx;
    return POLYCALL_CORE_SUCCESS;
}

// Clean up doctor
void polycall_doctor_cleanup(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx
) {
    if (!core_ctx || !doctor_ctx) {
        return;
    }
    
    // Free rules
    struct polycall_doctor_rule* rule = doctor_ctx->rules;
    while (rule) {
        struct polycall_doctor_rule* next = rule->next;
        polycall_core_free(core_ctx, rule);
        rule = next;
    }
    
    // Free issues
    if (doctor_ctx->issues) {
        polycall_core_free(core_ctx, doctor_ctx->issues);
    }
    
    // Free context
    polycall_core_free(core_ctx, doctor_ctx);
}

// Create default doctor configuration
polycall_doctor_config_t polycall_doctor_default_config(void) {
    polycall_doctor_config_t config;
    
    config.auto_fix = false;
    config.min_severity = POLYCALL_DOCTOR_SEVERITY_WARNING;
    config.rules_path = NULL;
    config.validate_schema = true;
    config.validate_security = true;
    config.validate_performance = true;
    config.validate_consistency = true;
    config.validate_dependencies = true;
    config.timeout_ms = 5000;
    
    return config;
}

// Register a custom validation rule
polycall_core_error_t polycall_doctor_register_rule(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    polycall_doctor_rule_fn rule,
    polycall_doctor_category_t category,
    const char* path,
    void* user_data,
    uint32_t* rule_id
) {
    if (!core_ctx || !doctor_ctx || !rule || !rule_id) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Allocate new rule
    struct polycall_doctor_rule* new_rule = polycall_core_malloc(core_ctx, sizeof(struct polycall_doctor_rule));
    if (!new_rule) {
        return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
    }
    
    // Initialize rule
    new_rule->rule_fn = rule;
    new_rule->category = category;
    new_rule->user_data = user_data;
    new_rule->id = doctor_ctx->next_rule_id++;
    new_rule->next = NULL;
    
    // Copy path if provided
    if (path) {
        strncpy(new_rule->path, path, POLYCALL_DOCTOR_MAX_PATH_LENGTH - 1);
        new_rule->path[POLYCALL_DOCTOR_MAX_PATH_LENGTH - 1] = '\0';
    } else {
        new_rule->path[0] = '\0';
    }
    
    // Add to list
    if (!doctor_ctx->rules) {
        doctor_ctx->rules = new_rule;
    } else {
        struct polycall_doctor_rule* last = doctor_ctx->rules;
        while (last->next) {
            last = last->next;
        }
        last->next = new_rule;
    }
    
    *rule_id = new_rule->id;
    return POLYCALL_CORE_SUCCESS;
}

// Unregister a custom validation rule
polycall_core_error_t polycall_doctor_unregister_rule(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    uint32_t rule_id
) {
    if (!core_ctx || !doctor_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    struct polycall_doctor_rule* rule = doctor_ctx->rules;
    struct polycall_doctor_rule* prev = NULL;
    
    while (rule) {
        if (rule->id == rule_id) {
            // Found the rule to remove
            if (prev) {
                prev->next = rule->next;
            } else {
                doctor_ctx->rules = rule->next;
            }
            
            polycall_core_free(core_ctx, rule);
            return POLYCALL_CORE_SUCCESS;
        }
        
        prev = rule;
        rule = rule->next;
    }
    
    return POLYCALL_CORE_ERROR_NOT_FOUND;
}

// Validate configuration
polycall_core_error_t polycall_doctor_validate(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx
) {
    if (!core_ctx || !doctor_ctx || !config_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Clear previous issues
    clear_issues(doctor_ctx);
    
    // Record validation time
    time(&doctor_ctx->last_validation_time);
    
    // Run validation rules
    struct polycall_doctor_rule* rule = doctor_ctx->rules;
    while (rule) {
        polycall_doctor_issue_t issue;
        memset(&issue, 0, sizeof(issue));
        
        // Check if rule matches path
        if (rule->path[0] == '\0' || strstr(rule->path, "*") != NULL) {
            // Run rule for all paths or patterns
            // For simplicity in this implementation, we're not doing pattern matching
            
            // Execute rule
            if (rule->rule_fn(doctor_ctx, config_ctx, NULL, rule->user_data, &issue)) {
                // Rule found an issue
                issue.category = rule->category;
                add_issue(doctor_ctx, &issue);
            }
        } else {
            // Run rule for specific path
            if (rule->rule_fn(doctor_ctx, config_ctx, rule->path, rule->user_data, &issue)) {
                // Rule found an issue
                issue.category = rule->category;
                add_issue(doctor_ctx, &issue);
            }
        }
        
        rule = rule->next;
    }
    
    // Auto-fix issues if enabled
    if (doctor_ctx->config.auto_fix) {
        polycall_doctor_fix_issues(core_ctx, doctor_ctx, config_ctx, NULL);
    }
    
    return POLYCALL_CORE_SUCCESS;
}

// Get validation issues
polycall_core_error_t polycall_doctor_get_issues(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    polycall_doctor_issue_t* issues,
    uint32_t max_issues,
    uint32_t* issue_count
) {
    if (!core_ctx || !doctor_ctx || !issues || !issue_count) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Copy issues
    uint32_t count = (doctor_ctx->issue_count < max_issues) ? 
                     doctor_ctx->issue_count : max_issues;
    
    memcpy(issues, doctor_ctx->issues, count * sizeof(polycall_doctor_issue_t));
    *issue_count = count;
    
    return POLYCALL_CORE_SUCCESS;
}

// Fix validation issues automatically
polycall_core_error_t polycall_doctor_fix_issues(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    uint32_t* fixed_count
) {
    if (!core_ctx || !doctor_ctx || !config_ctx) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Reset fixed count
    doctor_ctx->fixed_count = 0;
    
    // Process auto-fixable issues
    for (uint32_t i = 0; i < doctor_ctx->issue_count; i++) {
        polycall_doctor_issue_t* issue = &doctor_ctx->issues[i];
        
        if (issue->auto_fixable) {
            // Implement auto-fix logic based on issue category and path
            switch (issue->category) {
                case POLYCALL_DOCTOR_CATEGORY_SCHEMA:
                    // Example: Fix schema violations
                    // This would typically require more complex logic specific to each schema issue
                    break;
                    
                case POLYCALL_DOCTOR_CATEGORY_SECURITY:
                    // Example: Fix security issues with conservative defaults
                    if (strstr(issue->path, "security_level") != NULL) {
                        // Set security level to a safe default
                        polycall_config_set_int(core_ctx, config_ctx, 
                                              POLYCALL_CONFIG_SECTION_SECURITY,
                                              "security_level", 2);  // Medium security level
                        doctor_ctx->fixed_count++;
                    }
                    break;
                    
                case POLYCALL_DOCTOR_CATEGORY_PERFORMANCE:
                    // Example: Fix performance issues
                    if (strstr(issue->path, "timeout_ms") != NULL) {
                        // Increase timeout to recommended value
                        polycall_config_set_int(core_ctx, config_ctx,
                                              POLYCALL_CONFIG_SECTION_CORE,
                                              "timeout_ms", 30000);  // 30 second timeout
                        doctor_ctx->fixed_count++;
                    }
                    break;
                    
                default:
                    // Other issue types might not be auto-fixable
                    break;
            }
        }
    }
    
    // Return count if requested
    if (fixed_count) {
        *fixed_count = doctor_ctx->fixed_count;
    }
    
    return POLYCALL_CORE_SUCCESS;
}

// Generate optimization suggestions
polycall_core_error_t polycall_doctor_optimize(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    polycall_doctor_issue_t* suggestions,
    uint32_t max_suggestions,
    uint32_t* suggestion_count
) {
    if (!core_ctx || !doctor_ctx || !config_ctx || !suggestions || !suggestion_count) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Clear issues since we'll be using the same mechanism for suggestions
    clear_issues(doctor_ctx);
    
    // Generate performance suggestions
    if (doctor_ctx->config.validate_performance) {
        // Example: Check for buffer sizes and pool sizes
        polycall_doctor_issue_t issue;
        memset(&issue, 0, sizeof(issue));
        
        // Check memory pool size
        int64_t pool_size = polycall_config_get_int(core_ctx, config_ctx,
                                                  POLYCALL_CONFIG_SECTION_MEMORY,
                                                  "memory_pool_size", 0);
        
        if (pool_size > 0 && pool_size < 1048576) {  // Less than 1MB
            strcpy(issue.path, "memory:memory_pool_size");
            issue.severity = POLYCALL_DOCTOR_SEVERITY_WARNING;
            issue.category = POLYCALL_DOCTOR_CATEGORY_PERFORMANCE;
            strcpy(issue.message, "Memory pool size is small, may cause frequent allocations");
            strcpy(issue.suggestion, "Increase memory pool size to at least 1MB for better performance");
            issue.auto_fixable = true;
            
            add_issue(doctor_ctx, &issue);
        }
        
        // Check connection pool
        int64_t conn_pool = polycall_config_get_int(core_ctx, config_ctx,
                                                  POLYCALL_CONFIG_SECTION_NETWORK,
                                                  "connection_pool_size", 0);
        
        if (conn_pool > 0 && conn_pool < 10) {
            strcpy(issue.path, "network:connection_pool_size");
            issue.severity = POLYCALL_DOCTOR_SEVERITY_INFO;
            issue.category = POLYCALL_DOCTOR_CATEGORY_PERFORMANCE;
            strcpy(issue.message, "Connection pool size is small for production use");
            strcpy(issue.suggestion, "Increase connection pool size to 10-20 for better performance under load");
            issue.auto_fixable = true;
            
            add_issue(doctor_ctx, &issue);
        }
    }
    
    // Copy suggestions
    uint32_t count = (doctor_ctx->issue_count < max_suggestions) ? 
                     doctor_ctx->issue_count : max_suggestions;
    
    memcpy(suggestions, doctor_ctx->issues, count * sizeof(polycall_doctor_issue_t));
    *suggestion_count = count;
    
    return POLYCALL_CORE_SUCCESS;
}

// Verify configuration portability
polycall_core_error_t polycall_doctor_verify_portability(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    uint32_t* portability_score
) {
    if (!core_ctx || !doctor_ctx || !config_ctx || !portability_score) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Base score starts at 100
    uint32_t score = 100;
    
    // For now, just a simple implementation that checks some common portability issues
    
    // Check if using platform-specific paths
    char path_separator[16];
    if (polycall_config_get_string(core_ctx, config_ctx,
                                 POLYCALL_CONFIG_SECTION_CORE,
                                 "path_separator", path_separator, sizeof(path_separator),
                                 NULL) == POLYCALL_CORE_SUCCESS) {
        // Check if using Windows-style backslashes which are less portable
        if (strcmp(path_separator, "\\") == 0) {
            score -= 10;  // Deduct for Windows-specific separators
        }
    }
    
    // Check for absolute paths
    int64_t use_absolute_paths = polycall_config_get_int(core_ctx, config_ctx,
                                                      POLYCALL_CONFIG_SECTION_CORE,
                                                      "use_absolute_paths", 0);
    if (use_absolute_paths != 0) {
        score -= 15;  // Absolute paths reduce portability
    }
    
    // Other portability checks could include:
    // - Platform-specific features usage
    // - Character encoding settings
    // - File system assumptions
    // - Network interface bindings
    // - etc.
    
    *portability_score = score;
    return POLYCALL_CORE_SUCCESS;
}

// Generate configuration report
polycall_core_error_t polycall_doctor_generate_report(
    polycall_core_context_t* core_ctx,
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    const char* report_path,
    const char* report_format
) {
    if (!core_ctx || !doctor_ctx || !config_ctx || !report_path) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Default to text format if not specified
    if (!report_format) {
        report_format = "text";
    }
    
    // Open report file
    FILE* file = fopen(report_path, "w");
    if (!file) {
        return POLYCALL_CORE_ERROR_IO;
    }
    
    // Generate report header
    time_t now;
    time(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    if (strcmp(report_format, "json") == 0) {
        // JSON format
        fprintf(file, "{\n");
        fprintf(file, "  \"report_type\": \"LibPolyCall Configuration Report\",\n");
        fprintf(file, "  \"timestamp\": \"%s\",\n", time_str);
        
        // Issues section
        fprintf(file, "  \"issues\": [\n");
        
        for (uint32_t i = 0; i < doctor_ctx->issue_count; i++) {
            polycall_doctor_issue_t* issue = &doctor_ctx->issues[i];
            
            fprintf(file, "    {\n");
            fprintf(file, "      \"severity\": \"%s\",\n", 
                   issue->severity == POLYCALL_DOCTOR_SEVERITY_INFO ? "info" :
                   issue->severity == POLYCALL_DOCTOR_SEVERITY_WARNING ? "warning" :
                   issue->severity == POLYCALL_DOCTOR_SEVERITY_ERROR ? "error" : "critical");
            fprintf(file, "      \"category\": \"%s\",\n",
                   issue->category == POLYCALL_DOCTOR_CATEGORY_SCHEMA ? "schema" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_SECURITY ? "security" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_PERFORMANCE ? "performance" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_CONSISTENCY ? "consistency" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_PORTABILITY ? "portability" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_DEPENDENCY ? "dependency" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_DEPRECATION ? "deprecation" : "custom");
            fprintf(file, "      \"path\": \"%s\",\n", issue->path);
            fprintf(file, "      \"message\": \"%s\",\n", issue->message);
            fprintf(file, "      \"suggestion\": \"%s\",\n", issue->suggestion);
            fprintf(file, "      \"auto_fixable\": %s\n", issue->auto_fixable ? "true" : "false");
            
            fprintf(file, "    }%s\n", i < doctor_ctx->issue_count - 1 ? "," : "");
        }
        
        fprintf(file, "  ],\n");
        
        // Add summary
        fprintf(file, "  \"summary\": {\n");
        fprintf(file, "    \"total_issues\": %u,\n", doctor_ctx->issue_count);
        fprintf(file, "    \"auto_fixable\": %u,\n", doctor_ctx->fixed_count);
        fprintf(file, "    \"last_validation\": \"%s\"\n", 
               time_str);
        fprintf(file, "  }\n");
        
        fprintf(file, "}\n");
    } else if (strcmp(report_format, "html") == 0) {
        // HTML format
        fprintf(file, "<!DOCTYPE html>\n");
        fprintf(file, "<html>\n<head>\n");
        fprintf(file, "<title>LibPolyCall Configuration Report</title>\n");
        fprintf(file, "<style>\n");
        fprintf(file, "body { font-family: Arial, sans-serif; margin: 20px; }\n");
        fprintf(file, "h1 { color: #333; }\n");
        fprintf(file, "table { border-collapse: collapse; width: 100%%; margin-top: 20px; }\n");
        fprintf(file, "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n");
        fprintf(file, "th { background-color: #f2f2f2; }\n");
        fprintf(file, "tr:nth-child(even) { background-color: #f9f9f9; }\n");
        fprintf(file, ".info { color: blue; }\n");
        fprintf(file, ".warning { color: orange; }\n");
        fprintf(file, ".error { color: red; }\n");
        fprintf(file, ".critical { color: darkred; font-weight: bold; }\n");
        fprintf(file, "</style>\n");
        fprintf(file, "</head>\n<body>\n");
        
        fprintf(file, "<h1>LibPolyCall Configuration Report</h1>\n");
        fprintf(file, "<p><strong>Generated:</strong> %s</p>\n", time_str);
        
        // Issues table
        fprintf(file, "<h2>Configuration Issues</h2>\n");
        fprintf(file, "<table>\n");
        fprintf(file, "<tr><th>Severity</th><th>Category</th><th>Path</th>");
        fprintf(file, "<th>Issue</th><th>Suggestion</th><th>Auto-Fixable</th></tr>\n");
        
        for (uint32_t i = 0; i < doctor_ctx->issue_count; i++) {
            polycall_doctor_issue_t* issue = &doctor_ctx->issues[i];
            
            const char* severity_class = 
                issue->severity == POLYCALL_DOCTOR_SEVERITY_INFO ? "info" :
                issue->severity == POLYCALL_DOCTOR_SEVERITY_WARNING ? "warning" :
                issue->severity == POLYCALL_DOCTOR_SEVERITY_ERROR ? "error" : "critical";
            
            fprintf(file, "<tr>\n");
            fprintf(file, "  <td class=\"%s\">%s</td>\n", severity_class,
                   issue->severity == POLYCALL_DOCTOR_SEVERITY_INFO ? "Info" :
                   issue->severity == POLYCALL_DOCTOR_SEVERITY_WARNING ? "Warning" :
                   issue->severity == POLYCALL_DOCTOR_SEVERITY_ERROR ? "Error" : "Critical");
            fprintf(file, "  <td>%s</td>\n",
                   issue->category == POLYCALL_DOCTOR_CATEGORY_SCHEMA ? "Schema" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_SECURITY ? "Security" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_PERFORMANCE ? "Performance" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_CONSISTENCY ? "Consistency" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_PORTABILITY ? "Portability" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_DEPENDENCY ? "Dependency" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_DEPRECATION ? "Deprecation" : "Custom");
            fprintf(file, "  <td>%s</td>\n", issue->path);
            fprintf(file, "  <td>%s</td>\n", issue->message);
            fprintf(file, "  <td>%s</td>\n", issue->suggestion);
            fprintf(file, "  <td>%s</td>\n", issue->auto_fixable ? "Yes" : "No");
            fprintf(file, "</tr>\n");
        }
        
        fprintf(file, "</table>\n");
        
        // Summary
        fprintf(file, "<h2>Summary</h2>\n");
        fprintf(file, "<p>Total issues: <strong>%u</strong></p>\n", doctor_ctx->issue_count);
        fprintf(file, "<p>Auto-fixable issues: <strong>%u</strong></p>\n", doctor_ctx->fixed_count);
        
        fprintf(file, "</body>\n</html>\n");
    } else {
        // Default to plain text
        fprintf(file, "LibPolyCall Configuration Report\n");
        fprintf(file, "===============================\n\n");
        fprintf(file, "Generated: %s\n\n", time_str);
        
        fprintf(file, "Configuration Issues:\n");
        fprintf(file, "---------------------\n\n");
        
        for (uint32_t i = 0; i < doctor_ctx->issue_count; i++) {
            polycall_doctor_issue_t* issue = &doctor_ctx->issues[i];
            
            fprintf(file, "[%s] %s\n",
                   issue->severity == POLYCALL_DOCTOR_SEVERITY_INFO ? "INFO" :
                   issue->severity == POLYCALL_DOCTOR_SEVERITY_WARNING ? "WARNING" :
                   issue->severity == POLYCALL_DOCTOR_SEVERITY_ERROR ? "ERROR" : "CRITICAL",
                   issue->path);
            fprintf(file, "Category: %s\n",
                   issue->category == POLYCALL_DOCTOR_CATEGORY_SCHEMA ? "Schema" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_SECURITY ? "Security" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_PERFORMANCE ? "Performance" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_CONSISTENCY ? "Consistency" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_PORTABILITY ? "Portability" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_DEPENDENCY ? "Dependency" :
                   issue->category == POLYCALL_DOCTOR_CATEGORY_DEPRECATION ? "Deprecation" : "Custom");
            fprintf(file, "Issue: %s\n", issue->message);
            fprintf(file, "Suggestion: %s\n", issue->suggestion);
            fprintf(file, "Auto-Fixable: %s\n\n", issue->auto_fixable ? "Yes" : "No");
        }
        
        fprintf(file, "Summary:\n");
        fprintf(file, "--------\n");
        fprintf(file, "Total issues: %u\n", doctor_ctx->issue_count);
        fprintf(file, "Auto-fixable issues: %u\n", doctor_ctx->fixed_count);
    }
    
    fclose(file);
    return POLYCALL_CORE_SUCCESS;
}

// Built-in validation rule implementations
static bool validate_schema_rule(
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    const char* path,
    void* user_data,
    polycall_doctor_issue_t* issue
) {
    // In a real implementation, this would validate against a schema definition
    // For this example, we'll check for some required settings
    
    // Check for required core settings
    if (!polycall_config_exists(doctor_ctx->core_ctx, config_ctx, 
                               POLYCALL_CONFIG_SECTION_CORE, "version")) {
        strcpy(issue->path, "core:version");
        issue->severity = POLYCALL_DOCTOR_SEVERITY_ERROR;
        strcpy(issue->message, "Required core setting 'version' is missing");
        strcpy(issue->suggestion, "Add 'version' setting to core section (e.g., '1.0.0')");
        issue->auto_fixable = true;
        return true;
    }
    
    return false;
}

static bool validate_security_rule(
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    const char* path,
    void* user_data,
    polycall_doctor_issue_t* issue
) {
    // Check security settings
    
    // Check if security level is set
    if (!polycall_config_exists(doctor_ctx->core_ctx, config_ctx,
                              POLYCALL_CONFIG_SECTION_SECURITY, "security_level")) {
        strcpy(issue->path, "security:security_level");
        issue->severity = POLYCALL_DOCTOR_SEVERITY_ERROR;
        strcpy(issue->message, "Security level is not set");
        strcpy(issue->suggestion, "Set security_level to at least 1 (basic security)");
        issue->auto_fixable = true;
        return true;
    }
    
    // Check if security level is adequate for production
    int64_t security_level = polycall_config_get_int(doctor_ctx->core_ctx, config_ctx,
                                                   POLYCALL_CONFIG_SECTION_SECURITY,
                                                   "security_level", 0);
    
    if (security_level < 2) {
        strcpy(issue->path, "security:security_level");
        issue->severity = POLYCALL_DOCTOR_SEVERITY_WARNING;
        strcpy(issue->message, "Security level is too low for production use");
        strcpy(issue->suggestion, "Increase security_level to at least 2 (medium security)");
        issue->auto_fixable = true;
        return true;
    }
    
    return false;
}

static bool validate_performance_rule(
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    const char* path,
    void* user_data,
    polycall_doctor_issue_t* issue
) {
    // Check for performance issues
    
    // Check timeout values
    int64_t timeout = polycall_config_get_int(doctor_ctx->core_ctx, config_ctx,
                                           POLYCALL_CONFIG_SECTION_NETWORK,
                                           "timeout_ms", 0);
    
    if (timeout > 0 && timeout < 5000) {
        strcpy(issue->path, "network:timeout_ms");
        issue->severity = POLYCALL_DOCTOR_SEVERITY_WARNING;
        strcpy(issue->message, "Network timeout is very low and may cause issues under load");
        strcpy(issue->suggestion, "Increase timeout_ms to at least 5000 (5 seconds)");
        issue->auto_fixable = true;
        return true;
    }
    
    return false;
}

static bool validate_consistency_rule(
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    const char* path,
    void* user_data,
    polycall_doctor_issue_t* issue
) {
    // Check for configuration consistency issues
    
    // Example: Check if security features are enabled but security level is low
    bool security_enabled = polycall_config_get_bool(doctor_ctx->core_ctx, config_ctx,
                                                  POLYCALL_CONFIG_SECTION_CORE,
                                                  "enable_security", false);
    
    int64_t security_level = polycall_config_get_int(doctor_ctx->core_ctx, config_ctx,
                                                  POLYCALL_CONFIG_SECTION_SECURITY,
                                                  "security_level", 0);
    
    if (security_enabled && security_level < 1) {
        strcpy(issue->path, "core:enable_security <-> security:security_level");
        issue->severity = POLYCALL_DOCTOR_SEVERITY_ERROR;
        strcpy(issue->message, "Security is enabled but security level is set to 0");
        strcpy(issue->suggestion, "Either disable security features or set security_level to at least 1");
        issue->auto_fixable = true;
        return true;
    }
    
    return false;
}

static bool validate_deprecated_rule(
    polycall_doctor_context_t* doctor_ctx,
    polycall_config_context_t* config_ctx,
    const char* path,
    void* user_data,
    polycall_doctor_issue_t* issue
) {
    // Check for deprecated configuration options
    
    // Example: Check for old timeout format
    if (polycall_config_exists(doctor_ctx->core_ctx, config_ctx,
                             POLYCALL_CONFIG_SECTION_NETWORK, "timeout")) {
        strcpy(issue->path, "network:timeout");
        issue->severity = POLYCALL_DOCTOR_SEVERITY_WARNING;
        strcpy(issue->message, "The 'timeout' setting is deprecated");
        strcpy(issue->suggestion, "Use 'timeout_ms' instead with millisecond values");
        issue->auto_fixable = true;
        return true;
    }
    
    return false;
}
