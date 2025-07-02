# LibPolyCall crypto Component (Isolated Build)
# Sinphasé governance-compliant isolated component

project(polycall_crypto)

add_library(crypto STATIC
    crypto.c
)

target_include_directories(crypto
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}
    PRIVATE ${CMAKE_SOURCE_DIR}/libpolycall/include
)

# Component isolation for Sinphasé governance
set_target_properties(crypto PROPERTIES
    POSITION_INDEPENDENT_CODE ON
    C_STANDARD 11
    C_STANDARD_REQUIRED ON
)
