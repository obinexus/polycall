CC=gcc
CFLAGS=-Wall -Wextra -Werror -z noexecstack -fstack-protector-strong -fpie
LDFLAGS=-pie -Wl,-z,relro,-z,now

# Main targets
all: polic_demo enhanced_polic

# Original PoliC demo
polic_demo: poliC_demo.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

# Enhanced PoliC demo
enhanced_polic: enhanced_polic_demo.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

# Clean target
clean:
	rm -f polic_demo enhanced_polic

# Test targets
test: enhanced_polic
	./enhanced_polic

# Include security checks
security-check:
	@echo "Running security checks..."
	@echo "Checking for executable stack..."
	@readelf -l enhanced_polic | grep STACK | grep -q RWE || echo "Security check passed: No executable stack"
	@echo "Checking for RELRO..."
	@readelf -l enhanced_polic | grep -q "GNU_RELRO" && echo "Security check passed: RELRO enabled"
	@echo "Checking for PIE..."
	@readelf -h enhanced_polic | grep -q "Type:[[:space:]]*DYN" && echo "Security check passed: PIE enabled"

.PHONY: all clean test security-check


