# PolyCall - standalone GNU Make build
# =====================================
# This build is independent of CMake. It never invokes cmake and cmake never
# invokes it. Recipes target a POSIX shell (Linux, macOS, *BSD, and the MSYS2
# shells on Windows). Native cmd.exe / PowerShell users: use the CMake build.
#
# Common overrides:
#   CC AR CPPFLAGS CFLAGS LDFLAGS LDLIBS   - toolchain / flags
#   BUILD_DIR                              - isolated output root (per toolchain)
#   PREFIX DESTDIR                         - install location
#   BUILD_TYPE=debug|release               - optimisation / asserts
#
# Primary targets: all static shared cli test install clean help

# ---- require a POSIX shell -------------------------------------------------
# GnuWin32 make under cmd.exe / PowerShell has no sh, rm, mkdir -p, tr, ...
# Native Windows without an MSYS2 / Git-Bash shell must use the CMake build.
ifeq (,$(shell uname 2>/dev/null))
$(error This Makefile needs a POSIX shell (Linux, macOS, *BSD, WSL, MSYS2 or Git-Bash). On native Windows without one, use CMake: "cmake -S . -B build/win -DBUILD_TESTING=ON && cmake --build build/win" or ./build-windows.ps1 -- see docs/PLATFORM_REPORT.md)
endif

# ---- toolchain ------------------------------------------------------------
# Respect an explicit CC=...; otherwise prefer cc, fall back to gcc.
ifeq ($(origin CC),default)
  CC := $(shell command -v cc >/dev/null 2>&1 && echo cc || echo gcc)
endif
AR      ?= ar
INSTALL ?= install

BUILD_TYPE ?= release
ifeq ($(BUILD_TYPE),debug)
  OPT_FLAGS := -g -O0 -DDEBUG
else ifeq ($(BUILD_TYPE),release)
  OPT_FLAGS := -O2 -DNDEBUG
else
  $(error BUILD_TYPE must be 'debug' or 'release', got '$(BUILD_TYPE)')
endif

WARN_FLAGS := -Wall -Wextra
# C11 dialect. GCC/Clang use gnu11 so POSIX APIs the code needs (usleep,
# ssize_t, nanosleep, poll, posix_spawn, ...) are visible without scattering
# feature-test macros; MSVC uses /std:c11 via the CMake build.
STD_FLAGS  := -std=gnu11
PIC_FLAGS  := -fPIC

CPPFLAGS ?=
CFLAGS   ?=
LDFLAGS  ?=
LDLIBS   ?=

# ---- platform -----------------------------------------------------------
UNAME_S := $(shell uname -s 2>/dev/null || echo unknown)
CC_TARGET := $(shell $(CC) -dumpmachine 2>/dev/null || echo unknown)

IS_WINDOWS := 0
ifneq (,$(findstring mingw,$(CC_TARGET)))
  IS_WINDOWS := 1
endif
ifneq (,$(findstring cygwin,$(CC_TARGET)))
  IS_WINDOWS := 1
endif
ifneq (,$(findstring MINGW,$(UNAME_S)))
  IS_WINDOWS := 1
endif
ifneq (,$(findstring MSYS,$(UNAME_S)))
  IS_WINDOWS := 1
endif

ifeq ($(IS_WINDOWS),1)
  PLATFORM    := windows
  SHARED_EXT  := dll
  EXE_EXT     := .exe
  SHARED_LDFLAGS := -shared
  PLATFORM_LDLIBS := -lws2_32
  DEFINE_OS   := -D_WIN32 -D__USE_MINGW_ANSI_STDIO=1
else ifeq ($(UNAME_S),Darwin)
  PLATFORM    := macos
  SHARED_EXT  := dylib
  EXE_EXT     :=
  SHARED_LDFLAGS := -dynamiclib
  PLATFORM_LDLIBS := -lpthread
  DEFINE_OS   :=
else
  PLATFORM    := $(shell echo $(UNAME_S) | tr A-Z a-z)
  SHARED_EXT  := so
  EXE_EXT     :=
  SHARED_LDFLAGS := -shared
  PLATFORM_LDLIBS := -lpthread -ldl
  DEFINE_OS   :=
endif

# ---- layout -----------------------------------------------------------
SRC_DIR   := src
CLI_DIR   := src/cli
INC_DIR   := include
BUILD_DIR ?= build
OBJ_DIR   := $(BUILD_DIR)/obj
LIB_DIR   := $(BUILD_DIR)/lib
BIN_DIR   := $(BUILD_DIR)/bin

