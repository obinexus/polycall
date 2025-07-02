```c
// Initialize configuration manager
micro_config_manager_options_t options = micro_config_create_default_options();
options.global_config_path = "/etc/polycall/config.Polycallfile";
options.binding_config_path = "/etc/polycall/node.polycallrc";

micro_config_manager_t* config_manager = NULL;
polycall_core_error_t result = micro_config_manager_init(
    ctx, &config_manager, &options);

// Load configuration
micro_config_load_status_t load_status;
char error_message[1024];
result = micro_config_manager_load(
    ctx, config_manager, &load_status, error_message, sizeof(error_message));

// Apply configuration to micro context
result = micro_config_manager_apply(ctx, config_manager, micro_ctx);

// Get configuration for specific component
micro_component_config_t* bankcard_config = NULL;
result = micro_config_manager_get_component_config(
    ctx, config_manager, "bankcard", &bankcard_config);

// Clean up when done
micro_config_manager_cleanup(ctx, config_manager);
```