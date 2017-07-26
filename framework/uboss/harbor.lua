local uboss = require "uboss"

local harbor = {}

function harbor.globalname(name, handle)
	handle = handle or uboss.self()
	uboss.send(".cslave", "lua", "REGISTER", name, handle)
end

function harbor.queryname(name)
	return uboss.call(".cslave", "lua", "QUERYNAME", name)
end

function harbor.link(id)
	uboss.call(".cslave", "lua", "LINK", id)
end

function harbor.connect(id)
	uboss.call(".cslave", "lua", "CONNECT", id)
end

function harbor.linkmaster()
	uboss.call(".cslave", "lua", "LINKMASTER")
end

return harbor
