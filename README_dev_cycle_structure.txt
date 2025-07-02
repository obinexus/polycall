# Ad-hoc Dev Cycle Structure for Sinphasé Compliance

This document describes the organization of all ad-hoc scripts in the LibPolyCall project by software development lifecycle (SDLC) stage for TDD and waterfall compliance:

- 01-requirements: Requirements gathering, validation, and initial compliance checks
- 02-design: Design-time scripts, scaffolding, and configuration
- 03-implementation: Implementation, fixing includes/paths, code generation
- 04-testing: Test scripts, test runners, validation
- 05-compliance: Compliance and governance checks
- 06-audit: Audit, reporting, trace, and metrics
- 07-waterfall: Orchestration, full-cycle, and legacy scripts

All `.sh`, `.py`, and related scripts from `scripts/adhoc/` are to be moved into the appropriate folders. The Makefile and setup scripts should reference these new locations for Sinphasé governance and dev-cycle compliance.

Refer to `scripts/adhoc/README_dev_cycle_structure.txt` for details on folder usage and migration steps.
