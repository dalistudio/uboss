local uboss = require "uboss"

local clusterd
local cluster = {}

function cluster.call(node, address, ...)
	-- uboss.pack(...) will free by cluster.core.packrequest
	return uboss.call(clusterd, "lua", "req", node, address, uboss.pack(...))
end

function cluster.open(port)
	if type(port) == "string" then
		uboss.call(clusterd, "lua", "listen", port)
	else
		uboss.call(clusterd, "lua", "listen", "0.0.0.0", port)
	end
end

function cluster.reload()
	uboss.call(clusterd, "lua", "reload")
end

function cluster.proxy(node, name)
	return uboss.call(clusterd, "lua", "proxy", node, name)
end

function cluster.snax(node, name, address)
	local snax = require "snax"
	if not address then
		address = cluster.call(node, ".service", "QUERY", "snaxd" , name)
	end
	local handle = uboss.call(clusterd, "lua", "proxy", node, address)
	return snax.bind(handle, name)
end

function cluster.register(name, addr)
	assert(type(name) == "string")
	assert(addr == nil or type(addr) == "number")
	return uboss.call(clusterd, "lua", "register", name, addr)
end

function cluster.query(node, name)
	return uboss.call(clusterd, "lua", "req", node, 0, uboss.pack(name))
end

uboss.init(function()
	clusterd = uboss.uniqueservice("clusterd")
end)

return cluster
