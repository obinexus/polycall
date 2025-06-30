# Sinphasé Toolkit Makefile

.PHONY: help install check report refactor test

help:
	@echo "Sinphasé Toolkit Commands:"
	@echo "  install    - Install package in editable mode"
	@echo "  check      - Run governance check"
	@echo "  report     - Generate governance report" 
	@echo "  refactor   - Run refactoring (dry-run)"
	@echo "  test       - Run test suite"

install:
	pip install -e .

check:
	sinphase check

report:
	sinphase report

refactor:
	sinphase refactor --dry-run

test:
	pytest tests/ -v
