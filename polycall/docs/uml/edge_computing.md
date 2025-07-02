```mermaid
%%{init: {'theme': 'neutral'}}%%
flowchart TB
    %% Core Edge Computing Components
    subgraph "LibPolyCall Edge Computing Architecture"
        direction TB
        
        %% Compute Router
        ComputeRouter[/"Compute Router\n(compute_router.c)"/]
        ComputeRouter --> |Routes computation| NodeSelector
        ComputeRouter --> |Manages security| EdgeSecurity
        
        %% Node Selector
        NodeSelector[/"Node Selector\n(node_selector.c)"/]
        NodeSelector --> |Selects optimal nodes| FallbackMechanism
        
        %% Fallback Mechanism
        FallbackMechanism[/"Fallback Mechanism\n(fallback.c)"/]
        FallbackMechanism --> |Provides redundancy| EdgeSecurity
        
        %% Security
        EdgeSecurity[/"Edge Security\n(security.c)"/]
        EdgeSecurity --> |Integrates with| FFISecurity
        EdgeSecurity --> |Enforces policies| ComputeRouter
    end

    %% External Integrations
    subgraph "External Integrations"
        FFISecurity[/"FFI Security Context"/]
        NetworkProtocol[/"Network Protocol"/]
        MicroCommandSystem[/"Micro Command System"/]
    end

    %% Connections
    ComputeRouter --> NetworkProtocol
    NodeSelector --> MicroCommandSystem
    EdgeSecurity --> NetworkProtocol

    %% Style Enhancements
    classDef core fill:#f9f,stroke:#333,stroke-width:2px;
    classDef integration fill:#bbf,stroke:#333,stroke-width:2px;
    
    class ComputeRouter,NodeSelector,FallbackMechanism,EdgeSecurity core;
    class FFISecurity,NetworkProtocol,MicroCommandSystem integration;
    ```