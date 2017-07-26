local uboss = require "uboss"
local netpack = require "netpack"
local socket = require "socket"
local sproto = require "sproto"
local sprotoloader = require "sprotoloader"

local WATCHDOG
local host
local send_request

local CMD = {}
local REQUEST = {}
local client_fd

-- 请求：获取
function REQUEST:get()
	print("get", self.what)
	local r = uboss.call("SIMPLEDB", "lua", "get", self.what)
	return { result = r }
end

-- 请求：设置
function REQUEST:set()
	print("set", self.what, self.value)
	local r = uboss.call("SIMPLEDB", "lua", "set", self.what, self.value)
end

-- 请求：握手
function REQUEST:handshake()
	return { msg = "Welcome to uboss, I will send heartbeat every 5 sec." }
end

-- 请求：退出
function REQUEST:quit()
	uboss.call(WATCHDOG, "lua", "close", client_fd)
end

-- 本地函数：请求
local function request(name, args, response)
	local f = assert(REQUEST[name])
	local r = f(args)
	if response then
		return response(r)
	end
end

-- 本地函数：发送封包
local function send_package(pack)
	local package = string.pack(">s2", pack)
	socket.write(client_fd, package)
end

-- 注册协议
uboss.register_protocol {
	name = "client",
	id = uboss.PTYPE_CLIENT,
	unpack = function (msg, sz)
		return host:dispatch(msg, sz)
	end,
	dispatch = function (_, _, type, ...)
		-- 请求
		if type == "REQUEST" then
			local ok, result  = pcall(request, ...)
			if ok then
				if result then
					send_package(result)
				end
			else
				uboss.error(result)
			end
		else
		-- 响应
			assert(type == "RESPONSE")
			error "This example doesn't support request client"
		end
	end
}

-- 命令：启动
function CMD.start(conf)
	local fd = conf.client -- 客户端
	local gate = conf.gate -- 网关
	WATCHDOG = conf.watchdog -- 看门狗
	-- slot 1,2 set at main.lua
	host = sprotoloader.load(1):host "package"
	send_request = host:attach(sprotoloader.load(2))
	uboss.fork(function()
		while true do
			-- 发送封包：心跳包 0.5/s
			send_package(send_request "heartbeat")
			uboss.sleep(500)
		end
	end)

	client_fd = fd

	-- 转发
	uboss.call(gate, "lua", "forward", fd)
end

-- 命令：断开连接
function CMD.disconnect()
	-- todo: do something before exit
	uboss.exit()
end

-- 启动服务
uboss.start(function()
	-- 调度
	uboss.dispatch("lua", function(_,_, command, ...)
		local f = CMD[command]
		uboss.ret(uboss.pack(f(...)))
	end)
end)
