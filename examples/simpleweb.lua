local uboss = require "uboss"
local socket = require "socket"
local httpd = require "http.httpd"
local sockethelper = require "http.sockethelper"
local urllib = require "http.url"
local table = table
local string = string

local mode = ...

if mode == "agent" then

local function response(id, ...)
	local ok, err = httpd.write_response(sockethelper.writefunc(id), ...)
	if not ok then
		-- if err == sockethelper.socket_error , that means socket closed.
		uboss.error(string.format("fd = %d, %s", id, err))
	end
end

uboss.start(function()
	uboss.dispatch("lua", function (_,_,id)
		socket.start(id)
		-- limit request body size to 8192 (you can pass nil to unlimit)
		local code, url, method, header, body = httpd.read_request(sockethelper.readfunc(id), 8192)
		if code then
			if code ~= 200 then
				response(id, code)
			else
				local tmp = {}
				if header.host then
					table.insert(tmp, string.format("host: %s", header.host))
				end
				local path, query = urllib.parse(url)
				table.insert(tmp, string.format("path: %s", path))
				if query then
					local q = urllib.parse_query(query)
					for k, v in pairs(q) do
						table.insert(tmp, string.format("query: %s= %s", k,v))
					end
				end
				table.insert(tmp, "-----header----")
				for k,v in pairs(header) do
					table.insert(tmp, string.format("%s = %s",k,v))
				end
				table.insert(tmp, "-----body----\n" .. body)
				response(id, code, table.concat(tmp,"\n"))
			end
		else
			if url == sockethelper.socket_error then
				uboss.error("socket closed")
			else
				uboss.error(url)
			end
		end
		socket.close(id)
	end)
end)

else

uboss.start(function()
	local agent = {}
	for i= 1, 20 do
		agent[i] = uboss.newservice(SERVICE_NAME, "agent")
	end
	local balance = 1
	local id = socket.listen("0.0.0.0", 8001)
	uboss.error("Listen web port 8001")
	socket.start(id , function(id, addr)
		uboss.error(string.format("%s connected, pass it to agent :%08x", addr, agent[balance]))
		uboss.send(agent[balance], "lua", id)
		balance = balance + 1
		if balance > #agent then
			balance = 1
		end
	end)
end)

end
