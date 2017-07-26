local uboss = require "uboss"

local function timeout(t)
	print(t)
end

local function wakeup(co)
	for i=1,5 do
		uboss.sleep(50)
		uboss.wakeup(co)
	end
end

local function test()
	uboss.timeout(10, function() print("test timeout 10") end)
	for i=1,10 do
		print("test sleep",i,uboss.now())
		uboss.sleep(1)
	end
end

uboss.start(function()
	test()

	uboss.fork(wakeup, coroutine.running())
	uboss.timeout(300, function() timeout "Hello World" end)
	for i = 1, 10 do
		print(i, uboss.now())
		print(uboss.sleep(100))
	end
	uboss.exit()
	print("Test timer exit")

end)
