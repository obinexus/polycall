#!/usr/bin/env lua5.4
-- A generic polycall_rpc v1 CLI: one round trip, no retry.
--
--   lua5.4 polycall_call.lua <host:port> <service> <operation> <json-input> [deadline_ms]
--
-- Prints the RESPONSE payload exactly as received (for byte-identical
-- cross-language comparison in tests/conformance/) and exits with the same
-- code contract as the C CLI's `polycall call` (docs/RPC.md). Requires
-- LuaSocket at run time (see client.lua); NOT RUN in this environment.

local script_dir = arg[0]:match("(.*/)") or "./"
package.path = script_dir .. "?.lua;" .. package.path

local client = require("client")

local EXIT_OK, EXIT_OTHER, EXIT_USAGE = 0, 1, 2
local EXIT_CONFIG, EXIT_UNSUPPORTED = 3, 4
local EXIT_TRANSPORT, EXIT_DEADLINE, EXIT_AUTH = 5, 6, 7

local function exit_for(code)
  local map = {
    ["request.malformed"] = EXIT_CONFIG,
    ["input.invalid"] = EXIT_CONFIG,
    ["operation.unknown"] = EXIT_UNSUPPORTED,
    ["item.unknown"] = EXIT_UNSUPPORTED,
    ["deadline.exceeded"] = EXIT_DEADLINE,
    ["auth.denied"] = EXIT_AUTH,
  }
  return map[code] or EXIT_OTHER
end

if #arg < 3 or #arg > 5 then
  io.stderr:write("usage: polycall_call.lua <host:port> <service> <operation> <json-input> [deadline_ms]\n")
  os.exit(EXIT_USAGE)
end

local endpoint, service, operation = arg[1], arg[2], arg[3]
local input = arg[4] or "null"
local deadline_ms = tonumber(arg[5]) or 5000

local ok, is_ok_or_err, payload_or_nil, error_code = pcall(client.call, endpoint, service,
  operation, input, deadline_ms)

if not ok then
  io.stderr:write("polycall-call: " .. tostring(is_ok_or_err) .. "\n")
  os.exit(EXIT_TRANSPORT)
end

local is_ok, payload, code = is_ok_or_err, payload_or_nil, error_code
print(payload)
os.exit(is_ok and EXIT_OK or exit_for(code))
