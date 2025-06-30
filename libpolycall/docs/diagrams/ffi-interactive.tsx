import React, { useState } from 'react';

const FFIArchitectureExplorer = () => {
  const [selectedComponent, setSelectedComponent] = useState(null);
  const [showDescription, setShowDescription] = useState(true);

  const components = {
    ffiCore: {
      name: "FFI Core",
      description: "The central coordination layer managing cross-language function calls and type conversions",
      functions: [
        "polycall_ffi_init() - Initialize FFI module",
        "polycall_ffi_cleanup() - Clean up FFI module",
        "polycall_ffi_register_language() - Register a language bridge",
        "polycall_ffi_expose_function() - Expose a function to other languages",
        "polycall_ffi_call_function() - Call a function in another language"
      ],
      interfaces: ["Core Context", "Language Bridges", "Memory Manager", "Type System", "Security Context"]
    },
    typeSystem: {
      name: "Type System",
      description: "Comprehensive type mapping system for safe conversion between language-specific types",
      functions: [
        "polycall_type_register() - Register a type",
        "polycall_type_register_conversion() - Register a type conversion rule",
        "polycall_type_convert() - Convert a value between types",
        "polycall_type_create_struct() - Create a struct type",
        "polycall_type_create_array() - Create an array type"
      ],
      interfaces: ["FFI Core", "Language Bridges"]
    },
    memoryBridge: {
      name: "Memory Bridge",
      description: "Memory management system for secure sharing across language boundaries",
      functions: [
        "polycall_memory_share() - Share memory across languages",
        "polycall_memory_track_reference() - Track memory references",
        "polycall_memory_release() - Release shared memory",
        "polycall_memory_notify_gc() - Notify of garbage collection",
        "polycall_memory_create_snapshot() - Create memory state snapshot"
      ],
      interfaces: ["FFI Core", "Language Bridges", "Security Context"]
    },
    security: {
      name: "Security Layer",
      description: "Zero-trust security model for cross-language function calls",
      functions: [
        "polycall_security_verify_access() - Verify function access",
        "polycall_security_register_function() - Register function with security attributes",
        "polycall_security_audit_event() - Audit security-relevant events",
        "polycall_security_enforce_isolation() - Enforce isolation between components",
        "polycall_security_load_policy() - Load security policy from file"
      ],
      interfaces: ["FFI Core", "Memory Bridge", "Micro Command"]
    },
    languageBridges: {
      name: "Language Bridges",
      description: "Adapters connecting specific languages to the FFI Core",
      functions: [
        "convert_to_native() - Convert FFI values to language-specific values",
        "convert_from_native() - Convert language-specific values to FFI values",
        "register_function() - Register language-specific function",
        "call_function() - Call language-specific function",
        "handle_exception() - Handle language-specific exceptions"
      ],
      interfaces: ["FFI Core", "Host Languages", "Type System"]
    },
    protocolBridge: {
      name: "Protocol Bridge",
      description: "Connects FFI layer to PolyCall Protocol system for network communication",
      functions: [
        "polycall_protocol_route_to_ffi() - Route protocol message to FFI function",
        "polycall_protocol_ffi_result_to_message() - Convert FFI result to protocol message",
        "polycall_protocol_register_remote_function() - Register FFI function for remote calls",
        "polycall_protocol_call_remote_function() - Call a remote FFI function",
        "polycall_protocol_handle_message() - Handle protocol message"
      ],
      interfaces: ["FFI Core", "Protocol System"]
    },
    performance: {
      name: "Performance Manager",
      description: "Optimization mechanisms for cross-language function calls",
      functions: [
        "polycall_performance_trace_begin/end() - Trace function calls",
        "polycall_performance_check_cache() - Check for cached results",
        "polycall_performance_cache_result() - Cache function results",
        "polycall_performance_queue_call() - Queue function call for batching",
        "polycall_performance_execute_batch() - Execute batched calls"
      ],
      interfaces: ["FFI Core", "Type System"]
    }
  };

  const handleComponentClick = (componentKey) => {
    setSelectedComponent(componentKey);
  };

  return (
    <div className="bg-white p-6 rounded-lg shadow-lg max-w-4xl mx-auto">
      <h1 className="text-2xl font-bold mb-4 text-blue-800">LibPolyCall FFI Architecture Explorer</h1>
      
      {showDescription && (
        <div className="bg-blue-50 p-4 rounded-lg mb-6">
          <h2 className="text-lg font-semibold mb-2">Foreign Function Interface (FFI)</h2>
          <p className="text-gray-700">
            The FFI component enables seamless interoperability between multiple programming languages 
            while adhering to the Program-First design philosophy. It provides a secure, efficient, and 
            type-safe mechanism for cross-language function calls with comprehensive memory management 
            and security controls.
          </p>
          <button 
            className="text-blue-600 mt-2 hover:underline"
            onClick={() => setShowDescription(false)}
          >
            Hide Overview
          </button>
        </div>
      )}

      {!showDescription && (
        <button 
          className="text-blue-600 mb-4 hover:underline"
          onClick={() => setShowDescription(true)}
        >
          Show Overview
        </button>
      )}

      <div className="grid grid-cols-1 md:grid-cols-3 gap-4 mb-6">
        {Object.keys(components).map(key => (
          <div 
            key={key}
            className={`p-4 rounded-lg cursor-pointer transition-all ${
              selectedComponent === key 
                ? 'bg-blue-600 text-white shadow-md' 
                : 'bg-gray-100 hover:bg-gray-200'
            }`}
            onClick={() => handleComponentClick(key)}
          >
            <h3 className="font-bold">{components[key].name}</h3>
            <p className="text-sm truncate">
              {components[key].description.substring(0, 60)}...
            </p>
          </div>
        ))}
      </div>

      {selectedComponent && (
        <div className="border border-gray-200 rounded-lg p-4">
          <h2 className="text-xl font-bold mb-2">{components[selectedComponent].name}</h2>
          <p className="mb-4">{components[selectedComponent].description}</p>
          
          <div className="mb-4">
            <h3 className="text-lg font-semibold mb-2">Key Functions</h3>
            <ul className="list-disc pl-6">
              {components[selectedComponent].functions.map((func, index) => (
                <li key={index} className="mb-1">{func}</li>
              ))}
            </ul>
          </div>
          
          <div>
            <h3 className="text-lg font-semibold mb-2">Interfaces With</h3>
            <div className="flex flex-wrap gap-2">
              {components[selectedComponent].interfaces.map((interface_, index) => (
                <span key={index} className="bg-blue-100 text-blue-800 px-2 py-1 rounded-md text-sm">
                  {interface_}
                </span>
              ))}
            </div>
          </div>
        </div>
      )}

      <div className="mt-6 p-4 bg-gray-50 rounded-lg">
        <h3 className="font-semibold mb-2">Implementation Notes</h3>
        <p className="text-sm text-gray-700">
          LibPolyCall's FFI follows a modular approach with well-defined interfaces between components.
          The implementation prioritizes type safety, memory isolation, and secure cross-language 
          communication while maintaining high performance through techniques like memory pooling, 
          type caching, and call optimization.
        </p>
      </div>
    </div>
  );
};

export default FFIArchitectureExplorer;
