"""PolyCall Python configuration provider SDK (schema v2).

A ``polycall_config.py`` module defines ``configure()`` returning a validated
Python object (dataclass / dict). This helper normalises it and prints exactly
one canonical JSON envelope on stdout -- byte-identical to the C core's output,
so the C, Node and Python providers are provably equivalent. Diagnostics go to
stderr.

Standard library only; no third-party dependency.
"""

from __future__ import annotations

import re
import sys

SCHEMA_VERSION = 2
_IDENT = re.compile(r"^[A-Za-z0-9._-]+$")


class ProviderError(Exception):
    pass


# --- canonical serialiser (must match src/config/config2.c to the byte) ------

def _jstr(s: object) -> str:
    out = ['"']
    for ch in str(s):
        c = ord(ch)
        if ch == '"':
            out.append('\\"')
        elif ch == "\\":
            out.append("\\\\")
        elif ch == "\b":
            out.append("\\b")
        elif ch == "\f":
            out.append("\\f")
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\r":
            out.append("\\r")
        elif ch == "\t":
            out.append("\\t")
        elif c < 0x20:
            out.append("\\u%04x" % c)
        else:
            out.append(ch)
    out.append('"')
    return "".join(out)


def _u32(name: str, v: object) -> str:
    if not isinstance(v, int) or isinstance(v, bool) or v < 0 or v > 0xFFFFFFFF:
        raise ProviderError(f"{name} must be a non-negative integer, got {v!r}")
    return str(v)


def _port(name: str, v: object) -> str:
    if not isinstance(v, int) or isinstance(v, bool) or not (1 <= v <= 65535):
        raise ProviderError(f"{name} must be an integer in 1..65535, got {v!r}")
    return str(v)


def _get(obj: object, key: str, default=None):
    if isinstance(obj, dict):
        return obj.get(key, default)
    return getattr(obj, key, default)


def _tls_block(tls: object) -> str:
    mode = _get(tls, "mode", "off") or "off"
    if mode not in ("off", "server", "mutual"):
        raise ProviderError(f"tls.mode must be off|server|mutual, got {mode}")
    cert = _get(tls, "cert_ref") or None
    key = _get(tls, "key_ref") or None
    if mode == "off" and (cert or key):
        raise ProviderError("tls mode 'off' must not carry cert_ref/key_ref")
    if mode != "off":
        for n, v in (("cert_ref", cert), ("key_ref", key)):
            if not v or not (v.startswith("env:") or v.startswith("file:")):
                raise ProviderError(
                    f"tls.{n} must be an 'env:' or 'file:' reference, not an inline secret"
                )
    return (
        '{"mode":%s,"cert_ref":%s,"key_ref":%s}'
        % (
            _jstr(mode),
            "null" if cert is None else _jstr(cert),
            "null" if key is None else _jstr(key),
        )
    )


def _service_block(s: object) -> str:
    sid = _get(s, "id", "")
    lang = _get(s, "language", "")
    if not _IDENT.match(str(sid or "")):
        raise ProviderError(f"service id invalid: {sid!r}")
    if not _IDENT.match(str(lang or "")):
        raise ProviderError(f"service {sid!r} language invalid: {lang!r}")
    bind = _get(s, "bind_address") or "127.0.0.1"
    ws = _get(s, "workspace")
    if not ws or not isinstance(ws, str):
        raise ProviderError(f"service {sid!r} needs a workspace")
    timeout = _get(s, "timeout_ms")
    timeout = 30000 if timeout is None else timeout
    maxconn = _get(s, "max_connections")
    maxconn = 64 if maxconn is None else maxconn
    return (
        "{"
        '"id":%s,'
        '"language":%s,'
        '"bind_address":%s,'
        '"host_port":%s,'
        '"target_port":%s,'
        '"workspace":%s,'
        '"timeout_ms":%s,'
        '"max_connections":%s,'
        '"tls":%s'
        "}"
        % (
            _jstr(sid),
            _jstr(lang),
            _jstr(bind),
            _port("host_port", _get(s, "host_port")),
            _port("target_port", _get(s, "target_port")),
            _jstr(ws),
            _u32("timeout_ms", timeout),
            _u32("max_connections", maxconn),
            _tls_block(_get(s, "tls")),
        )
    )


def to_envelope(config: object) -> str:
    project = _get(config, "project", {})
    name = _get(project, "name", "")
    if not _IDENT.match(str(name or "")):
        raise ProviderError("project.name is required and must be an identifier")
    ns = _get(project, "extension_namespace") or None
    if ns and not _IDENT.match(ns):
        raise ProviderError("project.extension_namespace has invalid characters")

    services = list(_get(config, "services", []) or [])
    if not services:
        raise ProviderError("at least one service is required")
    services.sort(key=lambda s: str(_get(s, "id", "")))

    seen = set()
    for s in services:
        sid = _get(s, "id")
        if sid in seen:
            raise ProviderError(f"duplicate service id {sid!r}")
        seen.add(sid)

    return (
        "{"
        '"schema_version":%d,'
        '"project":{"name":%s,"extension_namespace":%s},'
        '"services":[%s]'
        "}"
        % (
            SCHEMA_VERSION,
            _jstr(name),
            "null" if ns is None else _jstr(ns),
            ",".join(_service_block(s) for s in services),
        )
    )


def emit_from_module(module_path: str) -> None:
    """Import an explicitly named module, call configure(), print the envelope."""
    import importlib.util
    import os

    spec = importlib.util.spec_from_file_location("polycall_user_config", module_path)
    if spec is None or spec.loader is None:
        raise ProviderError(f"cannot load config module: {module_path}")
    mod = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(mod)
    if not hasattr(mod, "configure"):
        raise ProviderError(f"{os.path.basename(module_path)} must define configure()")
    sys.stdout.write(to_envelope(mod.configure()))
