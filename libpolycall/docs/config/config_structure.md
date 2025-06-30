```mermaid
flowchart TD
    A[config] --> B[load]
    A --> C[save]
    A --> D[show]
    A --> E[set]
    A --> F[get]
    A --> G[reset]
    A --> H[validate]
    A --> I[export]
    A --> J[import]
    A --> K[mode]
    
    B --> B1["load [--global file] [--binding file]"]
    C --> C1["save [--global file] [--binding file]"]
    D --> D1["show component_type [component_name]"]
    E --> E1["set component_type component_name key=value"]
    F --> F1["get component_type component_name key"]
    G --> G1["reset [--global] [--binding]"]
    H --> H1["validate component_type component_name"]
    I --> I1["export file"]
    J --> J1["import file"]
    K --> K1["mode [--read-only | --read-write]"]
    ```