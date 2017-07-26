-- This is a deprecated module, use uboss.queue instead.

local uboss = require "uboss"
local c = require "uboss.core"

local mqueue = {}
local init_once
local thread_id
local message_queue = {}

-- 注册 queue 协议类型
uboss.register_protocol {
	name = "queue",
	-- please read uboss.h for magic number 8
	id = 8,
	pack = uboss.pack,
	unpack = uboss.unpack,
	dispatch = function(session, from, ...)
		table.insert(message_queue, {session = session, addr = from, ... })
		if thread_id then
			uboss.wakeup(thread_id)
			thread_id = nil
		end
	end
}

-- 执行函数
local function do_func(f, msg)
	return pcall(f, table.unpack(msg))
end

-- 消息调度
local function message_dispatch(f)
	while true do
		if #message_queue==0 then
			thread_id = coroutine.running()
			uboss.wait()
		else
			local msg = table.remove(message_queue,1)
			local session = msg.session
			if session == 0 then
				local ok, msg = do_func(f, msg)
				if ok then
					if msg then
						uboss.fork(message_dispatch,f)
						error(string.format("[:%x] send a message to [:%x] return something", msg.addr, uboss.self()))
					end
				else
					uboss.fork(message_dispatch,f)
					error(string.format("[:%x] send a message to [:%x] throw an error : %s", msg.addr, uboss.self(),msg))
				end
			else
				local data, size = uboss.pack(do_func(f,msg))
				-- 1 means response
				c.send(msg.addr, 1, session, data, size)
			end
		end
	end
end

-- 注册
function mqueue.register(f)
	assert(init_once == nil)
	init_once = true
	uboss.fork(message_dispatch,f)
end

-- 抓住
local function catch(succ, ...)
	if succ then
		return ...
	else
		error(...)
	end
end

-- 调用
function mqueue.call(addr, ...)
	return catch(uboss.call(addr, "queue", ...))
end

-- 发送
function mqueue.send(addr, ...)
	return uboss.send(addr, "queue", ...)
end

-- 消息队列的长度
function mqueue.size()
	return #message_queue
end

return mqueue
