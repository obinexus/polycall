# Zero-Trust QA Framework for Polycall v2

## Framework Architecture

### 1. Unit QA Pipeline (Pre-Integration)

```makefile
# Makefile.qa - Zero-Trust QA Testing Framework
# Integrates with existing Component.cmake infrastructure

QA_BUILD_DIR := $(BUILD_DIR)/qa
QA_METRICS_DIR := $(QA_BUILD_DIR)/metrics
DHCPC_SERVER := $(QA_BUILD_DIR)/dhcp-c-qa-server

# QA Test Categories
QA_CATEGORIES := TP TN FP FN

# Core Component QA Targets
CORE_QA_TARGETS := \
    polycall config auth micro edge \
    telemetry network protocol repl

# CLI Command QA Targets  
CLI_QA_TARGETS := \
    config hotwire telemetry topo \
    micro edge repl

# Primary QA Target
.PHONY: qa-validate-all qa-unit-core qa-unit-cli qa-integration qa-metrics

qa-validate-all: qa-unit-core qa-unit-cli qa-integration qa-metrics
    @echo "Zero-Trust QA Validation Complete"
    @$(MAKE) -s qa-report

# Unit QA: Core Components
qa-unit-core: prepare-qa-env start-dhcpc-server
    @echo "Starting Core Unit QA Validation..."
    @for component in $(CORE_QA_TARGETS); do \
        echo "Testing $$component..."; \
        $(MAKE) -s qa-test-component COMPONENT=$$component TYPE=core; \
    done

# Unit QA: CLI Commands
qa-unit-cli: prepare-qa-env
    @echo "Starting CLI Unit QA Validation..."
    @for command in $(CLI_QA_TARGETS); do \
        echo "Testing $$command..."; \
        $(MAKE) -s qa-test-command COMMAND=$$command; \
    done

# Component Testing with TP/TN/FP/FN
qa-test-component:
    @mkdir -p $(QA_METRICS_DIR)/$(COMPONENT)
    @for category in $(QA_CATEGORIES); do \
        ./$(DHCPC_SERVER) client localhost \
            "test-unit-$(TYPE)-$(COMPONENT)" $$category \
            --config tests/qa/$(COMPONENT).conf; \
    done

# REPL Configuration Testing
qa-test-repl-config:
    @echo "Testing REPL with seed configurations..."
    @for config in tests/qa/repl/*.seed; do \
        ./$(DHCPC_SERVER) client localhost \
            "repl" TP --config $$config --repl; \
    done
```

### 2. Polygtopt REPL Protection Framework

```c
/**
 * @file polygtopt_repl_qa.c
 * @brief REPL protection with polygtopt validation
 */

#include "polycall/core/polygtopt.h"

typedef struct {
    char* seed_config;
    uint32_t validation_flags;
    bool enforce_strict_mode;
} polygtopt_qa_config_t;

/**
 * @brief Initialize REPL with polygtopt protections
 */
int polygtopt_repl_init_qa(const char* config_path) {
    polygtopt_qa_config_t qa_config = {
        .seed_config = load_seed_config(config_path),
        .validation_flags = POLYGTOPT_VALIDATE_ALL,
        .enforce_strict_mode = true
    };
    
    // Validate seed state
    if (!polygtopt_validate_seed(&qa_config)) {
        return POLYCALL_ERROR_INVALID_SEED;
    }
    
    // Initialize REPL with protections
    return polycall_repl_init_protected(&qa_config);
}
```

### 3. QA Progression Timeline

```yaml
# qa-milestones.yaml
milestones:
  - id: M1.1
    name: "Test Framework Setup"
    deadline: "2025-01-15T23:59:59Z"
    tasks:
      - "CMake QA integration"
      - "DHCP-C server deployment"
      - "Coverage tooling setup"
    
  - id: M2.1-M2.6
    name: "Core Module Unit QA"
    deadline: "2025-03-31T23:59:59Z"
    components:
      - name: "polycall.c"
        tp_tests: 50
        tn_tests: 30
        fp_threshold: 0.02
        fn_threshold: 0.00
      
      - name: "config"
        tp_tests: 40
        tn_tests: 25
        fp_threshold: 0.02
        fn_threshold: 0.00
      
      - name: "auth"
        tp_tests: 60
        tn_tests: 40
        fp_threshold: 0.01  # Stricter for security
        fn_threshold: 0.00
    
  - id: M3.1-M3.7
    name: "CLI Command Unit QA"
    deadline: "2025-05-31T23:59:59Z"
    commands:
      - "polycall config --validate"
      - "polycall hotwire --enable micro"
      - "polycall telemetry --export"
      - "polycall topo --discover"
      - "polycall micro --isolate"
      - "polycall edge --sync"
      - "polycall repl -c seed.conf"
```

### 4. Language Binding Regression Framework

