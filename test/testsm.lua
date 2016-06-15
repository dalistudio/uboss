local uboss = require "uboss"
local sharemap = require "sharemap"

local mode = ...

if mode == "slave" then
--slave

local function dump(reader)
	reader:update()
	print("x=", reader.x)
	print("y=", reader.y)
	print("s=", reader.s)
end

uboss.start(function()
	local reader
	uboss.dispatch("lua", function(_,_,cmd,...)
		if cmd == "init" then
			reader = sharemap.reader(...)
		else
			assert(cmd == "ping")
			dump(reader)
		end
		uboss.ret()
	end)
end)

else
-- master
uboss.start(function()
	-- register share type schema
	sharemap.register("./test/sharemap.sp")
	local slave = uboss.newservice(SERVICE_NAME, "slave")
	local writer = sharemap.writer("foobar", { x=0,y=0,s="hello" })
	uboss.call(slave, "lua", "init", "foobar", writer:copy())
	writer.x = 1
	writer:commit()
	uboss.call(slave, "lua", "ping")
	writer.y = 2
	writer:commit()
	uboss.call(slave, "lua", "ping")
	writer.s = "world"
	writer:commit()
	uboss.call(slave, "lua", "ping")
end)

end
