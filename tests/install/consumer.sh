#!/bin/sh
# Install -> relocate -> consume. Proves the installed artifacts alone are
# enough: headers, libraries, CMake package files and pkg-config metadata,
# after the install tree has been MOVED to a different prefix.
#
# Usage: consumer.sh <build-dir> <source-root> [config]
#   (generator, platform and compiler are taken from <build-dir>/CMakeCache.txt
#    so the consumer is built with the same toolchain as the library)
set -u
B=${1:?build dir}
SRC=${2:?source root}
CFG=${3:-Release}
PASS=0; FAIL=0; SKIP=0
ok()   { PASS=$((PASS+1)); printf 'PASS  %s\n' "$1"; }
bad()  { FAIL=$((FAIL+1)); printf 'FAIL  %s\n' "$1"; }
skip() { SKIP=$((SKIP+1)); printf 'NOT RUN  %s\n' "$1"; }

WIN=0
case "$(uname -s 2>/dev/null)" in MINGW*|MSYS*|CYGWIN*) WIN=1 ;; esac
cache() { sed -n "s/^$1:[A-Z]*=//p" "$B/CMakeCache.txt" | head -1; }
GEN=$(cache CMAKE_GENERATOR)
PLAT=$(cache CMAKE_GENERATOR_PLATFORM)
CC_=$(cache CMAKE_C_COMPILER)
CXX_=$(cache CMAKE_CXX_COMPILER)
IS_MSVC=0; case "$CC_" in *cl.exe|*/cl) IS_MSVC=1 ;; esac
[ "$GEN" = "Visual Studio 17 2022" ] || [ "$GEN" = "Visual Studio 18 2026" ] && IS_MSVC=1

TMP=$(mktemp -d 2>/dev/null || echo "${TMPDIR:-/tmp}/pc_inst.$$"); mkdir -p "$TMP"
[ "$WIN" = 1 ] && TMP=$(cygpath -m "$TMP")
trap 'rm -rf "$TMP"' EXIT
A="$TMP/prefix-a"; R="$TMP/relocated/prefix-b"

echo "=== install ($GEN${PLAT:+ $PLAT}, $CFG) ==="
cmake --install "$B" --config "$CFG" --prefix "$A" >"$TMP/install.log" 2>&1 \
  && ok "cmake --install to $A" || { bad "install failed: $(cat "$TMP/install.log")"; echo "--- install: $PASS passed, $FAIL failed ---"; exit 1; }

LIBD=lib
[ -d "$A/lib64" ] && LIBD=lib64
[ -d "$A/lib/x86_64-linux-gnu" ] && LIBD=lib/x86_64-linux-gnu
for f in include/polycall/polycall.h include/polycall/polycall_export.h include/polycall/libpolycall.h \
         "$LIBD/cmake/polycall/polycallConfig.cmake" "$LIBD/cmake/polycall/polycallConfigVersion.cmake" \
         "$LIBD/cmake/polycall/polycallTargets.cmake" "$LIBD/pkgconfig/polycall.pc"; do
  [ -f "$A/$f" ] && ok "installed $f" || bad "missing $f"
done
if [ "$WIN" = 1 ]; then
  if [ "$IS_MSVC" = 1 ]; then
    for f in bin/polycall.dll bin/polycall.exe lib/polycall.lib lib/polycall_static.lib; do
      [ -f "$A/$f" ] && ok "installed $f" || bad "missing $f"
    done
    [ ! -f "$A/lib/polycall-static.lib" ] && ok "no colliding polycall-static.lib" || bad "old static name installed"
  else
    for f in bin/libpolycall.dll bin/polycall.exe lib/libpolycall.dll.a lib/libpolycall.a; do
      [ -f "$A/$f" ] && ok "installed $f (MinGW naming)" || bad "missing $f"
    done
    [ ! -f "$A/lib/polycall.lib" ] && ok "no GNU archive mislabelled as an MSVC .lib" || bad "found polycall.lib in a MinGW install"
  fi
else
  for f in "$LIBD/libpolycall.so" "$LIBD/libpolycall.so.1" "$LIBD/libpolycall.a" bin/polycall; do
    [ -e "$A/$f" ] && ok "installed $f" || bad "missing $f"
  done
  [ -L "$A/$LIBD/libpolycall.so" ] && [ -L "$A/$LIBD/libpolycall.so.1" ] \
    && ok "libpolycall.so -> .so.1 -> real file symlink chain" || bad "SONAME symlinks"
fi

echo "=== no build-machine paths in the installed tree ==="
SRCABS=$(cd "$SRC" && pwd); BABS=$(cd "$B" && pwd)
leaks=""
for p in "$SRCABS" "$BABS" $( [ "$WIN" = 1 ] && printf '%s %s' "$(cygpath -m "$SRCABS")" "$(cygpath -m "$BABS")" ); do
  hits=$(grep -rlaF "$p" "$A" 2>/dev/null | head -5)
  [ -n "$hits" ] && leaks="$leaks $hits"
  if [ "$WIN" = 1 ]; then
    wp=$(cygpath -w "$p" 2>/dev/null)
    hits=$(grep -rlaF "$wp" "$A" 2>/dev/null | head -5)
    [ -n "$hits" ] && leaks="$leaks $hits"
  fi
