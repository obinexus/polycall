/**
 * @file DOPAdapter.js
 * @brief DOP Adapter JavaScript Bridge Implementation
 * 
 * LibPolyCall DOP Adapter Framework - JavaScript Language Bridge
 * OBINexus Computing - Aegis Project Technical Infrastructure
 * 
 * Provides JavaScript/Node.js binding to the native DOP Adapter C library.
 * Enables secure cross-language component integration with Zero Trust enforcement.
 * Essential for web-based banking apps requiring component isolation.
 * 
 * @version 1.0.0
 * @date 2025-06-09
 */

const { createRequire } = require('module');
const require_resolve = createRequire(import.meta.url || __filename);

// Native DOP Adapter binding (compiled from C library)
let nativeDOPAdapter;
try {
    nativeDOPAdapter = require_resolve('../../../build/lib/polycall_dop_adapter.node');
} catch (error) {
    throw new Error(`Failed to load native DOP Adapter: ${error.message}`);
}

/**
 * DOP Adapter Error Classes
 */
class DOPAdapterError extends Error {
    constructor(message, code = 'DOP_ERROR_UNKNOWN') {
        super(message);
        this.name = 'DOPAdapterError';
        this.code = code;
    }
}

class DOPSecurityError extends DOPAdapterError {
    constructor(message) {
        super(message, 'DOP_ERROR_SECURITY_VIOLATION');
        this.name = 'DOPSecurityError';
    }
}

class DOPPermissionError extends DOPAdapterError {
    constructor(message) {
        super(message, 'DOP_ERROR_PERMISSION_DENIED');
        this.name = 'DOPPermissionError';
    }
}

class DOPIsolationError extends DOPAdapterError {
    constructor(message) {
        super(message, 'DOP_ERROR_ISOLATION_BREACH');
        this.name = 'DOPIsolationError';
    }
}

/**
 * DOP Value Type Constants
 */
const DOPValueType = {
    NULL: 0,
    BOOL: 1,
    INT32: 2,
    INT64: 3,
    UINT32: 4,
    UINT64: 5,
    FLOAT32: 6,
    FLOAT64: 7,
    STRING: 8,
    BYTES: 9,
    ARRAY: 10,
    OBJECT: 11,
    FUNCTION: 12,
    COMPONENT_REF: 13
};

/**
 * DOP Component State Constants
 */
const DOPComponentState = {
    UNINITIALIZED: 0,
    INITIALIZING: 1,
    READY: 2,
    EXECUTING: 3,
    SUSPENDED: 4,
    ERROR: 5,
    CLEANUP: 6,
    DESTROYED: 7
};

/**
 * DOP Isolation Level Constants
 */
const DOPIsolationLevel = {
    NONE: 0,
    BASIC: 1,
    STANDARD: 2,
    STRICT: 3,
    PARANOID: 4
};

/**
 * DOP Permission Flags
 */
const DOPPermissionFlags = {
    NONE: 0x00,
    MEMORY_READ: 0x01,
    MEMORY_WRITE: 0x02,
    INVOKE_LOCAL: 0x04,
    INVOKE_REMOTE: 0x08,
    FILE_ACCESS: 0x10,
    NETWORK: 0x20,
    PRIVILEGED: 0x40,
    ALL: 0xFF
};

/**
 * DOP Language Constants
 */
const DOPLanguage = {
    C: 0,
    JAVASCRIPT: 1,
    PYTHON: 2,
    JVM: 3,
    WASM: 4,
    UNKNOWN: 255
};

/**
 * Main DOP Adapter JavaScript Bridge Class
 */
class DOPAdapter {
    constructor() {
        this._nativeContext = null;
        this._components = new Map();
        this._initialized = false;
        this._eventListeners = new Map();
    }

