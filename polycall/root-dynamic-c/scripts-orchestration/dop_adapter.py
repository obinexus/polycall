"""
DOP Adapter Python Bridge Implementation

LibPolyCall DOP Adapter Framework - Python Language Bridge
OBINexus Computing - Aegis Project Technical Infrastructure

Provides Python binding to the native DOP Adapter C library.
Enables secure cross-language component integration with Zero Trust enforcement.
Essential for data science and ML components in banking applications.

@version 1.0.0
@date 2025-06-09
"""

import ctypes
import ctypes.util
import os
import sys
import json
import time
import threading
from typing import Any, Dict, List, Optional, Callable, Union
from enum import IntEnum
from dataclasses import dataclass, field
from pathlib import Path


class DOPError(Exception):
    """Base exception for DOP Adapter errors."""
    
    def __init__(self, message: str, code: str = "DOP_ERROR_UNKNOWN"):
        super().__init__(message)
        self.code = code


class DOPSecurityError(DOPError):
    """Security violation error."""
    
    def __init__(self, message: str):
        super().__init__(message, "DOP_ERROR_SECURITY_VIOLATION")


class DOPPermissionError(DOPError):
    """Permission denied error."""
    
    def __init__(self, message: str):
        super().__init__(message, "DOP_ERROR_PERMISSION_DENIED")


class DOPIsolationError(DOPError):
    """Isolation breach error."""
    
    def __init__(self, message: str):
        super().__init__(message, "DOP_ERROR_ISOLATION_BREACH")


class DOPValueType(IntEnum):
    """DOP value type constants."""
    NULL = 0
    BOOL = 1
    INT32 = 2
    INT64 = 3
    UINT32 = 4
    UINT64 = 5
    FLOAT32 = 6
    FLOAT64 = 7
    STRING = 8
    BYTES = 9
    ARRAY = 10
    OBJECT = 11
    FUNCTION = 12
    COMPONENT_REF = 13


class DOPComponentState(IntEnum):
    """DOP component state constants."""
    UNINITIALIZED = 0
    INITIALIZING = 1
    READY = 2
    EXECUTING = 3
    SUSPENDED = 4
    ERROR = 5
    CLEANUP = 6
    DESTROYED = 7


class DOPIsolationLevel(IntEnum):
    """DOP isolation level constants."""
    NONE = 0
    BASIC = 1
    STANDARD = 2
    STRICT = 3
    PARANOID = 4


class DOPPermissionFlags(IntEnum):
    """DOP permission flags."""
    NONE = 0x00
    MEMORY_READ = 0x01
    MEMORY_WRITE = 0x02
    INVOKE_LOCAL = 0x04
    INVOKE_REMOTE = 0x08
    FILE_ACCESS = 0x10
    NETWORK = 0x20
    PRIVILEGED = 0x40
    ALL = 0xFF


class DOPLanguage(IntEnum):
    """DOP language constants."""
    C = 0
    JAVASCRIPT = 1
    PYTHON = 2
    JVM = 3
    WASM = 4
    UNKNOWN = 255


@dataclass
class DOPSecurityPolicy:
    """DOP security policy configuration."""
    isolation_level: DOPIsolationLevel = DOPIsolationLevel.STANDARD
    allowed_permissions: int = DOPPermissionFlags.MEMORY_READ | DOPPermissionFlags.MEMORY_WRITE | DOPPermissionFlags.INVOKE_LOCAL
    denied_permissions: int = DOPPermissionFlags.NETWORK | DOPPermissionFlags.PRIVILEGED
    max_memory_usage: int = 1024 * 1024  # 1MB
    max_execution_time_ms: int = 5000  # 5 seconds
    audit_enabled: bool = True
    stack_protection_enabled: bool = True
    heap_protection_enabled: bool = True


@dataclass
class DOPMethodSignature:
    """DOP method signature definition."""
    method_name: str
    parameter_types: List[DOPValueType]
    return_type: DOPValueType
    required_permissions: int = DOPPermissionFlags.NONE
    max_execution_time_ms: int = 5000


@dataclass
class DOPComponentConfig:
    """DOP component configuration."""
    component_id: str
    component_name: str
    version: str = "1.0.0"
    language: DOPLanguage = DOPLanguage.PYTHON
    security_policy: DOPSecurityPolicy = field(default_factory=DOPSecurityPolicy)
    methods: List[DOPMethodSignature] = field(default_factory=list)
    language_specific_config: Optional[Dict[str, Any]] = None


