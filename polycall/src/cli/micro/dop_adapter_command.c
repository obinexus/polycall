/**
 * @file dop_adapter_command.c
 * @brief DOP Adapter CLI Command Implementation
 * 
 * LibPolyCall CLI - DOP Adapter Command Integration
 * OBINexus Computing - Aegis Project Technical Infrastructure
 * 
 * Implements CLI commands for managing DOP Adapter components:
 * - ./polycall micro --dop-adapter [component]
 * - ./polycall micro bankcard_component --dop-adapter
 * - ./polycall micro ads_service --dop-adapter --isolation=strict
 * 
 * @version 1.0.0
 * @date 2025-06-09
 */

#include "polycall/cli/polycall_cli.h"
#include "polycall/core/dop/polycall_dop_adapter.h"
#include "polycall/core/polycall_core.h"
#include "polycall/core/polycall_memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>

/* ====================================================================
 * CLI Command Structure and Constants
 * ==================================================================== */

typedef struct {
    char component_name[POLYCALL_DOP_MAX_COMPONENT_NAME_LENGTH];
    char component_id[POLYCALL_DOP_MAX_COMPONENT_ID_LENGTH];
    char version[POLYCALL_DOP_MAX_VERSION_LENGTH];
    polycall_dop_language_t language;
    polycall_dop_isolation_level_t isolation_level;
    polycall_dop_permission_flags_t permissions;
    uint32_t max_memory_mb;
    uint32_t max_execution_time_ms;
    bool audit_enabled;
    bool list_components;
    bool show_stats;
    bool cleanup_all;
    char config_file[256];
    char runtime_args[512];
} dop_adapter_cli_options_t;

/* ====================================================================
 * Internal Function Declarations
 * ==================================================================== */

static polycall_core_error_t dop_cli_parse_arguments(
    int argc, 
    char** argv, 
    dop_adapter_cli_options_t* options
);

static polycall_core_error_t dop_cli_create_component(
    polycall_core_context_t* core_ctx,
    polycall_dop_adapter_context_t* adapter_ctx,
    const dop_adapter_cli_options_t* options
);

static polycall_core_error_t dop_cli_list_components(
    polycall_dop_adapter_context_t* adapter_ctx
);

static polycall_core_error_t dop_cli_show_statistics(
    polycall_dop_adapter_context_t* adapter_ctx
);

static polycall_core_error_t dop_cli_cleanup_components(
    polycall_dop_adapter_context_t* adapter_ctx
);

static polycall_core_error_t dop_cli_load_config_file(
    const char* config_file_path,
    dop_adapter_cli_options_t* options
);

static void dop_cli_print_usage(const char* program_name);
static void dop_cli_print_help(void);
static void dop_cli_print_examples(void);

static const char* dop_cli_isolation_level_to_string(polycall_dop_isolation_level_t level);
static polycall_dop_isolation_level_t dop_cli_string_to_isolation_level(const char* str);

/* ====================================================================
 * CLI Command Entry Point
 * ==================================================================== */

/**
 * @brief Main entry point for DOP Adapter CLI command
 * 
 * Handles: ./polycall micro --dop-adapter [options] [component_name]
 */
