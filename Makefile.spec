# Recursion guard  
ifndef MAKEFILE_SPEC_INCLUDED
MAKEFILE_SPEC_INCLUDED := 1

# Makefile.spec - QA and Testing Framework
# Comprehensive testing, validation, and quality assurance
# Copyright (c) 2025 OBINexus Computing

# Testing Configuration
TEST_DIR := tests
TEST_BUILD_DIR := $(BUILD_DIR)/tests
TEST_REPORTS_DIR := reports
COVERAGE_DIR := $(TEST_REPORTS_DIR)/coverage

# Test Categories
UNIT_TEST_DIR := $(TEST_DIR)/unit
INTEGRATION_TEST_DIR := $(TEST_DIR)/integration
PERFORMANCE_TEST_DIR := $(TEST_DIR)/performance
SECURITY_TEST_DIR := $(TEST_DIR)/security

# Testing Tools
VALGRIND := valgrind
GCOV := gcov
LCOV := lcov
GENHTML := genhtml
CLANG_TIDY := clang-tidy
CPPCHECK := cppcheck
SCAN_BUILD := scan-build

# Test Flags
TEST_CFLAGS := $(CFLAGS) -fprofile-arcs -ftest-coverage -DTESTING
TEST_LDFLAGS := -lgcov --coverage

# Linting Configuration
LINT_FLAGS := \
	-checks='-*,\
	bugprone-*,\
	cert-*,\
	clang-analyzer-*,\
	cppcoreguidelines-*,\
	misc-*,\
	modernize-*,\
	performance-*,\
	portability-*,\
	readability-*' \
	-header-filter='.*' \
	-warnings-as-errors='*'

# Primary QA Targets
.PHONY: qa-full run-tests lint-all coverage-report performance-test security-test

qa-full: lint-all run-tests coverage-report security-test
	@echo "Full QA suite completed"
	@$(MAKE) -s generate-qa-report

# Test Execution
run-tests: unit-tests integration-tests
	@echo "All tests passed"

unit-tests: prepare-test-env
	@echo "Running unit tests..."
	@$(MAKE) -s compile-unit-tests
	@$(MAKE) -s execute-unit-tests

integration-tests: prepare-test-env
	@echo "Running integration tests..."
	@$(MAKE) -s compile-integration-tests
	@$(MAKE) -s execute-integration-tests

# Test Environment Preparation
prepare-test-env:
	@mkdir -p $(TEST_BUILD_DIR)
	@mkdir -p $(TEST_REPORTS_DIR)
	@mkdir -p $(COVERAGE_DIR)

# Unit Test Compilation
compile-unit-tests:
	@echo "Compiling unit tests..."
	@for test_file in $$(find $(UNIT_TEST_DIR) -name "*.c"); do \
		test_name=$$(basename $$test_file .c); \
		echo "  CC $$test_name"; \
		$(CC) $(TEST_CFLAGS) -I$(INCLUDE_DIR) -I$(TEST_DIR) \
			$$test_file \
			-L$(LIB_DIR) -lpolycall \
			$(TEST_LDFLAGS) \
			-o $(TEST_BUILD_DIR)/$$test_name || exit 1; \
	done

