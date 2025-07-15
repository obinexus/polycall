# PolyCall Runtime Makefile

.PHONY: test clean logs

test:
	@echo "Running PolyCall tests..."
	@./bin/polycall-wrap echo "Test successful"

clean:
	@echo "Cleaning runtime directory..."
	@rm -rf logs/* temp/* cache/*

logs:
	@echo "Recent telemetry logs:"
	@tail -20 logs/telemetry/commands.log 2>/dev/null || echo "No logs yet"

status:
	@echo "PolyCall Runtime Status"
	@echo "======================"
	@echo "Home: $(POLYCALL_HOME)"
	@echo "Binaries: $$(ls -1 bin/ | wc -l)"
	@echo "Logs: $$(find logs -name "*.log" | wc -l)"