@dataclass
class DOPComponentStats:
    """DOP component statistics."""
    invocation_count: int = 0
    total_execution_time_ns: int = 0
    average_execution_time_ns: int = 0
    total_memory_allocated: int = 0
    current_memory_usage: int = 0
    security_violations: int = 0
    current_state: DOPComponentState = DOPComponentState.UNINITIALIZED


class DOPValue:
    """DOP value wrapper for cross-language data exchange."""
    
    def __init__(self, value: Any, value_type: Optional[DOPValueType] = None):
        self.value = value
        self.type = value_type or self._infer_type(value)
    
    @staticmethod
    def _infer_type(value: Any) -> DOPValueType:
        """Infer DOP value type from Python value."""
        if value is None:
            return DOPValueType.NULL
        elif isinstance(value, bool):
            return DOPValueType.BOOL
        elif isinstance(value, int):
            if -2**31 <= value < 2**31:
                return DOPValueType.INT32
            else:
                return DOPValueType.INT64
        elif isinstance(value, float):
            return DOPValueType.FLOAT64
        elif isinstance(value, str):
            return DOPValueType.STRING
        elif isinstance(value, (bytes, bytearray)):
            return DOPValueType.BYTES
        elif isinstance(value, (list, tuple)):
            return DOPValueType.ARRAY
        elif isinstance(value, dict):
            return DOPValueType.OBJECT
        elif callable(value):
            return DOPValueType.FUNCTION
        else:
            return DOPValueType.OBJECT
    
    def to_dict(self) -> Dict[str, Any]:
        """Convert to dictionary representation."""
        return {
            "type": self.type.value,
            "value": self._serialize_value()
        }
    
    def _serialize_value(self) -> Any:
        """Serialize value for transmission."""
        if self.type == DOPValueType.NULL:
            return None
        elif self.type in (DOPValueType.BOOL, DOPValueType.INT32, DOPValueType.INT64, 
                          DOPValueType.UINT32, DOPValueType.UINT64, DOPValueType.FLOAT32, 
                          DOPValueType.FLOAT64, DOPValueType.STRING):
            return self.value
        elif self.type == DOPValueType.BYTES:
            return list(self.value) if isinstance(self.value, (bytes, bytearray)) else self.value
        elif self.type == DOPValueType.ARRAY:
            return [DOPValue(item).to_dict() for item in self.value]
        elif self.type == DOPValueType.OBJECT:
            if isinstance(self.value, dict):
                return {k: DOPValue(v).to_dict() for k, v in self.value.items()}
            else:
                return str(self.value)
        elif self.type == DOPValueType.FUNCTION:
            return str(self.value)
        else:
            return str(self.value)


class DOPNativeLibrary:
    """Wrapper for native DOP Adapter library."""
    
    def __init__(self):
        self._lib = None
        self._load_library()
        self._setup_function_signatures()
    
    def _load_library(self):
        """Load the native DOP Adapter library."""
        # Try different possible library paths
        lib_paths = [
            "./build/lib/libpolycall_dop_adapter.so",
            "./build/lib/libpolycall_dop_adapter.dylib",  # macOS
            "./lib/libpolycall_dop_adapter.so",
            f"{os.environ.get('POLYCALL_LIB_PATH', '')}/libpolycall_dop_adapter.so"
        ]
        
        for lib_path in lib_paths:
            if os.path.exists(lib_path):
                try:
                    self._lib = ctypes.CDLL(lib_path)
                    break
                except OSError as e:
                    continue
        
        if self._lib is None:
            # Try system library path
            lib_name = ctypes.util.find_library("polycall_dop_adapter")
            if lib_name:
                self._lib = ctypes.CDLL(lib_name)
            else:
                raise DOPError("Could not load native DOP Adapter library")
    
    def _setup_function_signatures(self):
        """Setup function signatures for the native library."""
        # Initialize function
        self._lib.polycall_dop_adapter_initialize.argtypes = [
            ctypes.c_void_p,  # core_ctx
            ctypes.POINTER(ctypes.c_void_p),  # adapter_ctx
            ctypes.c_void_p   # security_policy
        ]
        self._lib.polycall_dop_adapter_initialize.restype = ctypes.c_int
        
        # Cleanup function
        self._lib.polycall_dop_adapter_cleanup.argtypes = [ctypes.c_void_p]
        self._lib.polycall_dop_adapter_cleanup.restype = ctypes.c_int
        
        # Component registration
        self._lib.polycall_dop_component_register.argtypes = [
            ctypes.c_void_p,  # adapter_ctx
            ctypes.c_void_p,  # config
            ctypes.POINTER(ctypes.c_void_p)  # component
        ]
        self._lib.polycall_dop_component_register.restype = ctypes.c_int
        
        # Component invocation
        self._lib.polycall_dop_invoke.argtypes = [
            ctypes.c_void_p,  # adapter_ctx
            ctypes.c_char_p,  # component_id
            ctypes.c_char_p,  # method_name
            ctypes.c_void_p,  # parameters
            ctypes.c_size_t,  # parameter_count
            ctypes.c_void_p   # result
        ]
        self._lib.polycall_dop_invoke.restype = ctypes.c_int


