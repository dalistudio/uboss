local uboss = require "uboss"

local datacenter = {}

function datacenter.get(...)
	return uboss.call("DATACENTER", "lua", "QUERY", ...)
end

function datacenter.set(...)
	return uboss.call("DATACENTER", "lua", "UPDATE", ...)
end

function datacenter.wait(...)
	return uboss.call("DATACENTER", "lua", "WAIT", ...)
end

return datacenter

