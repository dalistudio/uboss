local uboss = require "uboss"
require "uboss.manager"	-- import uboss.register
local db = {}

local command = {}

function command.GET(key)
	return db[key]
end

function command.SET(key, value)
	local last = db[key]
	db[key] = value
	return last
end

uboss.start(function()
	uboss.dispatch("lua", function(session, address, cmd, ...)
		local f = command[string.upper(cmd)]
		if f then
			uboss.ret(uboss.pack(f(...)))
		else
			error(string.format("Unknown command %s", tostring(cmd)))
		end
	end)
	uboss.register "SIMPLEDB"
end)
