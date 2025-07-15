// MicroBank demonstration with topology
const MicroBank = {
    accounts: new Map(),
    topology: {
        authNode: null,
        transactionNode: null,
        auditNode: null
    },

    initialize: function() {
        this.topology.authNode = {
            id: 'auth-001',
            validate: function(transaction) { return transaction.pin === '1234'; }
        };
        this.topology.transactionNode = {
            id: 'trans-001',
            process: (from, to, amount) => {
                if (this.accounts.get(from).balance >= amount) {
                    this.accounts.get(from).balance -= amount;
                    this.accounts.get(to).balance += amount;
                    return true;
                }
                return false;
            }
        };
        this.accounts.set('ACC001', { balance: 1000, owner: 'Alice' });
        this.accounts.set('ACC002', { balance: 500, owner: 'Bob' });
        return 'MicroBank topology initialized with 3 nodes';
    },

    executeTransaction: function(from, to, amount, pin) {
        if (!this.topology.authNode.validate({ pin: pin })) {
            return 'AUTH_FAILED: Invalid credentials';
        }
        const result = this.topology.transactionNode.process(from, to, amount);
        return result ?
            'SUCCESS: Transferred $' + amount + ' from ' + from + ' to ' + to :
            'FAILED: Insufficient funds';
    }
};

function runBankingDemo() {
    const output = MicroBank.initialize();
    document.getElementById('bank-topology').innerHTML =
        '<div class="topology-view">' +
            '<div class="node">Auth Node: ' + MicroBank.topology.authNode.id + '</div>' +
            '<div class="node">Transaction Node: ' + MicroBank.topology.transactionNode.id + '</div>' +
            '<div class="node">Audit Node: READY</div>' +
        '</div>';
    const result = MicroBank.executeTransaction('ACC001', 'ACC002', 100, '1234');
    document.getElementById('bank-transactions').innerHTML = '<pre>' + result + '</pre>';
}
