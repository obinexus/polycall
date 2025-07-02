# LibPolyCall telemetry Component (Isolated Build)
# Sinphasé governance-compliant isolated component

project(polycall_telemetry)

add_library(telemetry STATIC
    telemetry.c
)

target_include_directories(telemetry
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/libpolycall/include
)

# Component isolation for Sinphasé governance
set_target_properties(telemetry PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)
