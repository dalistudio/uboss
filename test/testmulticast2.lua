local uboss = require "uboss"
local dc = require "datacenter"
local mc = require "multicast"

uboss.start(function()
	print("remote start")
	local console = uboss.newservice("console")
	local channel = dc.get "MCCHANNEL"
	if channel then
		print("remote channel", channel)
	else
		print("create local channel")
	end
	for i=1,10 do
		local sub = uboss.newservice("testmulticast", "sub")
		uboss.call(sub, "lua", "init", channel)
	end
	local c = mc.new {
		channel = channel ,
		dispatch = function(...) print("======>", ...) end,
	}
	c:subscribe()
	c:publish("Remote message")
	c:unsubscribe()
	c:publish("Remote message2")
	c:delete()
	uboss.exit()
end)
