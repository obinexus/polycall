# Mesh Topology Configuration
TOPOLOGY_TYPE = mesh
TOPOLOGY_NODES = make cmake meson custom
TOPOLOGY_REDUNDANCY = 3

.PHONY: mesh-build mesh-health-check mesh-distribute

mesh-build:
	@echo "[Mesh] Starting mesh network build..."
	@$(MAKE) mesh-distribute

mesh-health-check:
	@echo "[Mesh] Checking mesh network health..."
	@for node in make cmake meson; do \
		command -v $$node >/dev/null 2>&1 && echo "$$node: online" || echo "$$node: offline"; \
	done

mesh-distribute:
	@echo "[Mesh] Distributing build across mesh network..."
	@$(MAKE) -j$(PARALLEL_JOBS) build-make build-cmake build-meson || \
	$(MAKE) build-fallback