```python
#!/usr/bin/env python3
"""
regression_tracker.py - Language binding regression tracking
"""

import json
import subprocess
from datetime import datetime

class BindingRegressionTracker:
    def __init__(self):
        self.bindings = {
            "node-polycall@2.0": {
                "version": "2.0.0",
                "tests": ["async", "promise", "typescript"],
                "baseline": None
            },
            "pypolycall": {
                "version": "2.0.0",
                "tests": ["ctypes", "cffi", "mypy"],
                "baseline": None
            },
            "java-polycall": {
                "version": "2.0.0", 
                "tests": ["jni", "thread-safety", "spring"],
                "baseline": None
            }
        }
    
    def run_regression_test(self, binding_name):
        """Execute regression tests for a specific binding"""
        binding = self.bindings[binding_name]
        results = {}
        
        for test in binding["tests"]:
            cmd = f"make test-binding-{binding_name}-{test}"
            result = subprocess.run(cmd, shell=True, capture_output=True)
            
            results[test] = {
                "status": "PASS" if result.returncode == 0 else "FAIL",
                "timestamp": datetime.utcnow().isoformat() + "Z",
                "output": result.stdout.decode()
            }
        
        # Compare with baseline
        if binding["baseline"]:
            regression = self.detect_regression(results, binding["baseline"])
            if regression:
                self.alert_regression(binding_name, regression)
        
        return results
    
    def lock_baseline(self, binding_name):
        """Lock current test results as regression baseline"""
        results = self.run_regression_test(binding_name)
        self.bindings[binding_name]["baseline"] = results
        
        with open(f"baselines/{binding_name}.json", "w") as f:
            json.dump(results, f, indent=2)
        
        print(f"Baseline locked for {binding_name}")
```

### 5. Automated QA Execution Script

```bash
#!/bin/bash
# qa-runner.sh - Automated QA execution with milestone tracking

set -euo pipefail

QA_ROOT="$(dirname "$0")"
TIMESTAMP=$(date -u +%Y-%m-%dT%H:%M:%SZ)

# Start DHCP-C QA server
echo "Starting DHCP-C QA server..."
$QA_ROOT/dhcp-c-polycall-qa server 8067 &
DHCPC_PID=$!
trap "kill $DHCPC_PID 2>/dev/null || true" EXIT

# Wait for server to start
sleep 2

# Run Unit QA
echo "=== Unit QA Phase ==="
make qa-unit-core
make qa-unit-cli

# Run Integration QA  
echo "=== Integration QA Phase ==="
make qa-integration

# Run REPL Seed Tests
echo "=== REPL Configuration Tests ==="
for seed in tests/qa/repl/*.seed; do
    polycall repl -c "$seed" --qa-validate
done

# Generate Metrics Report
echo "=== QA Metrics Report ==="
cat > docs/qa-report-$TIMESTAMP.md <<EOF
# Polycall v2 QA Report
**Generated:** $TIMESTAMP

## Test Summary
$(make qa-metrics-summary)

## TP/TN/FP/FN Analysis
$(make qa-accuracy-report)

## Regression Status
- node-polycall@2.0: $(make check-regression-node)
- pypolycall: $(make check-regression-python)
- java-polycall: $(make check-regression-java)

## Milestone Progress
$(polycall qa --milestone-status)
EOF

echo "QA validation complete. Report: docs/qa-report-$TIMESTAMP.md"
```

### 6. Cost Function Compliance

```c
/**
 * @brief Sinphasé threshold validation for QA metrics
 */
double calculate_sinphase_cost(qa_metrics_t* metrics) {
    double fp_weight = 2.0;  // False positives are more costly
    double fn_weight = 5.0;  // False negatives are critical
    
    double cost = (fp_weight * metrics->fp_count + 
                   fn_weight * metrics->fn_count) / 
                  metrics->total_tests;
    
    return cost;  // Must be < 1.0 for compliance
}
```

## Implementation Checklist

- [x] DHCP-C QA integration script with zero-trust validation
- [ ] Unit QA for all core components (src/core/*)
- [ ] Unit QA for all CLI commands (src/cli/*)  
- [ ] REPL seed configuration testing
- [ ] Polygtopt framework integration
- [ ] TP/TN/FP/FN metric tracking
- [ ] Language binding regression baselines
- [ ] Automated milestone tracking
- [ ] Cost function compliance validation
- [ ] October 15, 2025 freeze preparation

## Usage Examples

```bash
# Run complete QA validation
./qa-runner.sh

# Test specific component with DHCP-C
./dhcp-c-polycall-qa client localhost "test-core-auth" TP

# Test REPL with seed config
./dhcp-c-polycall-qa client localhost "repl" TP \
    --config tests/qa/repl/basic.seed --repl

# Lock regression baseline
python3 regression_tracker.py lock node-polycall@2.0

# Generate milestone report
polycall qa --milestone-status > docs/progress-$(date +%Y%m%d).md
```
