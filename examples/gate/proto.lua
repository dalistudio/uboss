local sprotoparser = require "sprotoparser"

local proto = {}

-- 客户端 发送到 服务端的协议
proto.c2s = sprotoparser.parse [[
.package {
	type 0 : integer
	session 1 : integer
}

handshake 1 {
	response {
		msg 0  : string
	}
}

get 2 {
	request {
		what 0 : string
	}
	response {
		result 0 : string
	}
}

set 3 {
	request {
		what 0 : string
		value 1 : string
	}
}

quit 4 {}

]]

-- 服务端 发送到 客户端的协议
proto.s2c = sprotoparser.parse [[
.package {
	type 0 : integer
	session 1 : integer
}

heartbeat 1 {}
]]

return proto
