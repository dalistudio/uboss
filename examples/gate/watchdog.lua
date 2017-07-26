local uboss = require "uboss"
local netpack = require "netpack"

local CMD = {}
local SOCKET = {}
local gate
local agent = {}

-- 网络：打开
function SOCKET.open(fd, addr)
	uboss.error("New client from : " .. addr)
	
	-- 新服务：agent
	agent[fd] = uboss.newservice("agent")

	-- 调用
	uboss.call(agent[fd], "lua", "start", { gate = gate, client = fd, watchdog = uboss.self() })
end

-- 关闭代理
local function close_agent(fd)
	local a = agent[fd]
	agent[fd] = nil
	if a then
		uboss.call(gate, "lua", "kick", fd)
		-- disconnect never return
		uboss.send(a, "lua", "disconnect")
	end
end

-- 网络：关闭
function SOCKET.close(fd)
	print("socket close",fd)
	close_agent(fd)
end

-- 网络：错误
function SOCKET.error(fd, msg)
	print("socket error",fd, msg)
	close_agent(fd)
end

-- 网络：警告
function SOCKET.warning(fd, size)
	-- size K bytes havn't send out in fd
	print("socket warning", fd, size)
end

-- 网络：数据
function SOCKET.data(fd, msg)
end

-- 命令：启动
function CMD.start(conf)
	uboss.call(gate, "lua", "open" , conf)
end

-- 命令：关闭
function CMD.close(fd)
	close_agent(fd)
end

-- 启动服务
uboss.start(function()
	-- 调度
	uboss.dispatch("lua", function(session, source, cmd, subcmd, ...)
		if cmd == "socket" then
			local f = SOCKET[subcmd]
			f(...)
			-- socket api don't need return
		else
			local f = assert(CMD[cmd])

			-- 返回
			uboss.ret(uboss.pack(f(subcmd, ...)))
		end
	end)

	-- 新服务：gate
	gate = uboss.newservice("gate")
end)
