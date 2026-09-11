#!/bin/sh
# Test orchestrator (all stages).
#   - CLI contract (tests/cli/contract.sh)
#   - config2 typed model unit test
#   - provider equivalence + legacy migration (tests/config/equivalence.sh)
#   - cross-language runtime roundtrip (tests/runtime/roundtrip.sh)
#   - operation plugin loader: run --load (tests/runtime/plugin.sh)
#   - cross-language conformance harness (tests/conformance/run.sh)
#   - MicroVM strict-mode adapter (tests/microvm/strict_mode.sh)
#   - three independent C consumers: static link, import-lib link, dlopen
#
# Driven by the build systems (make test / ctest) which export:
#   BUILD_DIR CC EXE_EXT SHARED_EXT IS_WINDOWS
# Falls back to sensible values when run by hand. Understands GCC/Clang and MSVC.

set -u
BUILD_DIR=${BUILD_DIR:-build}
CC=${CC:-cc}
EXE_EXT=${EXE_EXT:-}
SHARED_EXT=${SHARED_EXT:-so}
IS_WINDOWS=${IS_WINDOWS:-0}

ROOT=$(pwd)
BIN="$BUILD_DIR/bin/polycall$EXE_EXT"
LIBDIR="$BUILD_DIR/lib"
TDIR="$BUILD_DIR/tests"
mkdir -p "$TDIR"

# python interpreter name for the CLI `python:` provider + client tests
if [ -z "${POLYCALL_PYTHON:-}" ]; then
  POLYCALL_PYTHON=$(command -v python3 2>/dev/null || command -v python 2>/dev/null || true)
  [ -n "$POLYCALL_PYTHON" ] && export POLYCALL_PYTHON
fi

IS_MSVC=0
case "$(basename "$CC" | tr 'A-Z' 'a-z')" in cl|cl.exe) IS_MSVC=1 ;; esac

pick() { for f in "$@"; do [ -f "$f" ] && { printf '%s\n' "$f"; return 0; }; done; return 1; }
STATIC_LIB=$(pick "$LIBDIR/libpolycall.a" "$LIBDIR/polycall-static.lib")
SHARED_LIB=$(pick "$LIBDIR/libpolycall.$SHARED_EXT" "$LIBDIR/polycall.$SHARED_EXT")
IMPLIB=$(pick "$LIBDIR/libpolycall.dll.a" "$LIBDIR/polycall.lib")
CPROV=$(pick "$LIBDIR/example_provider.$SHARED_EXT")

fail=0
note() { printf '%s\n' "$*"; }

[ -x "$BIN" ] || { note "run_all: missing CLI $BIN"; exit 1; }
[ -n "$STATIC_LIB" ] || { note "run_all: no static library under $LIBDIR"; exit 1; }
[ -n "$SHARED_LIB" ] || { note "run_all: no shared library under $LIBDIR"; exit 1; }

EXPECT_VER=$("$BIN" --version | sed 's/^polycall //')
note "=== expected library version: $EXPECT_VER  (toolchain: $([ $IS_MSVC = 1 ] && echo MSVC || echo gcc-like)) ==="

# build_exe OUT MODE SRC   -- MODE: static | shared | dlopen
build_exe() {
  _out=$1; _mode=$2; _src=$3; _log="$_out.log"
  if [ "$IS_MSVC" = 1 ]; then
    # cl accepts '-' for every option; using '/' trips MSYS path conversion.
    _def=""; _libs="ws2_32.lib"
    [ "$_mode" = static ] && _libs="\"$STATIC_LIB\" ws2_32.lib"
    [ "$_mode" = shared ] && { _def="-DPOLYCALL_USE_SHARED"; _libs="\"$IMPLIB\" ws2_32.lib"; }
    [ "$_mode" = dlopen ] && _libs=""
    # -MD: match the dynamic CRT CMake links polycall_static with, else the
    # __imp_* CRT stubs in the .lib go unresolved.
    eval cl -nologo -std:c11 -W3 -MD $_def "-I\"$ROOT/include\"" "\"$_src\"" \
         "-Fe\"$_out\"" "-Fo\"$TDIR/\"" -link $_libs >"$_log" 2>&1
  else
    _def=""; _libs="-lpthread"
    [ "$_mode" = static ] && _libs="\"$STATIC_LIB\" $([ "$IS_WINDOWS" = 1 ] && echo -lws2_32 || echo -lpthread)"
    [ "$_mode" = shared ] && {
      _def="-DPOLYCALL_USE_SHARED"
      if [ "$IS_WINDOWS" = 1 ]; then _libs="\"$IMPLIB\" -lws2_32"; else _libs="-L\"$LIBDIR\" -lpolycall -lpthread"; fi
    }
    [ "$_mode" = dlopen ] && { [ "$IS_WINDOWS" = 1 ] && _libs="" || _libs="-ldl"; }
    eval "\"$CC\"" -std=c11 $_def -I"\"$ROOT/include\"" "\"$_src\"" $_libs -o "\"$_out\"" >"$_log" 2>&1
  fi
}

