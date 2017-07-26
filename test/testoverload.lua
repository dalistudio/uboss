local uboss = require "uboss"

local mode = ...

if mode == "slave" then

local CMD = {}

function CMD.sum(n)
	uboss.error("for loop begin")
	local s = 0
	for i = 1, n do
		s = s + i
	end
	uboss.error("for loop end")
end

function CMD.blackhole()
end

uboss.start(function()
	uboss.dispatch("lua", function(_,_, cmd, ...)
		local f = CMD[cmd]
		f(...)
	end)
end)

else

uboss.start(function()
	local slave = uboss.newservice(SERVICE_NAME, "slave")
	for step = 1, 20 do
		uboss.error("overload test ".. step)
		for i = 1, 512 * step do
			uboss.send(slave, "lua", "blackhole")
		end
		uboss.sleep(step)
	end
	local n = 1000000000
	uboss.error(string.format("endless test n=%d", n))
	uboss.send(slave, "lua", "sum", n)
end)

end