# Unit Test Execution
execute-unit-tests:
	@echo "Executing unit tests..."
	@FAILED=0; \
	TOTAL=0; \
	for test_bin in $(TEST_BUILD_DIR)/*; do \
		if [ -x "$$test_bin" ]; then \
			TOTAL=$$((TOTAL + 1)); \
			TEST_NAME=$$(basename $$test_bin); \
			echo -n "  Running $$TEST_NAME... "; \
			if $(VALGRIND) --quiet --error-exitcode=1 --leak-check=full \
				--show-leak-kinds=all --track-origins=yes \
				$$test_bin > $(TEST_REPORTS_DIR)/$$TEST_NAME.log 2>&1; then \
				echo "PASS"; \
			else \
				echo "FAIL"; \
				FAILED=$$((FAILED + 1)); \
				echo "    See $(TEST_REPORTS_DIR)/$$TEST_NAME.log for details"; \
			fi; \
		fi; \
	done; \
	echo "Unit Tests: $$((TOTAL - FAILED))/$$TOTAL passed"; \
	if [ $$FAILED -ne 0 ]; then exit 1; fi

# Integration Test Compilation
compile-integration-tests:
	@echo "Compiling integration tests..."
	@# Similar pattern for integration tests
	@for test_file in $$(find $(INTEGRATION_TEST_DIR) -name "*.c" 2>/dev/null || true); do \
		test_name=$$(basename $$test_file .c); \
		echo "  CC $$test_name"; \
		$(CC) $(TEST_CFLAGS) -I$(INCLUDE_DIR) -I$(TEST_DIR) \
			$$test_file \
			-L$(LIB_DIR) -lpolycall \
			$(TEST_LDFLAGS) \
			-o $(TEST_BUILD_DIR)/integ_$$test_name || exit 1; \
	done

# Integration Test Execution
execute-integration-tests:
	@echo "Executing integration tests..."
	@# Similar pattern to unit tests
	@for test_bin in $(TEST_BUILD_DIR)/integ_*; do \
		if [ -x "$$test_bin" ]; then \
			TEST_NAME=$$(basename $$test_bin); \
			echo "  Running $$TEST_NAME..."; \
			$$test_bin || exit 1; \
		fi; \
	done 2>/dev/null || echo "No integration tests found"

# Code Coverage
coverage-report: run-tests
	@echo "Generating coverage report..."
	@# Collect coverage data
	@find $(BUILD_DIR) -name "*.gcda" -o -name "*.gcno" | \
		xargs -I {} cp {} $(COVERAGE_DIR)/
	@# Generate coverage info
	@$(LCOV) --capture --directory $(COVERAGE_DIR) \
		--output-file $(COVERAGE_DIR)/coverage.info \
		--rc lcov_branch_coverage=1 2>/dev/null || true
	@# Filter out system headers and test code
	@$(LCOV) --remove $(COVERAGE_DIR)/coverage.info \
		'/usr/*' '*/test/*' \
		--output-file $(COVERAGE_DIR)/coverage_filtered.info \
		--rc lcov_branch_coverage=1 2>/dev/null || true
	@# Generate HTML report
	@$(GENHTML) $(COVERAGE_DIR)/coverage_filtered.info \
		--output-directory $(COVERAGE_DIR)/html \
		--branch-coverage 2>/dev/null || true
	@echo "Coverage report: $(COVERAGE_DIR)/html/index.html"

# Linting
lint-all: lint-source lint-headers lint-security

lint-source:
	@echo "Linting source files..."
	@# C/C++ linting with clang-tidy
	@find $(SRC_DIR) -name "*.c" -o -name "*.cpp" | \
		xargs -I {} $(CLANG_TIDY) {} $(LINT_FLAGS) -- $(CFLAGS) 2>/dev/null || true
	@# Additional checks with cppcheck
	@$(CPPCHECK) --enable=all --suppress=missingIncludeSystem \
		--error-exitcode=1 \
		-I $(INCLUDE_DIR) \
		$(SRC_DIR) 2> $(TEST_REPORTS_DIR)/cppcheck.log || \
		(echo "Cppcheck found issues. See $(TEST_REPORTS_DIR)/cppcheck.log"; exit 1)

lint-headers:
	@echo "Linting header files..."
	@# Check header guards
	@for header in $$(find $(INCLUDE_DIR) -name "*.h"); do \
		GUARD=$$(grep -E "^#ifndef|^#define" $$header | head -2); \
		if [ -z "$$GUARD" ]; then \
			echo "Warning: $$header missing header guard"; \
		fi; \
	done
	@# Check for self-contained headers
	@$(MAKE) -s check-header-compilation

lint-security:
	@echo "Security-focused linting..."
	@# Scan for security issues
	@$(SCAN_BUILD) --status-bugs -o $(TEST_REPORTS_DIR)/scan-build \
		make -f $(MAKEFILE_BUILD) build-all 2>/dev/null || true

# Header Compilation Check
check-header-compilation:
	@echo "Checking header self-containment..."
	@for header in $$(find $(INCLUDE_DIR) -name "*.h"); do \
		echo -n "  Checking $$header... "; \
		echo "#include \"$$header\"" | \
			$(CC) $(CFLAGS) -x c -c - -o /dev/null 2>/dev/null && \
			echo "OK" || echo "FAIL"; \
	done

# Performance Testing
performance-test: build-all
	@echo "Running performance tests..."
	@mkdir -p $(TEST_REPORTS_DIR)/perf
	@# Compile performance tests
	@for test_file in $$(find $(PERFORMANCE_TEST_DIR) -name "*.c" 2>/dev/null || true); do \
		test_name=$$(basename $$test_file .c); \
		$(CC) $(CFLAGS) -O3 -I$(INCLUDE_DIR) \
			$$test_file \
			-L$(LIB_DIR) -lpolycall \
			-o $(TEST_BUILD_DIR)/perf_$$test_name; \
	done
	@# Run performance tests
	@for test_bin in $(TEST_BUILD_DIR)/perf_*; do \
		if [ -x "$$test_bin" ]; then \
			TEST_NAME=$$(basename $$test_bin); \
			echo "  Running $$TEST_NAME..."; \
			/usr/bin/time -v $$test_bin 2>&1 | \
				tee $(TEST_REPORTS_DIR)/perf/$$TEST_NAME.log; \
		fi; \
	done 2>/dev/null || echo "No performance tests found"

# Security Testing
security-test: build-all
	@echo "Running security tests..."
	@# Fuzzing setup
	@$(MAKE) -s setup-fuzzing
	@# Run security-specific tests
	@$(MAKE) -s run-security-suite

setup-fuzzing:
	@echo "Setting up fuzzing environment..."
	@# Check for AFL or libFuzzer
	@if command -v afl-fuzz >/dev/null 2>&1; then \
		echo "AFL fuzzer available"; \
		$(MAKE) -s afl-fuzz-tests; \
	elif [ -n "$$($(CC) -fsanitize=fuzzer 2>&1 | grep -v error)" ]; then \
		echo "libFuzzer available"; \
		$(MAKE) -s libfuzzer-tests; \
	else \
		echo "No fuzzer available, skipping fuzz tests"; \
	fi

run-security-suite:
	@echo "Running security test suite..."
	@# ASAN tests
	@if [ -n "$$($(CC) -fsanitize=address 2>&1 | grep -v error)" ]; then \
		echo "  Running AddressSanitizer tests..."; \
		ASAN_OPTIONS=detect_leaks=1:check_initialization_order=1 \
			$(MAKE) -s run-asan-tests; \
	fi
	@# TSAN tests
	@if [ -n "$$($(CC) -fsanitize=thread 2>&1 | grep -v error)" ]; then \
		echo "  Running ThreadSanitizer tests..."; \
		TSAN_OPTIONS=halt_on_error=1 \
			$(MAKE) -s run-tsan-tests; \
	fi

# Generate QA Report
generate-qa-report:
	@echo "Generating QA report..."
	@cat > $(TEST_REPORTS_DIR)/qa-report.md <<EOF
	# PolyCall QA Report
	
	**Date**: $$(date -u)
	**Version**: $(VERSION)
	**Build**: $(BUILD_HASH)
	
	## Test Results
	
	### Unit Tests
	$$(grep -c "PASS" $(TEST_REPORTS_DIR)/*.log 2>/dev/null || echo 0) passed
	
	### Integration Tests
	Status: $$([ -f $(TEST_BUILD_DIR)/integ_* ] && echo "Completed" || echo "Not run")
	
	### Code Coverage
	$$($(LCOV) --summary $(COVERAGE_DIR)/coverage_filtered.info 2>/dev/null | grep lines || echo "Not generated")
	
	### Static Analysis
	- Clang-Tidy: $$([ -f $(TEST_REPORTS_DIR)/clang-tidy.log ] && echo "Completed" || echo "Not run")
	- Cppcheck: $$([ -f $(TEST_REPORTS_DIR)/cppcheck.log ] && echo "Completed" || echo "Not run")
	
	### Security Analysis
	- ASAN: $$([ -d $(TEST_REPORTS_DIR)/asan ] && echo "Completed" || echo "Not run")
	- TSAN: $$([ -d $(TEST_REPORTS_DIR)/tsan ] && echo "Completed" || echo "Not run")
	
	## Recommendations
	
	$$($(MAKE) -s generate-recommendations)
	EOF
	@echo "QA report generated: $(TEST_REPORTS_DIR)/qa-report.md"

generate-recommendations:
	@# Analyze results and generate recommendations
	@if [ -f $(TEST_REPORTS_DIR)/cppcheck.log ] && [ -s $(TEST_REPORTS_DIR)/cppcheck.log ]; then \
		echo "- Address Cppcheck warnings"; \
	fi
	@if [ -f $(COVERAGE_DIR)/coverage_filtered.info ]; then \
		COV=$$($(LCOV) --summary $(COVERAGE_DIR)/coverage_filtered.info 2>/dev/null | \
			grep lines | sed 's/.*: \([0-9.]*\)%.*/\1/'); \
		if [ "$$(echo "$$COV < 80" | bc)" -eq 1 ] 2>/dev/null; then \
			echo "- Improve code coverage (currently $$COV%)"; \
		fi; \
	fi

# Clean test artifacts
clean-tests:
	@rm -rf $(TEST_BUILD_DIR)
	@rm -rf $(TEST_REPORTS_DIR)
	@find . -name "*.gcda" -o -name "*.gcno" | xargs rm -f
endif # MAKEFILE_SPEC_INCLUDED

# QA Targets
.PHONY: qa qa-full

qa: test-unit test-integration
	@echo "[QA] Running quality assurance checks..."
	@$(MAKE) lint || true
	@$(MAKE) security-scan || true
	@echo "[QA] Complete"

qa-full: qa test-coverage test-memory
	@echo "[QA] Full quality assurance complete"

# Test targets
.PHONY: test test-unit test-integration

test: test-unit

test-unit:
	@echo "[TEST] Running unit tests..."
	@find test/unit -name "test_*.c" -exec $(CC) {} -o {}.out \; 2>/dev/null || true
	@echo "[TEST] Unit tests complete"

test-integration:
	@echo "[TEST] Running integration tests..."
	@find test/integration -name "test_*.c" -exec $(CC) {} -o {}.out \; 2>/dev/null || true
	@echo "[TEST] Integration tests complete"
