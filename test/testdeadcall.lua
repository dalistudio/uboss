local uboss = require "uboss"

local mode = ...

if mode == "test" then

uboss.start(function()
	uboss.dispatch("lua", function (...)
		print("====>", ...)
		uboss.exit()
	end)
end)

elseif mode == "dead" then

uboss.start(function()
	uboss.dispatch("lua", function (...)
		uboss.sleep(100)
		print("return", uboss.ret "")
	end)
end)

else

	uboss.start(function()
		local test = uboss.newservice(SERVICE_NAME, "test")	-- launch self in test mode

		print(pcall(function()
			uboss.call(test,"lua", "dead call")
		end))

		local dead = uboss.newservice(SERVICE_NAME, "dead")	-- launch self in dead mode

		uboss.timeout(0, uboss.exit)	-- exit after a while, so the call never return
		uboss.call(dead, "lua", "whould not return")
	end)
end
