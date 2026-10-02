#!/bin/sh
# Test orchestrator.
#
#   sh tests/run_all.sh                 every section (make test)
#   sh tests/run_all.sh --only SECTION  one section (one ctest entry each)
#
# Sections: cli model equivalence roundtrip plugins conformance:core
#           conformance:go conformance:java microvm consumers daemon peer
#
# Exit codes: 0 every check ran and passed; 1 a check failed; 77 nothing
# failed but at least one check could NOT run (missing interpreter or
# toolchain). ctest maps 77 to "Skipped" -- a skipped check is never
# reported as passed. In full mode a skip fails `make test` unless
# ALLOW_SKIP=1 is set, and the summary names every skipped section.
#
# Driven by the build systems, which export BUILD_DIR CC EXE_EXT SHARED_EXT
# IS_WINDOWS; falls back to sensible values when run by hand.

set -u
BUILD_DIR=${BUILD_DIR:-build}
CC=${CC:-cc}
EXE_EXT=${EXE_EXT:-}
SHARED_EXT=${SHARED_EXT:-so}
IS_WINDOWS=${IS_WINDOWS:-0}
ONLY=""
if [ "${1:-}" = "--only" ]; then ONLY=${2:?"--only needs a section"}; fi

ROOT=$(pwd)
# absolute: sub-suites cd elsewhere (e.g. `go build -o` runs in the Go module)
mkdir -p "$BUILD_DIR" && BUILD_DIR=$(cd "$BUILD_DIR" && pwd)
# MSYS/Git-Bash: "C:/..." works for both MSYS tools and native programs,
# including paths embedded in selectors (node:/c/... would not be converted)
case "$(uname -s 2>/dev/null)" in
  MINGW*|MSYS*|CYGWIN*) ROOT=$(cygpath -m "$ROOT"); BUILD_DIR=$(cygpath -m "$BUILD_DIR") ;;
esac
case "$ONLY" in
  ""|ffi_unit|fuzz|cli|model|equivalence|roundtrip|plugins|conformance:core|conformance:go|  conformance:java|microvm|consumers|daemon|peer|nsigii) ;;
  *) printf 'FAIL  run_all: unknown section "%s"
' "$ONLY"; exit 1 ;;
esac
BIN="$BUILD_DIR/bin/polycall$EXE_EXT"
LIBDIR="$BUILD_DIR/lib"
TDIR="$BUILD_DIR/tests"
mkdir -p "$TDIR"
export POLYCALL_TELEMETRY_LOG="$TDIR/telemetry.jsonl"
unset POLYCALL_DEV_TOKEN 2>/dev/null || true

# python interpreter name for the CLI `python:` provider + client tests
if [ -z "${POLYCALL_PYTHON:-}" ]; then
  POLYCALL_PYTHON=$(command -v python3 2>/dev/null || command -v python 2>/dev/null || true)
  [ -n "$POLYCALL_PYTHON" ] && export POLYCALL_PYTHON
fi

IS_MSVC=0
case "$(basename "$CC" | tr 'A-Z' 'a-z')" in cl|cl.exe) IS_MSVC=1 ;; esac

pick() { for f in "$@"; do [ -f "$f" ] && { printf '%s\n' "$f"; return 0; }; done; return 1; }
# CMake: DLLs next to executables (bin/), libraries in lib/; Make: all in lib/
STATIC_LIB=$(pick "$LIBDIR/libpolycall.a" "$LIBDIR/polycall_static.lib")
SHARED_LIB=$(pick "$LIBDIR/libpolycall.$SHARED_EXT" "$BUILD_DIR/bin/polycall.$SHARED_EXT" \
                  "$BUILD_DIR/bin/libpolycall.$SHARED_EXT" "$LIBDIR/polycall.$SHARED_EXT")
IMPLIB=$(pick "$LIBDIR/libpolycall.dll.a" "$LIBDIR/polycall.lib")
CPROV=$(pick "$LIBDIR/example_provider.$SHARED_EXT")
export POLYCALL_TEST_SHARED_LIB="$SHARED_LIB"

note() { printf '%s\n' "$*"; }