check_ver() { # check_ver NAME EXE [args...]
  _n=$1; _e=$2; shift 2
  _o=$("$_e" "$@" 2>&1); _rc=$?
  if [ "$_rc" -eq 0 ] && [ "$_o" = "$EXPECT_VER" ]; then note "PASS  consumer $_n -> $_o"
  else note "FAIL  consumer $_n (rc=$_rc out='$_o' want='$EXPECT_VER')"; fail=1; fi
}

# ---------------------------------------------------------------- CLI contract
note ""; note "### CLI contract"
sh "$ROOT/tests/cli/contract.sh" "$BIN" "$ROOT/tests/fixtures" || fail=1

# ---------------------------------------------------------------- config2 model unit test
note ""; note "### config2 typed model unit test"
if build_exe "$TDIR/model_test$EXE_EXT" static "$ROOT/tests/config/model_test.c"; then
  "$TDIR/model_test$EXE_EXT" || fail=1
else
  note "FAIL  config2 model_test: compile/link failed"; cat "$TDIR/model_test$EXE_EXT.log"; fail=1
fi

# ---------------------------------------------------------------- provider equivalence
note ""; note "### provider equivalence + migration (Stage 2)"
if [ -n "$CPROV" ] && command -v node >/dev/null 2>&1; then
  sh "$ROOT/tests/config/equivalence.sh" "$BIN" "$ROOT" "$CPROV" || fail=1
elif [ -z "$CPROV" ]; then
  note "NOT RUN  provider equivalence (example_provider not built)"
else
  note "NOT RUN  provider equivalence (node not on PATH)"
fi

# ---------------------------------------------------------------- runtime roundtrip
note ""; note "### cross-language operation dispatch (Stage 3)"
sh "$ROOT/tests/runtime/roundtrip.sh" "$BIN" "$ROOT" || fail=1

# ---------------------------------------------------------------- operation plugin loader
note ""; note "### operation plugin loader: run --load (P1)"
sh "$ROOT/tests/runtime/plugin.sh" "$BIN" "$ROOT" "$LIBDIR" "$SHARED_EXT" || fail=1

# ---------------------------------------------------------------- cross-language conformance
note ""; note "### cross-language conformance harness (P2)"
sh "$ROOT/tests/conformance/run.sh" "$BIN" "$ROOT" "$LIBDIR" "$SHARED_EXT" || fail=1

# ---------------------------------------------------------------- microvm strict mode
note ""; note "### MicroVM strict-mode adapter (Stage 4, optional follow-up)"
sh "$ROOT/tests/microvm/strict_mode.sh" "$BIN" "$ROOT" || fail=1

# ---------------------------------------------------------------- consumers
note ""; note "### consumer 1: static linkage"
if build_exe "$TDIR/static_consumer$EXE_EXT" static "$ROOT/tests/consumers/static_consumer.c"; then
  check_ver "static" "$TDIR/static_consumer$EXE_EXT"
else note "FAIL  consumer static: compile/link failed"; cat "$TDIR/static_consumer$EXE_EXT.log"; fail=1; fi

note ""; note "### consumer 2: import-library / shared linkage"
if [ -z "$IMPLIB" ] && [ "$IS_WINDOWS" = 1 ]; then
  note "NOT RUN  consumer link-shared (no import library)"
elif build_exe "$TDIR/link_shared_consumer$EXE_EXT" shared "$ROOT/tests/consumers/link_shared_consumer.c"; then
  cp -f "$SHARED_LIB" "$TDIR/" 2>/dev/null || true
  LD_LIBRARY_PATH="$LIBDIR:${LD_LIBRARY_PATH:-}" DYLD_LIBRARY_PATH="$LIBDIR:${DYLD_LIBRARY_PATH:-}" \
    check_ver "link-shared" "$TDIR/link_shared_consumer$EXE_EXT"
else note "FAIL  consumer link-shared: compile/link failed"; cat "$TDIR/link_shared_consumer$EXE_EXT.log"; fail=1; fi

note ""; note "### consumer 3: explicit dynamic loading (dlopen / LoadLibrary)"
if build_exe "$TDIR/dlopen_consumer$EXE_EXT" dlopen "$ROOT/tests/consumers/dlopen_consumer.c"; then
  check_ver "dlopen" "$TDIR/dlopen_consumer$EXE_EXT" "$SHARED_LIB"
else note "FAIL  consumer dlopen: compile/link failed"; cat "$TDIR/dlopen_consumer$EXE_EXT.log"; fail=1; fi

note ""
if [ "$fail" -eq 0 ]; then note "### ALL TESTS PASSED"; else note "### TESTS FAILED"; fi
exit $fail
