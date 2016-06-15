local uboss = require "uboss"

local mode = ...

if mode == "slave" then

uboss.start(function()
	uboss.dispatch("lua", function(_,_, ...)
		uboss.ret(uboss.pack(...))
	end)
end)

else

uboss.start(function()
	local slave = uboss.newservice(SERVICE_NAME, "slave")
	local n = 100000
	local start = uboss.now()
	print("call salve", n, "times in queue")
	for i=1,n do
		uboss.call(slave, "lua")
	end
	print("qps = ", n/ (uboss.now() - start) * 100)

	start = uboss.now()

	local worker = 10
	local task = n/worker
	print("call salve", n, "times in parallel, worker = ", worker)

	for i=1,worker do
		uboss.fork(function()
			for i=1,task do
				uboss.call(slave, "lua")
			end
			worker = worker -1
			if worker == 0 then
				print("qps = ", n/ (uboss.now() - start) * 100)
			end
		end)
	end
end)

end
