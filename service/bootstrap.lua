local uboss = require "uboss"
local harbor = require "uboss.harbor"
require "uboss.manager"	-- import uboss.launch, ...
local memory = require "memory"

uboss.start(function()
	local sharestring = tonumber(uboss.getenv "sharestring" or 4096)
	memory.ssexpand(sharestring)

	local standalone = uboss.getenv "standalone"

	local launcher = assert(uboss.launch("luavm","launcher"))
	uboss.name(".launcher", launcher)

	local harbor_id = tonumber(uboss.getenv "harbor" or 0)
	if harbor_id == 0 then
		assert(standalone ==  nil)
		standalone = true
		uboss.setenv("standalone", "true")

		local ok, slave = pcall(uboss.newservice, "cdummy")
		if not ok then
			uboss.abort()
		end
		uboss.name(".cslave", slave)

	else
		if standalone then
			if not pcall(uboss.newservice,"cmaster") then
				uboss.abort()
			end
		end

		local ok, slave = pcall(uboss.newservice, "cslave")
		if not ok then
			uboss.abort()
		end
		uboss.name(".cslave", slave)
	end

	if standalone then
		local datacenter = uboss.newservice "datacenterd"
		uboss.name("DATACENTER", datacenter)
	end
	uboss.newservice "service_mgr"
	pcall(uboss.newservice,uboss.getenv "start" or "main")
	uboss.exit()
end)
