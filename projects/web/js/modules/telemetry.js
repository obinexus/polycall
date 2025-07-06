// Telemetry Module
export class TelemetryPanel {
    constructor() {
        this.metrics = {
            commandCount: 0,
            cryptoOps: 0,
            totalTime: 0,
            memoryUsage: 0
        };
        this.updateInterval = null;
    }
    
    start() {
        this.updateInterval = setInterval(() => this.update(), 1000);
    }
    
    stop() {
        if (this.updateInterval) {
            clearInterval(this.updateInterval);
            this.updateInterval = null;
        }
    }
    
    render() {
        const panel = document.createElement('div');
        panel.className = 'telemetry-panel';
        panel.innerHTML = `
            <h4>Live Telemetry</h4>
            <div class="metric">
                <span>Commands:</span>
                <span class="metric-value" id="cmd-count">0</span>
            </div>
            <div class="metric">
                <span>Crypto Ops:</span>
                <span class="metric-value" id="crypto-ops">0</span>
            </div>
            <div class="metric">
                <span>Avg Time:</span>
                <span class="metric-value" id="avg-time">0 ms</span>
            </div>
            <div class="metric">
                <span>Memory:</span>
                <span class="metric-value" id="mem-usage">0 KB</span>
            </div>
        `;
        return panel;
    }
    
    update() {
        const cmdCount = document.getElementById('cmd-count');
        const cryptoOps = document.getElementById('crypto-ops');
        const avgTime = document.getElementById('avg-time');
        const memUsage = document.getElementById('mem-usage');
        
        if (cmdCount) cmdCount.textContent = this.metrics.commandCount;
        if (cryptoOps) cryptoOps.textContent = this.metrics.cryptoOps;
        
        if (avgTime && this.metrics.commandCount > 0) {
            avgTime.textContent = 
                Math.round(this.metrics.totalTime / this.metrics.commandCount) + ' ms';
        }
        
        if (memUsage && performance.memory) {
            const memKB = Math.round(performance.memory.usedJSHeapSize / 1024);
            memUsage.textContent = memKB + ' KB';
        }
    }
    
    logCommand(command, result) {
        this.metrics.commandCount++;
        if (command.startsWith('crypto')) {
            this.metrics.cryptoOps++;
        }
        if (result.duration) {
            this.metrics.totalTime += result.duration;
        }
    }
}
