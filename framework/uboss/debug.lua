local table = table
local extern_dbgcmd = {}

local function init(uboss, export)
	local internal_info_func

	function uboss.info_func(func)
		internal_info_func = func
	end

	local dbgcmd

	local function init_dbgcmd()
		dbgcmd = {}

		function dbgcmd.MEM()
			local kb, bytes = collectgarbage "count"
			uboss.ret(uboss.pack(kb,bytes))
		end

		function dbgcmd.GC()

			collectgarbage "collect"
		end

		function dbgcmd.STAT()
			local stat = {}
			stat.mqlen = uboss.mqlen()
			stat.task = uboss.task()
			uboss.ret(uboss.pack(stat))
		end

		function dbgcmd.TASK()
			local task = {}
			uboss.task(task)
			uboss.ret(uboss.pack(task))
		end

		function dbgcmd.INFO(...)
			if internal_info_func then
				uboss.ret(uboss.pack(internal_info_func(...)))
			else
				uboss.ret(uboss.pack(nil))
			end
		end

		function dbgcmd.EXIT()
			uboss.exit()
		end

		function dbgcmd.RUN(source, filename)
			local inject = require "uboss.inject"
			local output = inject(uboss, source, filename , export.dispatch, uboss.register_protocol)
			collectgarbage "collect"
			uboss.ret(uboss.pack(table.concat(output, "\n")))
		end

		function dbgcmd.TERM(service)
			uboss.term(service)
		end

		function dbgcmd.REMOTEDEBUG(...)
			local remotedebug = require "uboss.remotedebug"
			remotedebug.start(export, ...)
		end

		function dbgcmd.SUPPORT(pname)
			return uboss.ret(uboss.pack(uboss.dispatch(pname) ~= nil))
		end

		function dbgcmd.PING()
			return uboss.ret()
		end

		function dbgcmd.LINK()
			-- no return, raise error when exit
		end

		return dbgcmd
	end -- function init_dbgcmd

	local function _debug_dispatch(session, address, cmd, ...)
		dbgcmd = dbgcmd or init_dbgcmd() -- lazy init dbgcmd
		local f = dbgcmd[cmd] or extern_dbgcmd[cmd]
		assert(f, cmd)
		f(...)
	end

	uboss.register_protocol {
		name = "debug",
		id = assert(uboss.PTYPE_DEBUG),
		pack = assert(uboss.pack),
		unpack = assert(uboss.unpack),
		dispatch = _debug_dispatch,
	}
end

local function reg_debugcmd(name, fn)
	extern_dbgcmd[name] = fn
end

return {
	init = init,
	reg_debugcmd = reg_debugcmd,
}