class DOPComponent:
    """Python wrapper for DOP components."""
    
    def __init__(self, adapter: 'DOPAdapter', native_component: ctypes.c_void_p, 
                 config: DOPComponentConfig, instance: Any):
        self._adapter = adapter
        self._native_component = native_component
        self._config = config
        self._instance = instance
        self._state = DOPComponentState.READY
        self._lock = threading.RLock()
    
    @property
    def component_id(self) -> str:
        """Get component ID."""
        return self._config.component_id
    
    @property
    def component_name(self) -> str:
        """Get component name."""
        return self._config.component_name
    
    @property
    def version(self) -> str:
        """Get component version."""
        return self._config.version
    
    @property
    def state(self) -> DOPComponentState:
        """Get component state."""
        return self._state
    
    async def invoke(self, method_name: str, parameters: List[Any] = None, 
                    options: Dict[str, Any] = None) -> Any:
        """Invoke a method on this component."""
        if parameters is None:
            parameters = []
        if options is None:
            options = {}
        
        with self._lock:
            if self._state != DOPComponentState.READY:
                raise DOPError(f"Component '{self.component_id}' is not ready for invocation")
            
            # Validate method exists
            if not hasattr(self._instance, method_name) or not callable(getattr(self._instance, method_name)):
                raise DOPError(f"Method '{method_name}' not found on component '{self.component_id}'")
            
            try:
                self._state = DOPComponentState.EXECUTING
                
                # Record execution start time
                start_time = time.time_ns()
                
                # Call the actual Python method
                method = getattr(self._instance, method_name)
                result = method(*parameters)
                
                # Handle async methods
                if hasattr(result, '__await__'):
                    result = await result
                
                end_time = time.time_ns()
                execution_time_ms = (end_time - start_time) / 1_000_000
                
                self._state = DOPComponentState.READY
                
                # Log invocation through adapter
                self._adapter._log_invocation(
                    self.component_id, method_name, execution_time_ms, "success"
                )
                
                return result
                
            except Exception as e:
                self._state = DOPComponentState.ERROR
                
                # Log error through adapter
                self._adapter._log_invocation(
                    self.component_id, method_name, 0, "error", str(e)
                )
                
                raise DOPError(f"Component invocation failed: {str(e)}")
    
    async def suspend(self):
        """Suspend component execution."""
        with self._lock:
            if self._state != DOPComponentState.READY:
                raise DOPError(f"Cannot suspend component in state: {self._state}")
            
            self._state = DOPComponentState.SUSPENDED
            self._adapter._emit_event('component_suspended', {'component_id': self.component_id})
    
    async def resume(self):
        """Resume component execution."""
        with self._lock:
            if self._state != DOPComponentState.SUSPENDED:
                raise DOPError(f"Cannot resume component in state: {self._state}")
            
            self._state = DOPComponentState.READY
            self._adapter._emit_event('component_resumed', {'component_id': self.component_id})
    
    async def get_statistics(self) -> DOPComponentStats:
        """Get component statistics."""
        # This would call into the native library to get actual statistics
        # For now, return a placeholder
        return DOPComponentStats(
            current_state=self._state
        )
    
    async def _cleanup(self):
        """Cleanup component resources."""
        if self._state == DOPComponentState.DESTROYED:
            return
        
        with self._lock:
            self._state = DOPComponentState.CLEANUP
            
            try:
                # Call cleanup method on instance if it exists
                if hasattr(self._instance, 'cleanup') and callable(self._instance.cleanup):
                    cleanup_method = getattr(self._instance, 'cleanup')
                    if hasattr(cleanup_method(), '__await__'):
                        await cleanup_method()
                    else:
                        cleanup_method()
                
                self._state = DOPComponentState.DESTROYED
                
            except Exception as e:
                self._state = DOPComponentState.ERROR
                raise DOPError(f"Component cleanup failed: {str(e)}")


