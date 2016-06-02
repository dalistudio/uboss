local c = require "uboss.core"
local tostring = tostring
local tonumber = tonumber
local coroutine = coroutine
local assert = assert
local pairs = pairs
local pcall = pcall

-- 性能剖析库
local profile = require "profile"

local coroutine_resume = profile.resume
local coroutine_yield = profile.yield

local proto = {}
local uboss = {
	-- read uboss.h
	PTYPE_TEXT = 0, -- 文本类型
	PTYPE_RESPONSE = 1, -- 响应类型
	PTYPE_MULTICAST = 2, -- 组播类型
	PTYPE_CLIENT = 3, -- 客户端类型
	PTYPE_SYSTEM = 4, -- 系统类型
	PTYPE_HARBOR = 5, -- 群集类型
	PTYPE_SOCKET = 6, -- 网络类型
	PTYPE_ERROR = 7, -- 错误类型
	PTYPE_QUEUE = 8,	-- 队列类型 used in deprecated mqueue, use uboss.queue instead
	PTYPE_DEBUG = 9, -- 调试类型
	PTYPE_LUA = 10, -- LUA类型
	PTYPE_SNAX = 11, -- SNAX 类型
}

-- 代码缓冲 code cache
uboss.cache = require "uboss.codecache"

-- 注册协议
function uboss.register_protocol(class)
	local name = class.name
	local id = class.id
	assert(proto[name] == nil)
	assert(type(name) == "string" and type(id) == "number" and id >=0 and id <=255)
	proto[name] = class
	proto[id] = class
end

local session_id_coroutine = {}
local session_coroutine_id = {}
local session_coroutine_address = {}
local session_response = {}
local unresponse = {}

local wakeup_session = {}
local sleep_session = {}

local watching_service = {}
local watching_session = {}
local dead_service = {}
local error_queue = {}
local fork_queue = {}

-- suspend is function
local suspend

-- 字符串转句柄
local function string_to_handle(str)
	return tonumber("0x" .. string.sub(str , 2))
end

----- monitor exit

-- 调度错误队列
local function dispatch_error_queue()
	local session = table.remove(error_queue,1)
	if session then
		local co = session_id_coroutine[session]
		session_id_coroutine[session] = nil
		return suspend(co, coroutine_resume(co, false))
	end
end

-- 错误调度
local function _error_dispatch(error_session, error_source)
	if error_session == 0 then
		-- service is down
		--  Don't remove from watching_service , because user may call dead service
		if watching_service[error_source] then
			dead_service[error_source] = true
		end
		for session, srv in pairs(watching_session) do
			if srv == error_source then
				table.insert(error_queue, session)
			end
		end
	else
		-- capture an error for error_session
		if watching_session[error_session] then
			table.insert(error_queue, error_session)
		end
	end
end

-- coroutine reuse

--协程池
local coroutine_pool = {}

