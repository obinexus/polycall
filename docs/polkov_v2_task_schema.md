# Polkov V2 Phase Tasks

This document outlines milestone-based task schemas for each agent involved in Polkov V2 development. Tasks are grouped by roadmap phase and include the command, expected output, validation step and dependencies.

## Legend
- **Owner**: Assigned agent
- **Phase**: Development phase number
- **Command**: Executable command or action
- **Target Output**: Expected artefact
- **Validation**: How completion is checked
- **Dependencies**: Required tasks before starting
- **Next Task(s)**: Follow up tasks when complete

## Task List

### Task-1: Audit LibPolyCall Modules
- **Owner**: Claude
- **Phase**: 1
- **Command**: Analyse source under `/core/{feature}` to produce complexity metrics
- **Target Output**: `docs/complexity_matrix.md`
- **Validation**: Matrix lists complexity and dependencies
- **Dependencies**: None
- **Next Task(s)**: Task-3, Task-6

### Task-2: Generate Build System for Core Modules
- **Owner**: Polybuild
- **Phase**: 1
- **Command**: Create `CMakeLists.txt` and make scripts for `libpolycall-{module}` libraries
- **Target Output**: `build/{release,debug}/{lib,obj}` directories
- **Validation**: `polycall.exe` links successfully
- **Dependencies**: Task-1
- **Next Task(s)**: Task-4

### Task-3: Setup Constitutional Zero-Trust Configs
- **Owner**: Claude
- **Phase**: 1
- **Command**: Lock `config.polycall` and `.pcfrc` override cascade
- **Target Output**: Valid `.polycallrc` hierarchy
- **Validation**: GUID verification and immutability
- **Dependencies**: Task-1
- **Next Task(s)**: Task-8

### Task-4: Link CLI Main Binary to Core
- **Owner**: Codex
- **Phase**: 1
- **Command**: Link `cli/main.c` with all `-lpolycall` libraries to produce `bin/polycall.exe`
- **Target Output**: Executable `polycall.exe`
- **Validation**: Runs with `-c config.polycall`
- **Dependencies**: Task-2
- **Next Task(s)**: Task-5

### Task-5: Generate CLI and Binding Commands
- **Owner**: Codex
- **Phase**: 2
- **Command**: `polycall generate-cli --from-spec` and generate language bindings
- **Target Output**: `cli/commands/`, `bindings/python-polycall/`
- **Validation**: CLI commands map to core functionality
- **Dependencies**: Task-4
- **Next Task(s)**: Task-7, Task-9

### Task-6: Integrate Sinphasé Complexity Governance
- **Owner**: SinphaseBot
- **Phase**: 3
- **Command**: `sinphase check --threshold=0.6`
- **Target Output**: Compliance report with recommendations
- **Validation**: Threshold pass or fail
- **Dependencies**: Task-1
- **Next Task(s)**: Task-10

### Task-7: Generate Modular QA Test Harness
- **Owner**: Codex
- **Phase**: 3
- **Command**: `polycall generate-tests --coverage=95`
- **Target Output**: `__tests__/unit`, `__tests__/integration`
- **Validation**: `make test-all` passes with >95% coverage
- **Dependencies**: Task-5
- **Next Task(s)**: Task-11

### Task-8: Enable Telemetry and Observability
- **Owner**: Telemetron
- **Phase**: 2
- **Command**: Configure Prometheus exporters and Grafana dashboards
- **Target Output**: Real time metrics interface
- **Validation**: Metrics visible within 30 seconds
- **Dependencies**: Task-3
- **Next Task(s)**: Task-12

### Task-9: Binding Schema Generation
- **Owner**: Codex
- **Phase**: 2
- **Command**: `polycall generate-binding --lang={lang}`
- **Target Output**: `bindings/{lang}-polycall/`
- **Validation**: Compiled bindings import successfully
- **Dependencies**: Task-5
- **Next Task(s)**: Task-7

### Task-10: Produce Governance Hardening Report
- **Owner**: SinphaseBot
- **Phase**: 3
- **Command**: `sinphase report --format=markdown`
- **Target Output**: `reports/governance_report.md`
- **Validation**: Report summarises compliance and violations
- **Dependencies**: Task-6
- **Next Task(s)**: Task-12

### Task-11: Run QA and Generate Certificate
- **Owner**: Uche Division QA
- **Phase**: 4
- **Command**: `make test-all && sinphase certify`
- **Target Output**: `reports/qa_certificate.md`
- **Validation**: Certificate attached if all checks pass
- **Dependencies**: Task-7
- **Next Task(s)**: Task-12

### Task-12: Deployment Preparation
- **Owner**: Polybuild
- **Phase**: 4
- **Command**: `polycall deploy --env=staging`
- **Target Output**: Staging deployment with telemetry active
- **Validation**: Health checks pass
- **Dependencies**: Task-8, Task-10, Task-11
- **Next Task(s)**: none

