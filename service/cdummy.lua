local uboss = require "uboss"
require "uboss.manager"	-- import uboss.launch, ...

local globalname = {}
local queryname = {}
local harbor = {}
local harbor_service

uboss.register_protocol {
	name = "harbor",
	id = uboss.PTYPE_HARBOR,
	pack = function(...) return ... end,
	unpack = uboss.tostring,
}

uboss.register_protocol {
	name = "text",
	id = uboss.PTYPE_TEXT,
	pack = function(...) return ... end,
	unpack = uboss.tostring,
}

local function response_name(name)
	local address = globalname[name]
	if queryname[name] then
		local tmp = queryname[name]
		queryname[name] = nil
		for _,resp in ipairs(tmp) do
			resp(true, address)
		end
	end
end

function harbor.REGISTER(name, handle)
	assert(globalname[name] == nil)
	globalname[name] = handle
	response_name(name)
	uboss.redirect(harbor_service, handle, "harbor", 0, "N " .. name)
end

function harbor.QUERYNAME(name)
	if name:byte() == 46 then	-- "." , local name
		uboss.ret(uboss.pack(uboss.localname(name)))
		return
	end
	local result = globalname[name]
	if result then
		uboss.ret(uboss.pack(result))
		return
	end
	local queue = queryname[name]
	if queue == nil then
		queue = { uboss.response() }
		queryname[name] = queue
	else
		table.insert(queue, uboss.response())
	end
end

function harbor.LINK(id)
	uboss.ret()
end

function harbor.CONNECT(id)
	uboss.error("Can't connect to other harbor in single node mode")
end

uboss.start(function()
	local harbor_id = tonumber(uboss.getenv "harbor")
	assert(harbor_id == 0)

	uboss.dispatch("lua", function (session,source,command,...)
		local f = assert(harbor[command])
		f(...)
	end)
	uboss.dispatch("text", function(session,source,command)
		-- ignore all the command
	end)

	harbor_service = assert(uboss.launch("harbor", harbor_id, uboss.self()))
end)