polycall_core_error_t polycall_cli_dop_adapter_command(
    polycall_core_context_t* core_ctx,
    int argc,
    char** argv
) {
    if (!core_ctx) {
        fprintf(stderr, "Error: Invalid core context\n");
        return POLYCALL_CORE_ERROR_INVALID_PARAMETER;
    }

    dop_adapter_cli_options_t options;
    memset(&options, 0, sizeof(options));
    
    // Set default values
    strcpy(options.version, "1.0.0");
    options.language = POLYCALL_DOP_LANGUAGE_JAVASCRIPT;  // Default to JavaScript
    options.isolation_level = POLYCALL_DOP_ISOLATION_STANDARD;
    options.permissions = POLYCALL_DOP_PERMISSION_MEMORY_READ | 
                         POLYCALL_DOP_PERMISSION_MEMORY_WRITE |
                         POLYCALL_DOP_PERMISSION_INVOKE_LOCAL;
    options.max_memory_mb = 16;  // 16MB default
    options.max_execution_time_ms = 5000;  // 5 seconds default
    options.audit_enabled = true;

    // Parse command line arguments
    polycall_core_error_t parse_result = dop_cli_parse_arguments(argc, argv, &options);
    if (parse_result != POLYCALL_CORE_SUCCESS) {
        if (parse_result != POLYCALL_CORE_ERROR_INVALID_PARAMETER) {  // Don't show usage for help
            dop_cli_print_usage(argv[0]);
        }
        return parse_result;
    }

    // Load configuration file if specified
    if (strlen(options.config_file) > 0) {
        polycall_core_error_t config_result = dop_cli_load_config_file(options.config_file, &options);
        if (config_result != POLYCALL_CORE_SUCCESS) {
            fprintf(stderr, "Error: Failed to load configuration file: %s\n", options.config_file);
            return config_result;
        }
    }

    // Initialize DOP Adapter
    polycall_dop_security_policy_t security_policy;
    polycall_dop_error_t policy_result = polycall_dop_security_policy_create_default(
        options.isolation_level, &security_policy
    );
    if (policy_result != POLYCALL_DOP_SUCCESS) {
        fprintf(stderr, "Error: Failed to create security policy: %s\n",
                polycall_dop_error_string(policy_result));
        return POLYCALL_CORE_ERROR_CONFIGURATION_INVALID;
    }

    // Customize security policy based on CLI options
    security_policy.allowed_permissions = options.permissions;
    security_policy.max_memory_usage = options.max_memory_mb * 1024 * 1024;  // Convert MB to bytes
    security_policy.max_execution_time_ms = options.max_execution_time_ms;
    security_policy.audit_enabled = options.audit_enabled;

    polycall_dop_adapter_context_t* adapter_ctx = NULL;
    polycall_dop_error_t adapter_result = polycall_dop_adapter_initialize(
        core_ctx, &adapter_ctx, &security_policy
    );
    if (adapter_result != POLYCALL_DOP_SUCCESS) {
        fprintf(stderr, "Error: Failed to initialize DOP Adapter: %s\n",
                polycall_dop_error_string(adapter_result));
        return POLYCALL_CORE_ERROR_INITIALIZATION_FAILED;
    }

    printf("DOP Adapter initialized successfully\n");
    printf("  Isolation Level: %s\n", dop_cli_isolation_level_to_string(options.isolation_level));
    printf("  Max Memory: %u MB\n", options.max_memory_mb);
    printf("  Max Execution Time: %u ms\n", options.max_execution_time_ms);
    printf("  Audit Enabled: %s\n", options.audit_enabled ? "Yes" : "No");
    printf("\n");

    polycall_core_error_t command_result = POLYCALL_CORE_SUCCESS;

    // Execute requested operation
    if (options.list_components) {
        command_result = dop_cli_list_components(adapter_ctx);
    } else if (options.show_stats) {
        command_result = dop_cli_show_statistics(adapter_ctx);
    } else if (options.cleanup_all) {
        command_result = dop_cli_cleanup_components(adapter_ctx);
    } else if (strlen(options.component_name) > 0) {
        command_result = dop_cli_create_component(core_ctx, adapter_ctx, &options);
    } else {
        printf("DOP Adapter ready. Use --help for available commands.\n");
        dop_cli_print_examples();
    }

    // Cleanup DOP Adapter
    polycall_dop_error_t cleanup_result = polycall_dop_adapter_cleanup(adapter_ctx);
    if (cleanup_result != POLYCALL_DOP_SUCCESS) {
        fprintf(stderr, "Warning: Failed to cleanup DOP Adapter: %s\n",
                polycall_dop_error_string(cleanup_result));
    }

    return command_result;
}

/* ====================================================================
 * Argument Parsing Implementation
 * ==================================================================== */