[ -x "$BIN" ] || { note "FAIL  run_all: missing CLI $BIN"; exit 1; }
[ -n "$STATIC_LIB" ] || { note "FAIL  run_all: no static library under $LIBDIR"; exit 1; }
[ -n "$SHARED_LIB" ] || { note "FAIL  run_all: no shared library under $BUILD_DIR"; exit 1; }

EXPECT_VER=$("$BIN" --version | sed 's/^polycall //' | tr -d '\r')

FAILED=""
SKIPPED=""
# section NAME CMD...: run, echo output, classify pass / fail / skip
section() {
  _name=$1; shift
  case "$ONLY" in ""|"$_name") ;; *) return 0 ;; esac
  note ""; note "### $_name"
  _log="$TDIR/section-$(printf '%s' "$_name" | tr ':/' '__').log"
  "$@" >"$_log" 2>&1
  _rc=$?
  cat "$_log"
  if [ "$_rc" -ne 0 ] && [ "$_rc" -ne 77 ]; then
    FAILED="$FAILED $_name"
    cp "$_log" "$_log.failed-$(date +%Y%m%dT%H%M%S)" 2>/dev/null   # kept: a rerun overwrites $_log
  elif grep -qE '^(FAIL|  FAIL) ' "$_log"; then
    FAILED="$FAILED $_name"
    cp "$_log" "$_log.failed-$(date +%Y%m%dT%H%M%S)" 2>/dev/null
  elif [ "$_rc" -eq 77 ] || grep -qE '^(NOT RUN|SKIP) ' "$_log"; then
    SKIPPED="$SKIPPED $_name"
  fi
}

# build_exe OUT MODE SRC [LANG]  -- MODE: static | shared | dlopen
# The CMake build compiles these programs itself (with the configured
# toolchain, no Developer-shell environment needed): use them when present.
# The GNU Make build has no such targets, so they are compiled here.
build_exe() {
  _out=$1; _mode=$2; _src=$3; _lang=${4:-c}; _log="$_out.log"
  _pre="$BUILD_DIR/bin/$(basename "$_out")"
  if [ -x "$_pre" ]; then
    cp -f "$_pre" "$_out" && : >"$_log"
    return 0
  fi
  if [ "$IS_MSVC" = 1 ]; then
    # cl accepts '-' for every option; using '/' trips MSYS path conversion.
    _def=""; _libs="ws2_32.lib"
    [ "$_mode" = static ] && _libs="\"$STATIC_LIB\" ws2_32.lib"
    [ "$_mode" = shared ] && { _def="-DPOLYCALL_USE_SHARED"; _libs="\"$IMPLIB\" ws2_32.lib"; }
    [ "$_mode" = dlopen ] && _libs=""
    _std="-std:c11"; [ "$_lang" = cxx ] && _std="-std:c++17 -EHsc"
    # -MD: match the dynamic CRT CMake links polycall_static with.
    eval cl -nologo $_std -W3 -MD $_def "-I\"$ROOT/include\"" "\"$_src\"" \
         "-Fe\"$_out\"" "-Fo\"$TDIR/\"" -link $_libs >"$_log" 2>&1
  else
    _cc=$CC; _std="-std=c11"
    if [ "$_lang" = cxx ]; then
      _std="-std=c++17"
      case "$CC" in *clang*) _cc=$(printf '%s' "$CC" | sed 's/clang$/clang++/') ;; *gcc*) _cc=$(printf '%s' "$CC" | sed 's/gcc$/g++/') ;; *) _cc=c++ ;; esac
    fi
    _def=""; _libs="-lpthread"
    [ "$_mode" = static ] && _libs="\"$STATIC_LIB\" $([ "$IS_WINDOWS" = 1 ] && echo -lws2_32 || echo '-lpthread -ldl -lm')"
    [ "$_mode" = shared ] && {
      _def="-DPOLYCALL_USE_SHARED"
      if [ "$IS_WINDOWS" = 1 ]; then _libs="\"$IMPLIB\" -lws2_32"; else _libs="\"$SHARED_LIB\" -lpthread -lm"; fi
    }
    [ "$_mode" = dlopen ] && { [ "$IS_WINDOWS" = 1 ] && _libs="" || _libs="-ldl"; }
    eval "\"$_cc\"" $_std -Wall -Wextra $_def -I"\"$ROOT/include\"" "\"$_src\"" $_libs -o "\"$_out\"" >"$_log" 2>&1
  fi
}

