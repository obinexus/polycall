"""Language-native PolyCall configuration in Python.

``configure()`` returns a plain Python structure (could equally be a
``@dataclass``). No DSL, no hand-written JSON. Same two-service "shop"
topology as the C and Node examples.
"""


def configure():
    return {
        "project": {
            "name": "shop",
            "extension_namespace": "obinexus",
        },
        "services": [
            {
                "id": "web",
                "language": "node",
                "bind_address": "0.0.0.0",
                "host_port": 3000,
                "target_port": 3001,
                "workspace": "/opt/shop/web",
            },
            {
                "id": "inventory",
                "language": "c",
                "host_port": 8080,
                "target_port": 9090,
                "workspace": "/opt/shop/inventory",
                "timeout_ms": 15000,
                "max_connections": 128,
            },
        ],
    }
