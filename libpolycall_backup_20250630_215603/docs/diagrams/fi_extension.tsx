import React from 'react';

const FFIArchitectureDiagram = () => {
  // Styling for the diagram components
  const styles = {
    container: {
      fontFamily: 'sans-serif',
      maxWidth: '100%',
      margin: '0 auto',
      padding: '20px',
      backgroundColor: '#f9f9f9',
      borderRadius: '8px',
      boxShadow: '0 2px 10px rgba(0, 0, 0, 0.1)'
    },
    title: {
      textAlign: 'center',
      color: '#333',
      marginBottom: '20px'
    },
    diagramContainer: {
      display: 'flex',
      flexDirection: 'column',
      gap: '20px'
    },
    mainComponents: {
      display: 'flex',
      justifyContent: 'space-between',
      gap: '20px',
      flexWrap: 'wrap'
    },
    componentBox: {
      padding: '15px',
      backgroundColor: '#fff',
      borderRadius: '6px',
      border: '1px solid #ddd',
      minWidth: '200px',
      flex: '1 1 200px',
      boxShadow: '0 2px 4px rgba(0, 0, 0, 0.05)'
    },
    moduleTitle: {
      borderBottom: '2px solid #3498db',
      paddingBottom: '8px',
      marginBottom: '12px',
      color: '#2c3e50',
      fontSize: '16px',
      fontWeight: 'bold'
    },
    componentList: {
      margin: '0',
      padding: '0 0 0 20px',
      fontSize: '14px'
    },
    listItem: {
      margin: '6px 0'
    },
    sectionTitle: {
      borderBottom: '1px solid #ddd',
      paddingBottom: '6px',
      marginTop: '30px',
      marginBottom: '15px',
      fontSize: '18px'
    },
    bridgesContainer: {
      display: 'flex',
      justifyContent: 'space-between',
      gap: '15px',
      flexWrap: 'wrap'
    },
    bridge: {
      flex: '1 1 150px',
      padding: '12px',
      backgroundColor: '#e8f4f8',
      borderRadius: '6px',
      border: '1px solid #bde0ec',
      boxShadow: '0 1px 3px rgba(0, 0, 0, 0.05)'
    },
    bridgeTitle: {
      fontWeight: 'bold',
      marginBottom: '8px',
      color: '#2980b9'
    },
    architectureFlowContainer: {
      margin: '20px 0',
      padding: '15px',
      backgroundColor: '#fff',
      borderRadius: '6px',
      border: '1px solid #ddd',
      boxShadow: '0 2px 4px rgba(0, 0, 0, 0.05)'
    },
    flowTitle: {
      fontWeight: 'bold',
      marginBottom: '10px',
      color: '#2c3e50',
      fontSize: '16px'
    },
    flowDescription: {
      fontSize: '14px',
      lineHeight: '1.5',
      color: '#333'
    },
    footer: {
      marginTop: '30px',
      fontSize: '14px',
      color: '#666',
      textAlign: 'center'
    }
  };

  return (
    <div style={styles.container}>
      <h1 style={styles.title}>LibPolyCall Foreign Function Interface (FFI) Architecture</h1>
      
      <div style={styles.diagramContainer}>
        {/* Core FFI Components */}
        <div style={styles.mainComponents}>
          {/* FFI Core */}
          <div style={styles.componentBox}>
            <h3 style={styles.moduleTitle}>FFI Core (ffi_core.h/c)</h3>
            <p>Central coordination for cross-language function calls</p>
            <ul style={styles.componentList}>
              <li style={styles.listItem}>Function registration & discovery</li>
              <li style={styles.listItem}>Call marshalling & dispatch</li>
              <li style={styles.listItem}>Type mapping coordination</li>
              <li style={styles.listItem}>Context management</li>
            </ul>
          </div>
          
          {/* Type System */}
          <div style={styles.componentBox}>
            <h3 style={styles.moduleTitle}>Type System (type_system.h/c)</h3>
            <p>Handles data type conversion between languages</p>
            <ul style={styles.componentList}>
              <li style={styles.listItem}>Type mapping & conversion</li>
              <li style={styles.listItem}>Data serialization/deserialization</li>
              <li style={styles.listItem}>Type registry management</li>
              <li style={styles.listItem}>Struct & complex type handling</li>
            </ul>
          </div>
          
          {/* Memory Bridge */}
          <div style={styles.componentBox}>
            <h3 style={styles.moduleTitle}>Memory Bridge (memory_bridge.h/c)</h3>
            <p>Manages memory sharing across language boundaries</p>
            <ul style={styles.componentList}>
              <li style={styles.listItem}>Shared memory allocation</li>
              <li style={styles.listItem}>Reference counting</li>
              <li style={styles.listItem}>Ownership tracking</li>
              <li style={styles.listItem}>GC coordination</li>
            </ul>
          </div>
        </div>
        
        <div style={styles.mainComponents}>
          {/* Security */}
          <div style={styles.componentBox}>
            <h3 style={styles.moduleTitle}>Security (security.h/c)</h3>
            <p>Zero-trust security model for cross-language calls</p>
            <ul style={styles.componentList}>
              <li style={styles.listItem}>Access control & permission management</li>
              <li style={styles.listItem}>Memory isolation</li>
              <li style={styles.listItem}>Function call validation</li>
              <li style={styles.listItem}>Audit logging & policy enforcement</li>
            </ul>
          </div>
          
          {/* Performance */}
          <div style={styles.componentBox}>
            <h3 style={styles.moduleTitle}>Performance (performance.h/c)</h3>
            <p>Optimizes cross-language function calls</p>
            <ul style={styles.componentList}>
              <li style={styles.listItem}>Result caching</li>
              <li style={styles.listItem}>Call batching</li>
              <li style={styles.listItem}>Performance metrics</li>
              <li style={styles.listItem}>Tracing & monitoring</li>
            </ul>
          </div>
          
          {/* Protocol Bridge */}
          <div style={styles.componentBox}>
            <h3 style={styles.moduleTitle}>Protocol Bridge (protocol_bridge.h/c)</h3>
            <p>Links FFI to network protocol system</p>
            <ul style={styles.componentList}>
              <li style={styles.listItem}>Remote function invocation</li>
              <li style={styles.listItem}>Message serialization</li>
              <li style={styles.listItem}>Route management</li>
              <li style={styles.listItem}>Network integration</li>
            </ul>
          </div>
        </div>
        
        {/* Language Bridges */}
        <h2 style={styles.sectionTitle}>Language Bridges</h2>
        <div style={styles.bridgesContainer}>
          <div style={styles.bridge}>
            <h4 style={styles.bridgeTitle}>C Bridge (c_bridge.h/c)</h4>
            <ul style={styles.componentList}>
              <li style={styles.listItem}>Native C function handling</li>
              <li style={styles.listItem}>Struct registration</li>
              <li style={styles.listItem}>Callback setup</li>
            </ul>
          </div>
          
          <div style={styles.bridge}>
            <h4 style={styles.bridgeTitle}>JavaScript Bridge (js_bridge.h/c)</h4>
            <ul style={styles.componentList}>
              <li style={styles.listItem}>JavaScript runtime integration</li>
              <li style={styles.listItem}>Promise handling</li>
              <li style={styles.listItem}>JS object conversion</li>
            </ul>
          </div>
          
          <div style={styles.bridge}>
            <h4 style={styles.bridgeTitle}>JVM Bridge (jvm_bridge.h/c)</h4>
            <ul style={styles.componentList}>
              <li style={styles.listItem}>Java method integration</li>
              <li style={styles.listItem}>JNI management</li>
              <li style={styles.listItem}>Exception handling</li>
            </ul>
          </div>
          
          <div style={styles.bridge}>
            <h4 style={styles.bridgeTitle}>Python Bridge (python_bridge.h/c)</h4>
            <ul style={styles.componentList}>
              <li style={styles.listItem}>Python interpreter integration</li>
              <li style={styles.listItem}>PyObject conversion</li>
              <li style={styles.listItem}>Reference management</li>
            </ul>
          </div>
        </div>
        
        {/* Architecture Flow */}
        <h2 style={styles.sectionTitle}>Key Interactions</h2>
        
        {/* Cross-Language Call Flow */}
        <div style={styles.architectureFlowContainer}>
          <h3 style={styles.flowTitle}>Cross-Language Function Call Flow</h3>
          <p style={styles.flowDescription}>
            1. Source language bridge receives call via its native API<br/>
            2. Bridge converts native arguments to FFI values<br/>
            3. Security layer verifies permissions for the call<br/>
            4. FFI core looks up target function and routes to target language<br/>
            5. Type system converts arguments to target language format<br/>
            6. Target language bridge executes function in its runtime<br/>
            7. Return value is converted back via type system<br/>
            8. Performance metrics are updated<br/>
            9. Result is returned to calling language
          </p>
        </div>
        
        {/* Memory Sharing Flow */}
        <div style={styles.architectureFlowContainer}>
          <h3 style={styles.flowTitle}>Memory Sharing Process</h3>
          <p style={styles.flowDescription}>
            1. Source language requests memory allocation via its bridge<br/>
            2. Memory bridge allocates from shared pool<br/>
            3. Ownership registry records source language as owner<br/>
            4. When shared with target language, memory permissions are verified<br/>
            5. Reference counting tracks all language references<br/>
            6. When references reach zero, memory is freed<br/>
            7. GC coordination prevents premature collection
          </p>
        </div>
        
        {/* Integration with Core System */}
        <div style={styles.architectureFlowContainer}>
          <h3 style={styles.flowTitle}>Integration with LibPolyCall Core & Micro/Edge Commands</h3>
          <p style={styles.flowDescription}>
            1. FFI Core registers with the Core Context system<br/>
            2. Protocol Bridge enables remote FFI calls over network<br/>
            3. Micro Command components can access FFI functions with proper permissions<br/>
            4. Edge Command system routes FFI calls to appropriate nodes<br/>
            5. Security policies are unified across all modules<br/>
            6. Hierarchical state system coordinates protocol states with FFI operations
          </p>
        </div>
      </div>
      
      <div style={styles.footer}>
        Based on LibPolyCall codebase structure and FFI architectural documentation
      </div>
    </div>
  );
};

export default FFIArchitectureDiagram;