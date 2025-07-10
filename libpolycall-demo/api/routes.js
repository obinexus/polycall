// Simple client-side routing for demo
const routes = {
    '/api/topology': function() {
        return {
            nodes: Array.from(LibPolyCall.topology.nodes.entries()),
            enforcer: LibPolyCall.topology.enforcer
        };
    },

    '/api/bank/balance/:account': function(params) {
        return {
            account: params.account,
            balance: MicroBank.accounts.get(params.account) ?
                MicroBank.accounts.get(params.account).balance : 0
        };
    },

    '/api/edge/cache/:nodeId': function(params) {
        return {
            nodeId: params.nodeId,
            cacheSize: EdgeCompute.nodes.get(params.nodeId) ?
                EdgeCompute.nodes.get(params.nodeId).cache.size : 0
        };
    }
};

function handleRoute(path) {
    for (const pattern in routes) {
        const regex = pattern.replace(/:(\w+)/g, '(?<$1>\\w+)');
        const match = path.match(new RegExp('^' + regex + '$'));
        if (match) {
            return routes[pattern](match.groups || {});
        }
    }
    return { error: 'Route not found' };
}
