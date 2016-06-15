local uboss = require "uboss"

uboss.register_protocol {
	name = "client",
	id = uboss.PTYPE_CLIENT,
	unpack = uboss.tostring,
}

local gate
local userid, subid

local CMD = {}

function CMD.login(source, uid, sid, secret)
	-- you may use secret to make a encrypted data stream
	uboss.error(string.format("%s is login", uid))
	gate = source
	userid = uid
	subid = sid
	-- you may load user data from database
end

local function logout()
	if gate then
		uboss.call(gate, "lua", "logout", userid, subid)
	end
	uboss.exit()
end

function CMD.logout(source)
	-- NOTICE: The logout MAY be reentry
	uboss.error(string.format("%s is logout", userid))
	logout()
end

function CMD.afk(source)
	-- the connection is broken, but the user may back
	uboss.error(string.format("AFK"))
end

uboss.start(function()
	-- If you want to fork a work thread , you MUST do it in CMD.login
	uboss.dispatch("lua", function(session, source, command, ...)
		local f = assert(CMD[command])
		uboss.ret(uboss.pack(f(source, ...)))
	end)

	uboss.dispatch("client", function(_,_, msg)
		-- the simple echo service
		uboss.sleep(10)	-- sleep a while
		uboss.ret(msg)
	end)
end)
