# Auto-generated Makefile for isolated component
# Sinphasé-compliant with single-pass compilation verification

CC = gcc
CFLAGS = -Wall -Werror -fPIC -O2 -I./include -I../../include
LDFLAGS = -shared

# Component identification
COMPONENT = ffi

# Source discovery
SRCS = $(wildcard src/*.c src/**/*.c)
OBJS = $(SRCS:.c=.o)
DEPS = $(wildcard include/*.h include/**/*.h)

# Target library
TARGET = lib$(COMPONENT)_isolated.so

# Dependency detection for validation
COMPONENT_DEPS := $(shell grep -h "^#include \"polycall" $(SRCS) 2>/dev/null | cut -d'"' -f2 | sort -u)

.PHONY: all clean verify-single-pass check-circular install

all: verify-single-pass check-circular $(TARGET)

# Verify single-pass compilation requirement
verify-single-pass:
	@echo "Verifying single-pass compilation requirement..."
	@if [ -n "$(COMPONENT_DEPS)" ]; then \
		echo "External dependencies detected: $(COMPONENT_DEPS)"; \
		for dep in $(COMPONENT_DEPS); do \
			if [ ! -f "../../include/$$dep" ]; then \
				echo "ERROR: Missing dependency $$dep"; \
				exit 1; \
			fi; \
		done; \
	fi
	@echo "Single-pass verification: PASSED"

# Check for circular dependencies
check-circular:
	@echo "Checking for circular dependencies..."
	@if find src -name "*.c" -o -name "*.h" | xargs grep -l "^#include.*$(COMPONENT)" 2>/dev/null | grep -q .; then \
		echo "WARNING: Potential circular dependency detected"; \
	fi

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
	@echo "Built isolated component: $(TARGET)"

%.o: %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $

clean:
	rm -f $(OBJS) $(TARGET)

install: $(TARGET)
	@mkdir -p ../../lib
	@cp $(TARGET) ../../lib/
	@echo "Installed: $(TARGET)"

# Generate dependency graph for documentation
dep-graph:
	@echo "digraph $(COMPONENT) {" > $(COMPONENT).dot
	@find src -name "*.c" -o -name "*.h" | xargs grep -h "^#include \"" | \
		sed 's/^#include "\(.*\)\.h"$$/  "$(COMPONENT)" -> "\1";/' >> $(COMPONENT).dot
	@echo "}" >> $(COMPONENT).dot
	@echo "Dependency graph saved to $(COMPONENT).dot"
