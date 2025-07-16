# Core components Makefile
BUILD_MODE ?= release
BUILD_DIR ?= ../../build

CORE_COMPONENTS = polycall config protocol network auth

CFLAGS = -Wall -Wextra -std=c11 -I../../include
ifeq ($(BUILD_MODE),debug)
    CFLAGS += -g -O0 -DDEBUG
else
    CFLAGS += -O3 -DNDEBUG
endif

.PHONY: all clean $(CORE_COMPONENTS)

# src/core/Makefile

.PHONY: all clean check-topology polycall

# Default target
all: check-topology polycall

# Topology check
check-topology:
	@echo "[CORE] Checking build topology..."
	@if [ ! -f .build-topology ]; then \
		echo "Missing .build-topology file"; \
		exit 1; \
	fi

# Build polycall target with proper error handling
polycall:
	@echo "[CORE] Building polycall..."
	@$(MAKE) -C polycall

clean:
	@echo "[CORE] Cleaning..."
	@$(MAKE) -C polycall clean
	@rm -rf $(BUILD_DIR)

all: $(CORE_COMPONENTS)

$(CORE_COMPONENTS):
	@echo "[CORE] Building $@..."
	@$(MAKE) -C $@ BUILD_DIR=$(BUILD_DIR) BUILD_MODE=$(BUILD_MODE)

clean:
	@for comp in $(CORE_COMPONENTS); do \
		$(MAKE) -C $$comp clean; \
	done
