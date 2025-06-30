```mermid
classDiagram
    class ConfigurationManager {
        +load_global_config()
        +load_component_config()
        +load_binding_config()
        +apply_config_to_context()
    }
    
    class GlobalConfig {
        +parse_polycallfile()
        +validate_config()
        +merge_configs()
    }
    
    class BindingConfig {
        +parse_polycallrc()
        +validate_binding_config()
        +apply_binding_specific_settings()
    }
    
    class ComponentConfig {
        +load_defaults()
        +override_from_global()
        +validate_component_specific()
    }
    
    class MicroConfig {
        +component_settings
        +service_settings
        +zero_trust_settings
    }
    
    class FFIConfig {
        +language_settings
        +type_mapping_settings
        +security_settings
    }
    
    class ProtocolConfig {
        +enhancement_settings
        +message_settings
        +connection_settings
    }
    
    class NetworkConfig {
        +transport_settings
        +security_settings
        +connection_pool_settings
    }
    
    class EdgeConfig {
        +discovery_settings
        +routing_settings
        +fallback_settings
    }
    
    class TelemetryConfig {
        +metrics_settings
        +tracing_settings
        +logging_settings
    }
    
    ConfigurationManager --> GlobalConfig
    ConfigurationManager --> BindingConfig
    ConfigurationManager --> ComponentConfig
    
    ComponentConfig <|-- MicroConfig
    ComponentConfig <|-- FFIConfig
    ComponentConfig <|-- ProtocolConfig
    ComponentConfig <|-- NetworkConfig
    ComponentConfig <|-- EdgeConfig
    ComponentConfig <|-- TelemetryConfig
    
    class ConfigParser {
        +parse_config_file()
        +build_config_tree()
        +validate_schema()
    }
    
    class Tokenizer {
        +tokenize()
        +get_next_token()
    }
    
    class AST {
        +root_node
        +add_node()
        +traverse()
    }
    
    class MacroExpander {
        +register_macro()
        +expand_macros()
    }
    
    class ExpressionEvaluator {
        +evaluate()
        -eval_logic_expr()
        -eval_arithmetic_expr()
    }
    
    GlobalConfig --> ConfigParser
    BindingConfig --> ConfigParser
    ConfigParser --> Tokenizer
    ConfigParser --> AST
    ConfigParser --> MacroExpander
    ConfigParser --> ExpressionEvaluator
    ```