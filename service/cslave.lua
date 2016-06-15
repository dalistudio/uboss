local uboss = require "uboss"
local socket = require "socket"
require "uboss.manager"	-- import uboss.launch, ...
local table = table

local slaves = {}
local connect_queue = {}
local globalname = {}
local queryname = {}
local harbor = {}
local harbor_service
local monitor = {}
local monitor_master_set = {}

local function read_package(fd)
	local sz = socket.read(fd, 1)
	assert(sz, "closed")
	sz = string.byte(sz)
	local content = assert(socket.read(fd, sz), "closed")
	return uboss.unpack(content)
end

local function pack_package(...)
	local message = uboss.packstring(...)
	local size = #message
	assert(size <= 255 , "too long")
	return string.char(size) .. message
end

local function monitor_clear(id)
	local v = monitor[id]
	if v then
		monitor[id] = nil
		for _, v in ipairs(v) do
			v(true)
		end
	end
end

local function connect_slave(slave_id, address)
	local ok, err = pcall(function()
		if slaves[slave_id] == nil then
			local fd = assert(socket.open(address), "Can't connect to "..address)
			uboss.error(string.format("Connect to harbor %d (fd=%d), %s", slave_id, fd, address))
			slaves[slave_id] = fd
			monitor_clear(slave_id)
			socket.abandon(fd)
			uboss.send(harbor_service, "harbor", string.format("S %d %d",fd,slave_id))
		end
	end)
	if not ok then
		uboss.error(err)
	end
end

local function ready()
	local queue = connect_queue
	connect_queue = nil
	for k,v in pairs(queue) do
		connect_slave(k,v)
	end
	for name,address in pairs(globalname) do
		uboss.redirect(harbor_service, address, "harbor", 0, "N " .. name)
	end
end

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

local function monitor_master(master_fd)
	while true do
		local ok, t, id_name, address = pcall(read_package,master_fd)
		if ok then
			if t == 'C' then
				if connect_queue then
					connect_queue[id_name] = address
				else
					connect_slave(id_name, address)
				end
			elseif t == 'N' then
				globalname[id_name] = address
				response_name(id_name)
				if connect_queue == nil then
					uboss.redirect(harbor_service, address, "harbor", 0, "N " .. id_name)
				end
			elseif t == 'D' then
				local fd = slaves[id_name]
				slaves[id_name] = false
				if fd then
					monitor_clear(id_name)
					socket.close(fd)
				end
			end
		else
			uboss.error("Master disconnect")
			for _, v in ipairs(monitor_master_set) do
				v(true)
			end
			socket.close(master_fd)
			break
		end
	end
end

local function accept_slave(fd)
	socket.start(fd)
	local id = socket.read(fd, 1)
	if not id then
		uboss.error(string.format("Connection (fd =%d) closed", fd))
		socket.close(fd)
		return
	end
	id = string.byte(id)
	if slaves[id] ~= nil then
		uboss.error(string.format("Slave %d exist (fd =%d)", id, fd))
		socket.close(fd)
		return
	end
	slaves[id] = fd
	monitor_clear(id)
	socket.abandon(fd)
	uboss.error(string.format("Harbor %d connected (fd = %d)", id, fd))
	uboss.send(harbor_service, "harbor", string.format("A %d %d", fd, id))
end

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

local function monitor_harbor(master_fd)
	return function(session, source, command)
		local t = string.sub(command, 1, 1)
		local arg = string.sub(command, 3)
		if t == 'Q' then
			-- query name
			if globalname[arg] then
				uboss.redirect(harbor_service, globalname[arg], "harbor", 0, "N " .. arg)
			else
				socket.write(master_fd, pack_package("Q", arg))
			end
		elseif t == 'D' then
			-- harbor down
			local id = tonumber(arg)
			if slaves[id] then
				monitor_clear(id)
			end
			slaves[id] = false
		else
			uboss.error("Unknown command ", command)
		end
	end
end

function harbor.REGISTER(fd, name, handle)
	assert(globalname[name] == nil)
	globalname[name] = handle
	response_name(name)
	socket.write(fd, pack_package("R", name, handle))
	uboss.redirect(harbor_service, handle, "harbor", 0, "N " .. name)
end

function harbor.LINK(fd, id)
	if slaves[id] then
		if monitor[id] == nil then
			monitor[id] = {}
		end
		table.insert(monitor[id], uboss.response())
	else
		uboss.ret()
	end
end

function harbor.LINKMASTER()
	table.insert(monitor_master_set, uboss.response())
end

function harbor.CONNECT(fd, id)
	if not slaves[id] then
		if monitor[id] == nil then
			monitor[id] = {}
		end
		table.insert(monitor[id], uboss.response())
	else
		uboss.ret()
	end
end

function harbor.QUERYNAME(fd, name)
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
		socket.write(fd, pack_package("Q", name))
		queue = { uboss.response() }
		queryname[name] = queue
	else
		table.insert(queue, uboss.response())
	end
end

uboss.start(function()
	local master_addr = uboss.getenv "master"
	local harbor_id = tonumber(uboss.getenv "harbor")
	local slave_address = assert(uboss.getenv "address")
	local slave_fd = socket.listen(slave_address)
	uboss.error("slave connect to master " .. tostring(master_addr))
	local master_fd = assert(socket.open(master_addr), "Can't connect to master")

	uboss.dispatch("lua", function (_,_,command,...)
		local f = assert(harbor[command])
		f(master_fd, ...)
	end)
	uboss.dispatch("text", monitor_harbor(master_fd))

	harbor_service = assert(uboss.launch("harbor", harbor_id, uboss.self()))

	local hs_message = pack_package("H", harbor_id, slave_address)
	socket.write(master_fd, hs_message)
	local t, n = read_package(master_fd)
	assert(t == "W" and type(n) == "number", "slave shakehand failed")
	uboss.error(string.format("Waiting for %d harbors", n))
	uboss.fork(monitor_master, master_fd)
	if n > 0 then
		local co = coroutine.running()
		socket.start(slave_fd, function(fd, addr)
			uboss.error(string.format("New connection (fd = %d, %s)",fd, addr))
			if pcall(accept_slave,fd) then
				local s = 0
				for k,v in pairs(slaves) do
					s = s + 1
				end
				if s >= n then
					uboss.wakeup(co)
				end
			end
		end)
		uboss.wait()
	end
	socket.close(slave_fd)
	uboss.error("Shakehand ready")
	uboss.fork(ready)
end)
