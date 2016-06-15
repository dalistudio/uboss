local uboss = require "uboss"
local netpack = require "netpack"

local CMD = {}
local SOCKET = {}
local gate
local agent = {}

function SOCKET.open(fd, addr)
	uboss.error("New client from : " .. addr)
	agent[fd] = uboss.newservice("agent")
	uboss.call(agent[fd], "lua", "start", { gate = gate, client = fd, watchdog = uboss.self() })
end

local function close_agent(fd)
	local a = agent[fd]
	agent[fd] = nil
	if a then
		uboss.call(gate, "lua", "kick", fd)
		-- disconnect never return
		uboss.send(a, "lua", "disconnect")
	end
end

function SOCKET.close(fd)
	print("socket close",fd)
	close_agent(fd)
end

function SOCKET.error(fd, msg)
	print("socket error",fd, msg)
	close_agent(fd)
end

function SOCKET.warning(fd, size)
	-- size K bytes havn't send out in fd
	print("socket warning", fd, size)
end

function SOCKET.data(fd, msg)
end

function CMD.start(conf)
	uboss.call(gate, "lua", "open" , conf)
end

function CMD.close(fd)
	close_agent(fd)
end

uboss.start(function()
	uboss.dispatch("lua", function(session, source, cmd, subcmd, ...)
		if cmd == "socket" then
			local f = SOCKET[subcmd]
			f(...)
			-- socket api don't need return
		else
			local f = assert(CMD[cmd])
			uboss.ret(uboss.pack(f(subcmd, ...)))
		end
	end)

	gate = uboss.newservice("gate")
end)
