local uboss = require "uboss"
local c = require "uboss.core"

-- 启动一个 C 模块
function uboss.launch(...)
	local addr = c.command("LAUNCH", table.concat({...}," "))
	if addr then
		return tonumber("0x" .. string.sub(addr , 2))
	end
end

-- 强行杀掉一个服务
function uboss.kill(name)
	if type(name) == "number" then
		uboss.send(".launcher","lua","REMOVE",name, true)
		name = uboss.address(name)
	end
	c.command("KILL",name)
end

-- 退出 uboss 进程
function uboss.abort()
	c.command("ABORT")
end

-- 全局
local function globalname(name, handle)
	local c = string.sub(name,1,1)
	assert(c ~= ':')
	if c == '.' then
		return false
	end

	assert(#name <= 16)	-- GLOBALNAME_LENGTH is 16, defined in uboss_harbor.h
	assert(tonumber(name) == nil)	-- global name can't be number

	local harbor = require "uboss.harbor"

	harbor.globalname(name, handle)

	return true
end

-- 给自己注册一个名字
function uboss.register(name)
	if not globalname(name) then
		c.command("REG", name)
	end
end

-- 为服务命名
function uboss.name(name, handle)
	if not globalname(name, handle) then
		c.command("NAME", name .. " " .. uboss.address(handle))
	end
end

local dispatch_message = uboss.dispatch_message

-- 将本服务实现为消息转发器，对一类消息进行转发
function uboss.forward_type(map, start_func)
	c.callback(function(ptype, msg, sz, ...)
		local prototype = map[ptype]
		if prototype then
			dispatch_message(prototype, msg, sz, ...)
		else
			dispatch_message(ptype, msg, sz, ...)
			c.trash(msg, sz)
		end
	end, true)
	uboss.timeout(0, function()
		uboss.init_service(start_func)
	end)
end

-- 过滤消息再处理
function uboss.filter(f ,start_func)
	c.callback(function(...)
		dispatch_message(f(...))
	end)
	uboss.timeout(0, function()
		uboss.init_service(start_func)
	end)
end

-- 给当前 uboss 进程设置一个全局的服务监控
function uboss.monitor(service, query)
	local monitor
	if query then
		monitor = uboss.queryservice(true, service)
	else
		monitor = uboss.uniqueservice(true, service)
	end
	assert(monitor, "Monitor launch failed")
	c.command("MONITOR", string.format(":%08x", monitor))
	return monitor
end

return uboss
