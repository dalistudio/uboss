local uboss = require "uboss"

local clusterd
local cluster = {}

function cluster.call(node, address, ...)
	-- uboss.pack(...) will free by cluster.core.packrequest
	return uboss.call(clusterd, "lua", "req", node, address, uboss.pack(...))
end

-- 群集：打开
function cluster.open(port)
	-- 端口类型是字符串时
	if type(port) == "string" then
		uboss.call(clusterd, "lua", "listen", port)
	else
		uboss.call(clusterd, "lua", "listen", "0.0.0.0", port)
	end
end

-- 群集：重载
function cluster.reload()
	uboss.call(clusterd, "lua", "reload")
end

-- 群集：代理
function cluster.proxy(node, name)
	return uboss.call(clusterd, "lua", "proxy", node, name)
end

-- 群集：扩展库
function cluster.snax(node, name, address)
	local snax = require "snax"
	if not address then
		address = cluster.call(node, ".service", "QUERY", "snaxd" , name)
	end
	local handle = uboss.call(clusterd, "lua", "proxy", node, address)
	return snax.bind(handle, name)
end

-- 群集：注册
function cluster.register(name, addr)
	assert(type(name) == "string")
	assert(addr == nil or type(addr) == "number")
	return uboss.call(clusterd, "lua", "register", name, addr)
end

-- 群集：查询
function cluster.query(node, name)
	return uboss.call(clusterd, "lua", "req", node, 0, uboss.pack(name))
end

-- 初始化
uboss.init(function()
	clusterd = uboss.uniqueservice("clusterd")
end)

return cluster