LIB_NAME    := libpolycall
STATIC_LIB  := $(LIB_DIR)/$(LIB_NAME).a
SHARED_LIB  := $(LIB_DIR)/$(LIB_NAME).$(SHARED_EXT)
IMPLIB      := $(LIB_DIR)/$(LIB_NAME).dll.a
EXECUTABLE  := $(BIN_DIR)/polycall$(EXE_EXT)

# core library sources: src/*.c (minus main) + src/config/*.c + src/runtime/*.c
CORE_SRCS := $(filter-out $(SRC_DIR)/main.c,$(wildcard $(SRC_DIR)/*.c)) \
             $(wildcard $(SRC_DIR)/config/*.c) \
             $(wildcard $(SRC_DIR)/runtime/*.c)
CLI_SRCS  := $(wildcard $(CLI_DIR)/*.c)
MAIN_SRC  := $(SRC_DIR)/main.c

CORE_OBJS := $(CORE_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)
# shared objects are compiled separately (different export macro)
CORE_SHOBJS := $(CORE_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/shared/%.o)
CLI_OBJS  := $(CLI_SRCS:$(CLI_DIR)/%.c=$(OBJ_DIR)/cli/%.o)
MAIN_OBJ  := $(OBJ_DIR)/main.o

DEPS := $(CORE_OBJS:.o=.d) $(CORE_SHOBJS:.o=.d) $(CLI_OBJS:.o=.d) $(MAIN_OBJ:.o=.d)

# ---- build metadata handed to `polycall doctor` ------------------------
CC_VERSION := $(shell $(CC) -dumpversion 2>/dev/null || echo unknown)
BUILD_DEFS := \
  -DPOLYCALL_BUILD_SYSTEM=\"gnumake\" \
  -DPOLYCALL_BUILD_TYPE=\"$(BUILD_TYPE)\" \
  -DPOLYCALL_BUILD_CC=\"$(notdir $(CC))\" \
  -DPOLYCALL_BUILD_TARGET=\"$(CC_TARGET)\" \
  -DPOLYCALL_BUILD_CFLAGS=\"$(BUILD_TYPE)\ $(CC_TARGET)\"

ALL_CPPFLAGS := -I$(INC_DIR) $(DEFINE_OS) $(CPPFLAGS)
ALL_CFLAGS   := $(STD_FLAGS) $(WARN_FLAGS) $(OPT_FLAGS) $(PIC_FLAGS) $(CFLAGS)
ALL_LDLIBS   := $(LDLIBS) $(PLATFORM_LDLIBS)

# ---- install layout -------------------------------------------------
PREFIX  ?= /usr/local
DESTDIR ?=
bindir  := $(PREFIX)/bin
libdir  := $(PREFIX)/lib
incdir  := $(PREFIX)/include/polycall

# ---- top-level targets -------------------------------------------------
.PHONY: all
all: static shared cli

.PHONY: static
static: $(STATIC_LIB)

.PHONY: shared
shared: $(SHARED_LIB)

.PHONY: cli
cli: $(EXECUTABLE)

# example in-process C configuration provider (loaded via provider path)
C_PROVIDER := $(LIB_DIR)/example_provider.$(SHARED_EXT)
.PHONY: provider-c
provider-c: $(C_PROVIDER)

$(C_PROVIDER): bindings/c-provider/example_provider.c $(STATIC_LIB) | $(LIB_DIR)
	$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) $(SHARED_LDFLAGS) -o $@ $< \
	  $(STATIC_LIB) $(LDFLAGS) $(ALL_LDLIBS)

# fixture operation plugin for `polycall run --load` (P1): the normal build
# and a deliberately-mismatched-ABI build from the same source file.
FIXTURE_PLUGIN     := $(LIB_DIR)/fixture_ops_plugin.$(SHARED_EXT)
FIXTURE_PLUGIN_BAD := $(LIB_DIR)/fixture_ops_plugin_bad_abi.$(SHARED_EXT)
.PHONY: plugin-fixture
plugin-fixture: $(FIXTURE_PLUGIN) $(FIXTURE_PLUGIN_BAD)

$(FIXTURE_PLUGIN): tests/fixtures/plugins/fixture_ops_plugin.c $(STATIC_LIB) | $(LIB_DIR)
	$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) $(SHARED_LDFLAGS) -o $@ $< \
	  $(STATIC_LIB) $(LDFLAGS) $(ALL_LDLIBS)

$(FIXTURE_PLUGIN_BAD): tests/fixtures/plugins/fixture_ops_plugin.c $(STATIC_LIB) | $(LIB_DIR)
	$(CC) $(ALL_CPPFLAGS) -DFIXTURE_ABI_MAJOR=999 $(ALL_CFLAGS) $(SHARED_LDFLAGS) -o $@ $< \
	  $(STATIC_LIB) $(LDFLAGS) $(ALL_LDLIBS)

# flagship demo plugin (P3): ledger.balance / ledger.transfer, loaded via
# `polycall run --load` -- see examples/ledger/demo.sh.
LEDGER_PLUGIN := $(LIB_DIR)/ledger_plugin.$(SHARED_EXT)
.PHONY: examples-ledger
examples-ledger: $(LEDGER_PLUGIN)

$(LEDGER_PLUGIN): examples/ledger/ledger_plugin.c $(STATIC_LIB) | $(LIB_DIR)
	$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) $(SHARED_LDFLAGS) -o $@ $< \
	  $(STATIC_LIB) $(LDFLAGS) $(ALL_LDLIBS)

.DEFAULT_GOAL := all

# ---- directories (order-only) ----------------------------------------
$(OBJ_DIR) $(OBJ_DIR)/config $(OBJ_DIR)/runtime \
$(OBJ_DIR)/shared $(OBJ_DIR)/shared/config $(OBJ_DIR)/shared/runtime \
$(OBJ_DIR)/cli $(LIB_DIR) $(BIN_DIR):
	@mkdir -p $@

# ---- toolchain fingerprint -----------------------------------------
# Changing compiler / target / flags drops incompatible objects before a build.
STAMP := $(BUILD_DIR)/.toolchain
.PHONY: FORCE
FORCE:
$(STAMP): FORCE | $(OBJ_DIR)
	@printf '%s\n' \
	  "cc=$(CC)" "cc_version=$(CC_VERSION)" "target=$(CC_TARGET)" \
	  "platform=$(PLATFORM)" "type=$(BUILD_TYPE)" \
	  "cppflags=$(ALL_CPPFLAGS)" "cflags=$(ALL_CFLAGS)" > $@.tmp
	@if [ ! -f $@ ] || ! cmp -s $@ $@.tmp; then \
	  echo "toolchain fingerprint changed - clearing $(OBJ_DIR)"; \
	  rm -rf $(OBJ_DIR); \
	  mkdir -p $(OBJ_DIR) $(OBJ_DIR)/config $(OBJ_DIR)/runtime \
	           $(OBJ_DIR)/shared $(OBJ_DIR)/shared/config $(OBJ_DIR)/shared/runtime \
	           $(OBJ_DIR)/cli; \
	  mv $@.tmp $@; \
	else rm -f $@.tmp; fi

# ---- compile rules --------------------------------------------------
# Static pattern rules (not implicit) so the three object sets never shadow
# one another: obj/cli/foo.o must not be captured by an obj/%.o pattern.

# core objects for the static library and the executable
$(CORE_OBJS): $(OBJ_DIR)/%.o: $(SRC_DIR)/%.c $(STAMP) | $(OBJ_DIR) $(OBJ_DIR)/config $(OBJ_DIR)/runtime
	$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -MMD -MP -c $< -o $@

# the process entry point
$(MAIN_OBJ): $(SRC_DIR)/main.c $(STAMP) | $(OBJ_DIR)
	$(CC) $(ALL_CPPFLAGS) $(ALL_CFLAGS) -MMD -MP -c $< -o $@

# CLI objects carry the build-info defines consumed by `polycall doctor`
$(CLI_OBJS): $(OBJ_DIR)/cli/%.o: $(CLI_DIR)/%.c $(STAMP) | $(OBJ_DIR)/cli
	$(CC) $(ALL_CPPFLAGS) $(BUILD_DEFS) $(ALL_CFLAGS) -MMD -MP -c $< -o $@

# shared-library objects: same core sources, DLL export macro on
$(CORE_SHOBJS): $(OBJ_DIR)/shared/%.o: $(SRC_DIR)/%.c $(STAMP) | $(OBJ_DIR)/shared $(OBJ_DIR)/shared/config $(OBJ_DIR)/shared/runtime
	$(CC) $(ALL_CPPFLAGS) -DPOLYCALL_BUILD_SHARED $(ALL_CFLAGS) -MMD -MP -c $< -o $@

# ---- library / executable link -----------------------------------
$(STATIC_LIB): $(CORE_OBJS) | $(LIB_DIR)
	$(AR) rcs $@ $(CORE_OBJS)

$(SHARED_LIB): $(CORE_SHOBJS) | $(LIB_DIR)
ifeq ($(IS_WINDOWS),1)
	$(CC) $(SHARED_LDFLAGS) -o $@ $(CORE_SHOBJS) \
	  -Wl,--out-implib,$(IMPLIB) $(LDFLAGS) $(ALL_LDLIBS)
else ifeq ($(PLATFORM),macos)
	$(CC) $(SHARED_LDFLAGS) -install_name @rpath/$(notdir $@) \
	  -o $@ $(CORE_SHOBJS) $(LDFLAGS) $(ALL_LDLIBS)
else
	$(CC) $(SHARED_LDFLAGS) -Wl,-soname,$(notdir $@) \
	  -o $@ $(CORE_SHOBJS) $(LDFLAGS) $(ALL_LDLIBS)
endif

# The CLI links the core statically for a self-contained tool.
$(EXECUTABLE): $(MAIN_OBJ) $(CLI_OBJS) $(STATIC_LIB) | $(BIN_DIR)
	$(CC) $(MAIN_OBJ) $(CLI_OBJS) $(STATIC_LIB) -o $@ $(LDFLAGS) $(ALL_LDLIBS)

# ---- tests --------------------------------------------------------
.PHONY: test
test: cli shared static provider-c plugin-fixture
	@BUILD_DIR="$(BUILD_DIR)" CC="$(CC)" EXE_EXT="$(EXE_EXT)" \
	  SHARED_EXT="$(SHARED_EXT)" IS_WINDOWS="$(IS_WINDOWS)" \
	  sh tests/run_all.sh

# ---- install (honours PREFIX and DESTDIR; no ldconfig) ------------
.PHONY: install
install: all
	@mkdir -p "$(DESTDIR)$(bindir)" "$(DESTDIR)$(libdir)" "$(DESTDIR)$(incdir)"
	$(INSTALL) -m 0644 $(INC_DIR)/polycall.h $(INC_DIR)/polycall_export.h \
	  $(INC_DIR)/polycall_cli.h $(INC_DIR)/polycall_config2.h \
	  $(INC_DIR)/polycall_provider.h $(INC_DIR)/polycall_runtime.h \
	  "$(DESTDIR)$(incdir)/"
	$(INSTALL) -m 0644 $(STATIC_LIB) "$(DESTDIR)$(libdir)/"
	$(INSTALL) -m 0755 $(SHARED_LIB) "$(DESTDIR)$(libdir)/"
ifeq ($(IS_WINDOWS),1)
	$(INSTALL) -m 0644 $(IMPLIB) "$(DESTDIR)$(libdir)/"
endif
	$(INSTALL) -m 0755 $(EXECUTABLE) "$(DESTDIR)$(bindir)/"
	@echo "installed to $(DESTDIR)$(PREFIX)"

.PHONY: uninstall
uninstall:
	rm -f "$(DESTDIR)$(bindir)/polycall$(EXE_EXT)"
	rm -f "$(DESTDIR)$(libdir)/$(LIB_NAME).a" \
	      "$(DESTDIR)$(libdir)/$(LIB_NAME).$(SHARED_EXT)" \
	      "$(DESTDIR)$(libdir)/$(LIB_NAME).dll.a"
	rm -rf "$(DESTDIR)$(incdir)"

# ---- housekeeping ----------------------------------------------
.PHONY: clean
clean:
	rm -rf $(BUILD_DIR)

.PHONY: help
help:
	@echo "PolyCall GNU Make build"
	@echo
	@echo "targets:"
	@echo "  all       static library + shared library + CLI (default)"
	@echo "  static    $(STATIC_LIB)"
	@echo "  shared    $(SHARED_LIB)$(if $(filter 1,$(IS_WINDOWS)), (+ $(IMPLIB)),)"
	@echo "  cli       $(EXECUTABLE)"
	@echo "  test      build the above and run tests/run_all.sh"
	@echo "  install   copy headers/libs/CLI under \$$(DESTDIR)\$$(PREFIX)"
	@echo "  clean     remove \$$(BUILD_DIR) ($(BUILD_DIR))"
	@echo
	@echo "options: CC AR CPPFLAGS CFLAGS LDFLAGS LDLIBS BUILD_DIR BUILD_TYPE PREFIX DESTDIR"
	@echo "current: CC=$(CC) target=$(CC_TARGET) platform=$(PLATFORM) BUILD_TYPE=$(BUILD_TYPE) BUILD_DIR=$(BUILD_DIR)"

-include $(DEPS)
