// Feature Showcase Module
export class FeatureShowcase {
    constructor() {
        this.features = [
            {
                icon: '🔐',
                title: 'Zero-Trust Security',
                description: 'Every operation requires cryptographic validation with session-specific seeds.'
            },
            {
                icon: '⚡',
                title: 'Edge Micro Support',
                description: 'Optimized for constrained environments with static allocation and minimal footprint.'
            },
            {
                icon: '🌐',
                title: 'Universal Compatibility',
                description: 'Runs in browsers, edge devices, legacy systems, and modern cloud infrastructure.'
            },
            {
                icon: '♿',
                title: 'Accessibility First',
                description: 'Built-in voice control, haptic feedback, and screen reader support.'
            },
            {
                icon: '🔥',
                title: 'Hot-Wire Integration',
                description: 'Bridge legacy COBOL to modern REST APIs with zero downtime.'
            },
            {
                icon: '📊',
                title: 'Real-Time Telemetry',
                description: 'Comprehensive monitoring with minimal overhead and privacy preservation.'
            }
        ];
    }
    
    render() {
        const featuresHTML = this.features.map(f => `
            <div class="feature-card">
                <h3>${f.icon} ${f.title}</h3>
                <p>${f.description}</p>
            </div>
        `).join('');
        
        return `
            <section class="features">
                ${featuresHTML}
            </section>
        `;
    }
}
