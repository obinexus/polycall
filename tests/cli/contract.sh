#!/bin/sh
# CLI contract tests. Usage: contract.sh <path-to-polycall> [fixtures-dir]
# Exits non-zero if any assertion fails. Emits one line per check.

set -u
POLYCALL=${1:?path to polycall binary required}
FIX=${2:-tests/fixtures}
PASS=0
FAIL=0
NOTRUN=0

_run() {            # _run "<argv...>" ; sets $STDOUT $STDERR $RC ; stdin = /dev/null
    STDOUT=$("$POLYCALL" "$@" </dev/null 2>"$TMP_ERR"); RC=$?
    STDERR=$(cat "$TMP_ERR")
    return 0
}
ok()   { PASS=$((PASS+1));   printf 'PASS  %s\n' "$1"; }
bad()  { FAIL=$((FAIL+1));   printf 'FAIL  %s\n' "$1"; }
skip() { NOTRUN=$((NOTRUN+1)); printf 'NOT RUN  %s\n' "$1"; }

TMP_ERR=$(mktemp 2>/dev/null || echo "${TMPDIR:-/tmp}/polycall_cli_err.$$")
trap 'rm -f "$TMP_ERR"' EXIT

# node for JSON validation (Python not on this runner)
JSON_TOOL=""
if command -v node >/dev/null 2>&1; then JSON_TOOL="node"; fi

json_ok() {   # json_ok <string> <expr returning truthy>  -> uses node
    [ -n "$JSON_TOOL" ] || return 2
    printf '%s' "$1" | node -e '
      let s="";process.stdin.on("data",d=>s+=d).on("end",()=>{
        let o; try{o=JSON.parse(s)}catch(e){process.exit(3)}
        const f=process.argv[1];
        process.exit(eval(f)?0:1);
      })' "$2"
}

echo "=== CLI contract: $POLYCALL ==="

# 1. no args -> help, exit 0
_run
{ [ "$RC" -eq 0 ] && echo "$STDOUT" | grep -q "usage: polycall"; } \
  && ok "no args -> help, exit 0" || bad "no args -> help, exit 0 (rc=$RC)"

# 2. no args must NOT drop into a REPL even with a closed stdin
echo "$STDOUT" | grep -q "polycall>" && bad "no args must not start REPL" \
  || ok "no args does not start REPL"

# 3. --version immediate, single line, no banner
_run --version
{ [ "$RC" -eq 0 ] && [ "$(printf '%s\n' "$STDOUT" | wc -l)" -eq 1 ] \
  && echo "$STDOUT" | grep -Eq '^polycall [0-9]+\.' \
  && ! echo "$STDOUT" | grep -q "Type 'help'"; } \
  && ok "--version immediate single line" || bad "--version immediate single line (rc=$RC out=$STDOUT)"

# 4. version subcommand equals --version
_run version
echo "$STDOUT" | grep -Eq '^polycall [0-9]+\.' && [ "$RC" -eq 0 ] \
  && ok "version subcommand" || bad "version subcommand (rc=$RC)"

# 5. --help lists commands
_run --help
{ [ "$RC" -eq 0 ] && echo "$STDOUT" | grep -q "commands:"; } \
  && ok "--help lists commands" || bad "--help lists commands (rc=$RC)"

# 6. help <command>
_run help doctor
{ [ "$RC" -eq 0 ] && echo "$STDOUT" | grep -q "doctor"; } \
  && ok "help doctor" || bad "help doctor (rc=$RC)"

# 7. unknown command -> 2
_run frobnicate
{ [ "$RC" -eq 2 ] && echo "$STDERR" | grep -q "unknown command"; } \
  && ok "unknown command -> exit 2" || bad "unknown command -> exit 2 (rc=$RC)"

# 8. unknown option -> 2
_run --bogus
{ [ "$RC" -eq 2 ] && echo "$STDERR" | grep -q "unknown option"; } \
  && ok "unknown option -> exit 2" || bad "unknown option -> exit 2 (rc=$RC)"

# 9. removed -f option -> 2 with pointer to 'run'
_run -f whatever.cfg
{ [ "$RC" -eq 2 ] && echo "$STDERR" | grep -q "run --config"; } \
  && ok "-f removed -> exit 2 (points at run)" || bad "-f removed -> exit 2 (rc=$RC)"

# 10. duplicate scalar option -> 2
_run --format json --format text doctor
[ "$RC" -eq 2 ] && ok "duplicate --format -> exit 2" || bad "duplicate --format -> exit 2 (rc=$RC)"

# 11. missing option value -> 2
_run --config
[ "$RC" -eq 2 ] && ok "missing --config value -> exit 2" || bad "missing --config value -> exit 2 (rc=$RC)"

# 12. bad --format value -> 2
_run --format yaml doctor
[ "$RC" -eq 2 ] && ok "bad --format value -> exit 2" || bad "bad --format value -> exit 2 (rc=$RC)"

# 13. JSON usage error is still one JSON object with ok:false on stdout
_run --format json --nope
if [ -n "$JSON_TOOL" ]; then
  { [ "$RC" -eq 2 ] && json_ok "$STDOUT" 'o.ok===false && o.schema_version===1 && o.error && o.error.code==="usage"'; } \
    && ok "JSON usage error shape" || bad "JSON usage error shape (rc=$RC out=$STDOUT)"
