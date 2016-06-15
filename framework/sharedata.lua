local uboss = require "uboss"
local sd = require "sharedata.corelib"

local service

uboss.init(function()
	service = uboss.uniqueservice "sharedatad"
end)

local sharedata = {}

local function monitor(name, obj, cobj)
	local newobj = cobj
	while true do
		newobj = uboss.call(service, "lua", "monitor", name, newobj)
		if newobj == nil then
			break
		end
		sd.update(obj, newobj)
	end
end

function sharedata.query(name)
	local obj = uboss.call(service, "lua", "query", name)
	local r = sd.box(obj)
	uboss.send(service, "lua", "confirm" , obj)
	uboss.fork(monitor,name, r, obj)
	return r
end

function sharedata.new(name, v, ...)
	uboss.call(service, "lua", "new", name, v, ...)
end

function sharedata.update(name, v, ...)
	uboss.call(service, "lua", "update", name, v, ...)
end

function sharedata.delete(name)
	uboss.call(service, "lua", "delete", name)
end

return sharedata
