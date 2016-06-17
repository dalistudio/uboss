local uboss = require "uboss"

local datacenter = {}

-- 数据中心：获取
function datacenter.get(...)
	return uboss.call("DATACENTER", "lua", "QUERY", ...)
end

-- 数据中心：设置
function datacenter.set(...)
	return uboss.call("DATACENTER", "lua", "UPDATE", ...)
end

-- 数据中心：等待
function datacenter.wait(...)
	return uboss.call("DATACENTER", "lua", "WAIT", ...)
end

return datacenter

