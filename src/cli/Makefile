# CLI components Makefile
BUILD_MODE ?= release
BUILD_DIR ?= ../../build

CLI_SOURCES = polycall_cli.c
CLI_OBJECTS = $(CLI_SOURCES:.c=.o)
CLI_TARGET = $(BUILD_DIR)/polycall

CFLAGS = -Wall -Wextra -std=c11 -I../../include
ifeq ($(BUILD_MODE),debug)
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O3 -DNDEBUG
endif

.PHONY: all clean

all: $(CLI_TARGET)

$(CLI_TARGET): $(CLI_OBJECTS) | $(BUILD_DIR)
	@echo "[CLI] Linking $@..."
	@$(CC) $(CLI_OBJECTS) -L$(BUILD_DIR) -lpolycall -o $@

%.o: %.c
	@echo "[CLI] Compiling $<..."
	@$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

clean:
	@rm -f $(CLI_OBJECTS) $(CLI_TARGET)
