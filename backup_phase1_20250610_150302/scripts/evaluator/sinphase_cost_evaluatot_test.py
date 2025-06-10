#!/usr/bin/env python3
"""
Enhanced Sinphasé Cost Evaluator Test Framework
OBINexus Aegis Project - Comprehensive Testing Suite
"""

import unittest
import tempfile
import json
import os
from pathlib import Path
from typing import Dict, List
from unittest.mock import patch, MagicMock

# Import the enhanced evaluator (assuming it's in the same directory)
from evaluator.enhanced_sinphase_evaluator import (
    EnhancedSinphaseCostEvaluator, 
    ComponentType, 
    IsolationLevel,
    SecurityPolicy,
    SinphaseMetrics,
    ComponentAnalysis
)

class TestSinphaseEvaluatorIntegration(unittest.TestCase):
    """Test suite for LibPolyCall integration validation"""
    
    def setUp(self):
        """Set up test environment with temporary project structure"""
        self.test_dir = tempfile.mkdtemp()
        self.project_root = Path(self.test_dir)
        
        # Create test security policy
        self.security_policy = SecurityPolicy(
            max_ffi_calls=5,
            max_external_deps=3,
            max_memory_allocations=50,
            isolation_enforcement=True,
            audit_required=True
        )
        
        # Initialize evaluator
        self.evaluator = EnhancedSinphaseCostEvaluator(
            self.test_dir,
            threshold=0.6,
            security_policy=self.security_policy
        )
        
    def tearDown(self):
        """Clean up test environment"""
        import shutil
        shutil.rmtree(self.test_dir)
        
    def _create_test_file(self, filename: str, content: str) -> Path:
        """Helper to create test files"""
        file_path = self.project_root / filename
        file_path.parent.mkdir(parents=True, exist_ok=True)
        file_path.write_text(content)
        return file_path
        
    def test_ffi_bridge_detection(self):
        """Test FFI bridge component detection accuracy"""
        ffi_content = '''
        #include "polycall/core/polycall_core.h"
        #include "polycall/core/ffi/polycall_ffi.h"
        
        polycall_core_error_t bridge_init(polycall_core_context_t* ctx) {
            // FFI bridge initialization
            return polycall_ffi_bridge_init(ctx);
        }
        
        extern "C" int native_function_call(void* data) {
            return polycall_ffi_call_native(data);
        }
        '''
        
        file_path = self._create_test_file("ffi_bridge.c", ffi_content)
        analysis = self.evaluator.evaluate_component(file_path)
        
        # Assertions for FFI bridge detection
        self.assertEqual(analysis.component_type, ComponentType.FFI_BRIDGE)
        self.assertGreater(analysis.metrics.ffi_calls, 0)
        self.assertTrue(analysis.isolation_required)
        
    def test_zero_trust_violation_detection(self):
        """Test Zero Trust security violation detection"""
        unsafe_content = '''
        #include <stdlib.h>
        #include <stdio.h>
        
        void unsafe_function() {
            // Zero Trust violations
            system("rm -rf /tmp/*");  // System call violation
            char* raw_ptr = (char*)malloc(1024);  // Raw pointer usage
            eval("dangerous_code");  // Code evaluation
            
            // Trust bypass attempt
            bypass_trust_boundary(raw_ptr);
        }
        
        void* trust_bypass() {
            return (void*)0xDEADBEEF;  // Unsafe cast
        }
        '''
        
        file_path = self._create_test_file("unsafe_component.c", unsafe_content)
        analysis = self.evaluator.evaluate_component(file_path)
        
        # Assertions for Zero Trust violations
        self.assertGreater(analysis.metrics.zero_trust_violations, 0)
        self.assertIn("Zero Trust violations detected", 
                     [v for v in analysis.violation_details if "Zero Trust" in v])
        self.assertEqual(analysis.isolation_level, IsolationLevel.VM)
        
    def test_polycall_config_integration(self):
        """Test LibPolyCall configuration file integration"""
        polycallrc_content = '''
        memory_limit: 1024
        allowed_protocol: https
        allowed_protocol: wss
        isolation_enforcement: true
        audit_required: true
        '''
        
        self._create_test_file(".polycallrc", polycallrc_content)
        
        # Reinitialize evaluator to trigger config loading
        evaluator = EnhancedSinphaseCostEvaluator(
            self.test_dir,
            threshold=0.6,
            security_policy=self.security_policy
        )
        
        # Verify configuration integration
        self.assertTrue(evaluator.has_polycall_config)
        self.assertEqual(evaluator.security_policy.max_memory_allocations, 1024)
        
    def test_component_type_detection_accuracy(self):
        """Test component type detection for various file types"""
        test_cases = [
            ("react_component.jsx", "import React from 'react';", ComponentType.REACT),
            ("vue_component.vue", "<template><div>Vue</div></template>", ComponentType.VUE),
            ("node_module.js", "const express = require('express');", ComponentType.NODE),
            ("python_module.py", "import ctypes", ComponentType.PYTHON),
            ("native_core.c", "#include <stdio.h>", ComponentType.C_NATIVE),
            ("wasm_module.wasm", "binary_content", ComponentType.WASM)
        ]
        
        for filename, content, expected_type in test_cases:
            with self.subTest(filename=filename):
                file_path = self._create_test_file(filename, content)
                detected_type = self.evaluator._detect_component_type(file_path)
                self.assertEqual(detected_type, expected_type)
                
    def test_metrics_calculation_accuracy(self):
        """Test enhanced metrics calculation accuracy"""
        complex_content = '''
        #include "polycall/core/polycall_core.h"
        #include "polycall/protocol/polycall_protocol.h"
        #include <stdlib.h>
        #include <memory.h>
        
        extern int external_function(void);
        
        polycall_core_error_t complex_function(polycall_core_context_t* ctx) {
            // Memory allocations
            void* mem1 = malloc(1024);
            void* mem2 = calloc(512, sizeof(int));
            
            // FFI calls
            polycall_ffi_bridge_init(ctx);
            polycall_ffi_call_native(mem1);
            
            // State transitions
            polycall_protocol_state_transition(ctx, NEW_STATE);
            
            // Complexity patterns
            for (int i = 0; i < 100; i++) {
                if (i % 2 == 0) {
                    while (i < 50) {
                        external_function();
                        break;
                    }
                }
            }
            
            free(mem1);
            free(mem2);
            return POLYCALL_CORE_SUCCESS;
        }
        '''
        
        file_path = self._create_test_file("complex_component.c", complex_content)
        metrics = self.evaluator._analyze_enhanced_metrics(file_path, complex_content)
        
        # Verify metrics accuracy
        self.assertGreaterEqual(metrics.include_depth, 3)  # At least 3 includes
        self.assertGreaterEqual(metrics.ffi_calls, 2)      # At least 2 FFI calls
        self.assertGreaterEqual(metrics.memory_allocations, 4)  # malloc, calloc, free calls
        self.assertGreaterEqual(metrics.protocol_state_changes, 1)  # State transition
        self.assertGreater(metrics.complexity, 0)          # Complexity from control structures
        
    def test_cost_calculation_with_type_multipliers(self):
        """Test cost calculation with component type multipliers"""
        base_metrics = SinphaseMetrics(
            include_depth=5,
            function_calls=10,
            external_deps=2,
            ffi_calls=3,
            complexity=0.5,
            link_deps=1,
            circular_penalty=0.0,
            temporal_pressure=0.0,
            memory_allocations=20,
            security_boundaries=1,
            zero_trust_violations=0,
            protocol_state_changes=2
        )
        
        # Test different component types
        ffi_cost = self.evaluator._calculate_enhanced_cost(base_metrics, ComponentType.FFI_BRIDGE)
        native_cost = self.evaluator._calculate_enhanced_cost(base_metrics, ComponentType.C_NATIVE)
        react_cost = self.evaluator._calculate_enhanced_cost(base_metrics, ComponentType.REACT)
        
        # FFI bridge should have highest cost due to 1.5x multiplier
        self.assertGreater(ffi_cost, native_cost)
        self.assertGreater(native_cost, react_cost)
        
    def test_security_violation_analysis(self):
        """Test security policy violation analysis"""
        high_ffi_metrics = SinphaseMetrics(
            include_depth=2,
            function_calls=5,
            external_deps=1,
            ffi_calls=15,  # Exceeds policy limit of 5
            complexity=0.3,
            link_deps=0,
            circular_penalty=0.0,
            temporal_pressure=0.0,
            memory_allocations=10,
            security_boundaries=0,
            zero_trust_violations=2,  # Security violations
            protocol_state_changes=1
        )
        
        violations = self.evaluator._analyze_security_violations(
            high_ffi_metrics, ComponentType.FFI_BRIDGE
        )
        
        # Should detect FFI call limit and Zero Trust violations
        self.assertTrue(any("FFI call limit exceeded" in v for v in violations))
        self.assertTrue(any("Zero Trust violations detected" in v for v in violations))
        
    def test_emergency_mode_threshold_adjustment(self):
        """Test emergency mode threshold adjustment"""
        original_threshold = self.evaluator.threshold
        
        # Simulate project evaluation in emergency mode
        with patch.object(self.evaluator, '_read_file_safe', return_value=""):
            with patch.object(Path, 'rglob', return_value=[]):
                self.evaluator.evaluate_project(emergency_mode=True)
                
        # Threshold should be reduced by 20%
        expected_threshold = min(original_threshold * 0.8, 0.4)
        self.assertEqual(self.evaluator.threshold, expected_threshold)
        
    def test_telemetry_data_generation(self):
        """Test GUID-based telemetry data generation"""
        test_content = "int main() { return 0; }"
        file_path = self._create_test_file("test.c", test_content)
        
        analysis = self.evaluator.evaluate_component(file_path)
        
        # Verify GUID generation
        self.assertIsNotNone(analysis.guid)
        self.assertEqual(len(analysis.guid), 16)  # SHA-256 truncated to 16 chars
        
        # Test telemetry data structure
        components = [analysis]
        with patch('builtins.open', create=True) as mock_open:
            self.evaluator._write_telemetry_data(components)
            mock_open.assert_called_once()
            
    def test_isolation_level_determination(self):
        """Test isolation level determination logic"""
        test_cases = [
            (ComponentType.FFI_BRIDGE, 0, IsolationLevel.CONTAINER),
            (ComponentType.C_NATIVE, 0, IsolationLevel.NONE),
            (ComponentType.REACT, 3, IsolationLevel.CONTAINER),
            (ComponentType.PYTHON, 6, IsolationLevel.VM),
            (ComponentType.NODE, 1, IsolationLevel.SANDBOX)
        ]
        
        for component_type, violations, expected_level in test_cases:
            with self.subTest(component_type=component_type, violations=violations):
                level = self.evaluator._determine_isolation_level(component_type, violations)
                self.assertEqual(level, expected_level)
                
    def test_recommendation_generation(self):
        """Test actionable recommendation generation"""
        # Create mock components with various violation patterns
        components = [
            # FFI bridge with violations
            ComponentAnalysis(
                component_path="ffi_bridge.c",
                component_type=ComponentType.FFI_BRIDGE,
                isolation_level=IsolationLevel.CONTAINER,
                cost=0.8,
                metrics=SinphaseMetrics(0,0,0,0,0,0,0,0,0,0,2,0),  # Zero Trust violations
                phase_state="ANALYZED",
                violation_details=["FFI violations"],
                isolation_required=True,
                security_policy_violations=["FFI limit exceeded"],
                guid="test123"
            ),
            # High cost component
            ComponentAnalysis(
                component_path="complex.c",
                component_type=ComponentType.C_NATIVE,
                isolation_level=IsolationLevel.VM,
                cost=1.2,  # High cost
                metrics=SinphaseMetrics(0,0,0,0,0,0,0,0,0,0,0,0),
                phase_state="ANALYZED",
                violation_details=["High cost"],
                isolation_required=True,
                security_policy_violations=[],
                guid="test456"
            )
        ]
        
        recommendations = self.evaluator._generate_recommendations(components)
        
        # Verify appropriate recommendations are generated
        self.assertTrue(any("FFI Bridge components" in r for r in recommendations))
        self.assertTrue(any("Zero Trust violations" in r for r in recommendations))
        self.assertTrue(any("High-cost components" in r for r in recommendations))
        self.assertTrue(any("VM-level isolation" in r for r in recommendations))

