# OBINexus PolyCall Root Makefile
# Fixed non-recursive delegation orchestrator

# Recursion guard
ifndef POLYCALL_MAKEFILE_INCLUDED
POLYCALL_MAKEFILE_INCLUDED := 1

# Version and metadata
VERSION := 0.7.0
BUILD_DATE := $(shell date +%Y%m%d)
BUILD_HASH := $(shell git rev-parse --short HEAD 2>/dev/null || echo "nogit")

# Build configuration
BUILD_MODE ?= release
EDGE_MICRO ?= enabled
SECURITY_LEVEL ?= paranoid

# Directory structure
ROOT_DIR := $(shell pwd)
BUILD_DIR := $(ROOT_DIR)/build
SRC_DIR := $(ROOT_DIR)/src
INCLUDE_DIR := $(ROOT_DIR)/include
TOOLS_DIR := $(ROOT_DIR)/tools

# Export common variables
export VERSION BUILD_DATE BUILD_HASH
export BUILD_MODE EDGE_MICRO SECURITY_LEVEL
export ROOT_DIR BUILD_DIR SRC_DIR INCLUDE_DIR TOOLS_DIR
export POLYCALL_MAKEFILE_INCLUDED

# Default target
.DEFAULT_GOAL := all

# Non-recursive make invocation
SUBMAKE = $(MAKE) --no-print-directory -f

#################
# Main Targets  #
#################

.PHONY: all
all:
	@$(SUBMAKE) Makefile.build all

.PHONY: help
help:
	@echo "OBINexus PolyCall Build System v$(VERSION)"
	@echo "========================================"
	@echo ""
	@echo "Main targets:"
	@echo "  all          - Build everything (default)"
	@echo "  build        - Build core components"
	@echo "  test         - Run test suite"
	@echo "  qa           - Run QA checks"
	@echo "  clean        - Clean build artifacts"
	@echo "  install      - Install polycall"
	@echo ""
	@echo "Subsystem targets:"
	@echo "  Build:       build-* targets (see 'make help-build')"
	@echo "  Purity:      check-*, verify-* targets (see 'make help-purity')"
	@echo "  Testing:     test-*, qa-* targets (see 'make help-spec')"
	@echo "  Vendor:      vendor-*, browser-* targets (see 'make help-vendor')"
	@echo "  Projects:    setup-*, config-* targets (see 'make help-projects')"

#################
# Build Targets #
#################

.PHONY: build build-all build-core build-cli build-edge build-micro
.PHONY: edge-deploy micro-deploy release install clean

build:
	@$(SUBMAKE) Makefile.build build

build-all:
	@$(SUBMAKE) Makefile.build build-all

build-core:
	@$(SUBMAKE) Makefile.build build-core

build-cli:
	@$(SUBMAKE) Makefile.build build-cli

build-edge:
	@$(SUBMAKE) Makefile.build build-edge

build-micro:
	@$(SUBMAKE) Makefile.build build-micro

edge-deploy:
	@$(SUBMAKE) Makefile.build edge-deploy

micro-deploy:
	@$(SUBMAKE) Makefile.build micro-deploy

release:
	@$(SUBMAKE) Makefile.build release

install:
	@$(SUBMAKE) Makefile.build install

clean:
	@$(SUBMAKE) Makefile.build clean

##################
# Purity Targets #
##################

.PHONY: check-commands check-memory check-security check-compliance
.PHONY: verify-auth verify-network verify-protocol
.PHONY: audit-code audit-deps security-scan purity-report clean-audit

check-commands:
	@$(SUBMAKE) Makefile.purity check-commands

check-memory:
	@$(SUBMAKE) Makefile.purity check-memory

check-security:
	@$(SUBMAKE) Makefile.purity check-security

check-compliance:
	@$(SUBMAKE) Makefile.purity check-compliance

verify-auth:
	@$(SUBMAKE) Makefile.purity verify-auth

verify-network:
	@$(SUBMAKE) Makefile.purity verify-network

verify-protocol:
	@$(SUBMAKE) Makefile.purity verify-protocol

audit-code:
	@$(SUBMAKE) Makefile.purity audit-code

audit-deps:
	@$(SUBMAKE) Makefile.purity audit-deps

security-scan:
	@$(SUBMAKE) Makefile.purity security-scan

purity-report:
	@$(SUBMAKE) Makefile.purity purity-report

clean-audit:
	@$(SUBMAKE) Makefile.purity clean-audit

#################
# Spec Targets  #
#################

.PHONY: test qa qa-full qa-quick qa-integration qa-unit
.PHONY: test-all test-unit test-integration test-edge test-micro
.PHONY: test-network test-protocol test-auth test-accessibility
.PHONY: coverage coverage-report benchmark stress-test
.PHONY: validate-api validate-schema clean-test

