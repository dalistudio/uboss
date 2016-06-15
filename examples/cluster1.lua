local uboss = require "uboss"
local cluster = require "cluster"
local snax = require "snax"

uboss.start(function()
	local sdb = uboss.newservice("simpledb")
	-- register name "sdb" for simpledb, you can use cluster.query() later.
	-- See cluster2.lua
	cluster.register("sdb", sdb)

	print(uboss.call(sdb, "lua", "SET", "a", "foobar"))
	print(uboss.call(sdb, "lua", "SET", "b", "foobar2"))
	print(uboss.call(sdb, "lua", "GET", "a"))
	print(uboss.call(sdb, "lua", "GET", "b"))
	cluster.open "db"
	cluster.open "db2"
	-- unique snax service
	snax.uniqueservice "pingserver"
end)
