local uboss = require "uboss"

local mode = ...

if mode == "TICK" then
-- this service whould response the request every 1s.

local response_queue = {}

local function response()
	while true do
		uboss.sleep(100)	-- sleep 1s
		for k,v in ipairs(response_queue) do
			v(true, uboss.now())		-- true means succ, false means error
			response_queue[k] = nil
		end
	end
end

uboss.start(function()
	uboss.fork(response)
	uboss.dispatch("lua", function()
		table.insert(response_queue, uboss.response())
	end)
end)

else

local function request(tick, i)
	print(i, "call", uboss.now())
	print(i, "response", uboss.call(tick, "lua"))
	print(i, "end", uboss.now())
end

uboss.start(function()
	local tick = uboss.newservice(SERVICE_NAME, "TICK")

	for i=1,5 do
		uboss.fork(request, tick, i)
		uboss.sleep(10)
	end
end)

end