-- 创建协程
local function co_create(f)
	local co = table.remove(coroutine_pool)
	if co == nil then
		co = coroutine.create(function(...)
			f(...)
			while true do
				f = nil
				coroutine_pool[#coroutine_pool+1] = co
				f = coroutine_yield "EXIT"
				f(coroutine_yield())
			end
		end)
	else
		coroutine_resume(co, f)
	end
	return co
end

-- 调度唤醒
local function dispatch_wakeup()
	local co = next(wakeup_session)
	if co then
		wakeup_session[co] = nil
		local session = sleep_session[co]
		if session then
			session_id_coroutine[session] = "BREAK"
			return suspend(co, coroutine_resume(co, false, "BREAK"))
		end
	end
end

-- 释放观察
local function release_watching(address)
	local ref = watching_service[address]
	if ref then
		ref = ref - 1
		if ref > 0 then
			watching_service[address] = ref
		else
			watching_service[address] = nil
		end
	end
end

-- suspend is local function
-- 暂停
function suspend(co, result, command, param, size)
	if not result then
		local session = session_coroutine_id[co]
		if session then -- coroutine may fork by others (session is nil)
			local addr = session_coroutine_address[co]
			if session ~= 0 then
				-- only call response error
				c.send(addr, uboss.PTYPE_ERROR, session, "")
			end
			session_coroutine_id[co] = nil
			session_coroutine_address[co] = nil
		end
		error(debug.traceback(co,tostring(command)))
	end
	if command == "CALL" then
		session_id_coroutine[param] = co
	elseif command == "SLEEP" then
		session_id_coroutine[param] = co
		sleep_session[co] = param
	elseif command == "RETURN" then
		local co_session = session_coroutine_id[co]
		local co_address = session_coroutine_address[co]
		if param == nil or session_response[co] then
			error(debug.traceback(co))
		end
		session_response[co] = true
		local ret
		if not dead_service[co_address] then
			ret = c.send(co_address, uboss.PTYPE_RESPONSE, co_session, param, size) ~= nil
			if not ret then
				-- If the package is too large, returns nil. so we should report error back
				c.send(co_address, uboss.PTYPE_ERROR, co_session, "")
			end
		elseif size ~= nil then
			c.trash(param, size)
			ret = false
		end
		return suspend(co, coroutine_resume(co, ret))
	elseif command == "RESPONSE" then
		local co_session = session_coroutine_id[co]
		local co_address = session_coroutine_address[co]
		if session_response[co] then
			error(debug.traceback(co))
		end
		local f = param
		local function response(ok, ...)
			if ok == "TEST" then
				if dead_service[co_address] then
					release_watching(co_address)
					unresponse[response] = nil
					f = false
					return false
				else
					return true
				end
			end
			if not f then
				if f == false then
					f = nil
					return false
				end
				error "Can't response more than once"
			end

			local ret
			if not dead_service[co_address] then
				if ok then
					ret = c.send(co_address, uboss.PTYPE_RESPONSE, co_session, f(...)) ~= nil
					if not ret then
						-- If the package is too large, returns false. so we should report error back
						c.send(co_address, uboss.PTYPE_ERROR, co_session, "")
					end
				else
					ret = c.send(co_address, uboss.PTYPE_ERROR, co_session, "") ~= nil
				end
			else
				ret = false
			end
			release_watching(co_address)
			unresponse[response] = nil
			f = nil
			return ret
		end
		watching_service[co_address] = watching_service[co_address] + 1
		session_response[co] = true
		unresponse[response] = true
		return suspend(co, coroutine_resume(co, response))
	elseif command == "EXIT" then
		-- coroutine exit
		local address = session_coroutine_address[co]
		release_watching(address)
		session_coroutine_id[co] = nil
		session_coroutine_address[co] = nil
		session_response[co] = nil
	elseif command == "QUIT" then
		-- service exit
		return
	elseif command == "USER" then
		-- See uboss.coutine for detail
		error("Call uboss.coroutine.yield out of uboss.coroutine.resume\n" .. debug.traceback(co))
	elseif command == nil then
		-- debug trace
		return
	else
		error("Unknown command : " .. command .. "\n" .. debug.traceback(co))
	end
	dispatch_wakeup()
	dispatch_error_queue()
end

-- 超时
function uboss.timeout(ti, func)
	local session = c.intcommand("TIMEOUT",ti)
	assert(session)
	local co = co_create(func)
	assert(session_id_coroutine[session] == nil)
	session_id_coroutine[session] = co
end

-- 休眠
function uboss.sleep(ti)
	local session = c.intcommand("TIMEOUT",ti)
	assert(session)
	local succ, ret = coroutine_yield("SLEEP", session)
	sleep_session[coroutine.running()] = nil
	if succ then
		return
	end
	if ret == "BREAK" then
		return "BREAK"
	else
		error(ret)
	end
end

--
function uboss.yield()
	return uboss.sleep(0)
end

-- 等待
function uboss.wait(co)
	local session = c.genid()
	local ret, msg = coroutine_yield("SLEEP", session)
	co = co or coroutine.running()
	sleep_session[co] = nil
	session_id_coroutine[session] = nil
end

local self_handle

-- 自己
function uboss.self()
	if self_handle then
		return self_handle
	end
	self_handle = string_to_handle(c.command("REG"))
	return self_handle
end

-- 本地名字
function uboss.localname(name)
	local addr = c.command("QUERY", name)
	if addr then
		return string_to_handle(addr)
	end
end

uboss.now = c.now

local starttime

-- 开始时间
function uboss.starttime()
	if not starttime then
		starttime = c.intcommand("STARTTIME")
	end
	return starttime
end

-- 获得时间
function uboss.time()
	return uboss.now()/100 + (starttime or uboss.starttime())
end

-- 退出
function uboss.exit()
	fork_queue = {}	-- no fork coroutine can be execute after uboss.exit
	uboss.send(".launcher","lua","REMOVE",uboss.self(), false)
	-- report the sources that call me
	for co, session in pairs(session_coroutine_id) do
		local address = session_coroutine_address[co]
		if session~=0 and address then
			c.redirect(address, 0, uboss.PTYPE_ERROR, session, "")
		end
	end
	for resp in pairs(unresponse) do
		resp(false)
	end
	-- report the sources I call but haven't return
	local tmp = {}
	for session, address in pairs(watching_session) do
		tmp[address] = true
	end
	for address in pairs(tmp) do
		c.redirect(address, 0, uboss.PTYPE_ERROR, 0, "")
	end
	c.command("EXIT")
	-- quit service
	coroutine_yield "QUIT"
end

-- 获得环境变量
function uboss.getenv(key)
	return (c.command("GETENV",key))
end

-- 设置环境变量
function uboss.setenv(key, value)
	c.command("SETENV",key .. " " ..value)
end

-- 发送消息
function uboss.send(addr, typename, ...)
	local p = proto[typename]
	return c.send(addr, p.id, 0 , p.pack(...))
end

-- 生成ID 
uboss.genid = assert(c.genid)

uboss.redirect = function(dest,source,typename,...)
	return c.redirect(dest, source, proto[typename].id, ...)
end

uboss.pack = assert(c.pack)
uboss.packstring = assert(c.packstring)
uboss.unpack = assert(c.unpack)
uboss.tostring = assert(c.tostring)
uboss.trash = assert(c.trash)

local function yield_call(service, session)
	watching_session[session] = service
	local succ, msg, sz = coroutine_yield("CALL", session)
	watching_session[session] = nil
	if not succ then
		error "call failed"
	end
	return msg,sz
end

function uboss.call(addr, typename, ...)
	local p = proto[typename]
	local session = c.send(addr, p.id , nil , p.pack(...))
	if session == nil then
		error("call to invalid address " .. uboss.address(addr))
	end
	return p.unpack(yield_call(addr, session))
end

function uboss.rawcall(addr, typename, msg, sz)
	local p = proto[typename]
	local session = assert(c.send(addr, p.id , nil , msg, sz), "call to invalid address")
	return yield_call(addr, session)
end

function uboss.ret(msg, sz)
	msg = msg or ""
	return coroutine_yield("RETURN", msg, sz)
end

-- 响应
function uboss.response(pack)
	pack = pack or uboss.pack
	return coroutine_yield("RESPONSE", pack)
end

function uboss.retpack(...)
	return uboss.ret(uboss.pack(...))
end

-- 唤醒
function uboss.wakeup(co)
	if sleep_session[co] and wakeup_session[co] == nil then
		wakeup_session[co] = true
		return true
	end
end

-- 调度
function uboss.dispatch(typename, func)
	local p = proto[typename]
	if func then
		local ret = p.dispatch
		p.dispatch = func
		return ret
	else
		return p and p.dispatch
	end
end

-- 未知请求
local function unknown_request(session, address, msg, sz, prototype)
	uboss.error(string.format("Unknown request (%s): %s", prototype, c.tostring(msg,sz)))
	error(string.format("Unknown session : %d from %x", session, address))
end

-- 调度未知请求
function uboss.dispatch_unknown_request(unknown)
	local prev = unknown_request
	unknown_request = unknown
	return prev
end

-- 未知响应
local function unknown_response(session, address, msg, sz)
	uboss.error(string.format("Response message : %s" , c.tostring(msg,sz)))
	error(string.format("Unknown session : %d from %x", session, address))
end

-- 调度未知响应
function uboss.dispatch_unknown_response(unknown)
	local prev = unknown_response
	unknown_response = unknown
	return prev
end

function uboss.fork(func,...)
	local args = table.pack(...)
	local co = co_create(function()
		func(table.unpack(args,1,args.n))
	end)
	table.insert(fork_queue, co)
	return co
end

-- 调度 raw 消息
local function raw_dispatch_message(prototype, msg, sz, session, source)
	-- uboss.PTYPE_RESPONSE = 1, read uboss.h
	if prototype == 1 then
		local co = session_id_coroutine[session]
		if co == "BREAK" then
			session_id_coroutine[session] = nil
		elseif co == nil then
			unknown_response(session, source, msg, sz)
		else
			session_id_coroutine[session] = nil
			suspend(co, coroutine_resume(co, true, msg, sz))
		end
	else
		local p = proto[prototype]
		if p == nil then
			if session ~= 0 then
				c.send(source, uboss.PTYPE_ERROR, session, "")
			else
				unknown_request(session, source, msg, sz, prototype)
			end
			return
		end
		local f = p.dispatch
		if f then
			local ref = watching_service[source]
			if ref then
				watching_service[source] = ref + 1
			else
				watching_service[source] = 1
			end
			local co = co_create(f)
			session_coroutine_id[co] = session
			session_coroutine_address[co] = source
			suspend(co, coroutine_resume(co, session,source, p.unpack(msg,sz)))
		else
			unknown_request(session, source, msg, sz, proto[prototype].name)
		end
	end
end

-- 调度消息
function uboss.dispatch_message(...)
	local succ, err = pcall(raw_dispatch_message,...)
	while true do
		local key,co = next(fork_queue)
		if co == nil then
			break
		end
		fork_queue[key] = nil
		local fork_succ, fork_err = pcall(suspend,co,coroutine_resume(co))
		if not fork_succ then
			if succ then
				succ = false
				err = tostring(fork_err)
			else
				err = tostring(err) .. "\n" .. tostring(fork_err)
			end
		end
	end
	assert(succ, tostring(err))
end

-- 新服务
function uboss.newservice(name, ...)
	return uboss.call(".launcher", "lua" , "LAUNCH", "snlua", name, ...)
end

-- 惟一的服务
function uboss.uniqueservice(global, ...)
	if global == true then
		return assert(uboss.call(".service", "lua", "GLAUNCH", ...))
	else
		return assert(uboss.call(".service", "lua", "LAUNCH", global, ...))
	end
end

-- 查询服务
function uboss.queryservice(global, ...)
	if global == true then
		return assert(uboss.call(".service", "lua", "GQUERY", ...))
	else
		return assert(uboss.call(".service", "lua", "QUERY", global, ...))
	end
end

-- 地址
function uboss.address(addr)
	if type(addr) == "number" then
		return string.format(":%08x",addr)
	else
		return tostring(addr)
	end
end

-- 群集地址
function uboss.harbor(addr)
	return c.harbor(addr)
end

uboss.error = c.error

----- register protocol
do
	local REG = uboss.register_protocol

	REG {
		name = "lua",
		id = uboss.PTYPE_LUA,
		pack = uboss.pack,
		unpack = uboss.unpack,
	}

	REG {
		name = "response",
		id = uboss.PTYPE_RESPONSE,
	}

	REG {
		name = "error",
		id = uboss.PTYPE_ERROR,
		unpack = function(...) return ... end,
		dispatch = _error_dispatch,
	}
end

local init_func = {}

-- 初始化
function uboss.init(f, name)
	assert(type(f) == "function")
	if init_func == nil then
		f()
	else
		table.insert(init_func, f)
		if name then
			assert(type(name) == "string")
			assert(init_func[name] == nil)
			init_func[name] = f
		end
	end
end

-- 初始化所有
local function init_all()
	local funcs = init_func
	init_func = nil
	if funcs then
		for _,f in ipairs(funcs) do
			f()
		end
	end
end

local function ret(f, ...)
	f()
	return ...
end

-- 初始化模板
local function init_template(start, ...)
	init_all()
	init_func = {}
	return ret(init_all, start(...))
end

function uboss.pcall(start, ...)
	return xpcall(init_template, debug.traceback, start, ...)
end

-- 初始化服务
function uboss.init_service(start)
	local ok, err = uboss.pcall(start)
	if not ok then
		uboss.error("init service failed: " .. tostring(err))
		uboss.send(".launcher","lua", "ERROR")
		uboss.exit()
	else
		uboss.send(".launcher","lua", "LAUNCHOK")
	end
end

-- 启动
function uboss.start(start_func)
	c.callback(uboss.dispatch_message)
	uboss.timeout(0, function()
		uboss.init_service(start_func)
	end)
end

-- 终止
function uboss.endless()
	return c.command("ENDLESS")~=nil
end

-- 消息队列的长度
function uboss.mqlen()
	return c.intcommand "MQLEN"
end

-- 任务
function uboss.task(ret)
	local t = 0
	for session,co in pairs(session_id_coroutine) do
		if ret then
			ret[session] = debug.traceback(co)
		end
		t = t + 1
	end
	return t
end

function uboss.term(service)
	return _error_dispatch(0, service)
end

function uboss.memlimit(bytes)
	debug.getregistry().memlimit = bytes
	uboss.memlimit = nil	-- set only once
end

local function clear_pool()
	coroutine_pool = {}
end

-- 注入内部调试框架 Inject internal debug framework
local debug = require "uboss.debug"
debug(uboss, {
	dispatch = uboss.dispatch_message,
	clear = clear_pool,
	suspend = suspend,
})

return uboss
