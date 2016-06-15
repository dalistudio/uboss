local uboss = require "uboss"

uboss.start(function()
	local loginserver = uboss.newservice("logind")
	local gate = uboss.newservice("gated", loginserver)

	uboss.call(gate, "lua", "open" , {
		port = 8888,
		maxclient = 64,
		servername = "sample",
	})
end)
