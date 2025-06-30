# Sinphasé Toolkit Makefile

.PHONY: help install check report refactor test

help:
\t@echo "Sinphasé Toolkit Commands:"
\t@echo "  install    - Install package in editable mode"
\t@echo "  check      - Run governance check"
\t@echo "  report     - Generate governance report" 
\t@echo "  refactor   - Run refactoring (dry-run)"
\t@echo "  test       - Run test suite"

install:
\tpip install -e .

check:
\tsinphase check

report:
\tsinphase report

refactor:
\tsinphase refactor --dry-run

test:
\tpytest tests/ -v
