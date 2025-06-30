# Polkov V2 Task Schema

The following task schema outlines executable steps for Polkov V2.

## Task-1: Audit LibPolyCall v2.0.0 Modules
- **Owner**: Claude
- **Phase**: 1
- **Command**: `analyze source under /core/{feature}`
- **Target Output**: `docs/complexity_matrix.md`
- **Validation**: Complexity and dependency scores
- **Dependencies**: None
- **Next Tasks**: Task-3, Task-6

## Task-2: Generate Build System for Core Modules
- **Owner**: Polybuild
- **Phase**: 1
- **Command**: CMakeLists.txt and make scripts
- **Target Output**: `build/{release,debug}/{lib,obj}`
- **Validation**: Linkage with cli/main.c
- **Dependencies**: Task-1
- **Next Tasks**: Task-4

## Task-3: Setup Constitutional Zero-Trust Configs
- **Owner**: Claude
- **Phase**: 1
- **Command**: Lock `config.polycall` and enforce `.pcfrc`
- **Target Output**: `.polycallrc`
- **Validation**: GUID verification
- **Dependencies**: Task-1
- **Next Tasks**: Task-8

## Task-4: Link CLI Main Binary to Core
- **Owner**: Codex
- **Phase**: 1
- **Command**: Link `cli/main.c` with `-lpolycall`
- **Target Output**: `bin/polycall.exe`
- **Validation**: Executes with `-c config.polycall`
- **Dependencies**: Task-2
- **Next Tasks**: Task-5

## Task-5: Generate CLI + Binding Commands
- **Owner**: Codex
- **Phase**: 2
- **Command**: `polycall generate-cli --from-spec`
- **Target Output**: `cli/commands/`, `bindings/python-polycall/`
- **Validation**: CLI tools run
- **Dependencies**: Task-4
- **Next Tasks**: Task-7, Task-9

## Task-6: Integrate Sinphasé Complexity Governance
- **Owner**: SinphaseBot
- **Phase**: 3
- **Command**: `sinphase check --threshold=0.6`
- **Target Output**: compliance report
- **Validation**: threshold pass/fail
- **Dependencies**: Task-1
- **Next Tasks**: Task-10

## Task-7: Generate Modular QA Test Harness
- **Owner**: Codex
- **Phase**: 3
- **Command**: `polycall generate-tests --coverage=95`
- **Target Output**: `__tests__/unit`, `__tests__/integration`
- **Validation**: `make test-all`
- **Dependencies**: Task-5
- **Next Tasks**: Task-11

## Task-8: Enable Telemetry and Observability
- **Owner**: Telemetron
- **Phase**: 2
- **Command**: Configure Prometheus, GUID tracing
- **Target Output**: Grafana dashboards
- **Validation**: Data visible in 30s
- **Dependencies**: Task-3
- **Next Tasks**: Task-12
