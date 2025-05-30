# Compiler and flags
CC := gcc
CFLAGS := -Wall -Wextra -I./include -fPIC -std=c11
LDFLAGS := -pthread -lssl -lcrypto

# Platform-specific settings
ifeq ($(OS),Windows_NT)
    LDFLAGS += -lws2_32
    CFLAGS += -D_WIN32
    SHARED_EXT := dll
    EXE_EXT := .exe
else
    SHARED_EXT := so
    EXE_EXT :=
endif

# Debug/Release flags
DEBUG_FLAGS := -g -DDEBUG
RELEASE_FLAGS := -O2 -DNDEBUG

# Directories
SRC_DIR := src
CORE_DIR := $(SRC_DIR)/core
CLI_DIR := $(SRC_DIR)/cli
INC_DIR := include
BUILD_DIR := build
LIB_DIR := $(BUILD_DIR)/lib
BIN_DIR := $(BUILD_DIR)/bin
OBJ_DIR := $(BUILD_DIR)/obj

# Component directories
CORE_COMPONENTS := polycall protocol config network ffi auth telemetry accessibility micro edge socket repl polycallfile polycallrc
CLI_COMPONENTS := commands common accessibility auth config edge ffi micro network protocol repl socket telemetry

# Source files
CORE_SRCS := $(shell find $(CORE_DIR) -name "*.c")
CLI_SRCS := $(shell find $(CLI_DIR) -name "*.c" ! -name "main.c")
MAIN_SRC := $(CLI_DIR)/main.c

# Object files
CORE_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(CORE_SRCS))
CLI_OBJS := $(patsubst $(SRC_DIR)/%.c,$(OBJ_DIR)/%.o,$(CLI_SRCS))
MAIN_OBJ := $(OBJ_DIR)/cli/main.o

# Libraries and executable
CORE_LIB := $(LIB_DIR)/libpolycall_core.$(SHARED_EXT)
CLI_LIB := $(LIB_DIR)/libpolycall_cli.a
EXECUTABLE := $(BIN_DIR)/polycall$(EXE_EXT)

# Default target
.PHONY: all
all: dirs $(CORE_LIB) $(CLI_LIB) $(EXECUTABLE)

# Create necessary directories
.PHONY: dirs
dirs:
	@mkdir -p $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)
	@mkdir -p $(foreach comp,$(CORE_COMPONENTS),$(OBJ_DIR)/core/$(comp))
	@mkdir -p $(foreach comp,$(CLI_COMPONENTS),$(OBJ_DIR)/cli/$(comp))

# Debug build
.PHONY: debug
debug: CFLAGS += $(DEBUG_FLAGS)
debug: all

# Release build
.PHONY: release
release: CFLAGS += $(RELEASE_FLAGS)
release: all

# Compile source files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Create core library
$(CORE_LIB): $(CORE_OBJS)
	@mkdir -p $(dir $@)
	$(CC) -shared -o $@ $^ $(LDFLAGS)

# Create CLI library
$(CLI_LIB): $(CLI_OBJS)
	@mkdir -p $(dir $@)
	ar rcs $@ $^

# Link executable
$(EXECUTABLE): $(MAIN_OBJ) $(CLI_LIB) $(CORE_LIB)
	@mkdir -p $(dir $@)
	$(CC) $< -o $@ -L$(LIB_DIR) -lpolycall_cli -lpolycall_core $(LDFLAGS)

# Clean build files
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

# CMake compatibility
.PHONY: cmake
cmake:
	mkdir -p build
	cd build && cmake .. && $(MAKE)

# Help target
.PHONY: help
help:
	@echo "Available targets:"
	@echo "  all        - Build everything (default)"
	@echo "  debug      - Build with debug flags"
	@echo "  release    - Build with release flags"
	@echo "  clean      - Remove build files"
	@echo "  cmake      - Build using CMake"
	@echo "  help       - Show this help message"