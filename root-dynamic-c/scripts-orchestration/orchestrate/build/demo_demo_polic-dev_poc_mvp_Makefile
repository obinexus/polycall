CC=gcc
CFLAGS=-O2 -Wall -Wextra -Werror -pedantic -z noexecstack -fstack-protector-strong -fpie -D_FORTIFY_SOURCE=2
LDFLAGS=-pie -Wl,-z,relro,-z,now

# Main targets
all: polic_demo

# Original PoliC demo
polic_demo: poliC_demo.c
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@

# Clean target
clean:
	rm -f polic_demo

# Test targets
test: polic_demo
	./polic_demo

# Include security checks
security-check:
	@echo "Running security checks..."
	@echo "Checking for executable stack..."
	@readelf -l polic_demo | grep STACK | grep -q RWE || echo "Security check passed: No executable stack"
	@echo "Checking for RELRO..."
	@readelf -l polic_demo | grep -q "GNU_RELRO" && echo "Security check passed: RELRO enabled"
	@echo "Checking for PIE..."
	@readelf -h polic_demo | grep -q "Type:[[:space:]]*DYN" && echo "Security check passed: PIE enabled"
	@echo "Checking for stack canary..."
	@readelf -s polic_demo | grep -q "__stack_chk_fail" && echo "Security check passed: Stack canary enabled"

.PHONY: all clean test security-check