class TestSinphasePerformanceBenchmarks(unittest.TestCase):
    """Performance benchmark test suite"""
    
    def setUp(self):
        self.test_dir = tempfile.mkdtemp()
        self.evaluator = EnhancedSinphaseCostEvaluator(self.test_dir, 0.6)
        
    def tearDown(self):
        import shutil
        shutil.rmtree(self.test_dir)
        
    def test_large_file_analysis_performance(self):
        """Test performance with large source files"""
        import time
        
        # Generate large file content
        large_content = """
        #include <stdio.h>
        
        int main() {
        """ + "\n".join([f"    printf(\"Line {i}\\n\");" for i in range(10000)]) + """
            return 0;
        }
        """
        
        file_path = Path(self.test_dir) / "large_file.c"
        file_path.write_text(large_content)
        
        start_time = time.time()
        analysis = self.evaluator.evaluate_component(file_path)
        end_time = time.time()
        
        # Analysis should complete within reasonable time
        self.assertLess(end_time - start_time, 1.0)  # Less than 1 second
        self.assertIsNotNone(analysis)
        
    def test_project_evaluation_scalability(self):
        """Test project evaluation with multiple files"""
        import time
        
        # Create multiple test files
        for i in range(100):
            file_path = Path(self.test_dir) / f"file_{i}.c"
            file_path.write_text(f"int function_{i}() {{ return {i}; }}")
            
        start_time = time.time()
        result = self.evaluator.evaluate_project()
        end_time = time.time()
        
        # Project evaluation should complete efficiently
        self.assertLess(end_time - start_time, 5.0)  # Less than 5 seconds
        self.assertTrue(result)

if __name__ == '__main__':
    # Configure test runner
    loader = unittest.TestLoader()
    suite = unittest.TestSuite()
    
    # Add test suites
    suite.addTests(loader.loadTestsFromTestCase(TestSinphaseEvaluatorIntegration))
    suite.addTests(loader.loadTestsFromTestCase(TestSinphasePerformanceBenchmarks))
    
    # Run tests with verbose output
    runner = unittest.TextTestRunner(verbosity=2, buffer=True)
    result = runner.run(suite)
    
    # Exit with appropriate code
    exit(0 if result.wasSuccessful() else 1)