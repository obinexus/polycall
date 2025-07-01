# LibPolyCall Dependencies Finder
# Locates and configures external dependencies

# Platform-specific dependencies
if(WIN32)
    # Windows-specific dependencies
elseif(UNIX AND NOT APPLE)
    # Linux-specific dependencies
    find_package(PkgConfig)
elseif(APPLE)
    # macOS-specific dependencies
endif()

# Optional feature dependencies
if(ENABLE_BENCHMARKS)
    find_package(benchmark QUIET)
    if(NOT benchmark_FOUND)
        message(STATUS "Google Benchmark not found, benchmarks will be disabled")
        set(ENABLE_BENCHMARKS OFF CACHE BOOL "" FORCE)
    endif()
endif()
