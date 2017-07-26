local uboss = require "uboss"
local queue = require "uboss.queue"
local snax = require "snax"

local i = 0
local hello = "hello"

function response.ping(hello)
	uboss.sleep(100)
	return hello
end

-- response.sleep and accept.hello share one lock
local lock

function accept.sleep(queue, n)
	if queue then
		lock(
		function()
			print("queue=",queue, n)
			uboss.sleep(n)
		end)
	else
		print("queue=",queue, n)
		uboss.sleep(n)
	end
end

function accept.hello()
	lock(function()
	i = i + 1
	print (i, hello)
	end)
end

function accept.exit(...)
	snax.exit(...)
end

function response.error()
	error "throw an error"
end

function init( ... )
	print ("ping server start:", ...)
	snax.enablecluster()	-- enable cluster call
	-- init queue
	lock = queue()
end

function exit(...)
	print ("ping server exit:", ...)
end
