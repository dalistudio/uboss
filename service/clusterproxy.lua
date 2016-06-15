local uboss = require "uboss"
local cluster = require "cluster"
require "uboss.manager"	-- inject uboss.forward_type

local node, address = ...

uboss.register_protocol {
	name = "system",
	id = uboss.PTYPE_SYSTEM,
	unpack = function (...) return ... end,
}

local forward_map = {
	[uboss.PTYPE_SNAX] = uboss.PTYPE_SYSTEM,
	[uboss.PTYPE_LUA] = uboss.PTYPE_SYSTEM,
	[uboss.PTYPE_RESPONSE] = uboss.PTYPE_RESPONSE,	-- don't free response message
}

uboss.forward_type( forward_map ,function()
	local clusterd = uboss.uniqueservice("clusterd")
	local n = tonumber(address)
	if n then
		address = n
	end
	uboss.dispatch("system", function (session, source, msg, sz)
		uboss.ret(uboss.rawcall(clusterd, "lua", uboss.pack("req", node, address, msg, sz)))
	end)
end)