else
  skip "JSON usage error shape (no node)"
fi

# 14. doctor text
_run doctor
{ [ "$RC" -eq 0 ] && echo "$STDOUT" | grep -q "platform" && echo "$STDOUT" | grep -q "ABI version"; } \
  && ok "doctor text, exit 0" || bad "doctor text, exit 0 (rc=$RC)"

# 15. doctor --format json parses and is well-formed
_run --format json doctor
if [ -n "$JSON_TOOL" ]; then
  { [ "$RC" -eq 0 ] && json_ok "$STDOUT" 'o.ok===true && o.command==="doctor" && o.data && o.data.checks && o.data.checks.core_context_construct===true'; } \
    && ok "doctor JSON shape" || bad "doctor JSON shape (rc=$RC out=$STDOUT)"
else
  skip "doctor JSON shape (no node)"
fi

# 16. --version --format json
_run --format json --version
if [ -n "$JSON_TOOL" ]; then
  json_ok "$STDOUT" 'o.ok===true && o.data && typeof o.data.version==="string"' \
    && ok "version JSON shape" || bad "version JSON shape (out=$STDOUT)"
else
  skip "version JSON shape (no node)"
fi

# 17. config validate on the repo Polycallfile -> 0
_run config validate Polycallfile
[ "$RC" -eq 0 ] && ok "config validate Polycallfile -> 0" || bad "config validate Polycallfile -> 0 (rc=$RC)"

# 18. config validate on a missing file -> config error (3)
_run config validate this/does/not/exist.cfg
[ "$RC" -eq 3 ] && ok "config validate missing -> exit 3" || bad "config validate missing -> exit 3 (rc=$RC)"

# 19. config with no subcommand -> 2
_run config
{ [ "$RC" -eq 2 ] && echo "$STDERR" | grep -q "missing subcommand"; } \
  && ok "config without subcommand -> exit 2" || bad "config without subcommand -> exit 2 (rc=$RC)"

# 20. runtime commands: missing required args -> usage error (exit 2), no hang
_run status
[ "$RC" -eq 2 ] && ok "status without --endpoint -> exit 2" || bad "status without --endpoint (rc=$RC)"
_run stop
[ "$RC" -eq 2 ] && ok "stop without --endpoint -> exit 2" || bad "stop without --endpoint (rc=$RC)"
_run call
[ "$RC" -eq 2 ] && ok "call without SERVICE OPERATION -> exit 2" || bad "call without args (rc=$RC)"
_run call inventory get
[ "$RC" -eq 2 ] && ok "call without --endpoint -> exit 2" || bad "call without --endpoint (rc=$RC)"

# 20a. run --load: argument handling and fast-fail paths (P1). These never
# bind a socket, so they return promptly without backgrounding anything --
# see tests/runtime/plugin.sh for the full load/call/status flow.
_run run --load
[ "$RC" -eq 2 ] && ok "run --load with no value -> exit 2" || bad "run --load no value (rc=$RC)"
_run run --load this/path/does/not/exist.so
[ "$RC" -eq 4 ] && ok "run --load nonexistent path -> exit 4" || bad "run --load nonexistent (rc=$RC)"

# 20b. bindings list is implemented (Stage 2) -> exit 0, names the c provider
_run bindings list
{ [ "$RC" -eq 0 ] && echo "$STDOUT" | grep -q "^  c "; } \
  && ok "bindings list -> exit 0, reports providers" || bad "bindings list (rc=$RC)"

# 20c. a command that DOES take local options still rejects an unknown one
_run config validate --nope
[ "$RC" -eq 2 ] && ok "config validate --nope -> exit 2" || bad "config validate --nope (rc=$RC)"

# 21. path with spaces
if [ -f "$FIX/with space/Polycallfile" ]; then
  _run config validate "$FIX/with space/Polycallfile"
  [ "$RC" -eq 0 ] && ok "path with spaces accepted" || bad "path with spaces accepted (rc=$RC)"
else
  skip "path with spaces (fixture missing)"
fi

# 22. CRLF config file
if [ -f "$FIX/crlf/Polycallfile" ]; then
  _run config validate "$FIX/crlf/Polycallfile"
  [ "$RC" -eq 0 ] && ok "CRLF config accepted" || bad "CRLF config accepted (rc=$RC)"
else
  skip "CRLF config (fixture missing)"
fi

# 23. quoted path preserved through argv (no re-tokenising)
if [ -f "$FIX/with space/Polycallfile" ]; then
  _run --format json config validate "$FIX/with space/Polycallfile"
  [ "$RC" -eq 0 ] && ok "quoted path preserved with --format json" || bad "quoted path preserved (rc=$RC)"
else
  skip "quoted path preserved (fixture missing)"
fi

# 24. '--' ends option parsing
_run config validate -- --weird-but-a-path
{ [ "$RC" -eq 3 ] || [ "$RC" -eq 0 ]; } && ok "'--' ends option parsing (treated as path)" \
  || bad "'--' ends option parsing (rc=$RC, expected 0/3 not 2)"

echo "--- contract: $PASS passed, $FAIL failed, $NOTRUN not run ---"
[ "$FAIL" -eq 0 ]
