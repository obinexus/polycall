```mermaid
classDiagram
    class polycall_repl_context {
        -polycall_core_context_t* core_ctx
        -command_history_t* history
        -char* history_file
        -char* prompt
        -bool enable_history
        -bool enable_completion
        -bool enable_syntax_highlighting
        -bool enable_log_inspection
        -bool enable_zero_trust_inspection
        -bool running
        -void* user_data
    }

    class command_history_t {
        -history_entry_t* head
        -history_entry_t* tail
        -history_entry_t* current
        -int count
        -int max_entries
    }

    class history_entry_t {
        -char* command
        -history_entry_t* prev
        -history_entry_t* next
    }

    class polycall_repl_config_t {
        +bool enable_history
        +bool enable_completion
        +bool enable_syntax_highlighting
        +bool enable_log_inspection
        +bool enable_zero_trust_inspection
        +char* history_file
        +char* prompt
        +int max_history_entries
    }

    class command_t {
        +char* name
        +char* description
        +command_handler_t handler
        +char* usage
        +bool requires_context
    }

    class command_registry {
        -command_t commands[MAX_COMMANDS]
        -int command_count
        +cli_register_command()
        +cli_execute_command()
        +cli_list_commands()
        +cli_get_command_help()
    }

    %% Relationships
    polycall_repl_context --o command_history_t : contains
    command_history_t --o history_entry_t : contains
    polycall_repl_context ..> polycall_repl_config_t : configured by
    command_registry --o command_t : registers
    polycall_repl_context --> command_registry : uses
    ```