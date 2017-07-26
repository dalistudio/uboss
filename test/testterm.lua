local uboss = require "uboss"

local function term()
	uboss.error("Sleep one second, and term the call to UNEXIST")
	uboss.sleep(100)
	local self = uboss.self()
	uboss.send(uboss.self(), "debug", "TERM", "UNEXIST")
end

uboss.start(function()
	uboss.fork(term)
	uboss.error("call an unexist named service UNEXIST, may block")
	pcall(uboss.call, "UNEXIST", "lua", "test")
	uboss.error("unblock the unexisted service call")
end)
