// Edge Computing CDN Cache Demonstration
const EdgeCompute = {
    nodes: new Map(),
    cachePolicy: {
        maxSize: 100,
        ttl: 3600
    },

    deployEdgeNode: function(nodeId, location) {
        const node = {
            id: nodeId,
            location: location,
            cache: new Map(),
            metrics: {
                hits: 0,
                misses: 0,
                latency: Math.random() * 50 + 10
            }
        };
        this.nodes.set(nodeId, node);
        return node;
    },

    cacheContent: function(nodeId, key, content) {
        const node = this.nodes.get(nodeId);
        if (!node) return 'ERROR: Node not found';
        node.cache.set(key, {
            content: content,
            timestamp: Date.now(),
            accessCount: 0
        });
        return 'Cached ' + key + ' on node ' + nodeId;
    },

    retrieveContent: function(nodeId, key) {
        const node = this.nodes.get(nodeId);
        if (!node) return null;
        const cached = node.cache.get(key);
        if (cached) {
            node.metrics.hits++;
            cached.accessCount++;
            return cached.content;
        } else {
            node.metrics.misses++;
            return null;
        }
    }
};

function runEdgeDemo() {
    EdgeCompute.deployEdgeNode('edge-us-west', 'California');
    EdgeCompute.deployEdgeNode('edge-us-east', 'Virginia');
    EdgeCompute.cacheContent('edge-us-west', 'video-001.mp4', 'Video Stream Data');
    EdgeCompute.cacheContent('edge-us-east', 'api-response-users', '{"users": []}');
    document.getElementById('edge-topology').innerHTML =
        '<div class="topology-view">' +
            Array.from(EdgeCompute.nodes.entries()).map(function(entry) {
                var id = entry[0];
                var node = entry[1];
                return '<div class="node"><strong>' + id + '</strong><br>' +
                    'Location: ' + node.location + '<br>' +
                    'Latency: ' + node.metrics.latency.toFixed(2) + 'ms</div>';
            }).join('') +
        '</div>';
    document.getElementById('cache-status').innerHTML =
        '<pre>Cache Status:\n' +
            Array.from(EdgeCompute.nodes.entries()).map(function(entry) {
                var id = entry[0];
                var node = entry[1];
                return id + ': ' + node.cache.size + ' items cached';
            }).join('\n') +
        '</pre>';
}
