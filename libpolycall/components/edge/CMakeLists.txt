# LibPolyCall edge Component (Isolated Build)
# Sinphasé governance-compliant isolated component

project(polycall_edge)

add_library(edge STATIC
    edge.c
)

target_include_directories(edge
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/libpolycall/include
)

# Component isolation for Sinphasé governance
set_target_properties(edge PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)
