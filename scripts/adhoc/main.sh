#!/bin/bash
# Ad-hoc Module Orchestration System
# Phase: Build Orchestration

set -e

PROJECT_ROOT="$(cd "$(dirname "$0")/../.." && pwd)"

# Commands
case "${1:-help}" in
    build)
        echo "[ADHOC] Build phase"
        make -C "$PROJECT_ROOT" compile-core compile-cli
        ;;
    test)
        echo "[ADHOC] Test phase"
        make -C "$PROJECT_ROOT" test
        ;;
    qa)
        echo "[ADHOC] QA phase"
        bash "$PROJECT_ROOT/scripts/adhoc/compliance-check.sh"
        ;;
    cycle)
        echo "[ADHOC] Full cycle"
        $0 build && $0 test && $0 qa
        ;;
    *)
        echo "Usage: $0 {build|test|qa|cycle}"
        ;;
esac
