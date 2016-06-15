local uboss = require "uboss"
-- You should use uboss.coroutine instead of origin coroutine in uboss
local coroutine = require "uboss.coroutine"
local profile = require "profile"

local function status(co)
	repeat
		local status = coroutine.status(co)
		print("STATUS", status)
		uboss.sleep(100)
	until status == "suspended"

	repeat
		local ok, n = assert(coroutine.resume(co))
		print("status thread", n)
	until not n
	uboss.exit()
end

local function test(n)
	local co = coroutine.running()
	print ("begin", co, coroutine.thread(co))	-- false
	uboss.fork(status, co)
	for i=1,n do
		uboss.sleep(100)
		coroutine.yield(i)
	end
	print ("end", co)
end

local function main()
	local f = coroutine.wrap(test)
	coroutine.yield "begin"
	for i=1,3 do
		local n = f(5)
		print("main thread",n)
	end
	coroutine.yield "end"
	print("main thread time:", profile.stop(coroutine.thread()))
end

uboss.start(function()
	print("Main thead :", coroutine.thread())	-- true
	print(coroutine.resume(coroutine.running()))	-- always return false

	profile.start()

	local f = coroutine.wrap(main)
	print("main step", f())
	print("main step", f())
	print("main step", f())
--	print("main thread time:", profile.stop())
end)