    /**
     * Initialize the DOP Adapter with security policy
     * @param {Object} config Configuration object
     * @param {Object} config.securityPolicy Security policy configuration
     * @param {number} config.securityPolicy.isolationLevel Isolation level
     * @param {number} config.securityPolicy.allowedPermissions Allowed permissions bitmask
     * @param {number} config.securityPolicy.maxMemoryUsage Maximum memory usage in bytes
     * @param {number} config.securityPolicy.maxExecutionTime Maximum execution time in ms
     * @param {boolean} config.securityPolicy.auditEnabled Enable audit logging
     * @returns {Promise<void>}
     */
    async initialize(config = {}) {
        if (this._initialized) {
            throw new DOPAdapterError('DOP Adapter already initialized');
        }

        // Create default security policy
        const defaultSecurityPolicy = {
            isolationLevel: DOPIsolationLevel.STANDARD,
            allowedPermissions: DOPPermissionFlags.MEMORY_READ | 
                              DOPPermissionFlags.MEMORY_WRITE | 
                              DOPPermissionFlags.INVOKE_LOCAL,
            deniedPermissions: DOPPermissionFlags.NETWORK | DOPPermissionFlags.PRIVILEGED,
            maxMemoryUsage: 1024 * 1024, // 1MB
            maxExecutionTime: 5000, // 5 seconds
            auditEnabled: true,
            stackProtectionEnabled: true,
            heapProtectionEnabled: true
        };

        const securityPolicy = { ...defaultSecurityPolicy, ...config.securityPolicy };

        try {
            // Initialize native DOP Adapter context
            this._nativeContext = await nativeDOPAdapter.initialize(securityPolicy);
            this._initialized = true;

            // Setup event handling
            this._setupEventHandling();

            this._emit('initialized', { securityPolicy });
        } catch (error) {
            throw new DOPAdapterError(`Failed to initialize DOP Adapter: ${error.message}`);
        }
    }

    /**
     * Register a JavaScript component
     * @param {Object} componentConfig Component configuration
     * @param {string} componentConfig.componentId Unique component identifier
     * @param {string} componentConfig.componentName Human-readable component name
     * @param {string} componentConfig.version Component version
     * @param {Object} componentConfig.securityPolicy Component security policy
     * @param {Array} componentConfig.methods Array of method signatures
     * @param {Object} componentInstance Component implementation instance
     * @returns {Promise<DOPComponent>}
     */
    async registerComponent(componentConfig, componentInstance) {
        if (!this._initialized) {
            throw new DOPAdapterError('DOP Adapter not initialized');
        }

        // Validate component configuration
        this._validateComponentConfig(componentConfig);

        // Ensure component instance has required methods
        if (!componentInstance || typeof componentInstance !== 'object') {
            throw new DOPAdapterError('Component instance must be an object');
        }

        // Create enhanced config with JavaScript-specific settings
        const enhancedConfig = {
            ...componentConfig,
            language: DOPLanguage.JAVASCRIPT,
            memoryStrategy: 'pool', // Use pool-based memory allocation
            languageSpecificConfig: {
                runtime: 'nodejs',
                version: process.version,
                isolateMemoryLimit: componentConfig.securityPolicy?.maxMemoryUsage || 1024 * 1024
            }
        };

        try {
            // Register component with native adapter
            const nativeComponent = await nativeDOPAdapter.registerComponent(
                this._nativeContext, enhancedConfig
            );

            // Create JavaScript wrapper component
            const component = new DOPComponent(
                this, 
                nativeComponent, 
                componentConfig, 
                componentInstance
            );

            // Store component reference
            this._components.set(componentConfig.componentId, component);

            this._emit('componentRegistered', { 
                componentId: componentConfig.componentId,
                componentName: componentConfig.componentName 
            });

            return component;
        } catch (error) {
            throw new DOPAdapterError(`Failed to register component: ${error.message}`);
        }
    }

    /**
     * Unregister a component
     * @param {string} componentId Component identifier
     * @returns {Promise<void>}
     */
    async unregisterComponent(componentId) {
        if (!this._components.has(componentId)) {
            throw new DOPAdapterError(`Component '${componentId}' not found`);
        }

        const component = this._components.get(componentId);
        
        try {
            // Cleanup component
            await component._cleanup();
            
            // Remove from registry
            this._components.delete(componentId);

            this._emit('componentUnregistered', { componentId });
        } catch (error) {
            throw new DOPAdapterError(`Failed to unregister component: ${error.message}`);
        }
    }

    /**
     * Invoke a method on a component
     * @param {string} componentId Target component identifier
     * @param {string} methodName Method name to invoke
     * @param {Array} parameters Method parameters
     * @param {Object} options Invocation options
     * @returns {Promise<any>} Method result
     */
    async invoke(componentId, methodName, parameters = [], options = {}) {
        if (!this._components.has(componentId)) {
            throw new DOPAdapterError(`Component '${componentId}' not found`);
        }

        const component = this._components.get(componentId);
        return await component.invoke(methodName, parameters, options);
    }

    /**
     * Get component by ID
     * @param {string} componentId Component identifier
     * @returns {DOPComponent|null}
     */
    getComponent(componentId) {
        return this._components.get(componentId) || null;
    }

    /**
     * List all registered components
     * @returns {Array<string>} Array of component IDs
     */
    listComponents() {
        return Array.from(this._components.keys());
    }

