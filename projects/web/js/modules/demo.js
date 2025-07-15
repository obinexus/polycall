// Demo Interface Module
export class DemoInterface {
    constructor() {
        this.onExecute = null;
    }
    
    render() {
        return `
            <section class="demo-section">
                <h2>Interactive Demo</h2>
                <p>Try PolyCall directly in your browser:</p>
                <div class="interactive-demo">
                    <div class="demo-input">
                        <h4>Command Input</h4>
                        <textarea id="command-input" 
                            placeholder="Enter PolyCall command...&#10;Examples:&#10;echo Hello, World!&#10;crypto hash --algorithm=sha256 --data='test'&#10;status"></textarea>
                        <button id="execute-btn">Execute</button>
                    </div>
                    <div class="demo-output">
                        <h4>Output</h4>
                        <div class="code-demo">
                            <pre id="command-output">Ready for input...</pre>
                        </div>
                    </div>
                </div>
            </section>
        `;
    }
    
    async execute() {
        const input = document.getElementById('command-input');
        const output = document.getElementById('command-output');
        const button = document.getElementById('execute-btn');
        
        if (!input || !output) return;
        
        const command = input.value.trim();
        
        if (!command) {
            output.textContent = 'Please enter a command';
            return;
        }
        
        button.disabled = true;
        output.textContent = 'Executing...';
        
        try {
            if (this.onExecute) {
                const result = await this.onExecute(command);
                output.textContent = 
                    `Exit Code: ${result.exitCode}\n` +
                    `Duration: ${result.duration}ms\n\n` +
                    `${result.output}`;
            }
        } catch (error) {
            output.textContent = `Error: ${error.message}`;
        } finally {
            button.disabled = false;
        }
    }
}

// Set up event listeners when module loads
setTimeout(() => {
    const btn = document.getElementById('execute-btn');
    const input = document.getElementById('command-input');
    
    if (btn) {
        btn.addEventListener('click', () => {
            if (window.polycallApp && window.polycallApp.demo) {
                window.polycallApp.demo.execute();
            }
        });
    }
    
    if (input) {
        input.addEventListener('keydown', (e) => {
            if (e.ctrlKey && e.key === 'Enter') {
                if (window.polycallApp && window.polycallApp.demo) {
                    window.polycallApp.demo.execute();
                }
            }
        });
    }
}, 100);
