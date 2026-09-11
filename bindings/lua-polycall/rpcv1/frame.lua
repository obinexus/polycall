-- polycall_rpc v1 framing (see docs/RPC.md). Pure byte-packing: no sockets,
-- no dependency beyond stdlib string.pack/string.unpack (Lua 5.3+).
--
-- Frame: 16-byte header (magic "PCR1", type, flags, 2-byte reserved,
-- big-endian 4-byte correlation id, big-endian 4-byte length) followed by a
-- UTF-8 JSON payload. No Lua table is ever placed on the wire.

local frame = {}

frame.HEADER_LEN = 16
frame.MAX_PAYLOAD = 1 << 20 -- 1 MiB

frame.TYPE_REQUEST  = 1
frame.TYPE_RESPONSE = 2
frame.TYPE_CONTROL  = 3
frame.TYPE_REPLY    = 4

local HDR_FMT = ">c4 B B I2 I4 I4"

-- encode(ftype, corr, payload) -> bytes (header .. payload)
function frame.encode(ftype, corr, payload)
  payload = payload or ""
  assert(#payload <= frame.MAX_PAYLOAD, "payload too large")
  return string.pack(HDR_FMT, "PCR1", ftype, 0, 0, corr, #payload) .. payload
end

-- decode_header(bytes) -> type, corr, length  (bytes must be exactly 16)
function frame.decode_header(bytes)
  assert(#bytes == frame.HEADER_LEN, "header must be 16 bytes")
  local magic, ftype, _flags, _rsvd, corr, length = string.unpack(HDR_FMT, bytes)
  assert(magic == "PCR1", "bad frame magic")
  assert(length <= frame.MAX_PAYLOAD, "payload too large")
  return ftype, corr, length
end

return frame
