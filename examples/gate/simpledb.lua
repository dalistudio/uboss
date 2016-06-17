local uboss = require "uboss"
require "uboss.manager"	-- import uboss.register
local db = {}

local command = {}

-- 命令：获取
function command.GET(key)
	return db[key]
end

-- 命令：设置
function command.SET(key, value)
	local last = db[key]
	db[key] = value
	return last
end

-- 启动服务
uboss.start(function()
	-- 调度
	uboss.dispatch("lua", function(session, address, cmd, ...)
		local f = command[string.upper(cmd)]
		if f then
			-- 返回：封包
			uboss.ret(uboss.pack(f(...)))
		else
			error(string.format("Unknown command %s", tostring(cmd)))
		end
	end)

	-- 注册本服务
	uboss.register "SIMPLEDB"
end)
