-- You should use this module (uboss.coroutine) instead of origin lua coroutine in uboss framework

local coroutine = coroutine
-- origin lua coroutine module
local coroutine_resume = coroutine.resume
local coroutine_yield = coroutine.yield
local coroutine_status = coroutine.status
local coroutine_running = coroutine.running

local select = select
local ubossco = {}

ubossco.isyieldable = coroutine.isyieldable
ubossco.running = coroutine.running
ubossco.status = coroutine.status

local uboss_coroutines = setmetatable({}, { __mode = "kv" })

function ubossco.create(f)
	local co = coroutine.create(f)
	-- mark co as a uboss coroutine
	uboss_coroutines[co] = true
	return co
end

do -- begin ubossco.resume

	local profile = require "profile"
	-- uboss use profile.resume_co/yield_co instead of coroutine.resume/yield

	local uboss_resume = profile.resume_co
	local uboss_yield = profile.yield_co

	local function unlock(co, ...)
		uboss_coroutines[co] = true
		return ...
	end

	local function uboss_yielding(co, from, ...)
		uboss_coroutines[co] = false
		return unlock(co, uboss_resume(co, from, uboss_yield(from, ...)))
	end

	local function resume(co, from, ok, ...)
		if not ok then
			return ok, ...
		elseif coroutine_status(co) == "dead" then
			-- the main function exit
			uboss_coroutines[co] = nil
			return true, ...
		elseif (...) == "USER" then
			return true, select(2, ...)
		else
			-- blocked in uboss framework, so raise the yielding message
			return resume(co, from, uboss_yielding(co, from, ...))
		end
	end

	-- record the root of coroutine caller (It should be a uboss thread)
	local coroutine_caller = setmetatable({} , { __mode = "kv" })

function ubossco.resume(co, ...)
	local co_status = uboss_coroutines[co]
	if not co_status then
		if co_status == false then
			-- is running
			return false, "cannot resume a uboss coroutine suspend by uboss framework"
		end
		if coroutine_status(co) == "dead" then
			-- always return false, "cannot resume dead coroutine"
			return coroutine_resume(co, ...)
		else
			return false, "cannot resume none uboss coroutine"
		end
	end
	local from = coroutine_running()
	local caller = coroutine_caller[from] or from
	coroutine_caller[co] = caller
	return resume(co, caller, coroutine_resume(co, ...))
end

function ubossco.thread(co)
	co = co or coroutine_running()
	if uboss_coroutines[co] ~= nil then
		return coroutine_caller[co] , false
	else
		return co, true
	end
end

end -- end of ubossco.resume

function ubossco.status(co)
	local status = coroutine.status(co)
	if status == "suspended" then
		if uboss_coroutines[co] == false then
			return "blocked"
		else
			return "suspended"
		end
	else
		return status
	end
end

function ubossco.yield(...)
	return coroutine_yield("USER", ...)
end

do -- begin ubossco.wrap

	local function wrap_co(ok, ...)
		if ok then
			return ...
		else
			error(...)
		end
	end

function ubossco.wrap(f)
	local co = ubossco.create(function(...)
		return f(...)
	end)
	return function(...)
		return wrap_co(ubossco.resume(co, ...))
	end
end

end	-- end of ubossco.wrap

return ubossco