    /**
     * Get adapter statistics
     * @returns {Promise<Object>} Adapter statistics
     */
    async getStatistics() {
        if (!this._initialized) {
            throw new DOPAdapterError('DOP Adapter not initialized');
        }

        try {
            const nativeStats = await nativeDOPAdapter.getStatistics(this._nativeContext);
            
            return {
                ...nativeStats,
                componentCount: this._components.size,
                jsHeapUsed: process.memoryUsage().heapUsed,
                jsHeapTotal: process.memoryUsage().heapTotal
            };
        } catch (error) {
            throw new DOPAdapterError(`Failed to get statistics: ${error.message}`);
        }
    }

    /**
     * Cleanup and destroy the DOP Adapter
     * @returns {Promise<void>}
     */
    async cleanup() {
        if (!this._initialized) {
            return;
        }

        try {
            // Cleanup all components
            for (const [componentId, component] of this._components) {
                try {
                    await component._cleanup();
                } catch (error) {
                    console.error(`Failed to cleanup component '${componentId}': ${error.message}`);
                }
            }
            this._components.clear();

            // Cleanup native context
            if (this._nativeContext) {
                await nativeDOPAdapter.cleanup(this._nativeContext);
                this._nativeContext = null;
            }

            this._initialized = false;
            this._emit('cleanup');
        } catch (error) {
            throw new DOPAdapterError(`Failed to cleanup DOP Adapter: ${error.message}`);
        }
    }

    /**
     * Add event listener
     * @param {string} event Event name
     * @param {Function} listener Event listener function
     */
    on(event, listener) {
        if (!this._eventListeners.has(event)) {
            this._eventListeners.set(event, []);
        }
        this._eventListeners.get(event).push(listener);
    }

    /**
     * Remove event listener
     * @param {string} event Event name
     * @param {Function} listener Event listener function
     */
    off(event, listener) {
        if (this._eventListeners.has(event)) {
            const listeners = this._eventListeners.get(event);
            const index = listeners.indexOf(listener);
            if (index !== -1) {
                listeners.splice(index, 1);
            }
        }
    }

    // Private methods

    _validateComponentConfig(config) {
        if (!config || typeof config !== 'object') {
            throw new DOPAdapterError('Component configuration must be an object');
        }

        if (!config.componentId || typeof config.componentId !== 'string') {
            throw new DOPAdapterError('Component ID must be a non-empty string');
        }

        if (!config.componentName || typeof config.componentName !== 'string') {
            throw new DOPAdapterError('Component name must be a non-empty string');
        }

        if (!config.version || typeof config.version !== 'string') {
            throw new DOPAdapterError('Component version must be a non-empty string');
        }

        if (config.methods && !Array.isArray(config.methods)) {
            throw new DOPAdapterError('Component methods must be an array');
        }
    }

    _setupEventHandling() {
        // Setup native event forwarding if supported
        if (nativeDOPAdapter.setupEventForwarding) {
            nativeDOPAdapter.setupEventForwarding(this._nativeContext, (event) => {
                this._emit(event.type, event.data);
            });
        }
    }

    _emit(event, data = null) {
        if (this._eventListeners.has(event)) {
            const listeners = this._eventListeners.get(event);
            for (const listener of listeners) {
                try {
                    listener(data);
                } catch (error) {
                    console.error(`Error in event listener for '${event}': ${error.message}`);
                }
            }
        }
    }
}

/**
 * DOP Component JavaScript Wrapper Class
 */
class DOPComponent {
    constructor(adapter, nativeComponent, config, instance) {
        this._adapter = adapter;
        this._nativeComponent = nativeComponent;
        this._config = config;
        this._instance = instance;
        this._state = DOPComponentState.READY;
    }

    /**
     * Get component ID
     * @returns {string}
     */
    get componentId() {
        return this._config.componentId;
    }

    /**
     * Get component name
     * @returns {string}
     */
    get componentName() {
        return this._config.componentName;
    }

    /**
     * Get component version
     * @returns {string}
     */
    get version() {
        return this._config.version;
    }

    /**
     * Get component state
     * @returns {number}
     */
    get state() {
        return this._state;
    }

