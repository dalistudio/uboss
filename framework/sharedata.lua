local uboss = require "uboss"
local sd = require "sharedata.corelib"

local service

-- 初始化
uboss.init(function()
	-- 启动唯一服务
	service = uboss.uniqueservice "sharedatad"
end)

local sharedata = {}

-- 监视
local function monitor(name, obj, cobj)
	local newobj = cobj
	while true do
		newobj = uboss.call(service, "lua", "monitor", name, newobj)
		if newobj == nil then
			break
		end
		sd.update(obj, newobj)
	end
end

-- 查询共享数据
function sharedata.query(name)
	local obj = uboss.call(service, "lua", "query", name)
	local r = sd.box(obj)
	uboss.send(service, "lua", "confirm" , obj)
	uboss.fork(monitor,name, r, obj)
	return r
end

-- 新建共享数据
function sharedata.new(name, v, ...)
	uboss.call(service, "lua", "new", name, v, ...)
end

-- 更新共享数据
function sharedata.update(name, v, ...)
	uboss.call(service, "lua", "update", name, v, ...)
end

-- 删除共享数据
function sharedata.delete(name)
	uboss.call(service, "lua", "delete", name)
end

return sharedata
