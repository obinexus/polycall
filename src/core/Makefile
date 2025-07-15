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

all: $(CORE_COMPONENTS)

$(CORE_COMPONENTS):
	@echo "[CORE] Building $@..."
	@$(MAKE) -C $@ BUILD_DIR=$(BUILD_DIR) BUILD_MODE=$(BUILD_MODE)

clean:
	@for comp in $(CORE_COMPONENTS); do \
		$(MAKE) -C $$comp clean; \
	done