    /**
     * Invoke a method on this component
     * @param {string} methodName Method name
     * @param {Array} parameters Method parameters
     * @param {Object} options Invocation options
     * @returns {Promise<any>}
     */
    async invoke(methodName, parameters = [], options = {}) {
        if (this._state !== DOPComponentState.READY) {
            throw new DOPAdapterError(`Component '${this.componentId}' is not ready for invocation`);
        }

        // Validate method exists
        if (!this._instance[methodName] || typeof this._instance[methodName] !== 'function') {
            throw new DOPAdapterError(`Method '${methodName}' not found on component '${this.componentId}'`);
        }

        try {
            this._state = DOPComponentState.EXECUTING;

            // Convert parameters to DOP values
            const dopParameters = this._convertToDOPValues(parameters);

            // Execute through native adapter for security validation
            const startTime = process.hrtime.bigint();
            
            // Call the actual JavaScript method
            const result = await this._instance[methodName](...parameters);
            
            const endTime = process.hrtime.bigint();
            const executionTime = Number(endTime - startTime) / 1000000; // Convert to milliseconds

            // Convert result to DOP value
            const dopResult = this._convertToDOPValue(result);

            this._state = DOPComponentState.READY;

            // Log invocation through native adapter
            await nativeDOPAdapter.logInvocation(
                this._adapter._nativeContext,
                this._nativeComponent,
                methodName,
                executionTime,
                'success'
            );

            return result;
        } catch (error) {
            this._state = DOPComponentState.ERROR;
            
            // Log error through native adapter
            await nativeDOPAdapter.logInvocation(
                this._adapter._nativeContext,
                this._nativeComponent,
                methodName,
                0,
                'error',
                error.message
            );

            throw new DOPAdapterError(`Component invocation failed: ${error.message}`);
        }
    }

    /**
     * Suspend component execution
     * @returns {Promise<void>}
     */
    async suspend() {
        if (this._state !== DOPComponentState.READY) {
            throw new DOPAdapterError(`Cannot suspend component in state: ${this._state}`);
        }

        this._state = DOPComponentState.SUSPENDED;
        this._adapter._emit('componentSuspended', { componentId: this.componentId });
    }

    /**
     * Resume component execution
     * @returns {Promise<void>}
     */
    async resume() {
        if (this._state !== DOPComponentState.SUSPENDED) {
            throw new DOPAdapterError(`Cannot resume component in state: ${this._state}`);
        }

        this._state = DOPComponentState.READY;
        this._adapter._emit('componentResumed', { componentId: this.componentId });
    }

    /**
     * Get component statistics
     * @returns {Promise<Object>}
     */
    async getStatistics() {
        try {
            return await nativeDOPAdapter.getComponentStatistics(
                this._adapter._nativeContext,
                this._nativeComponent
            );
        } catch (error) {
            throw new DOPAdapterError(`Failed to get component statistics: ${error.message}`);
        }
    }

    // Private methods

    async _cleanup() {
        if (this._state === DOPComponentState.DESTROYED) {
            return;
        }

        this._state = DOPComponentState.CLEANUP;

        try {
            // Call cleanup method on instance if it exists
            if (this._instance.cleanup && typeof this._instance.cleanup === 'function') {
                await this._instance.cleanup();
            }

            // Cleanup native component
            await nativeDOPAdapter.unregisterComponent(
                this._adapter._nativeContext,
                this._nativeComponent
            );

            this._state = DOPComponentState.DESTROYED;
        } catch (error) {
            this._state = DOPComponentState.ERROR;
            throw error;
        }
    }

    _convertToDOPValues(jsValues) {
        return jsValues.map(value => this._convertToDOPValue(value));
    }

    _convertToDOPValue(jsValue) {
        if (jsValue === null || jsValue === undefined) {
            return { type: DOPValueType.NULL, value: null };
        }
        
        if (typeof jsValue === 'boolean') {
            return { type: DOPValueType.BOOL, value: jsValue };
        }
        
        if (typeof jsValue === 'number') {
            if (Number.isInteger(jsValue)) {
                return { type: DOPValueType.INT32, value: jsValue };
            } else {
                return { type: DOPValueType.FLOAT64, value: jsValue };
            }
        }
        
        if (typeof jsValue === 'string') {
            return { type: DOPValueType.STRING, value: jsValue };
        }
        
        if (Array.isArray(jsValue)) {
            return {
                type: DOPValueType.ARRAY,
                value: jsValue.map(item => this._convertToDOPValue(item))
            };
        }
        
        if (typeof jsValue === 'object') {
            return { type: DOPValueType.OBJECT, value: jsValue };
        }
        
        if (typeof jsValue === 'function') {
            return { type: DOPValueType.FUNCTION, value: jsValue.toString() };
        }
        
        return { type: DOPValueType.OBJECT, value: jsValue };
    }
}

// Export classes and constants
module.exports = {
    DOPAdapter,
    DOPComponent,
    DOPAdapterError,
    DOPSecurityError,
    DOPPermissionError,
    DOPIsolationError,
    DOPValueType,
    DOPComponentState,
    DOPIsolationLevel,
    DOPPermissionFlags,
    DOPLanguage
};
