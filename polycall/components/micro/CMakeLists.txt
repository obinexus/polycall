# LibPolyCall micro Component (Isolated Build)
# Sinphasé governance-compliant isolated component

project(polycall_micro)

add_library(micro STATIC
    micro.c
)

target_include_directories(micro
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/libpolycall/include
)

# Component isolation for Sinphasé governance
set_target_properties(micro PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)