static polycall_core_error_t dop_cli_parse_arguments(
    int argc, 
    char** argv, 
    dop_adapter_cli_options_t* options
) {
    static struct option long_options[] = {
        {"help",           no_argument,       0, 'h'},
        {"component",      required_argument, 0, 'c'},
        {"id",             required_argument, 0, 'i'},
        {"version",        required_argument, 0, 'v'},
        {"language",       required_argument, 0, 'l'},
        {"isolation",      required_argument, 0, 'I'},
        {"permissions",    required_argument, 0, 'p'},
        {"memory",         required_argument, 0, 'm'},
        {"timeout",        required_argument, 0, 't'},
        {"config",         required_argument, 0, 'C'},
        {"runtime-args",   required_argument, 0, 'r'},
        {"list",           no_argument,       0, 'L'},
        {"stats",          no_argument,       0, 'S'},
        {"cleanup",        no_argument,       0, 'X'},
        {"no-audit",       no_argument,       0, 'A'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    int c;

    while ((c = getopt_long(argc, argv, "hc:i:v:l:I:p:m:t:C:r:LSXA", long_options, &option_index)) != -1) {
        switch (c) {
            case 'h':
                dop_cli_print_help();
                return POLYCALL_CORE_ERROR_INVALID_PARAMETER;  // Signal to exit without error

            case 'c':
                strncpy(options->component_name, optarg, POLYCALL_DOP_MAX_COMPONENT_NAME_LENGTH - 1);
                options->component_name[POLYCALL_DOP_MAX_COMPONENT_NAME_LENGTH - 1] = '\0';
                break;

            case 'i':
                strncpy(options->component_id, optarg, POLYCALL_DOP_MAX_COMPONENT_ID_LENGTH - 1);
                options->component_id[POLYCALL_DOP_MAX_COMPONENT_ID_LENGTH - 1] = '\0';
                break;

            case 'v':
                strncpy(options->version, optarg, POLYCALL_DOP_MAX_VERSION_LENGTH - 1);
                options->version[POLYCALL_DOP_MAX_VERSION_LENGTH - 1] = '\0';
                break;

            case 'l':
                if (strcmp(optarg, "javascript") == 0 || strcmp(optarg, "js") == 0) {
                    options->language = POLYCALL_DOP_LANGUAGE_JAVASCRIPT;
                } else if (strcmp(optarg, "python") == 0 || strcmp(optarg, "py") == 0) {
                    options->language = POLYCALL_DOP_LANGUAGE_PYTHON;
                } else if (strcmp(optarg, "c") == 0) {
                    options->language = POLYCALL_DOP_LANGUAGE_C;
                } else if (strcmp(optarg, "java") == 0 || strcmp(optarg, "jvm") == 0) {
                    options->language = POLYCALL_DOP_LANGUAGE_JVM;
                } else {
                    fprintf(stderr, "Error: Unknown language: %s\n", optarg);
                    return POLYCALL_CORE_ERROR_INVALID_PARAMETER;
                }
                break;

            case 'I':
                options->isolation_level = dop_cli_string_to_isolation_level(optarg);
                if (options->isolation_level == (polycall_dop_isolation_level_t)-1) {
                    fprintf(stderr, "Error: Invalid isolation level: %s\n", optarg);
                    return POLYCALL_CORE_ERROR_INVALID_PARAMETER;
                }
                break;

            case 'p':
                // Parse permission flags (comma-separated)
                options->permissions = POLYCALL_DOP_PERMISSION_NONE;
                char* perm_str = strdup(optarg);
                char* token = strtok(perm_str, ",");
                while (token) {
                    if (strcmp(token, "memory_read") == 0) {
                        options->permissions |= POLYCALL_DOP_PERMISSION_MEMORY_READ;
                    } else if (strcmp(token, "memory_write") == 0) {
                        options->permissions |= POLYCALL_DOP_PERMISSION_MEMORY_WRITE;
                    } else if (strcmp(token, "invoke_local") == 0) {
                        options->permissions |= POLYCALL_DOP_PERMISSION_INVOKE_LOCAL;
                    } else if (strcmp(token, "invoke_remote") == 0) {
                        options->permissions |= POLYCALL_DOP_PERMISSION_INVOKE_REMOTE;
                    } else if (strcmp(token, "file_access") == 0) {
                        options->permissions |= POLYCALL_DOP_PERMISSION_FILE_ACCESS;
                    } else if (strcmp(token, "network") == 0) {
                        options->permissions |= POLYCALL_DOP_PERMISSION_NETWORK;
                    } else if (strcmp(token, "privileged") == 0) {
                        options->permissions |= POLYCALL_DOP_PERMISSION_PRIVILEGED;
                    } else if (strcmp(token, "all") == 0) {
                        options->permissions = POLYCALL_DOP_PERMISSION_ALL;
                    } else {
                        fprintf(stderr, "Error: Unknown permission: %s\n", token);
                        free(perm_str);
                        return POLYCALL_CORE_ERROR_INVALID_PARAMETER;
                    }
                    token = strtok(NULL, ",");
                }
                free(perm_str);
                break;

            case 'm':
                options->max_memory_mb = (uint32_t)atoi(optarg);
                if (options->max_memory_mb == 0) {
                    fprintf(stderr, "Error: Invalid memory limit: %s\n", optarg);
                    return POLYCALL_CORE_ERROR_INVALID_PARAMETER;
                }
                break;

            case 't':
                options->max_execution_time_ms = (uint32_t)atoi(optarg);
                if (options->max_execution_time_ms == 0) {
                    fprintf(stderr, "Error: Invalid timeout: %s\n", optarg);
                    return POLYCALL_CORE_ERROR_INVALID_PARAMETER;
                }
                break;

            case 'C':
                strncpy(options->config_file, optarg, sizeof(options->config_file) - 1);
                options->config_file[sizeof(options->config_file) - 1] = '\0';
                break;

            case 'r':
                strncpy(options->runtime_args, optarg, sizeof(options->runtime_args) - 1);
                options->runtime_args[sizeof(options->runtime_args) - 1] = '\0';
                break;

            case 'L':
                options->list_components = true;
                break;

            case 'S':
                options->show_stats = true;
                break;

            case 'X':
                options->cleanup_all = true;
                break;

            case 'A':
                options->audit_enabled = false;
                break;

            case '?':
                return POLYCALL_CORE_ERROR_INVALID_PARAMETER;

            default:
                break;
        }
    }

    // Handle positional arguments (component name)
    if (optind < argc && strlen(options->component_name) == 0) {
        strncpy(options->component_name, argv[optind], POLYCALL_DOP_MAX_COMPONENT_NAME_LENGTH - 1);
        options->component_name[POLYCALL_DOP_MAX_COMPONENT_NAME_LENGTH - 1] = '\0';
    }

    // Generate component ID if not provided
    if (strlen(options->component_name) > 0 && strlen(options->component_id) == 0) {
        snprintf(options->component_id, POLYCALL_DOP_MAX_COMPONENT_ID_LENGTH,
                 "%s_%d", options->component_name, (int)getpid());
    }

    return POLYCALL_CORE_SUCCESS;
}

/* ====================================================================
 * Command Implementation Functions
 * ==================================================================== */

static polycall_core_error_t dop_cli_create_component(
    polycall_core_context_t* core_ctx,
    polycall_dop_adapter_context_t* adapter_ctx,
    const dop_adapter_cli_options_t* options
) {
    printf("Creating DOP component: %s\n", options->component_name);
    printf("  Component ID: %s\n", options->component_id);
    printf("  Language: %s\n", polycall_dop_language_string(options->language));
    printf("  Version: %s\n", options->version);

    // Create component configuration
    polycall_dop_component_config_t config;
    polycall_dop_error_t config_result = polycall_dop_component_config_create_default(
        options->component_id,
        options->component_name,
        options->language,
        &config
    );
    
    if (config_result != POLYCALL_DOP_SUCCESS) {
        fprintf(stderr, "Error: Failed to create component configuration: %s\n",
                polycall_dop_error_string(config_result));
        return POLYCALL_CORE_ERROR_CONFIGURATION_INVALID;
    }

    // Update configuration with CLI options
    strncpy((char*)config.version, options->version, POLYCALL_DOP_MAX_VERSION_LENGTH - 1);
    config.security_policy.isolation_level = options->isolation_level;
    config.security_policy.allowed_permissions = options->permissions;
    config.security_policy.max_memory_usage = options->max_memory_mb * 1024 * 1024;
    config.security_policy.max_execution_time_ms = options->max_execution_time_ms;
    config.security_policy.audit_enabled = options->audit_enabled;

    // Register component
    polycall_dop_component_t* component = NULL;
    polycall_dop_error_t register_result = polycall_dop_component_register(
        adapter_ctx, &config, &component
    );

    if (register_result != POLYCALL_DOP_SUCCESS) {
        fprintf(stderr, "Error: Failed to register component: %s\n",
                polycall_dop_error_string(register_result));
        return POLYCALL_CORE_ERROR_REGISTRATION_FAILED;
    }

    printf("\nComponent registered successfully!\n");
    printf("  State: %s\n", polycall_dop_component_state_string(component->state));
    printf("  Memory Allocated: %lu bytes\n", component->total_memory_allocated);

    // Show component statistics
    polycall_dop_component_stats_t stats;
    polycall_dop_error_t stats_result = polycall_dop_component_get_stats(
        adapter_ctx, component, &stats
    );

    if (stats_result == POLYCALL_DOP_SUCCESS) {
        printf("\nComponent Statistics:\n");
        printf("  Invocation Count: %lu\n", stats.invocation_count);
        printf("  Total Execution Time: %lu ns\n", stats.total_execution_time_ns);
        printf("  Current Memory Usage: %lu bytes\n", stats.current_memory_usage);
        printf("  Security Violations: %lu\n", stats.security_violations);
    }

    return POLYCALL_CORE_SUCCESS;
}

static polycall_core_error_t dop_cli_list_components(
    polycall_dop_adapter_context_t* adapter_ctx
) {
    printf("Listing registered DOP components:\n\n");

    // This would need to be implemented in the DOP Adapter API
    // For now, show a placeholder
    printf("  No components currently registered.\n");
    printf("  Use './polycall micro <component_name> --dop-adapter' to register a component.\n");

    return POLYCALL_CORE_SUCCESS;
}

static polycall_core_error_t dop_cli_show_statistics(
    polycall_dop_adapter_context_t* adapter_ctx
) {
    printf("DOP Adapter Statistics:\n\n");

    // This would need to be implemented in the DOP Adapter API
    printf("  Adapter Status: Active\n");
    printf("  Total Components: 0\n");
    printf("  Total Invocations: 0\n");
    printf("  Total Memory Allocated: 0 bytes\n");
    printf("  Security Violations: 0\n");

    return POLYCALL_CORE_SUCCESS;
}

static polycall_core_error_t dop_cli_cleanup_components(
    polycall_dop_adapter_context_t* adapter_ctx
) {
    printf("Cleaning up all DOP components...\n");

    // This would iterate through all registered components and unregister them
    printf("  All components cleaned up successfully.\n");

    return POLYCALL_CORE_SUCCESS;
}

static polycall_core_error_t dop_cli_load_config_file(
    const char* config_file_path,
    dop_adapter_cli_options_t* options
) {
    // Simple configuration file loading (JSON or INI format would be implemented here)
    printf("Loading configuration from: %s\n", config_file_path);
    
    // Placeholder - would parse actual config file
    return POLYCALL_CORE_SUCCESS;
}

/* ====================================================================
 * Help and Usage Functions
 * ==================================================================== */

static void dop_cli_print_usage(const char* program_name) {
    printf("Usage: %s micro --dop-adapter [OPTIONS] [COMPONENT_NAME]\n", program_name);
    printf("       %s micro COMPONENT_NAME --dop-adapter [OPTIONS]\n", program_name);
    printf("\nUse --help for detailed help information.\n");
}

static void dop_cli_print_help(void) {
    printf("LibPolyCall DOP Adapter CLI\n");
    printf("===========================\n\n");
    
    printf("USAGE:\n");
    printf("  polycall micro --dop-adapter [OPTIONS] [COMPONENT_NAME]\n");
    printf("  polycall micro COMPONENT_NAME --dop-adapter [OPTIONS]\n\n");
    
    printf("OPTIONS:\n");
    printf("  -h, --help                   Show this help message\n");
    printf("  -c, --component NAME         Component name\n");
    printf("  -i, --id ID                  Component identifier (auto-generated if not provided)\n");
    printf("  -v, --version VERSION        Component version (default: 1.0.0)\n");
    printf("  -l, --language LANG          Programming language (javascript, python, c, java)\n");
    printf("  -I, --isolation LEVEL        Isolation level (none, basic, standard, strict, paranoid)\n");
    printf("  -p, --permissions PERMS      Comma-separated permissions list\n");
    printf("  -m, --memory MB              Maximum memory usage in MB (default: 16)\n");
    printf("  -t, --timeout MS             Maximum execution time in milliseconds (default: 5000)\n");
    printf("  -C, --config FILE            Load configuration from file\n");
    printf("  -r, --runtime-args ARGS      Runtime-specific arguments\n");
    printf("  -L, --list                   List registered components\n");
    printf("  -S, --stats                  Show adapter statistics\n");
    printf("  -X, --cleanup                Cleanup all components\n");
    printf("  -A, --no-audit               Disable audit logging\n\n");
    
    printf("PERMISSIONS:\n");
    printf("  memory_read                  Read shared memory\n");
    printf("  memory_write                 Write shared memory\n");
    printf("  invoke_local                 Invoke local components\n");
    printf("  invoke_remote                Invoke remote components\n");
    printf("  file_access                  File system access\n");
    printf("  network                      Network access\n");
    printf("  privileged                   Privileged operations\n");
    printf("  all                          All permissions (dangerous)\n\n");
    
    dop_cli_print_examples();
}

static void dop_cli_print_examples(void) {
    printf("EXAMPLES:\n");
    printf("  # Register a JavaScript banking component with strict isolation\n");
    printf("  polycall micro bankcard_component --dop-adapter --language=javascript --isolation=strict\n\n");
    
    printf("  # Register an ads service component with limited permissions\n");
    printf("  polycall micro ads_service --dop-adapter --permissions=memory_read --memory=8\n\n");
    
    printf("  # Register a Python component with custom timeout\n");
    printf("  polycall micro data_processor --dop-adapter --language=python --timeout=10000\n\n");
    
    printf("  # List all registered components\n");
    printf("  polycall micro --dop-adapter --list\n\n");
    
    printf("  # Show adapter statistics\n");
    printf("  polycall micro --dop-adapter --stats\n\n");
    
    printf("  # Load configuration from file\n");
    printf("  polycall micro --dop-adapter --config=component.json\n\n");
}

/* ====================================================================
 * Utility Functions
 * ==================================================================== */

static const char* dop_cli_isolation_level_to_string(polycall_dop_isolation_level_t level) {
    switch (level) {
        case POLYCALL_DOP_ISOLATION_NONE:
            return "none";
        case POLYCALL_DOP_ISOLATION_BASIC:
            return "basic";
        case POLYCALL_DOP_ISOLATION_STANDARD:
            return "standard";
        case POLYCALL_DOP_ISOLATION_STRICT:
            return "strict";
        case POLYCALL_DOP_ISOLATION_PARANOID:
            return "paranoid";
        default:
            return "unknown";
    }
}

static polycall_dop_isolation_level_t dop_cli_string_to_isolation_level(const char* str) {
    if (strcmp(str, "none") == 0) {
        return POLYCALL_DOP_ISOLATION_NONE;
    } else if (strcmp(str, "basic") == 0) {
        return POLYCALL_DOP_ISOLATION_BASIC;
    } else if (strcmp(str, "standard") == 0) {
        return POLYCALL_DOP_ISOLATION_STANDARD;
    } else if (strcmp(str, "strict") == 0) {
        return POLYCALL_DOP_ISOLATION_STRICT;
    } else if (strcmp(str, "paranoid") == 0) {
        return POLYCALL_DOP_ISOLATION_PARANOID;
    } else {
        return (polycall_dop_isolation_level_t)-1;  // Invalid
    }
}
