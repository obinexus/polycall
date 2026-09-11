-- polycall_rpc v1 client (see docs/RPC.md), added alongside the existing
-- (legacy-protocol) bindings/lua-polycall per docs/TODO.md P2.
--
-- Wire framing (frame.lua) is stdlib-only and unit-verified (frame_test.lua,
-- including a byte-for-byte comparison against a reference frame from an
-- already-verified client). The live socket connection below needs
-- LuaSocket ("socket.core" / the "socket" rock), which is NOT installed in
-- this environment -- see docs/backward_compatibility.md for the exact
-- install step and NOT RUN status. `require("socket")` is deferred to
-- connect() so frame.lua stays usable (and tested) without it.

local frame = require("frame")

local client = {}

local function escape(s)
  return (s:gsub('[%c"\\]', function(c)
    if c == '"' then return '\\"'
    elseif c == '\\' then return '\\\\'
    elseif c == '\n' then return '\\n'
    elseif c == '\r' then return '\\r'
    elseif c == '\t' then return '\\t'
    else return string.format('\\u%04x', c:byte())
    end
  end))
end

-- split_endpoint("host:port") -> host, port
local function split_endpoint(ep)
  local host, port = ep:match("^(.+):(%d+)$")
  assert(host and port, "invalid endpoint, expected host:port")
  return host, tonumber(port)
end

local function read_exact(sock, n)
  local buf = {}
  local got = 0
  while got < n do
    local chunk, err, partial = sock:receive(n - got)
    chunk = chunk or partial
    if not chunk or #chunk == 0 then
      error(err or "connection closed")
    end
    buf[#buf + 1] = chunk
    got = got + #chunk
  end
  return table.concat(buf)
end

-- call(endpoint, service, operation, raw_input_json, deadline_ms)
--   -> ok (bool), raw_payload (string, exactly as received), error_code (string|nil)
-- Performs exactly ONE round trip and never retries -- callers must not loop
-- this for non-idempotent operations.
function client.call(endpoint, service, operation, raw_input_json, deadline_ms)
  local socket = require("socket") -- LuaSocket; see the module comment above
  deadline_ms = deadline_ms or 5000
  raw_input_json = raw_input_json or "null"

  local body = string.format('{"service":"%s","operation":"%s","deadline_ms":%d,"input":%s}',
    escape(service), escape(operation), deadline_ms, raw_input_json)

  local host, port = split_endpoint(endpoint)
  local sock = assert(socket.tcp())
  sock:settimeout((deadline_ms + 1000) / 1000)
  local ok, err = sock:connect(host, port)
  if not ok then
    sock:close()
    error("transport: " .. tostring(err))
  end

  local frame_bytes = frame.encode(frame.TYPE_REQUEST, 1, body)
  local sent, serr = sock:send(frame_bytes)
  if not sent then
    sock:close()
    error("transport: " .. tostring(serr))
  end

  local ok2, hdr_or_err = pcall(read_exact, sock, frame.HEADER_LEN)
  if not ok2 then
    sock:close()
    error("transport: " .. tostring(hdr_or_err))
  end
  local ftype, _corr, length = frame.decode_header(hdr_or_err)
  local payload = length > 0 and read_exact(sock, length) or ""
  sock:close()

  if ftype ~= frame.TYPE_RESPONSE then
    error("unexpected frame type " .. tostring(ftype))
  end

  local is_ok = payload:match('"ok"%s*:%s*true') ~= nil
  local error_code = nil
  if not is_ok then
    error_code = payload:match('"code"%s*:%s*"([^"]*)"')
  end
  return is_ok, payload, error_code
end

return client