class DOPAdapter:
    """Main DOP Adapter Python bridge class."""
    
    def __init__(self):
        self._native_lib = None
        self._native_context = None
        self._components: Dict[str, DOPComponent] = {}
        self._initialized = False
        self._event_listeners: Dict[str, List[Callable]] = {}
        self._lock = threading.RLock()
    
    async def initialize(self, config: Dict[str, Any] = None) -> None:
        """Initialize the DOP Adapter with security policy."""
        if self._initialized:
            raise DOPError("DOP Adapter already initialized")
        
        if config is None:
            config = {}
        
        # Load native library
        self._native_lib = DOPNativeLibrary()
        
        # Create default security policy
        default_security_policy = DOPSecurityPolicy()
        if 'security_policy' in config:
            # Merge with provided security policy
            policy_config = config['security_policy']
            for key, value in policy_config.items():
                if hasattr(default_security_policy, key):
                    setattr(default_security_policy, key, value)
        
        # Initialize native adapter context
        # This would involve calling the native library
        # For now, simulate initialization
        self._native_context = ctypes.c_void_p(0x12345678)  # Placeholder
        self._initialized = True
        
        self._emit_event('initialized', {'security_policy': default_security_policy})
    
    async def register_component(self, component_config: DOPComponentConfig, 
                                component_instance: Any) -> DOPComponent:
        """Register a Python component."""
        if not self._initialized:
            raise DOPError("DOP Adapter not initialized")
        
        # Validate component configuration
        self._validate_component_config(component_config)
        
        # Ensure component instance is valid
        if component_instance is None:
            raise DOPError("Component instance cannot be None")
        
        with self._lock:
            if component_config.component_id in self._components:
                raise DOPError(f"Component '{component_config.component_id}' already registered")
            
            # Create enhanced config with Python-specific settings
            enhanced_config = DOPComponentConfig(
                component_id=component_config.component_id,
                component_name=component_config.component_name,
                version=component_config.version,
                language=DOPLanguage.PYTHON,
                security_policy=component_config.security_policy,
                methods=component_config.methods,
                language_specific_config={
                    'runtime': 'cpython',
                    'version': sys.version,
                    'interpreter_path': sys.executable,
                    **(component_config.language_specific_config or {})
                }
            )
            
            # Register component with native adapter
            # This would involve calling the native library
            native_component = ctypes.c_void_p(0x87654321)  # Placeholder
            
            # Create Python wrapper component
            component = DOPComponent(self, native_component, enhanced_config, component_instance)
            
            # Store component reference
            self._components[component_config.component_id] = component
            
            self._emit_event('component_registered', {
                'component_id': component_config.component_id,
                'component_name': component_config.component_name
            })
            
            return component
    
    async def unregister_component(self, component_id: str) -> None:
        """Unregister a component."""
        if component_id not in self._components:
            raise DOPError(f"Component '{component_id}' not found")
        
        component = self._components[component_id]
        
        try:
            await component._cleanup()
            
            with self._lock:
                del self._components[component_id]
            
            self._emit_event('component_unregistered', {'component_id': component_id})
            
        except Exception as e:
            raise DOPError(f"Failed to unregister component: {str(e)}")
    
    async def invoke(self, component_id: str, method_name: str, 
                    parameters: List[Any] = None, options: Dict[str, Any] = None) -> Any:
        """Invoke a method on a component."""
        if component_id not in self._components:
            raise DOPError(f"Component '{component_id}' not found")
        
        component = self._components[component_id]
        return await component.invoke(method_name, parameters, options)
    
    def get_component(self, component_id: str) -> Optional[DOPComponent]:
        """Get component by ID."""
        return self._components.get(component_id)
    
    def list_components(self) -> List[str]:
        """List all registered components."""
        return list(self._components.keys())
    
    async def get_statistics(self) -> Dict[str, Any]:
        """Get adapter statistics."""
        if not self._initialized:
            raise DOPError("DOP Adapter not initialized")
        
        import psutil
        process = psutil.Process()
        memory_info = process.memory_info()
        
        return {
            'component_count': len(self._components),
            'python_memory_rss': memory_info.rss,
            'python_memory_vms': memory_info.vms,
            'python_version': sys.version,
            'python_executable': sys.executable
        }
    
    async def cleanup(self) -> None:
        """Cleanup and destroy the DOP Adapter."""
        if not self._initialized:
            return
        
        try:
            # Cleanup all components
            component_ids = list(self._components.keys())
            for component_id in component_ids:
                try:
                    await self.unregister_component(component_id)
                except Exception as e:
                    print(f"Failed to cleanup component '{component_id}': {str(e)}")
            
            # Cleanup native context
            if self._native_context:
                # This would call the native cleanup function
                self._native_context = None
            
            self._initialized = False
            self._emit_event('cleanup')
            
        except Exception as e:
            raise DOPError(f"Failed to cleanup DOP Adapter: {str(e)}")
    
    def on(self, event: str, listener: Callable) -> None:
        """Add event listener."""
        if event not in self._event_listeners:
            self._event_listeners[event] = []
        self._event_listeners[event].append(listener)
    
    def off(self, event: str, listener: Callable) -> None:
        """Remove event listener."""
        if event in self._event_listeners:
            try:
                self._event_listeners[event].remove(listener)
            except ValueError:
                pass
    
    # Private methods
    
    def _validate_component_config(self, config: DOPComponentConfig) -> None:
        """Validate component configuration."""
        if not config.component_id:
            raise DOPError("Component ID must be provided")
        
        if not config.component_name:
            raise DOPError("Component name must be provided")
        
        if not config.version:
            raise DOPError("Component version must be provided")
        
        # Validate component ID format
        if not config.component_id.replace('_', '').replace('-', '').isalnum():
            raise DOPError("Component ID must contain only alphanumeric characters, underscores, and hyphens")
        
        # Validate methods if provided
        for method in config.methods:
            if not method.method_name:
                raise DOPError("Method name must be provided")
            
            if not method.method_name.isidentifier():
                raise DOPError(f"Method name '{method.method_name}' is not a valid Python identifier")
    
    def _emit_event(self, event: str, data: Any = None) -> None:
        """Emit an event to all listeners."""
        if event in self._event_listeners:
            for listener in self._event_listeners[event]:
                try:
                    listener(data)
                except Exception as e:
                    print(f"Error in event listener for '{event}': {str(e)}")
    
    def _log_invocation(self, component_id: str, method_name: str, 
                       execution_time_ms: float, status: str, error_message: str = None) -> None:
        """Log component invocation."""
        # This would call into the native library for audit logging
        log_data = {
            'component_id': component_id,
            'method_name': method_name,
            'execution_time_ms': execution_time_ms,
            'status': status,
            'timestamp': time.time_ns()
        }
        
        if error_message:
            log_data['error_message'] = error_message
        
        self._emit_event('invocation_logged', log_data)


