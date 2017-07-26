local uboss = require "uboss"
local sprotoloader = require "sprotoloader"

-- 最大64个客户连接
local max_client = 64

uboss.start(function()
	uboss.error("Server start")

	-- 启动唯一的服务：加载协议 ./examples/gate/protoloader.lua
	uboss.uniqueservice("protoloader")

	-- 启动新服务：控制台 ./service/console.lua
	local console = uboss.newservice("console")

	-- 启动新服务：调试控制台 ./service/debug_console.lua
	uboss.newservice("debug_console",8000)

	-- 启动新服务：简单数据库 ./examples/gate/simpledb.lua
	uboss.newservice("simpledb")

	-- 启动新服务：看门狗 ./examples/gate/watchdog.lua
	local watchdog = uboss.newservice("watchdog")

	-- 以 lua 模式 启动 看门狗
	uboss.call(watchdog, "lua", "start", {
		port = 8888, -- 端口
		maxclient = max_client, -- 最大客户连接
		nodelay = true, -- 无延迟
	})

	uboss.error("Watchdog listen on", 8888)

	-- 退出 uboss
	uboss.exit()
end)
