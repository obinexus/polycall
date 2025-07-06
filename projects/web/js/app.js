// PolyCall Web Application
// Version: 0.1.0-dev
// Build: no-git

import { polycall } from './polycall-loader.js';
import { TelemetryPanel } from './modules/telemetry.js';
import { DemoInterface } from './modules/demo.js';
import { FeatureShowcase } from './modules/features.js';

class PolyCallApp {
    constructor() {
        this.telemetry = new TelemetryPanel();
        this.demo = new DemoInterface();
        this.features = new FeatureShowcase();
        this.initialized = false;
    }
    
    async init() {
        console.log('Initializing PolyCall v0.1.0-dev...');
        
        try {
            // Update status
            this.updateStatus('loading');
            
            // Initialize WASM
            await polycall.initialize();
            
            // Generate crypto seed
            const seed = await this.generateCryptoSeed();
            
            // Create context
            this.context = polycall.createContext(seed);
            
            // Set up UI
            this.setupUI();
            
            // Start telemetry
            this.telemetry.start();
            
            this.initialized = true;
            this.updateStatus('ready');
            
        } catch (error) {
            console.error('Initialization failed:', error);
            this.updateStatus('error');
        }
    }
    
    async generateCryptoSeed() {
        const array = new Uint8Array(32);
        crypto.getRandomValues(array);
        return btoa(String.fromCharCode.apply(null, array));
    }
    
    setupUI() {
        // Render components
        document.getElementById('main-header').innerHTML = this.renderHeader();
        document.getElementById('main-content').innerHTML = this.renderContent();
        document.getElementById('telemetry-panel').appendChild(
            this.telemetry.render()
        );
        
        // Attach event handlers
        this.attachEventHandlers();
    }
    
    renderHeader() {
        return `
            <div class="container">
                <div class="hero">
                    <h1>PolyCall</h1>
                    <p>Command-Driven Polymorphic Runtime System</p>
                    <p>OBINexus Computing - Hot-Wiring Architecture</p>
                    <span id="wasm-status" class="status loading">Loading WASM...</span>
                </div>
            </div>
        `;
    }
    
    renderContent() {
        return `
            <div class="container">
                ${this.features.render()}
                ${this.demo.render()}
            </div>
        `;
    }
    
    attachEventHandlers() {
        // Demo interface handlers
        this.demo.onExecute = async (command) => {
            const startTime = performance.now();
            const result = await this.executeCommand(command);
            result.clientDuration = performance.now() - startTime;
            this.telemetry.logCommand(command, result);
            return result;
        };
    }
    
    async executeCommand(commandStr) {
        const parts = commandStr.trim().split(/\s+/);
        const command = parts[0];
        const args = parts.slice(1);
        
        return await polycall.execute(this.context, command, args);
    }
    
    updateStatus(status) {
        const statusEl = document.getElementById('wasm-status');
        if (statusEl) {
            statusEl.className = `status ${status}`;
            statusEl.textContent = status === 'ready' ? 'WASM Ready' : 
                                   status === 'error' ? 'WASM Error' : 'Loading WASM...';
        }
    }
}

// Initialize app when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    const app = new PolyCallApp();
    window.polycallApp = app;
    app.init();
});
