/**
 * hospitalcorp-nodepolycall-healthcare - Domain-Specific LibPolyCall Binding
 * Language: Node.js
 * Domain: healthcare
 * Compliance Level: enhanced
 */

const { SecurePolyCallClient } = require('node-polycall-secure');
const { HealthcareComplianceLayer } = require('./healthcare-extensions');
const { AuditTrail } = require('./audit');

class Hospitalcorp-nodepolycall-healthcareClient extends SecurePolyCallClient {
    constructor(configPath = null) {
        super(configPath);
        this.compliance = new HealthcareComplianceLayer(this.context);
        this.audit = new AuditTrail({ domain: 'healthcare' });
    }
    
    async executeCompliantOperation(operation, options = {}) {
        return this.audit.withOperationContext(operation, async () => {
            await this.compliance.preExecutionValidation(operation, options);
            const result = await super.executeOperation(operation, options);
            await this.compliance.postExecutionValidation(result);
            return result;
        });
    }
}

module.exports = {
    Hospitalcorp-nodepolycall-healthcareClient,
    domain: 'healthcare',
    complianceLevel: 'enhanced',
    version: '2.0.0'
};
