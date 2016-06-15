local uboss = require "uboss"
local mc = require "multicast"
local dc = require "datacenter"

local mode = ...

if mode == "sub" then

uboss.start(function()
	uboss.dispatch("lua", function (_,_, cmd, channel)
		assert(cmd == "init")
		local c = mc.new {
			channel = channel ,
			dispatch = function (channel, source, ...)
				print(string.format("%s <=== %s %s",uboss.address(uboss.self()),uboss.address(source), channel), ...)
			end
		}
		print(uboss.address(uboss.self()), "sub", c)
		c:subscribe()
		uboss.ret(uboss.pack())
	end)
end)

else

uboss.start(function()
	local channel = mc.new()
	print("New channel", channel)
	for i=1,10 do
		local sub = uboss.newservice(SERVICE_NAME, "sub")
		uboss.call(sub, "lua", "init", channel.channel)
	end

	dc.set("MCCHANNEL", channel.channel)	-- for multi node test

	print(uboss.address(uboss.self()), "===>", channel)
	channel:publish("Hello World")
end)

end
