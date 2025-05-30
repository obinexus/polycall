/**
#include "polycall/core/edge/edge_config.h"

 * @file edge_config.c
 * @brief Implementation of Configuration Management for LibPolyCall Edge Computing System
 * @author Based on Nnamdi Okpala's design (OBINexusComputing)
 *
 * Provides implementation for configuration parsing, validation, storage, and application
 * for the LibPolyCall edge computing system.
 */

 #include "polycall/core/edge/edge_config.h"

 
 /**
  * @brief Default configuration schema string
  */
 static const char* DEFAULT_EDGE_CONFIG_SCHEMA = 
     "edge {\n"
     "  component: {\n"
     "    type: string, required: true, allowed: [\"compute\", \"storage\", \"gateway\", \"sensor\", \"actuator\", \"coordinator\", \"custom\"]\n"
     "    task_policy: string, required: true, allowed: [\"queue\", \"immediate\", \"priority\", \"deadline\", \"fair_share\"]\n"
     "    isolation: string, required: true, allowed: [\"none\", \"memory\", \"resources\", \"security\", \"strict\"]\n"
     "    max_memory_mb: integer, required: true, min: 1, max: 1048576\n"
     "    max_tasks: integer, required: true, min: 1, max: 1000000\n"
     "    max_nodes: integer, required: true, min: 1, max: 10000\n"
     "    task_timeout_ms: integer, required: true, min: 1, max: 3600000\n"
     "    enable_auto_discovery: boolean, required: true\n"
     "    discovery_port: integer, required: true, min: 1024, max: 65535\n"
     "    command_port: integer, required: true, min: 1024, max: 65535\n"
     "    data_port: integer, required: true, min: 1024, max: 65535\n"
     "    enable_telemetry: boolean, required: true\n"
     "    enable_load_balancing: boolean, required: true\n"
     "    enable_dynamic_scaling: boolean, required: true\n"
     "    log_path: string\n"
     "  }\n"
     "  router {\n"
     "    selection_strategy: string, required: true, allowed: [\"performance\", \"efficiency\", \"availability\", \"proximity\", \"security\"]\n"
     "    max_routing_attempts: integer, required: true, min: 1, max: 100\n"
     "    task_timeout_ms: integer, required: true, min: 1, max: 3600000\n"
     "    enable_fallback: boolean, required: true\n"
     "    enable_load_balancing: boolean, required: true\n"
     "    performance_threshold: float, required: true, min: 0.0, max: 1.0\n"
     "  }\n"
     "  fallback {\n"
     "    max_fallback_nodes: integer, required: true, min: 0, max: 1000\n"
     "    retry_delay_ms: integer, required: true, min: 0, max: 60000\n"
     "    enable_partial_execution: boolean, required: true\n"
     "    log_fallback_events: boolean, required: true\n"
     "  }\n"
     "  security {\n"
     "    enforce_node_authentication: boolean, required: true\n"
     "    enable_end_to_end_encryption: boolean, required: true\n"
     "    validate_node_integrity: boolean, required: true\n"
     "    security_token_lifetime_ms: integer, required: true, min: 1000, max: 86400000\n"
     "  }\n"
     "  runtime {\n"
     "    max_concurrent_tasks: integer, required: true, min: 1, max: 1000\n"
     "    task_queue_size: integer, required: true, min: 1, max: 10000\n"
     "    enable_priority_scheduling: boolean, required: true\n"
     "    enable_task_preemption: boolean, required: true\n"
     "    task_time_slice_ms: integer, required: true, min: 1, max: 10000\n"
     "    cpu_utilization_target: float, required: true, min: 0.0, max: 1.0\n"
     "    memory_utilization_target: float, required: true, min: 0.0, max: 1.0\n"
     "  }\n"
     "  components {\n"
     "    *: {\n"
     "      extends: \"edge.component\"\n"
     "    }\n"
     "  }\n"
     "}\n";
 
 /**
  * @brief Default edge configuration string
  */
 static const char* DEFAULT_EDGE_CONFIG = 
     "edge {\n"
     "  component {\n"
     "    type = \"compute\"\n"
     "    task_policy = \"queue\"\n"
     "    isolation = \"memory\"\n"
     "    max_memory_mb = 512\n"
     "    max_tasks = 100\n"
     "    max_nodes = 16\n"
     "    task_timeout_ms = 5000\n"
     "    discovery_port = 7700\n"
     "    command_port = 7701\n"
     "    data_port = 7702\n"
     "    enable_auto_discovery = true\n"
     "    enable_telemetry = true\n"
     "    enable_load_balancing = true\n"
     "    enable_dynamic_scaling = false\n"
     "  }\n"
     "  router {\n"
     "    selection_strategy = \"performance\"\n"
     "    max_routing_attempts = 3\n"
     "    task_timeout_ms = 5000\n"
     "    enable_fallback = true\n"
     "    enable_load_balancing = true\n"
     "    performance_threshold = 0.7\n"
     "  }\n"
     "  fallback {\n"
     "    max_fallback_nodes = 2\n"
     "    retry_delay_ms = 100\n"
     "    enable_partial_execution = true\n"
     "    log_fallback_events = true\n"
     "  }\n"
     "  security {\n"
     "    enforce_node_authentication = true\n"
     "    enable_end_to_end_encryption = true\n"
     "    validate_node_integrity = true\n"
     "    security_token_lifetime_ms = 3600000\n"
     "  }\n"
     "  runtime {\n"
     "    max_concurrent_tasks = 4\n"
     "    task_queue_size = 64\n"
     "    enable_priority_scheduling = true\n"
     "    enable_task_preemption = false\n"
     "    task_time_slice_ms = 100\n"
     "    cpu_utilization_target = 0.8\n"
     "    memory_utilization_target = 0.7\n"
     "  }\n"
     "}\n";
 
 /**
  * @brief Add value to config value cache
  */
 static polycall_core_error_t add_to_value_cache(
     polycall_edge_config_manager_t* config_manager,
     const char* path,
     polycall_config_value_t value,
     polycall_edge_config_source_t source
 ) {
     // Check cache capacity
     if (config_manager->value_cache_count >= config_manager->value_cache_capacity) {
         // Expand cache
         size_t new_capacity = config_manager->value_cache_capacity * 2;
         if (new_capacity == 0) {
             new_capacity = 16; // Initial capacity
         }
         
         config_value_entry_t* new_cache = polycall_core_malloc(
             config_manager->core_ctx,
             sizeof(config_value_entry_t) * new_capacity
         );
         
         if (!new_cache) {
             return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
         }
         
         // Copy existing entries
         if (config_manager->value_cache) {
             memcpy(
                 new_cache,
                 config_manager->value_cache,
                 sizeof(config_value_entry_t) * config_manager->value_cache_count
             );
             
             // Free old cache
             polycall_core_free(config_manager->core_ctx, config_manager->value_cache);
         }
         
         // Update cache
         config_manager->value_cache = new_cache;
         config_manager->value_cache_capacity = new_capacity;
     }
     
     // Add to cache
     config_manager->value_cache[config_manager->value_cache_count].value = value;
     config_manager->value_cache[config_manager->value_cache_count].source = source;
     config_manager->value_cache[config_manager->value_cache_count].timestamp = (uint64_t)time(NULL);
     config_manager->value_cache[config_manager->value_cache_count].is_modified = true;
     
     config_manager->value_cache_count++;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Find configuration node by path
  */
 static polycall_config_node_t* find_config_node(
     polycall_config_node_t* root,
     const char* path
 ) {
     if (!root || !path) {
         return NULL;
     }
     
     return polycall_config_find_node(root, path);
 }
 
 /**
  * @brief Load default configuration
  */
 static polycall_core_error_t load_default_config(
     polycall_edge_config_manager_t* config_manager
 ) {
     // Parse default configuration
     polycall_core_error_t result = polycall_config_parser_parse_string(
         config_manager->parser,
         DEFAULT_EDGE_CONFIG,
         &config_manager->defaults_root
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // Parse default schema
     return polycall_config_parser_parse_schema_string(
         config_manager->parser,
         DEFAULT_EDGE_CONFIG_SCHEMA,
         &config_manager->schema_root
     );
 }
 
 /**
  * @brief Validate configuration against schema
  */
 static polycall_core_error_t validate_configuration(
     polycall_edge_config_manager_t* config_manager,
     polycall_config_node_t* config_node,
     polycall_config_node_t* schema_node,
     polycall_edge_config_load_status_t* load_status
 ) {
     // Initialize validation context
     polycall_config_validation_context_t validation_ctx;
     validation_ctx.validation_level = config_manager->options.validation_level;
     validation_ctx.error_message[0] = '\0';
     
     // Validate configuration
     polycall_core_error_t result = polycall_config_validate(
         config_manager->parser,
         config_node,
         schema_node,
         &validation_ctx
     );
     
     // Update load status
     if (result != POLYCALL_CORE_SUCCESS) {
         load_status->success = false;
         load_status->invalid_entries += validation_ctx.invalid_entries;
         load_status->security_violations += validation_ctx.security_violations;
         
         if (validation_ctx.failed_section) {
             load_status->failed_section = validation_ctx.failed_section;
         }
         
         if (validation_ctx.error_message[0] != '\0') {
             load_status->error_message = validation_ctx.error_message;
         }
     }
     
     // Return validation result
     return result;
 }
 
 /**
  * @brief Load configuration from file
  */
 static polycall_core_error_t load_config_from_file(
     polycall_edge_config_manager_t* config_manager,
     const char* file_path,
     polycall_config_node_t** config_root,
     polycall_edge_config_load_status_t* load_status
 ) {
     if (!file_path || !config_root) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Access utility context
     polycall_core_context_t* core_ctx = config_manager->core_ctx;
     
     // Check if file exists and is readable
     FILE* file = fopen(file_path, "r");
     if (!file) {
         // File does not exist or is not readable
         if (config_manager->options.allow_missing_global) {
             // Use default configuration
             *config_root = NULL;
             return POLYCALL_CORE_SUCCESS;
         } else {
             // Missing config file is an error
             if (load_status) {
                 load_status->success = false;
                 load_status->error_message = "Configuration file not found or not readable";
             }
             return POLYCALL_CORE_ERROR_FILE_NOT_FOUND;
         }
     }
     
     // Close file (just checking existence)
     fclose(file);
     
     // Parse configuration file
     polycall_core_error_t result = polycall_config_parser_parse_file(
         config_manager->parser,
         file_path,
         config_root
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         if (load_status) {
             load_status->success = false;
             load_status->error_message = "Failed to parse configuration file";
         }
         return result;
     }
     
     // Validate configuration against schema
     if (config_manager->schema_root && config_manager->options.validation_level > EDGE_CONFIG_VALIDATE_NONE) {
         result = validate_configuration(
             config_manager,
             *config_root,
             config_manager->schema_root,
             load_status
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             // Clean up
             polycall_config_node_destroy(config_manager->parser, *config_root);
             *config_root = NULL;
             return result;
         }
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Merge configuration nodes
  */
 static polycall_core_error_t merge_config_nodes(
     polycall_edge_config_manager_t* config_manager,
     polycall_config_node_t* target,
     polycall_config_node_t* source,
     polycall_edge_config_load_status_t* load_status
 ) {
     if (!target || !source) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     polycall_config_merge_options_t merge_options;
     merge_options.overwrite_existing = true;
     merge_options.merge_arrays = true;
     merge_options.track_overrides = true;
     
     // Merge configurations
     polycall_core_error_t result = polycall_config_merge(
         config_manager->parser,
         target,
         source,
         &merge_options
     );
     
     // Update load status
     if (load_status) {
         load_status->overridden_entries += merge_options.overridden_entries;
     }
     
     return result;
 }
 
 /**
  * @brief Set string value in edge component config
  */
 static void set_string_config(
     polycall_core_context_t* core_ctx,
     char** target,
     const char* value
 ) {
     if (!target) {
         return;
     }
     
     // Free existing value if any
     if (*target) {
         polycall_core_free(core_ctx, *target);
         *target = NULL;
     }
     
     // Set new value if provided
     if (value) {
         *target = polycall_core_malloc(core_ctx, strlen(value) + 1);
         if (*target) {
             strcpy(*target, value);
         }
     }
 }
 
 /**
  * @brief Apply configuration to component structure
  */
 static polycall_core_error_t apply_component_config(
     polycall_edge_config_manager_t* config_manager,
     polycall_config_node_t* config_node,
     polycall_edge_component_config_t* component_config
 ) {
     if (!config_manager || !config_node || !component_config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     polycall_core_context_t* core_ctx = config_manager->core_ctx;
     polycall_config_parser_t* parser = config_manager->parser;
     
     // Start with default configuration
     *component_config = polycall_edge_component_default_config();
     
  
     // Helper function to look up component type from string
     auto polycall_edge_component_type_t lookup_component_type(const char* type_str) {
         if (!type_str) return EDGE_COMPONENT_TYPE_COMPUTE;
         
         if (strcmp(type_str, "compute") == 0) return EDGE_COMPONENT_TYPE_COMPUTE;
         if (strcmp(type_str, "storage") == 0) return EDGE_COMPONENT_TYPE_STORAGE;
         if (strcmp(type_str, "gateway") == 0) return EDGE_COMPONENT_TYPE_GATEWAY;
         if (strcmp(type_str, "sensor") == 0) return EDGE_COMPONENT_TYPE_SENSOR;
         if (strcmp(type_str, "actuator") == 0) return EDGE_COMPONENT_TYPE_ACTUATOR;
         if (strcmp(type_str, "coordinator") == 0) return EDGE_COMPONENT_TYPE_COORDINATOR;
         if (strcmp(type_str, "custom") == 0) return EDGE_COMPONENT_TYPE_CUSTOM;
         
         return EDGE_COMPONENT_TYPE_COMPUTE;
     }
     
     // Helper function to look up task policy from string
     auto polycall_edge_task_policy_t lookup_task_policy(const char* policy_str) {
         if (!policy_str) return EDGE_TASK_POLICY_QUEUE;
         
         if (strcmp(policy_str, "queue") == 0) return EDGE_TASK_POLICY_QUEUE;
         if (strcmp(policy_str, "immediate") == 0) return EDGE_TASK_POLICY_IMMEDIATE;
         if (strcmp(policy_str, "priority") == 0) return EDGE_TASK_POLICY_PRIORITY;
         if (strcmp(policy_str, "deadline") == 0) return EDGE_TASK_POLICY_DEADLINE;
         if (strcmp(policy_str, "fair_share") == 0) return EDGE_TASK_POLICY_FAIR_SHARE;
         
         return EDGE_TASK_POLICY_QUEUE;
     }
     
     // Helper function to look up isolation level from string
     auto polycall_isolation_level_t lookup_isolation_level(const char* level_str) {
         if (!level_str) return POLYCALL_ISOLATION_MEMORY;
         
         if (strcmp(level_str, "none") == 0) return POLYCALL_ISOLATION_NONE;
         if (strcmp(level_str, "memory") == 0) return POLYCALL_ISOLATION_MEMORY;
         if (strcmp(level_str, "resources") == 0) return POLYCALL_ISOLATION_RESOURCES;
         if (strcmp(level_str, "security") == 0) return POLYCALL_ISOLATION_SECURITY;
         if (strcmp(level_str, "strict") == 0) return POLYCALL_ISOLATION_STRICT;
         
         return POLYCALL_ISOLATION_MEMORY;
     }
     
     // Helper function to look up selection strategy from string
     auto polycall_node_selection_strategy_t lookup_selection_strategy(const char* strategy_str) {
         if (!strategy_str) return POLYCALL_NODE_SELECT_PERFORMANCE;
         
         if (strcmp(strategy_str, "performance") == 0) return POLYCALL_NODE_SELECT_PERFORMANCE;
         if (strcmp(strategy_str, "efficiency") == 0) return POLYCALL_NODE_SELECT_EFFICIENCY;
         if (strcmp(strategy_str, "availability") == 0) return POLYCALL_NODE_SELECT_AVAILABILITY;
         if (strcmp(strategy_str, "proximity") == 0) return POLYCALL_NODE_SELECT_PROXIMITY;
         if (strcmp(strategy_str, "security") == 0) return POLYCALL_NODE_SELECT_SECURITY;
         
         return POLYCALL_NODE_SELECT_PERFORMANCE;
     }
     
     // Apply basic component settings
     GET_STRING("name", component_config->component_name);
     GET_STRING("id", component_config->component_id);
     GET_ENUM("type", component_config->type, lookup_component_type);
     GET_ENUM("task_policy", component_config->task_policy, lookup_task_policy);
     GET_ENUM("isolation", component_config->isolation, lookup_isolation_level);
     
     // Apply resource limits
     GET_INT("max_memory_mb", component_config->max_memory_mb);
     GET_INT("max_tasks", component_config->max_tasks);
     GET_INT("max_nodes", component_config->max_nodes);
     GET_INT("task_timeout_ms", component_config->task_timeout_ms);
     
     // Apply networking settings
     GET_INT("discovery_port", component_config->discovery_port);
     GET_INT("command_port", component_config->command_port);
     GET_INT("data_port", component_config->data_port);
     GET_BOOL("enable_auto_discovery", component_config->enable_auto_discovery);
     
     // Apply advanced settings
     GET_BOOL("enable_telemetry", component_config->enable_telemetry);
     GET_BOOL("enable_load_balancing", component_config->enable_load_balancing);
     GET_BOOL("enable_dynamic_scaling", component_config->enable_dynamic_scaling);
     GET_STRING("log_path", component_config->log_path);
     
     // Apply router configuration
     polycall_config_node_t* router_node = find_config_node(config_node, "router");
     if (router_node) {
         GET_ENUM("router.selection_strategy", component_config->router_config.selection_strategy, lookup_selection_strategy);
         GET_INT("router.max_routing_attempts", component_config->router_config.max_routing_attempts);
         GET_INT("router.task_timeout_ms", component_config->router_config.task_timeout_ms);
         GET_BOOL("router.enable_fallback", component_config->router_config.enable_fallback);
         GET_BOOL("router.enable_load_balancing", component_config->router_config.enable_load_balancing);
         GET_FLOAT("router.performance_threshold", component_config->router_config.performance_threshold);
     }
     
     // Apply fallback configuration
     polycall_config_node_t* fallback_node = find_config_node(config_node, "fallback");
     if (fallback_node) {
         GET_INT("fallback.max_fallback_nodes", component_config->fallback_config.max_fallback_nodes);
         GET_INT("fallback.retry_delay_ms", component_config->fallback_config.retry_delay_ms);
         GET_BOOL("fallback.enable_partial_execution", component_config->fallback_config.enable_partial_execution);
         GET_BOOL("fallback.log_fallback_events", component_config->fallback_config.log_fallback_events);
     }
     
     // Apply security configuration
     polycall_config_node_t* security_node = find_config_node(config_node, "security");
     if (security_node) {
         GET_BOOL("security.enforce_node_authentication", component_config->security_config.enforce_node_authentication);
         GET_BOOL("security.enable_end_to_end_encryption", component_config->security_config.enable_end_to_end_encryption);
         GET_BOOL("security.validate_node_integrity", component_config->security_config.validate_node_integrity);
         GET_INT("security.security_token_lifetime_ms", component_config->security_config.security_token_lifetime_ms);
     }
     
     // Apply runtime configuration
     polycall_config_node_t* runtime_node = find_config_node(config_node, "runtime");
     if (runtime_node) {
         GET_INT("runtime.max_concurrent_tasks", component_config->runtime_config.max_concurrent_tasks);
         GET_INT("runtime.task_queue_size", component_config->runtime_config.task_queue_size);
         GET_BOOL("runtime.enable_priority_scheduling", component_config->runtime_config.enable_priority_scheduling);
         GET_BOOL("runtime.enable_task_preemption", component_config->runtime_config.enable_task_preemption);
         GET_INT("runtime.task_time_slice_ms", component_config->runtime_config.task_time_slice_ms);
         GET_FLOAT("runtime.cpu_utilization_target", component_config->runtime_config.cpu_utilization_target);
         GET_FLOAT("runtime.memory_utilization_target", component_config->runtime_config.memory_utilization_target);
     }
     
     // Clean up macros
     #undef GET_STRING
     #undef GET_INT
     #undef GET_FLOAT
     #undef GET_BOOL
     #undef GET_ENUM
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Initialize edge configuration manager
  */
 polycall_core_error_t polycall_edge_config_manager_init(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t** config_manager,
     const polycall_edge_config_manager_options_t* options
 ) {
     if (!core_ctx || !config_manager) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Allocate configuration manager
     polycall_edge_config_manager_t* new_manager = 
         polycall_core_malloc(core_ctx, sizeof(polycall_edge_config_manager_t));
     if (!new_manager) {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
     
     // Initialize manager
     memset(new_manager, 0, sizeof(polycall_edge_config_manager_t));
     new_manager->core_ctx = core_ctx;
     
     // Set options
     if (options) {
         memcpy(&new_manager->options, options, sizeof(polycall_edge_config_manager_options_t));
     } else {
         new_manager->options = polycall_edge_config_manager_default_options();
     }
     
     // Initialize configuration parser
     polycall_config_parser_options_t parser_options;
     memset(&parser_options, 0, sizeof(parser_options));
     parser_options.case_sensitive = false;
     parser_options.allow_env_vars = new_manager->options.apply_environment_vars;
     parser_options.trace_changes = new_manager->options.trace_config_changes;
     
     polycall_core_error_t result = polycall_config_parser_init(
         core_ctx,
         &new_manager->parser,
         &parser_options
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_core_free(core_ctx, new_manager);
         return result;
     }
     
     // Load default configuration
     result = load_default_config(new_manager);
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_config_parser_cleanup(core_ctx, new_manager->parser);
         polycall_core_free(core_ctx, new_manager);
         return result;
     }
     
     // Set initial configuration root
     if (new_manager->options.merge_with_defaults) {
         // Clone default configuration
         result = polycall_config_node_clone(
             new_manager->parser,
             new_manager->defaults_root,
             &new_manager->root
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             polycall_config_node_destroy(new_manager->parser, new_manager->defaults_root);
             polycall_config_node_destroy(new_manager->parser, new_manager->schema_root);
             polycall_config_parser_cleanup(core_ctx, new_manager->parser);
             polycall_core_free(core_ctx, new_manager);
             return result;
         }
     } else {
         // Create empty configuration
         result = polycall_config_node_create(
             new_manager->parser,
             &new_manager->root,
             "root",
             POLYCALL_CONFIG_NODE_SECTION
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             polycall_config_node_destroy(new_manager->parser, new_manager->defaults_root);
             polycall_config_node_destroy(new_manager->parser, new_manager->schema_root);
             polycall_config_parser_cleanup(core_ctx, new_manager->parser);
             polycall_core_free(core_ctx, new_manager);
             return result;
         }
     }
     
     // Initialize components node
     result = polycall_config_node_create(
         new_manager->parser,
         &new_manager->components,
         "components",
         POLYCALL_CONFIG_NODE_SECTION
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_config_node_destroy(new_manager->parser, new_manager->root);
         polycall_config_node_destroy(new_manager->parser, new_manager->defaults_root);
         polycall_config_node_destroy(new_manager->parser, new_manager->schema_root);
         polycall_config_parser_cleanup(core_ctx, new_manager->parser);
         polycall_core_free(core_ctx, new_manager);
         return result;
     }
     
     // Add components node to root
     result = polycall_config_node_add_child(
         new_manager->parser,
         new_manager->root,
         new_manager->components
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_config_node_destroy(new_manager->parser, new_manager->components);
         polycall_config_node_destroy(new_manager->parser, new_manager->root);
         polycall_config_node_destroy(new_manager->parser, new_manager->defaults_root);
         polycall_config_node_destroy(new_manager->parser, new_manager->schema_root);
         polycall_config_parser_cleanup(core_ctx, new_manager->parser);
         polycall_core_free(core_ctx, new_manager);
         return result;
     }
     
     // Initialize load status
     new_manager->last_load_status.success = true;
     new_manager->last_load_status.total_entries = 0;
     new_manager->last_load_status.invalid_entries = 0;
     new_manager->last_load_status.overridden_entries = 0;
     new_manager->last_load_status.security_violations = 0;
     new_manager->last_load_status.failed_section = NULL;
     new_manager->last_load_status.error_message = NULL;
     
     *config_manager = new_manager;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Load configurations from specified sources
  */
 polycall_core_error_t polycall_edge_config_manager_load(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     polycall_edge_config_load_status_t* load_status,
     char* error_buffer,
     size_t error_buffer_size
 ) {
     if (!core_ctx || !config_manager) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Initialize load status
     polycall_edge_config_load_status_t local_status;
     memset(&local_status, 0, sizeof(local_status));
     local_status.success = true;
     
     if (!load_status) {
         load_status = &local_status;
     } else {
         memset(load_status, 0, sizeof(polycall_edge_config_load_status_t));
         load_status->success = true;
     }
     
     // Track if configuration has changed
     config_manager->has_changes = false;
     
     // Load global configuration if specified
     if (config_manager->options.global_config_path) {
         polycall_core_error_t result = load_config_from_file(
             config_manager,
             config_manager->options.global_config_path,
             &config_manager->global_root,
             load_status
         );
         
         if (result != POLYCALL_CORE_SUCCESS && !config_manager->options.allow_missing_global) {
             // Copy error message if provided
             if (error_buffer && error_buffer_size > 0 && load_status->error_message) {
                 strncpy(error_buffer, load_status->error_message, error_buffer_size - 1);
                 error_buffer[error_buffer_size - 1] = '\0';
             }
             
             // Update manager load status
             memcpy(&config_manager->last_load_status, load_status, sizeof(polycall_edge_config_load_status_t));
             
             return result;
         }
         
         // Merge global configuration if loaded
         if (config_manager->global_root) {
             // Merge with current configuration
             result = merge_config_nodes(
                 config_manager,
                 config_manager->root,
                 config_manager->global_root,
                 load_status
             );
             
             if (result != POLYCALL_CORE_SUCCESS) {
                 // Copy error message if provided
                 if (error_buffer && error_buffer_size > 0 && load_status->error_message) {
                     strncpy(error_buffer, load_status->error_message, error_buffer_size - 1);
                     error_buffer[error_buffer_size - 1] = '\0';
                 }
                 
                 // Update manager load status
                 memcpy(&config_manager->last_load_status, load_status, sizeof(polycall_edge_config_load_status_t));
                 
                 return result;
             }
             
             config_manager->has_changes = true;
         }
     }
     
     // Load component-specific configuration if specified
     if (config_manager->options.component_config_path) {
         polycall_core_error_t result = load_config_from_file(
             config_manager,
             config_manager->options.component_config_path,
             &config_manager->component_root,
             load_status
         );
         
         if (result != POLYCALL_CORE_SUCCESS && !config_manager->options.allow_missing_global) {
             // Copy error message if provided
             if (error_buffer && error_buffer_size > 0 && load_status->error_message) {
                 strncpy(error_buffer, load_status->error_message, error_buffer_size - 1);
                 error_buffer[error_buffer_size - 1] = '\0';
             }
             
             // Update manager load status
             memcpy(&config_manager->last_load_status, load_status, sizeof(polycall_edge_config_load_status_t));
             
             return result;
         }
         
         // Merge component configuration if loaded
         if (config_manager->component_root) {
             // Merge with current configuration
             result = merge_config_nodes(
                 config_manager,
                 config_manager->root,
                 config_manager->component_root,
                 load_status
             );
             
             if (result != POLYCALL_CORE_SUCCESS) {
                 // Copy error message if provided
                 if (error_buffer && error_buffer_size > 0 && load_status->error_message) {
                     strncpy(error_buffer, load_status->error_message, error_buffer_size - 1);
                     error_buffer[error_buffer_size - 1] = '\0';
                 }
                 
                 // Update manager load status
                 memcpy(&config_manager->last_load_status, load_status, sizeof(polycall_edge_config_load_status_t));
                 
                 return result;
             }
             
             config_manager->has_changes = true;
         }
     }
     
     // Update manager load status
     memcpy(&config_manager->last_load_status, load_status, sizeof(polycall_edge_config_load_status_t));
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Apply configuration to edge component
  */
 polycall_core_error_t polycall_edge_config_manager_apply(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     polycall_edge_component_t* component
 ) {
     if (!core_ctx || !config_manager || !component) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get component name
     char component_name[256];
     polycall_edge_component_config_t current_config;
     
     polycall_core_error_t result = polycall_edge_component_get_config(
         core_ctx,
         component,
         &current_config
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // Use component name or ID
     if (current_config.component_name) {
         strncpy(component_name, current_config.component_name, sizeof(component_name) - 1);
         component_name[sizeof(component_name) - 1] = '\0';
     } else if (current_config.component_id) {
         strncpy(component_name, current_config.component_id, sizeof(component_name) - 1);
         component_name[sizeof(component_name) - 1] = '\0';
     } else {
         // No name or ID available
         return POLYCALL_CORE_ERROR_INVALID_STATE;
     }
     
     // Get component configuration
     polycall_edge_component_config_t new_config;
     result = polycall_edge_config_manager_get_component_config(
         core_ctx,
         config_manager,
         component_name,
         &new_config
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // Update component configuration
     return polycall_edge_component_update_config(
         core_ctx,
         component,
         &new_config
     );
 }
 
 /**
  * @brief Get component configuration from configuration manager
  */
 polycall_core_error_t polycall_edge_config_manager_get_component_config(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     const char* component_name,
     polycall_edge_component_config_t* config
 ) {
     if (!core_ctx || !config_manager || !component_name || !config) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get namespace prefix
     const char* namespace_prefix = config_manager->options.config_namespace;
     if (!namespace_prefix || namespace_prefix[0] == '\0') {
         namespace_prefix = "edge";
     }
     
     // Check if component has specific configuration
     snprintf(config_manager->path_buffer, sizeof(config_manager->path_buffer),
              "%s.components.%s", namespace_prefix, component_name);
     
     polycall_config_node_t* component_node = find_config_node(
         config_manager->root,
         config_manager->path_buffer
     );
     
     // If component-specific configuration not found, use default
     if (!component_node) {
         // Look up default component configuration
         snprintf(config_manager->path_buffer, sizeof(config_manager->path_buffer),
                  "%s.component", namespace_prefix);
         
         component_node = find_config_node(
             config_manager->root,
             config_manager->path_buffer
         );
         
         if (!component_node) {
             // Use default configuration
             *config = polycall_edge_component_default_config();
             return POLYCALL_CORE_SUCCESS;
         }
     }
     
     // Apply component configuration
     return apply_component_config(
         config_manager,
         component_node,
         config
     );
 }
 
 /**
  * @brief Get string value from configuration
  */
 polycall_core_error_t polycall_edge_config_manager_get_string(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     const char* path,
     const char* default_value,
     char** result
 ) {
     if (!core_ctx || !config_manager || !path || !result) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get namespace prefix
     const char* namespace_prefix = config_manager->options.config_namespace;
     if (!namespace_prefix || namespace_prefix[0] == '\0') {
         namespace_prefix = "edge";
     }
     
     // Build full path
     char full_path[512];
     if (strncmp(path, namespace_prefix, strlen(namespace_prefix)) == 0) {
         strncpy(full_path, path, sizeof(full_path) - 1);
     } else {
         snprintf(full_path, sizeof(full_path), "%s.%s", namespace_prefix, path);
     }
     
     // Get value
     polycall_config_value_t value;
     polycall_core_error_t result = polycall_config_get_value(
         config_manager->parser,
         config_manager->root,
         full_path,
         &value
     );
     
     if (result != POLYCALL_CORE_SUCCESS || 
         (value.type != POLYCALL_CONFIG_VALUE_FLOAT && value.type != POLYCALL_CONFIG_VALUE_INTEGER)) {
         // Use default value
         *result = default_value;
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Return float value (convert from integer if needed)
     if (value.type == POLYCALL_CONFIG_VALUE_FLOAT) {
         *result = value.float_value;
     } else {
         *result = (double)value.integer_value;
     }
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get boolean value from configuration
  */
 polycall_core_error_t polycall_edge_config_manager_get_bool(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     const char* path,
     bool default_value,
     bool* result
 ) {
     if (!core_ctx || !config_manager || !path || !result) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get namespace prefix
     const char* namespace_prefix = config_manager->options.config_namespace;
     if (!namespace_prefix || namespace_prefix[0] == '\0') {
         namespace_prefix = "edge";
     }
     
     // Build full path
     char full_path[512];
     if (strncmp(path, namespace_prefix, strlen(namespace_prefix)) == 0) {
         strncpy(full_path, path, sizeof(full_path) - 1);
     } else {
         snprintf(full_path, sizeof(full_path), "%s.%s", namespace_prefix, path);
     }
     
     // Get value
     polycall_config_value_t value;
     polycall_core_error_t result_code = polycall_config_get_value(
         config_manager->parser,
         config_manager->root,
         full_path,
         &value
     );
     
     if (result_code != POLYCALL_CORE_SUCCESS) {
         // Use default value if path not found
         *result = default_value;
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Handle different value types
     switch (value.type) {
         case POLYCALL_CONFIG_VALUE_BOOLEAN:
             *result = value.boolean_value;
             break;
             
         case POLYCALL_CONFIG_VALUE_INTEGER:
             *result = (value.integer_value != 0);
             break;
             
         case POLYCALL_CONFIG_VALUE_STRING:
             // Handle string values like "true", "yes", "1"
             if (!value.string_value) {
                 *result = default_value;
             } else if (strcasecmp(value.string_value, "true") == 0 ||
                        strcasecmp(value.string_value, "yes") == 0 ||
                        strcasecmp(value.string_value, "1") == 0 ||
                        strcasecmp(value.string_value, "on") == 0 ||
                        strcasecmp(value.string_value, "enabled") == 0) {
                 *result = true;
             } else {
                 *result = false;
             }
             break;
             
         default:
             *result = default_value;
             break;
     }
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Set configuration value (runtime update)
  */
 polycall_core_error_t polycall_edge_config_manager_set_value(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     const char* path,
     polycall_config_value_type_t value_type,
     const void* value
 ) {
     if (!core_ctx || !config_manager || !path || !value) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get namespace prefix
     const char* namespace_prefix = config_manager->options.config_namespace;
     if (!namespace_prefix || namespace_prefix[0] == '\0') {
         namespace_prefix = "edge";
     }
     
     // Build full path
     char full_path[512];
     if (strncmp(path, namespace_prefix, strlen(namespace_prefix)) == 0) {
         strncpy(full_path, path, sizeof(full_path) - 1);
     } else {
         snprintf(full_path, sizeof(full_path), "%s.%s", namespace_prefix, path);
     }
     
     // Prepare value to set
     polycall_config_value_t config_value;
     memset(&config_value, 0, sizeof(config_value));
     config_value.type = value_type;
     
     // Set value based on type
     switch (value_type) {
         case POLYCALL_CONFIG_VALUE_STRING:
             config_value.string_value = (const char*)value;
             break;
             
         case POLYCALL_CONFIG_VALUE_INTEGER:
             config_value.integer_value = *((int64_t*)value);
             break;
             
         case POLYCALL_CONFIG_VALUE_FLOAT:
             config_value.float_value = *((double*)value);
             break;
             
         case POLYCALL_CONFIG_VALUE_BOOLEAN:
             config_value.boolean_value = *((bool*)value);
             break;
             
         default:
             return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Set value in configuration
     polycall_core_error_t result = polycall_config_set_value(
         config_manager->parser,
         config_manager->root,
         full_path,
         &config_value
     );
     
     if (result == POLYCALL_CORE_SUCCESS) {
         // Add to value cache
         add_to_value_cache(
             config_manager,
             full_path,
             config_value,
             EDGE_CONFIG_SOURCE_RUNTIME
         );
         
         config_manager->has_changes = true;
     }
     
     return result;
 }
 
 /**
  * @brief Save current configuration to file
  */
 polycall_core_error_t polycall_edge_config_manager_save(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     const char* file_path,
     bool include_defaults
 ) {
     if (!core_ctx || !config_manager || !file_path) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Determine which configuration to save
     polycall_config_node_t* config_to_save = config_manager->root;
     
     // Filter defaults if needed
     if (!include_defaults && config_manager->defaults_root) {
         // Create a filtered configuration
         polycall_config_node_t* filtered_config = NULL;
         polycall_core_error_t result = polycall_config_node_clone(
             config_manager->parser,
             config_manager->root,
             &filtered_config
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             return result;
         }
         
         // Remove default values
         polycall_config_filter_options_t filter_options;
         filter_options.remove_defaults = true;
         filter_options.default_config = config_manager->defaults_root;
         
         result = polycall_config_filter(
             config_manager->parser,
             filtered_config,
             &filter_options
         );
         
         if (result != POLYCALL_CORE_SUCCESS) {
             polycall_config_node_destroy(config_manager->parser, filtered_config);
             return result;
         }
         
         config_to_save = filtered_config;
     }
     
     // Save configuration to file
     polycall_core_error_t result = polycall_config_save_to_file(
         config_manager->parser,
         config_to_save,
         file_path
     );
     
     // Clean up filtered configuration if created
     if (config_to_save != config_manager->root) {
         polycall_config_node_destroy(config_manager->parser, config_to_save);
     }
     
     return result;
 }
 
 /**
  * @brief Reset configuration to defaults
  */
 polycall_core_error_t polycall_edge_config_manager_reset(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager
 ) {
     if (!core_ctx || !config_manager) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Clean up current configuration
     if (config_manager->root) {
         polycall_config_node_destroy(config_manager->parser, config_manager->root);
         config_manager->root = NULL;
     }
     
     // Clone default configuration
     polycall_core_error_t result = polycall_config_node_clone(
         config_manager->parser,
         config_manager->defaults_root,
         &config_manager->root
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // Reset components node
     if (config_manager->components) {
         polycall_config_node_destroy(config_manager->parser, config_manager->components);
     }
     
     result = polycall_config_node_create(
         config_manager->parser,
         &config_manager->components,
         "components",
         POLYCALL_CONFIG_NODE_SECTION
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         return result;
     }
     
     // Add components node to root
     result = polycall_config_node_add_child(
         config_manager->parser,
         config_manager->root,
         config_manager->components
     );
     
     if (result != POLYCALL_CORE_SUCCESS) {
         polycall_config_node_destroy(config_manager->parser, config_manager->components);
         config_manager->components = NULL;
         return result;
     }
     
     // Clear value cache
     if (config_manager->value_cache) {
         polycall_core_free(core_ctx, config_manager->value_cache);
         config_manager->value_cache = NULL;
         config_manager->value_cache_count = 0;
         config_manager->value_cache_capacity = 0;
     }
     
     // Mark as changed
     config_manager->has_changes = true;
     
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Create default configuration manager options
  */
 polycall_edge_config_manager_options_t polycall_edge_config_manager_default_options(void) {
     polycall_edge_config_manager_options_t options;
     
     // Initialize to zero
     memset(&options, 0, sizeof(options));
     
     // Set default values
     options.global_config_path = "/etc/polycall/config.Polycallfile";
     options.component_config_path = NULL;
     options.schema_path = NULL;
     options.allow_missing_global = true;
     options.apply_environment_vars = true;
     options.validation_level = EDGE_CONFIG_VALIDATE_CONSTRAINTS;
     options.trace_config_changes = false;
     options.merge_with_defaults = true;
     options.config_namespace = "edge";
     
     return options;
 }
 
 /**
  * @brief Clean up edge configuration manager
  */
 void polycall_edge_config_manager_cleanup(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager
 ) {
     if (!core_ctx || !config_manager) {
         return;
     }
     
     // Clean up configuration nodes
     if (config_manager->root) {
         polycall_config_node_destroy(config_manager->parser, config_manager->root);
     }
     
     if (config_manager->defaults_root) {
         polycall_config_node_destroy(config_manager->parser, config_manager->defaults_root);
     }
     
     if (config_manager->global_root) {
         polycall_config_node_destroy(config_manager->parser, config_manager->global_root);
     }
     
     if (config_manager->component_root) {
         polycall_config_node_destroy(config_manager->parser, config_manager->component_root);
     }
     
     if (config_manager->schema_root) {
         polycall_config_node_destroy(config_manager->parser, config_manager->schema_root);
     }
     
     // Components node is cleaned up with root
     
     // Clean up value cache
     if (config_manager->value_cache) {
         polycall_core_free(core_ctx, config_manager->value_cache);
     }
     
     // Clean up parser
     if (config_manager->parser) {
         polycall_config_parser_cleanup(core_ctx, config_manager->parser);
     }
     
     // Free manager structure
     polycall_core_free(core_ctx, config_manager);
 }

/**
 * @brief Get string value from configuration
 */
polycall_core_error_t polycall_edge_config_manager_get_string(
    polycall_core_context_t* core_ctx,
    polycall_edge_config_manager_t* config_manager,
    const char* path,
    const char* default_value,
    char** result
) {
    if (!core_ctx || !config_manager || !path || !result) {
        return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
    }
    
    // Get namespace prefix
    const char* namespace_prefix = config_manager->options.config_namespace;
    if (!namespace_prefix || namespace_prefix[0] == '\0') {
        namespace_prefix = "edge";
    }
    
    // Build full path
    char full_path[512];
    if (strncmp(path, namespace_prefix, strlen(namespace_prefix)) == 0) {
        strncpy(full_path, path, sizeof(full_path) - 1);
    } else {
        snprintf(full_path, sizeof(full_path), "%s.%s", namespace_prefix, path);
    }
    
    // Get value
    polycall_config_value_t value;
    polycall_core_error_t result_code = polycall_config_get_value(
        config_manager->parser,
        config_manager->root,
        full_path,
        &value
    );
     
    if (result_code != POLYCALL_CORE_SUCCESS || value.type != POLYCALL_CONFIG_VALUE_STRING) {
         // Use default value
         if (default_value) {
             *result = polycall_core_malloc(core_ctx, strlen(default_value) + 1);
             if (*result) {
                 strcpy(*result, default_value);
                 return POLYCALL_CORE_SUCCESS;
             } else {
                 return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
             }
         } else {
             *result = NULL;
             return POLYCALL_CORE_SUCCESS;
         }
     }
     
     // Copy string value
     *result = polycall_core_malloc(core_ctx, strlen(value.string_value) + 1);
     if (*result) {
         strcpy(*result, value.string_value);
         return POLYCALL_CORE_SUCCESS;
     } else {
         return POLYCALL_CORE_ERROR_OUT_OF_MEMORY;
     }
 }
 
 /**
  * @brief Get integer value from configuration
  */
 polycall_core_error_t polycall_edge_config_manager_get_int(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     const char* path,
     int64_t default_value,
     int64_t* result
 ) {
     if (!core_ctx || !config_manager || !path || !result) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get namespace prefix
     const char* namespace_prefix = config_manager->options.config_namespace;
     if (!namespace_prefix || namespace_prefix[0] == '\0') {
         namespace_prefix = "edge";
     }
     
     // Build full path
     char full_path[512];
     if (strncmp(path, namespace_prefix, strlen(namespace_prefix)) == 0) {
         strncpy(full_path, path, sizeof(full_path) - 1);
     } else {
         snprintf(full_path, sizeof(full_path), "%s.%s", namespace_prefix, path);
     }
     
     // Get value
     polycall_config_value_t value;
     polycall_core_error_t result = polycall_config_get_value(
         config_manager->parser,
         config_manager->root,
         full_path,
         &value
     );
     
     if (result != POLYCALL_CORE_SUCCESS || value.type != POLYCALL_CONFIG_VALUE_INTEGER) {
         // Use default value
         *result = default_value;
         return POLYCALL_CORE_SUCCESS;
     }
     
     // Return integer value
     *result = value.integer_value;
     return POLYCALL_CORE_SUCCESS;
 }
 
 /**
  * @brief Get float value from configuration
  */
 polycall_core_error_t polycall_edge_config_manager_get_float(
     polycall_core_context_t* core_ctx,
     polycall_edge_config_manager_t* config_manager,
     const char* path,
     double default_value,
     double* result
 ) {
     if (!core_ctx || !config_manager || !path || !result) {
         return POLYCALL_CORE_ERROR_INVALID_PARAMETERS;
     }
     
     // Get namespace prefix
     const char* namespace_prefix = config_manager->options.config_namespace;
     if (!namespace_prefix || namespace_prefix[0] == '\0') {
         namespace_prefix = "edge";
     }
     
     // Build full path
     char full_path[512];
     if (strncmp(path, namespace_prefix, strlen(namespace_prefix)) == 0) {
         strncpy(full_path, path, sizeof(full_path) - 1);
     } else {
         snprintf(full_path, sizeof(full_path), "%s.%s", namespace_prefix, path);
     }
     
     // Get value
    polycall_config_value_t value;
     polycall_core_error_t result = polycall_config_get_value(
         config_manager->parser,
         config_manager->root,
         full_path,
         &value
     );

        if (result != POLYCALL_CORE_SUCCESS || value.type != POLYCALL_CONFIG_VALUE_FLOAT) {
            // Use default value
            *result = default_value;
            return POLYCALL_CORE_SUCCESS;
        }

        // Return float value
        *result = value.float_value;
        return POLYCALL_CORE_SUCCESS;
    }
    