run_model() {
  if build_exe "$TDIR/model_test$EXE_EXT" static "$ROOT/tests/config/model_test.c"; then
    "$TDIR/model_test$EXE_EXT"
  else
    note "FAIL  config2 model_test: compile/link failed"; cat "$TDIR/model_test$EXE_EXT.log"; return 1
  fi
}

run_equivalence() {
  if [ -z "$CPROV" ]; then note "NOT RUN  provider equivalence (example_provider not built)"; return 77; fi
  if ! command -v node >/dev/null 2>&1; then note "NOT RUN  provider equivalence (node not on PATH)"; return 77; fi
  sh "$ROOT/tests/config/equivalence.sh" "$BIN" "$ROOT" "$CPROV"
}

check_ver() { # check_ver NAME EXE [args...]
  _n=$1; _e=$2; shift 2
  _o=$("$_e" "$@" 2>&1 | tr -d '\r'); _rc=$?
  if [ "$_rc" -eq 0 ] && [ "$_o" = "$EXPECT_VER" ]; then note "PASS  consumer $_n -> $_o"
  else note "FAIL  consumer $_n (rc=$_rc out='$_o' want='$EXPECT_VER')"; return 1; fi
}

run_consumers() {
  _f=0
  if build_exe "$TDIR/static_consumer$EXE_EXT" static "$ROOT/tests/consumers/static_consumer.c"; then
    check_ver "static" "$TDIR/static_consumer$EXE_EXT" || _f=1
  else note "FAIL  consumer static: compile/link failed"; cat "$TDIR/static_consumer$EXE_EXT.log"; _f=1; fi

  if build_exe "$TDIR/link_shared_consumer$EXE_EXT" shared "$ROOT/tests/consumers/link_shared_consumer.c"; then
    cp -f "$SHARED_LIB" "$TDIR/" 2>/dev/null || true
    LD_LIBRARY_PATH="$(dirname "$SHARED_LIB"):${LD_LIBRARY_PATH:-}" \
      check_ver "link-shared" "$TDIR/link_shared_consumer$EXE_EXT" || _f=1
  else note "FAIL  consumer link-shared: compile/link failed"; cat "$TDIR/link_shared_consumer$EXE_EXT.log"; _f=1; fi

  if build_exe "$TDIR/dlopen_consumer$EXE_EXT" dlopen "$ROOT/tests/consumers/dlopen_consumer.c"; then
    check_ver "dlopen" "$TDIR/dlopen_consumer$EXE_EXT" "$SHARED_LIB" || _f=1
  else note "FAIL  consumer dlopen: compile/link failed"; cat "$TDIR/dlopen_consumer$EXE_EXT.log"; _f=1; fi

  if [ "${POLYCALL_TEST_CXX:-1}" = 0 ]; then
    note "NOT RUN  consumer c++ (POLYCALL_TEST_CXX=0: no C++ compiler configured)"
  elif build_exe "$TDIR/cxx_consumer$EXE_EXT" shared "$ROOT/tests/consumers/cxx_consumer.cpp" cxx; then
    LD_LIBRARY_PATH="$(dirname "$SHARED_LIB"):${LD_LIBRARY_PATH:-}" \
      check_ver "c++ (shared)" "$TDIR/cxx_consumer$EXE_EXT" || _f=1
  else note "FAIL  consumer c++: compile/link failed"; cat "$TDIR/cxx_consumer$EXE_EXT.log"; _f=1; fi

  if build_exe "$TDIR/compat_consumer$EXE_EXT" static "$ROOT/tests/consumers/compat_consumer.c"; then
    check_ver "deprecated <libpolycall.h> still compiles" "$TDIR/compat_consumer$EXE_EXT" || _f=1
    if [ -s "$TDIR/compat_consumer$EXE_EXT.log" ]; then
      if grep -qi "deprecated" "$TDIR/compat_consumer$EXE_EXT.log"; then
        note "PASS  <libpolycall.h> emits a deprecation notice"
      else
        note "FAIL  <libpolycall.h> did not warn: $(cat "$TDIR/compat_consumer$EXE_EXT.log")"; _f=1
      fi
    fi   # (CMake-built: the notice is checked by the install consumer test)
  else note "FAIL  consumer compat: compile/link failed"; cat "$TDIR/compat_consumer$EXE_EXT.log"; _f=1; fi
  return $_f
}

