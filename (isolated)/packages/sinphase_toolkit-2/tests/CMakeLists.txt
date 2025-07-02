# LibPolyCall Test Suite

enable_testing()

# Unit tests
add_subdirectory(unit)

# Integration tests
if(BUILD_INTEGRATION_TESTS)
    add_subdirectory(integration)
endif()

# Benchmarks
if(ENABLE_BENCHMARKS)
    add_subdirectory(benchmarks)
endif()
