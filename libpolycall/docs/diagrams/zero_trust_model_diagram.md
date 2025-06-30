```mermaid
classDiagram
    class PolycallIgnoreContext {
        -uint32_t magic
        -polycall_core_context_t* core_ctx
        -char** patterns
        -size_t pattern_count
        -size_t patterns_capacity
        -bool case_sensitive
        +polycall_ignore_context_init()
        +polycall_ignore_context_cleanup()
        +polycall_ignore_add_pattern()
        +polycall_ignore_load_file()
        +polycall_ignore_should_ignore()
    }
    
    class PolycallfileIgnoreContext {
        -uint32_t magic
        -polycall_core_context_t* core_ctx
        -polycall_ignore_context_t* ctx
        -char* ignore_file_path
        +polycallfile_ignore_init()
        +polycallfile_ignore_cleanup()
        +polycallfile_ignore_add_pattern()
        +polycallfile_ignore_load_file()
        +polycallfile_ignore_should_ignore()
        +polycallfile_ignore_add_defaults()
    }
    
    class BindingConfigContext {
        -uint32_t magic
        -polycall_core_context_t* core_ctx
        -binding_config_section_t* sections
        -size_t section_count
        -polycall_ignore_context_t* ignore_ctx
        -bool use_ignore_patterns
        +polycall_binding_config_init()
        +polycall_binding_config_load()
        +polycall_binding_config_should_ignore()
        +polycall_binding_config_load_ignore_file()
    }
    
    class ConfigFactory {
        -polycall_core_context_t* core_ctx
        -polycall_schema_context_t* schema_ctx
        -polycall_ignore_context_t* ignore_ctx
        +polycall_config_factory_extract_component()
        +polycall_config_factory_load_from_file()
    }
    
    class ZeroTrustValidation {
        +validate_file_permissions()
        +validate_file_path()
        +validate_file_contents()
        +check_path_security()
    }
    
    PolycallIgnoreContext <-- PolycallfileIgnoreContext : uses
    PolycallIgnoreContext <-- BindingConfigContext : uses
    ZeroTrustValidation --> PolycallIgnoreContext : validates
    ZeroTrustValidation --> PolycallfileIgnoreContext : validates
    ConfigFactory --> PolycallIgnoreContext : uses
    ```