# Convenience functions for common operations

def create_default_security_policy(isolation_level: DOPIsolationLevel = DOPIsolationLevel.STANDARD) -> DOPSecurityPolicy:
    """Create a default security policy with specified isolation level."""
    policy = DOPSecurityPolicy()
    policy.isolation_level = isolation_level
    
    # Adjust permissions based on isolation level
    if isolation_level == DOPIsolationLevel.NONE:
        policy.allowed_permissions = DOPPermissionFlags.ALL
        policy.max_memory_usage = 128 * 1024 * 1024  # 128MB
    elif isolation_level == DOPIsolationLevel.BASIC:
        policy.allowed_permissions = (DOPPermissionFlags.MEMORY_READ | 
                                    DOPPermissionFlags.MEMORY_WRITE | 
                                    DOPPermissionFlags.INVOKE_LOCAL)
        policy.max_memory_usage = 64 * 1024 * 1024  # 64MB
    elif isolation_level == DOPIsolationLevel.STANDARD:
        policy.allowed_permissions = (DOPPermissionFlags.MEMORY_READ | 
                                    DOPPermissionFlags.INVOKE_LOCAL)
        policy.max_memory_usage = 32 * 1024 * 1024  # 32MB
    elif isolation_level == DOPIsolationLevel.STRICT:
        policy.allowed_permissions = DOPPermissionFlags.MEMORY_READ
        policy.max_memory_usage = 16 * 1024 * 1024  # 16MB
    elif isolation_level == DOPIsolationLevel.PARANOID:
        policy.allowed_permissions = DOPPermissionFlags.NONE
        policy.max_memory_usage = 8 * 1024 * 1024  # 8MB
    
    policy.denied_permissions = ~policy.allowed_permissions & DOPPermissionFlags.ALL
    
    return policy


