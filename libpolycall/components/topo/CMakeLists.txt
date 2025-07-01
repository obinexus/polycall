# LibPolyCall topo Component (Isolated Build)
# Sinphasé governance-compliant isolated component

project(polycall_topo)

add_library(topo STATIC
    topo.c
)

target_include_directories(topo
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/libpolycall/include
)

# Component isolation for Sinphasé governance
set_target_properties(topo PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)