run_bin() { # run_bin NAME ARGS... : a C test program from <build>/bin
  _b="$BUILD_DIR/bin/$1$EXE_EXT"; shift
  if [ ! -x "$_b" ]; then note "NOT RUN  $_b was not built"; return 77; fi
  LD_LIBRARY_PATH="$(dirname "$SHARED_LIB"):${LD_LIBRARY_PATH:-}" "$_b" "$@"
}

section ffi_unit     run_bin polycall_ffi_unit "$TDIR"
section fuzz         run_bin polycall_fuzz 20000 "$TDIR"
section cli          sh "$ROOT/tests/cli/contract.sh" "$BIN" "$ROOT/tests/fixtures"
section model        run_model
section equivalence  run_equivalence
section roundtrip    sh "$ROOT/tests/runtime/roundtrip.sh" "$BIN" "$ROOT"
section plugins      sh "$ROOT/tests/runtime/plugin.sh" "$BIN" "$ROOT" "$LIBDIR" "$SHARED_EXT"
section conformance:core env CONFORMANCE_ONLY=core sh "$ROOT/tests/conformance/run.sh" "$BIN" "$ROOT" "$LIBDIR" "$SHARED_EXT"
section conformance:go   env CONFORMANCE_ONLY=go   sh "$ROOT/tests/conformance/run.sh" "$BIN" "$ROOT" "$LIBDIR" "$SHARED_EXT"
section conformance:java env CONFORMANCE_ONLY=java sh "$ROOT/tests/conformance/run.sh" "$BIN" "$ROOT" "$LIBDIR" "$SHARED_EXT"
section microvm      sh "$ROOT/tests/microvm/strict_mode.sh" "$BIN" "$ROOT"
section consumers    run_consumers
section daemon       sh "$ROOT/tests/daemon/daemon.sh" "$BIN" "$ROOT"
section peer         sh "$ROOT/tests/peer/p2p.sh" "$BIN" "$ROOT" "$BUILD_DIR/bin/polycall_fault_proxy$EXE_EXT"

run_nsigii() { # Go Alpha (cgo) + Python Beta (ctypes / HTTP fallback), two processes
  _py=$(command -v python3 2>/dev/null || command -v python 2>/dev/null || true)
  if [ -z "$_py" ]; then note "NOT RUN  NSIGII two-process test (python3 not on PATH)"; return 77; fi
  set -- --root "$ROOT" --include "$ROOT/include" --libdir "$LIBDIR" --shared-lib "$SHARED_LIB"          --proxy "$BUILD_DIR/bin/polycall_fault_proxy$EXE_EXT"
  [ "$IS_WINDOWS" = 1 ] && [ -n "$IMPLIB" ] && set -- "$@" --import-lib "$IMPLIB"
  "$_py" "$ROOT/tests/peer/nsigii_test.py" "$@"
}
section nsigii       run_nsigii

note ""
if [ -n "$FAILED" ]; then
  note "### TESTS FAILED:$FAILED"
  [ -n "$SKIPPED" ] && note "### also skipped (not run):$SKIPPED"
  exit 1
fi
if [ -n "$SKIPPED" ]; then
  note "### NOT ALL TESTS RAN -- skipped:$SKIPPED"
  if [ -z "$ONLY" ] && [ "${ALLOW_SKIP:-0}" = 1 ]; then
    note "### (ALLOW_SKIP=1: treating the skips as acceptable for this run)"
    exit 0
  fi
  exit 77
fi
note "### ALL TESTS PASSED${ONLY:+ (section $ONLY)}"
exit 0
