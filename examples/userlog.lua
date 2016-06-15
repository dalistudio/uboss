local uboss = require "uboss"
require "uboss.manager"

uboss.register_protocol {
	name = "text",
	id = uboss.PTYPE_TEXT,
	unpack = uboss.tostring,
	dispatch = function(_, address, msg)
		print(string.format(":%08x(%.2f): %s", address, uboss.time(), msg))
	end
}

uboss.register_protocol {
	name = "SYSTEM",
	id = uboss.PTYPE_SYSTEM,
	unpack = function(...) return ... end,
	dispatch = function()
		-- reopen signal
		print("SIGHUP")
	end
}

uboss.start(function()
	uboss.register ".logger"
end)
