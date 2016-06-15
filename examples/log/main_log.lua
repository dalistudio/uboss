local uboss = require "uboss"
local harbor = require "uboss.harbor"
require "uboss.manager"	-- import uboss.monitor

local function monitor_master()
	harbor.linkmaster()
	print("master is down")
	uboss.exit()
end

uboss.start(function()
	print("Log server start")
	uboss.monitor "simplemonitor"
	local log = uboss.newservice("globallog")
	uboss.fork(monitor_master)
end)

