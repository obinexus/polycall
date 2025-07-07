# Test suite CMakeLists.txt
cmake_minimum_required(VERSION 3.13)

enable_testing()

# Add test subdirectories
add_subdirectory(unit)
add_subdirectory(integration)
