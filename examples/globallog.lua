local uboss = require "uboss"
require "uboss.manager"	-- import uboss.register

uboss.start(function()
	uboss.dispatch("lua", function(session, address, ...)
		print("[GLOBALLOG]", uboss.address(address), ...)
	end)
	uboss.register ".log"
	uboss.register "LOG"
end)
