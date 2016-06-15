local uboss = require "uboss"
local debugchannel = require "debugchannel"

local CMD = {}

local channel

function CMD.start(address, fd)
	assert(channel == nil, "start more than once")
	uboss.error(string.format("Attach to :%08x", address))
	local handle
	channel, handle = debugchannel.create()
	uboss.call(address, "debug", "REMOTEDEBUG", fd, handle)
	-- todo hook
	uboss.ret(uboss.pack(nil))
	uboss.exit()
end

function CMD.cmd(cmdline)
	channel:write(cmdline)
end

uboss.start(function()
	uboss.dispatch("lua", function(_,_,cmd,...)
		local f = CMD[cmd]
		f(...)
	end)
end)
