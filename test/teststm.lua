local uboss = require "uboss"
local stm = require "stm"

local mode = ...

if mode == "slave" then

uboss.start(function()
	uboss.dispatch("lua", function (_,_, obj)
		local obj = stm.newcopy(obj)
		print("read:", obj(uboss.unpack))
		uboss.ret()
		uboss.error("sleep and read")
		for i=1,10 do
			uboss.sleep(10)
			print("read:", obj(uboss.unpack))
		end
		uboss.exit()
	end)
end)

else

uboss.start(function()
	local slave = uboss.newservice(SERVICE_NAME, "slave")
	local obj = stm.new(uboss.pack(1,2,3,4,5))
	local copy = stm.copy(obj)
	uboss.call(slave, "lua", copy)
	for i=1,5 do
		uboss.sleep(20)
		print("write", i)
		obj(uboss.pack("hello world", i))
	end
 	uboss.exit()
end)
end
