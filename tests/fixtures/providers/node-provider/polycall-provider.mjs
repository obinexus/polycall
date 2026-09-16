// PolyCall Node configuration provider SDK (schema v2).
//
// A `polycall.config.mjs` is a real ES module that exports native JS values.
// This helper normalises those values and prints exactly ONE canonical JSON
// envelope on stdout -- byte-identical to what the C core emits, so the C,
// Node and Python providers are provably equivalent. Diagnostics go to stderr.

const SCHEMA_VERSION = 2;

// --- canonical serialiser (must match src/config/config2.c to the byte) ---

function jstr(s) {
  let out = '"';
  for (const ch of String(s)) {
    const c = ch.codePointAt(0);
    if (ch === '"') out += '\\"';
    else if (ch === '\\') out += '\\\\';
    else if (ch === '\b') out += '\\b';
    else if (ch === '\f') out += '\\f';
    else if (ch === '\n') out += '\\n';
    else if (ch === '\r') out += '\\r';
    else if (ch === '\t') out += '\\t';
    else if (c < 0x20) out += '\\u' + c.toString(16).padStart(4, '0');
    else out += ch;
  }
  return out + '"';
}

function u32(name, v) {
  if (!Number.isInteger(v) || v < 0 || v > 4294967295) {
    throw new Error(`${name} must be a non-negative integer, got ${JSON.stringify(v)}`);
  }
  return String(v);
}

function port(name, v) {
  if (!Number.isInteger(v) || v < 1 || v > 65535) {
    throw new Error(`${name} must be an integer in 1..65535, got ${JSON.stringify(v)}`);
  }
  return String(v);
}

const IDENT = /^[A-Za-z0-9._-]+$/;

function tlsBlock(tls) {
  const mode = tls && tls.mode ? tls.mode : 'off';
  if (!['off', 'server', 'mutual'].includes(mode)) {
    throw new Error(`tls.mode must be off|server|mutual, got ${mode}`);
  }
  const cert = tls && tls.cert_ref ? tls.cert_ref : null;
  const key = tls && tls.key_ref ? tls.key_ref : null;
  if (mode === 'off' && (cert || key)) {
    throw new Error("tls mode 'off' must not carry cert_ref/key_ref");
  }
  if (mode !== 'off') {
    for (const [n, v] of [['cert_ref', cert], ['key_ref', key]]) {
      if (!v || !(/^env:/.test(v) || /^file:/.test(v))) {
        throw new Error(`tls.${n} must be an 'env:' or 'file:' reference, not an inline secret`);
      }
    }
  }
  return `{"mode":${jstr(mode)},"cert_ref":${cert === null ? 'null' : jstr(cert)},` +
         `"key_ref":${key === null ? 'null' : jstr(key)}}`;
}

function serviceBlock(s) {
  if (!s || !IDENT.test(s.id || '')) throw new Error(`service id invalid: ${JSON.stringify(s && s.id)}`);
  if (!IDENT.test(s.language || '')) throw new Error(`service '${s.id}' language invalid: ${JSON.stringify(s.language)}`);
  const bind = s.bind_address || '127.0.0.1';
  const ws = s.workspace;
  if (!ws || typeof ws !== 'string') throw new Error(`service '${s.id}' needs a workspace`);
  const timeout = s.timeout_ms == null ? 30000 : s.timeout_ms;
  const maxconn = s.max_connections == null ? 64 : s.max_connections;
  return '{' +
    `"id":${jstr(s.id)},` +
    `"language":${jstr(s.language)},` +
    `"bind_address":${jstr(bind)},` +
    `"host_port":${port('host_port', s.host_port)},` +
    `"target_port":${port('target_port', s.target_port)},` +
    `"workspace":${jstr(ws)},` +
    `"timeout_ms":${u32('timeout_ms', timeout)},` +
    `"max_connections":${u32('max_connections', maxconn)},` +
    `"tls":${tlsBlock(s.tls)}` +
  '}';
}

export function toEnvelope(config) {
  const project = config.project || {};
  if (!IDENT.test(project.name || '')) {
    throw new Error(`project.name is required and must match ${IDENT}`);
  }
  const ns = project.extension_namespace || null;
  if (ns && !IDENT.test(ns)) throw new Error('project.extension_namespace has invalid characters');

  const services = [...(config.services || [])];
  if (services.length === 0) throw new Error('at least one service is required');
  services.sort((a, b) => (a.id < b.id ? -1 : a.id > b.id ? 1 : 0));

  const seen = new Set();
  for (const s of services) {
    if (seen.has(s.id)) throw new Error(`duplicate service id '${s.id}'`);
    seen.add(s.id);
  }

  return '{' +
    `"schema_version":${SCHEMA_VERSION},` +
    `"project":{"name":${jstr(project.name)},` +
      `"extension_namespace":${ns === null ? 'null' : jstr(ns)}},` +
    `"services":[${services.map(serviceBlock).join(',')}]` +
  '}';
}

// Load a config module, print its envelope, exit. Used by run.mjs.
export async function emitFromModule(modulePath) {
  const mod = await import(modulePath);
  const config = mod.default ?? mod.config;
  if (!config) throw new Error(`${modulePath} must export a default configuration object`);
  process.stdout.write(toEnvelope(config));
}
