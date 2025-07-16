# Make-specific build configuration
BUILD_MODE ?= release
BUILD_DIR ?= build
PARALLEL_JOBS ?= $(shell nproc 2>/dev/null || echo 4)

# Compiler configuration
CC = gcc
CXX = g++
CFLAGS = -std=c11 -Wall -Wextra -I include -I lib/shared
CXXFLAGS = -std=c++17 -Wall -Wextra -I include -I lib/shared

ifeq ($(BUILD_MODE),debug)
    CFLAGS += -g -O0 -DDEBUG
    CXXFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O3 -DNDEBUG
    CXXFLAGS += -O3 -DNDEBUG
endif

# Source files
CORE_SOURCES = $(wildcard src/core/**/*.c src/core/*.c)
CLI_SOURCES = $(wildcard src/cli/**/*.c src/cli/*.c)
FFI_SOURCES = $(wildcard src/ffi/**/*.c src/ffi/*.c)

# Object files
CORE_OBJECTS = $(CORE_SOURCES:.c=.o)
CLI_OBJECTS = $(CLI_SOURCES:.c=.o)
FFI_OBJECTS = $(FFI_SOURCES:.c=.o)

# Targets
CORE_LIB = $(BUILD_DIR)/libpolycall-core.a
CLI_BIN = $(BUILD_DIR)/polycall
FFI_LIB = $(BUILD_DIR)/libpolycall-ffi.so

.PHONY: build-core build-cli build-ffi build-simple clean-make

# Main build target
build-core: $(CORE_LIB) $(CLI_BIN) $(FFI_LIB)

# Simple build target (minimal dependencies)
build-simple: $(BUILD_DIR)/polycall-simple

# Core library
$(CORE_LIB): $(CORE_OBJECTS) | $(BUILD_DIR)
	@echo "[Make] Creating core library..."
	@ar rcs $@ $^

# CLI executable
$(CLI_BIN): $(CLI_OBJECTS) $(CORE_LIB) | $(BUILD_DIR)
	@echo "[Make] Linking CLI executable..."
	@$(CC) $(CLI_OBJECTS) -L$(BUILD_DIR) -lpolycall-core -o $@

# FFI library
$(FFI_LIB): $(FFI_OBJECTS) $(CORE_LIB) | $(BUILD_DIR)
	@echo "[Make] Creating FFI library..."
	@$(CC) -shared $(FFI_OBJECTS) -L$(BUILD_DIR) -lpolycall-core -o $@

# Simple executable (all-in-one)
$(BUILD_DIR)/polycall-simple: $(CORE_SOURCES) $(CLI_SOURCES) | $(BUILD_DIR)
	@echo "[Make] Building simple executable..."
	@$(CC) $(CFLAGS) $^ -o $@

# Object files
%.o: %.c
	@echo "[Make] Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

# Create build directory
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# Clean
clean-make:
	@rm -f $(CORE_OBJECTS) $(CLI_OBJECTS) $(FFI_OBJECTS)
	@rm -f $(CORE_LIB) $(CLI_BIN) $(FFI_LIB)
	@rm -f $(BUILD_DIR)/polycall-simple
