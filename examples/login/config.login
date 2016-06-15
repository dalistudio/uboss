thread = 8
logger = nil
harbor = 0
start = "main"
bootstrap = "luavm bootstrap"	-- The service for bootstrap
lua_service = "./service/?.lua;./examples/login/?.lua;./framework/?.lua"
lua_loader = "framework/loader.lua"
cpath = "./cservice/?.so"
