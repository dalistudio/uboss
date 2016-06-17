local uboss = require "uboss"

local command = {}
local database = {}
local wait_queue = {}
local mode = {}

-- 查询
local function query(db, key, ...)
	if key == nil then
		return db
	else
		return query(db[key], ...)
	end
end

-- 命令：查询
function command.QUERY(key, ...)
	local d = database[key]
	if d then
		return query(d, ...)
	end
end

-- 更新
local function update(db, key, value, ...)
	if select("#",...) == 0 then
		local ret = db[key]
		db[key] = value
		return ret, value
	else
		if db[key] == nil then
			db[key] = {}
		end
		return update(db[key], value, ...)
	end
end

-- 唤醒
local function wakeup(db, key1, ...)
	if key1 == nil then
		return
	end
	local q = db[key1]
	if q == nil then
		return
	end
	if q[mode] == "queue" then
		db[key1] = nil
		if select("#", ...) ~= 1 then
			-- throw error because can't wake up a branch
			for _,response in ipairs(q) do
				response(false)
			end
		else
			return q
		end
	else
		-- it's branch
		return wakeup(q , ...)
	end
end

-- 命令：更新
function command.UPDATE(...)
	local ret, value = update(database, ...)
	if ret or value == nil then
		return ret
	end
	local q = wakeup(wait_queue, ...)
	if q then
		for _, response in ipairs(q) do
			response(true,value)
		end
	end
end

local function waitfor(db, key1, key2, ...)
	if key2 == nil then
		-- push queue
		local q = db[key1]
		if q == nil then
			q = { [mode] = "queue" }
			db[key1] = q
		else
			assert(q[mode] == "queue")
		end
		table.insert(q, uboss.response())
	else
		local q = db[key1]
		if q == nil then
			q = { [mode] = "branch" }
			db[key1] = q
		else
			assert(q[mode] == "branch")
		end
		return waitfor(q, key2, ...)
	end
end

-- 启动服务
uboss.start(function()
	uboss.dispatch("lua", function (_, _, cmd, ...)
		if cmd == "WAIT" then
			local ret = command.QUERY(...)
			if ret then
				uboss.ret(uboss.pack(ret))
			else
				waitfor(wait_queue, ...)
			end
		else
			local f = assert(command[cmd])
			uboss.ret(uboss.pack(f(...)))
		end
	end)
end)
