# P2P Topology Configuration
TOPOLOGY_TYPE = p2p
TOPOLOGY_NODES = make cmake meson
TOPOLOGY_REDUNDANCY = 2

.PHONY: p2p-build p2p-health-check p2p-failover

p2p-build:
	@echo "[P2P] Starting peer-to-peer build..."
	@$(MAKE) build-primary || $(MAKE) p2p-failover

p2p-health-check:
	@echo "[P2P] Checking node health..."
	@command -v make >/dev/null 2>&1 || echo "Make node offline"
	@command -v cmake >/dev/null 2>&1 || echo "CMake node offline"
	@command -v meson >/dev/null 2>&1 || echo "Meson node offline"

p2p-failover:
	@echo "[P2P] Primary node failed, attempting failover..."
	@$(MAKE) build-cmake || $(MAKE) build-meson || $(MAKE) build-make