test:
	@$(SUBMAKE) Makefile.spec test

qa:
	@$(SUBMAKE) Makefile.spec qa

qa-full:
	@$(SUBMAKE) Makefile.spec qa-full

qa-quick:
	@$(SUBMAKE) Makefile.spec qa-quick

qa-integration:
	@$(SUBMAKE) Makefile.spec qa-integration

qa-unit:
	@$(SUBMAKE) Makefile.spec qa-unit

test-all:
	@$(SUBMAKE) Makefile.spec test-all

test-unit:
	@$(SUBMAKE) Makefile.spec test-unit

test-integration:
	@$(SUBMAKE) Makefile.spec test-integration

test-edge:
	@$(SUBMAKE) Makefile.spec test-edge

test-micro:
	@$(SUBMAKE) Makefile.spec test-micro

test-network:
	@$(SUBMAKE) Makefile.spec test-network

test-protocol:
	@$(SUBMAKE) Makefile.spec test-protocol

test-auth:
	@$(SUBMAKE) Makefile.spec test-auth

test-accessibility:
	@$(SUBMAKE) Makefile.spec test-accessibility

coverage:
	@$(SUBMAKE) Makefile.spec coverage

coverage-report:
	@$(SUBMAKE) Makefile.spec coverage-report

benchmark:
	@$(SUBMAKE) Makefile.spec benchmark

stress-test:
	@$(SUBMAKE) Makefile.spec stress-test

validate-api:
	@$(SUBMAKE) Makefile.spec validate-api

validate-schema:
	@$(SUBMAKE) Makefile.spec validate-schema

clean-test:
	@$(SUBMAKE) Makefile.spec clean-test

##################
# Vendor Targets #
##################

.PHONY: vendor-test test-all-browsers test-chrome test-firefox test-safari test-edge
.PHONY: browser-matrix wasm-build serve-demo clean-vendor

vendor-test:
	@$(SUBMAKE) Makefile.vendor vendor-test

test-all-browsers:
	@$(SUBMAKE) Makefile.vendor test-all-browsers

test-chrome:
	@$(SUBMAKE) Makefile.vendor test-chrome

test-firefox:
	@$(SUBMAKE) Makefile.vendor test-firefox

test-safari:
	@$(SUBMAKE) Makefile.vendor test-safari

test-edge:
	@$(SUBMAKE) Makefile.vendor test-edge

browser-matrix:
	@$(SUBMAKE) Makefile.vendor browser-matrix

wasm-build:
	@$(SUBMAKE) Makefile.vendor wasm-build

serve-demo:
	@$(SUBMAKE) Makefile.vendor serve-demo

clean-vendor:
	@$(SUBMAKE) Makefile.vendor clean-vendor

####################
# Project Targets  #
####################

.PHONY: setup setup-dev setup-prod config-edge config-micro
.PHONY: generate-docs update-schemas validate-config init-workspace
.PHONY: clean-config help-projects

setup:
	@$(SUBMAKE) Makefile.projects setup

setup-dev:
	@$(SUBMAKE) Makefile.projects setup-dev

setup-prod:
	@$(SUBMAKE) Makefile.projects setup-prod

config-edge:
	@$(SUBMAKE) Makefile.projects config-edge

config-micro:
	@$(SUBMAKE) Makefile.projects config-micro

generate-docs:
	@$(SUBMAKE) Makefile.projects generate-docs

update-schemas:
	@$(SUBMAKE) Makefile.projects update-schemas

validate-config:
	@$(SUBMAKE) Makefile.projects validate-config

init-workspace:
	@$(SUBMAKE) Makefile.projects init-workspace

clean-config:
	@$(SUBMAKE) Makefile.projects clean-config

help-projects:
	@$(SUBMAKE) Makefile.projects help

###################
# Help Subsystems #
###################

.PHONY: help-build help-purity help-spec help-vendor

help-build:
	@$(SUBMAKE) Makefile.build help

help-purity:
	@$(SUBMAKE) Makefile.purity help

help-spec:
	@$(SUBMAKE) Makefile.spec help

help-vendor:
	@$(SUBMAKE) Makefile.vendor help

########################
# Compound Operations  #
########################

.PHONY: full-build full-test full-clean verify-all

full-build: clean build test qa
	@echo "Full build completed successfully"

full-test: test-all qa-full coverage-report
	@echo "Full test suite completed"

full-clean: clean clean-test clean-audit clean-vendor clean-config
	@echo "Full cleanup completed"

verify-all: check-commands security-scan validate-api validate-schema
	@echo "All verifications passed"

# End recursion guard
endif