done
[ -z "$leaks" ] && ok "no source/build directory path embedded in any installed file" \
  || bad "absolute build paths leaked into:$leaks"

echo "=== relocate the install tree ==="
mkdir -p "$TMP/relocated"
mv "$A" "$R" && [ ! -e "$A" ] && ok "moved $A -> $R" || bad "relocation move"

echo "=== external CMake consumer (find_package, installed files only) ==="
CB="$TMP/consumer-build"
set -- -S "$SRC/tests/install/consumer" -B "$CB" -G "$GEN" -DCMAKE_PREFIX_PATH="$R"
[ -n "$PLAT" ] && set -- "$@" -A "$PLAT"
case "$GEN" in Visual*) ;; *) set -- "$@" -DCMAKE_BUILD_TYPE="$CFG" -DCMAKE_C_COMPILER="$CC_"
  [ -n "$CXX_" ] && [ "$CXX_" != "CMAKE_CXX_COMPILER-NOTFOUND" ] && set -- "$@" -DCMAKE_CXX_COMPILER="$CXX_" ;; esac
if cmake "$@" >"$TMP/cfg.log" 2>&1; then ok "find_package(polycall 1.1 COMPONENTS shared static) from the relocated prefix"
else bad "consumer configure: $(tail -20 "$TMP/cfg.log")"; fi
if cmake --build "$CB" --config "$CFG" >"$TMP/build.log" 2>&1; then ok "consumers compiled and linked (C shared, C static, C++, compat)"
else bad "consumer build: $(grep -iE 'error|undefined' "$TMP/build.log" | head -10)"; fi
grep -qi "deprecated" "$TMP/build.log" && ok "<libpolycall.h> emits its deprecation notice" || bad "no deprecation notice for <libpolycall.h>"

exe() { for d in "$CB/$CFG" "$CB"; do [ -f "$d/$1$( [ "$WIN" = 1 ] && echo .exe)" ] && { printf '%s\n' "$d/$1$( [ "$WIN" = 1 ] && echo .exe)"; return; }; done; }
WANT=$(sed -n 's/^#define POLYCALL_ABI_VERSION_STRING "\(.*\)"/\1/p' "$R/include/polycall/polycall_export.h")
run_c() { # run_c NAME LABEL
  _e=$(exe "$1")
  if [ -z "$_e" ]; then bad "$2: not built"; return; fi
  if [ "$WIN" = 1 ]; then _o=$(PATH="$R/bin:$PATH" "$_e" 2>&1 | tr -d '\r')
  else _o=$(LD_LIBRARY_PATH="$R/$LIBD" "$_e" 2>&1); fi
  [ "$_o" = "$WANT" ] && ok "$2 -> $_o" || bad "$2 (got '$_o', want '$WANT')"
}
run_c consumer_c_shared "C consumer, shared library (peer exchange over loopback)"
run_c consumer_c_static "C consumer, static library"
run_c consumer_cxx "C++ consumer"
run_c consumer_compat "consumer of the deprecated header"

