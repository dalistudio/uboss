local uboss = require "uboss"

local list = {}

local function timeout_check(ti)
	if not next(list) then
		return
	end
	uboss.sleep(ti)	-- sleep 10 sec
	for k,v in pairs(list) do
		uboss.error("timout",ti,k,v)
	end
end

uboss.start(function()
	uboss.error("ping all")
	local list_ret = uboss.call(".launcher", "lua", "LIST")
	for addr, desc in pairs(list_ret) do
		list[addr] = desc
		uboss.fork(function()
			uboss.call(addr,"debug","INFO")
			list[addr] = nil
		end)
	end
	uboss.sleep(0)
	timeout_check(100)
	timeout_check(400)
	timeout_check(500)
	uboss.exit()
end)