def create_method_signature(method_name: str, parameter_types: List[DOPValueType] = None, 
                          return_type: DOPValueType = DOPValueType.NULL,
                          required_permissions: int = DOPPermissionFlags.NONE) -> DOPMethodSignature:
    """Create a method signature."""
    if parameter_types is None:
        parameter_types = []
    
    return DOPMethodSignature(
        method_name=method_name,
        parameter_types=parameter_types,
        return_type=return_type,
        required_permissions=required_permissions
    )


# Example usage and testing
if __name__ == "__main__":
    import asyncio
    
    # Example component implementation
    class BankingCalculator:
        """Example banking calculator component."""
        
        def calculate_interest(self, principal: float, rate: float, time: float) -> float:
            """Calculate simple interest."""
            return principal * rate * time / 100
        
        def calculate_compound_interest(self, principal: float, rate: float, 
                                     time: float, frequency: int = 1) -> float:
            """Calculate compound interest."""
            return principal * ((1 + rate / (100 * frequency)) ** (frequency * time))
        
        def validate_account_number(self, account_number: str) -> bool:
            """Validate account number format."""
            return len(account_number) >= 10 and account_number.isdigit()
        
        async def cleanup(self):
            """Cleanup method called during component destruction."""
            print("BankingCalculator cleanup completed")
    
    async def main():
        """Example usage of DOP Adapter Python bridge."""
        
        # Initialize adapter
        adapter = DOPAdapter()
        await adapter.initialize({
            'security_policy': {
                'isolation_level': DOPIsolationLevel.STANDARD,
                'max_memory_usage': 32 * 1024 * 1024,
                'max_execution_time_ms': 10000
            }
        })
        
        # Create component configuration
        config = DOPComponentConfig(
            component_id="banking_calculator_001",
            component_name="Banking Calculator",
            version="1.0.0",
            security_policy=create_default_security_policy(DOPIsolationLevel.STANDARD),
            methods=[
                create_method_signature("calculate_interest", 
                                       [DOPValueType.FLOAT64, DOPValueType.FLOAT64, DOPValueType.FLOAT64],
                                       DOPValueType.FLOAT64),
                create_method_signature("calculate_compound_interest",
                                       [DOPValueType.FLOAT64, DOPValueType.FLOAT64, DOPValueType.FLOAT64, DOPValueType.INT32],
                                       DOPValueType.FLOAT64),
                create_method_signature("validate_account_number",
                                       [DOPValueType.STRING],
                                       DOPValueType.BOOL)
            ]
        )
        
        # Register component
        calculator_instance = BankingCalculator()
        component = await adapter.register_component(config, calculator_instance)
        
        print(f"Registered component: {component.component_id}")
        
        # Test component invocation
        try:
            interest = await adapter.invoke("banking_calculator_001", "calculate_interest", [1000.0, 5.0, 2.0])
            print(f"Simple interest: {interest}")
            
            compound_interest = await adapter.invoke("banking_calculator_001", "calculate_compound_interest", 
                                                   [1000.0, 5.0, 2.0, 12])
            print(f"Compound interest: {compound_interest}")
            
            is_valid = await adapter.invoke("banking_calculator_001", "validate_account_number", ["1234567890"])
            print(f"Account number valid: {is_valid}")
            
        except Exception as e:
            print(f"Invocation error: {e}")
        
        # Get statistics
        stats = await adapter.get_statistics()
        print(f"Adapter statistics: {stats}")
        
        # Cleanup
        await adapter.cleanup()
        print("Adapter cleanup completed")
    
    # Run example
    asyncio.run(main())
