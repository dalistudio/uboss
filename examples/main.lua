local uboss = require "uboss"
local sprotoloader = require "sprotoloader"

local max_client = 64

uboss.start(function()
	uboss.error("Server start")
	uboss.uniqueservice("protoloader")
	local console = uboss.newservice("console")
	uboss.newservice("debug_console",8000)
	uboss.newservice("simpledb")
	local watchdog = uboss.newservice("watchdog")
	uboss.call(watchdog, "lua", "start", {
		port = 8888,
		maxclient = max_client,
		nodelay = true,
	})
	uboss.error("Watchdog listen on", 8888)
	uboss.exit()
end)
