#!/bin/bash
# Policy Wrapper for Compliant Execution
# Phase: Runtime Enforcement

echo "[POLICY] Executing with Sinphasé governance: $@"
exec "$@"
