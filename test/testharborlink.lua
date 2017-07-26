local uboss = require "uboss"
local harbor = require "uboss.harbor"

uboss.start(function()
	print("wait for harbor 2")
	print("run uboss examples/config_log please")
	harbor.connect(2)
	print("harbor 2 connected")
	print("LOG =", uboss.address(harbor.queryname "LOG"))
	harbor.link(2)
	print("disconnected")
end)
