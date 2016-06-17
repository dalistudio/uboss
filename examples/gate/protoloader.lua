-- module proto as examples/proto.lua
package.path = "./examples/gate/?.lua;" .. package.path

local uboss = require "uboss"
local sprotoparser = require "sprotoparser"
local sprotoloader = require "sprotoloader"
local proto = require "proto"

-- 启动服务
uboss.start(function()
	sprotoloader.save(proto.c2s, 1)
	sprotoloader.save(proto.s2c, 2)
	-- don't call uboss.exit() , because sproto.core may unload and the global slot become invalid
end)