echo "=== runtime dependency loading / file formats ==="
if [ "$WIN" = 1 ] && [ "$IS_MSVC" = 1 ]; then
  DUMPBIN=$(ls "/c/Program Files (x86)/Microsoft Visual Studio/"*/BuildTools/VC/Tools/MSVC/*/bin/Hostx64/x64/dumpbin.exe \
                "/c/Program Files/Microsoft Visual Studio/"*/*/VC/Tools/MSVC/*/bin/Hostx64/x64/dumpbin.exe 2>/dev/null | head -1)
  if [ -n "$DUMPBIN" ]; then
    "$DUMPBIN" -nologo -dependents "$(exe consumer_c_shared)" | grep -qi "polycall.dll" \
      && ok "shared consumer imports polycall.dll (linked via polycall.lib)" || bad "shared consumer does not import polycall.dll"
    "$DUMPBIN" -nologo -dependents "$(exe consumer_c_static)" | grep -qi "polycall.dll" \
      && bad "static consumer still imports polycall.dll" || ok "static consumer has no polycall.dll dependency"
    "$DUMPBIN" -nologo -headers "$R/bin/polycall.dll" | grep -q "8664 machine (x64)" && ok "polycall.dll is PE32+ x64" || bad "polycall.dll machine"
    "$DUMPBIN" -nologo -headers "$R/lib/polycall.lib" 2>/dev/null | grep -qi "DLL name *: polycall.dll" \
      && ok "polycall.lib is the import library for polycall.dll" || bad "polycall.lib is not an import library"
    "$DUMPBIN" -nologo -linkermember:1 "$R/lib/polycall_static.lib" 2>/dev/null | grep -q "polycall_get_version" \
      && ok "polycall_static.lib is a static library with the code" || bad "polycall_static.lib content"
    n=$("$DUMPBIN" -nologo -exports "$R/bin/polycall.dll" | awk '/ordinal hint/{f=1;next} f&&NF>=4{print $4}' | sort | tr -d '\r' | diff - "$SRC/abi/polycall.symbols" >/dev/null && echo same)
    [ "$n" = same ] && ok "polycall.dll exports exactly abi/polycall.symbols" || bad "DLL export set differs from abi/polycall.symbols"
  else
    skip "PE inspection (dumpbin not found)"
  fi
elif [ "$WIN" = 1 ]; then
  OBJDUMP=$(command -v objdump)
  if [ -n "$OBJDUMP" ]; then
    "$OBJDUMP" -p "$(exe consumer_c_shared)" | grep -qi "DLL Name: libpolycall.dll" \
      && ok "shared consumer imports libpolycall.dll (via libpolycall.dll.a)" || bad "shared consumer imports"
    "$OBJDUMP" -f "$R/bin/libpolycall.dll" | grep -q "pei-x86-64" && ok "libpolycall.dll is PE32+ x86-64" || bad "libpolycall.dll format"
    nm "$R/lib/libpolycall.dll.a" 2>/dev/null | grep -q "__imp_polycall_get_version" \
      && ok "libpolycall.dll.a is a GNU import library" || bad "libpolycall.dll.a content"
    "$OBJDUMP" -p "$R/bin/libpolycall.dll" | grep "DLL Name" | tr -d '\r' | sort -u | sed 's/^/      /'
  else
    skip "PE inspection (objdump not found)"
  fi
else
  if command -v readelf >/dev/null 2>&1; then
    readelf -d "$R/$LIBD/libpolycall.so.1" | grep -q "SONAME.*libpolycall.so.1" && ok "SONAME libpolycall.so.1" || bad "SONAME"
    readelf -d "$R/$LIBD/libpolycall.so.1" | grep -qE "RPATH|RUNPATH" && bad "library carries an RPATH" || ok "no RPATH/RUNPATH in the library"
    readelf -h "$R/$LIBD/libpolycall.so.1" | grep -q "X86-64\|AArch64" && ok "ELF machine: $(readelf -h "$R/$LIBD/libpolycall.so.1" | sed -n 's/ *Machine: *//p')" || bad "ELF machine"
    readelf -d "$R/bin/polycall" | grep -q "libpolycall" && bad "CLI depends on the shared library" || ok "CLI is self-contained (static core)"
    LD_LIBRARY_PATH="$R/$LIBD" ldd "$(exe consumer_c_shared)" | grep -q "$R/$LIBD/libpolycall.so.1" \
      && ok "shared consumer resolves libpolycall.so.1 from the relocated prefix" || bad "dynamic resolution"
  else
    skip "ELF inspection (readelf not found)"
  fi
  if command -v pkg-config >/dev/null 2>&1; then
    CF=$(PKG_CONFIG_PATH="$R/$LIBD/pkgconfig" pkg-config --cflags polycall)
    LB=$(PKG_CONFIG_PATH="$R/$LIBD/pkgconfig" pkg-config --libs polycall)
    _inc=$(printf '%s' "$CF" | sed -n 's/^-I\([^ ]*\).*/\1/p')
    if [ -n "$_inc" ] && [ -f "$_inc/polycall.h" ] && [ "$(cd "$_inc" && pwd -P)" = "$(cd "$R/include/polycall" && pwd -P)" ]; then
      ok "pkg-config --cflags resolves into the relocated prefix ($CF)"
    else bad "pkg-config cflags: $CF"; fi
    if ${CC_:-cc} $CF "$SRC/tests/install/consumer/consumer.c" $LB -o "$TMP/pc_consumer" 2>"$TMP/pc.log" \
       && [ "$(LD_LIBRARY_PATH="$R/$LIBD" "$TMP/pc_consumer")" = "$WANT" ]; then
      ok "pkg-config consumer builds and runs"
    else bad "pkg-config consumer: $(cat "$TMP/pc.log")"; fi
    if ${CC_:-cc} $CF "$SRC/tests/install/consumer/consumer.c" \
         $(PKG_CONFIG_PATH="$R/$LIBD/pkgconfig" pkg-config --libs --static polycall | sed "s#-lpolycall#$R/$LIBD/libpolycall.a#") \
         -o "$TMP/pc_static" 2>"$TMP/pcs.log" && [ "$("$TMP/pc_static")" = "$WANT" ]; then
      ok "pkg-config --static flags link the static library"
    else bad "pkg-config static: $(cat "$TMP/pcs.log")"; fi
  else
    skip "pkg-config consumer (pkg-config not installed)"
  fi
fi

echo "--- install: $PASS passed, $FAIL failed, $SKIP not run ---"
[ "$FAIL" -eq 0 ] || exit 1
[ "$SKIP" -eq 0 ] || exit 77
exit 0
