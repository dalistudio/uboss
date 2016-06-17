local uboss = require "uboss"
local harbor = require "uboss.harbor"
require "uboss.manager"	-- import uboss.launch, ...
local memory = require "memory"

uboss.start(function()
	local sharestring = tonumber(uboss.getenv "sharestring" or 4096)
	memory.ssexpand(sharestring)

	-- 获得 standalone 配置项判断你启动的是一个 master 节点还是 slave 节点
	local standalone = uboss.getenv "standalone"

	-- 加载 luavm
	local launcher = assert(uboss.launch("luavm","launcher"))

	-- 注册名称
	uboss.name(".launcher", launcher)

	-- 获得节点编号
	local harbor_id = tonumber(uboss.getenv "harbor" or 0)

	-- 判断是否为单节点网络
	if harbor_id == 0 then
		assert(standalone ==  nil)

		-- 设置为单节点
		standalone = true
		uboss.setenv("standalone", "true")

		-- 启动 cdummy 服务，拦截对外广播的全局名字变更
		local ok, slave = pcall(uboss.newservice, "cdummy")
		if not ok then
			uboss.abort() -- 结束进程
		end
		uboss.name(".cslave", slave) -- 注册 从节点名称

	else
	-- 非单节点网络
		if standalone then
			-- 启动主节点
			if not pcall(uboss.newservice,"cmaster") then
				uboss.abort()
			end
		end

		-- 启动从节点
		local ok, slave = pcall(uboss.newservice, "cslave")
		if not ok then
			uboss.abort()
		end
		uboss.name(".cslave", slave)
	end

	if standalone then
		-- 启动数据中心，共享数据
		local datacenter = uboss.newservice "datacenterd"
		uboss.name("DATACENTER", datacenter)
	end

	-- 启动服务管理器
	uboss.newservice "service_mgr"

	-- 调用 start 或 main 函数
	pcall(uboss.newservice,uboss.getenv "start" or "main")
	uboss.exit()
end)
