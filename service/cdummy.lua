local uboss = require "uboss"
require "uboss.manager"	-- import uboss.launch, ...

local globalname = {}
local queryname = {}
local harbor = {}
local harbor_service

-- 注册 harbor 协议类型
uboss.register_protocol {
	name = "harbor",
	id = uboss.PTYPE_HARBOR,
	pack = function(...) return ... end,
	unpack = uboss.tostring,
}

-- 注册 text 协议类型
uboss.register_protocol {
	name = "text",
	id = uboss.PTYPE_TEXT,
	pack = function(...) return ... end,
	unpack = uboss.tostring,
}

-- 响应名称
local function response_name(name)
	local address = globalname[name]
	if queryname[name] then
		local tmp = queryname[name]
		queryname[name] = nil
		for _,resp in ipairs(tmp) do
			resp(true, address)
		end
	end
end

-- 节点注册
function harbor.REGISTER(name, handle)
	assert(globalname[name] == nil)
	globalname[name] = handle
	response_name(name)
	uboss.redirect(harbor_service, handle, "harbor", 0, "N " .. name)
end

-- 节点查询名称
function harbor.QUERYNAME(name)
	if name:byte() == 46 then	-- "." , local name
		uboss.ret(uboss.pack(uboss.localname(name)))
		return
	end
	local result = globalname[name]
	if result then
		uboss.ret(uboss.pack(result))
		return
	end
	local queue = queryname[name]
	if queue == nil then
		queue = { uboss.response() }
		queryname[name] = queue
	else
		table.insert(queue, uboss.response())
	end
end

-- 节点链接
function harbor.LINK(id)
	uboss.ret()
end

-- 节点连接
function harbor.CONNECT(id)
	uboss.error("Can't connect to other harbor in single node mode")
end

-- 启动服务
uboss.start(function()
	-- 获得节点编号
	local harbor_id = tonumber(uboss.getenv "harbor")
	assert(harbor_id == 0)

	-- 调度 lua 类型消息
	uboss.dispatch("lua", function (session,source,command,...)
		local f = assert(harbor[command])
		f(...)
	end)

	-- 调度 text 类型消息
	uboss.dispatch("text", function(session,source,command)
		-- ignore all the command
	end)

	-- 加载 节点模块
	harbor_service = assert(uboss.launch("harbor", harbor_id, uboss.self()))
end)
