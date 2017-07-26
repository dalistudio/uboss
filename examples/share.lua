local uboss = require "uboss"
local sharedata = require "sharedata"

local mode = ...

if mode == "host" then

uboss.start(function()
	uboss.error("new foobar")
	sharedata.new("foobar", { a=1, b= { "hello",  "world" } })

	uboss.fork(function()
		uboss.sleep(200)	-- sleep 2s
		uboss.error("update foobar a = 2")
		sharedata.update("foobar", { a =2 })
		uboss.sleep(200)	-- sleep 2s
		uboss.error("update foobar a = 3")
		sharedata.update("foobar", { a = 3, b = { "change" } })
		uboss.sleep(100)
		uboss.error("delete foobar")
		sharedata.delete "foobar"
	end)
end)

else


uboss.start(function()
	uboss.newservice(SERVICE_NAME, "host")

	local obj = sharedata.query "foobar"

	local b = obj.b
	uboss.error(string.format("a=%d", obj.a))

	for k,v in ipairs(b) do
		uboss.error(string.format("b[%d]=%s", k,v))
	end

	-- test lua serialization
	local s = uboss.packstring(obj)
	local nobj = uboss.unpack(s)
	for k,v in pairs(nobj) do
		uboss.error(string.format("nobj[%s]=%s", k,v))
	end
	for k,v in ipairs(nobj.b) do
		uboss.error(string.format("nobj.b[%d]=%s", k,v))
	end

	for i = 1, 5 do
		uboss.sleep(100)
		uboss.error("second " ..i)
		for k,v in pairs(obj) do
			uboss.error(string.format("%s = %s", k , tostring(v)))
		end
	end

	local ok, err = pcall(function()
		local tmp = { b[1], b[2] }	-- b is invalid , so pcall should failed
	end)

	if not ok then
		uboss.error(err)
	end

	-- obj. b is not the same with local b
	for k,v in ipairs(obj.b) do
		uboss.error(string.format("b[%d] = %s", k , tostring(v)))
	end

	collectgarbage()
	uboss.error("sleep")
	uboss.sleep(100)
	b = nil
	collectgarbage()
	uboss.error("sleep")
	uboss.sleep(100)

	uboss.exit()
end)

end
