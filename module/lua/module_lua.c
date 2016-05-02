#include "uboss.h"

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// uBoss 的 luaVM 沙盒
struct lua {
	lua_State * L; // luaVM
	struct uboss_context * ctx; // uBoss 上下文
};

// LUA_CACHELIB may defined in patched lua for shared proto
#ifdef LUA_CACHELIB

#define codecache luaopen_cache

#else

static int
cleardummy(lua_State *L) {
  return 0;
}

static int 
codecache(lua_State *L) {
	luaL_Reg l[] = {
		{ "clear", cleardummy },
		{ "mode", cleardummy },
		{ NULL, NULL },
	};
	luaL_newlib(L,l);
	lua_getglobal(L, "loadfile");
	lua_setfield(L, -2, "loadfile");
	return 1;
}

#endif

static int 
traceback (lua_State *L) {
	const char *msg = lua_tostring(L, 1);
	if (msg)
		luaL_traceback(L, L, msg, 1);
	else {
		lua_pushliteral(L, "(no error message)");
	}
	return 1;
}

static void
_report_launcher_error(struct uboss_context *ctx) {
	// sizeof "ERROR" == 5
	uboss_sendname(ctx, 0, ".launcher", PTYPE_TEXT, 0, "ERROR", 5);
}

static const char *
optstring(struct uboss_context *ctx, const char *key, const char * str) {
	const char * ret = uboss_command(ctx, "GETENV", key);
	if (ret == NULL) {
		return str;
	}
	return ret;
}

// 初始化
static int
_init(struct lua *l, struct uboss_context *ctx, const char * args, size_t sz) {
	lua_State *L = l->L; // 设置 luaVM 状态机的地址
	l->ctx = ctx; // 设置 uBoss 上下文
	lua_gc(L, LUA_GCSTOP, 0); // 设置回收机制的标志
	lua_pushboolean(L, 1);  /* signal for libraries to ignore env. vars. */
	lua_setfield(L, LUA_REGISTRYINDEX, "LUA_NOENV");
	luaL_openlibs(L); // 打开 lua 标准库
	lua_pushlightuserdata(L, ctx); // 压入用户数据
	lua_setfield(L, LUA_REGISTRYINDEX, "uboss_context");
	luaL_requiref(L, "uboss.codecache", codecache , 0);
	lua_pop(L,1); // 弹出

	// 设置变量
	const char *path = optstring(ctx, "lua_path","./service/lua/?.lua;./service/lua/?/init.lua");
	lua_pushstring(L, path);
	lua_setglobal(L, "LUA_PATH");
	const char *cpath = optstring(ctx, "lua_cpath","./lib/lua/?.so");
	lua_pushstring(L, cpath);
	lua_setglobal(L, "LUA_CPATH");
	const char *service = optstring(ctx, "luaservice", "./service/lua/?.lua");
	lua_pushstring(L, service);
	lua_setglobal(L, "LUA_SERVICE");
	const char *preload = uboss_command(ctx, "GETENV", "preload");
	lua_pushstring(L, preload);
	lua_setglobal(L, "LUA_PRELOAD");

	lua_pushcfunction(L, traceback);
	assert(lua_gettop(L) == 1);

	const char * loader = optstring(ctx, "lualoader", "./service/lua/loader.lua");

	int r = luaL_loadfile(L,loader);
	if (r != LUA_OK) {
		uboss_error(ctx, "Can't load %s : %s", loader, lua_tostring(L, -1));
		_report_launcher_error(ctx);
		return 1;
	}
	lua_pushlstring(L, args, sz);
	r = lua_pcall(L,1,0,1);
	if (r != LUA_OK) {
		uboss_error(ctx, "lua loader error : %s", lua_tostring(L, -1));
		_report_launcher_error(ctx);
		return 1;
	}
	lua_settop(L,0);

	lua_gc(L, LUA_GCRESTART, 0);

	return 0;
}

// 启动
static int
_launch(struct uboss_context * context, void *ud, int type, int session, uint32_t source , const void * msg, size_t sz) {
	assert(type == 0 && session == 0); // 断言
	struct lua *l = ud; // 结构地址
	uboss_callback(context, NULL, NULL); // 设置回调函数
	int err = _init(l, context, msg, sz); // 初始化
	if (err) {
		uboss_command(context, "EXIT", NULL); // 退出命令
	}

	return 0;
}

// 初始化
int
lua_init(struct lua *l, struct uboss_context *ctx, const char * args) {
	int sz = strlen(args); // 计算参数的长度
	char * tmp = uboss_malloc(sz); // 分配内存空间
	memcpy(tmp, args, sz); // 复制参数到内存空间
	uboss_callback(ctx, l , _launch); // 设置回调函数
	const char * self = uboss_command(ctx, "REG", NULL); // 执行注册命令
	uint32_t handle_id = strtoul(self+1, NULL, 16); // 获得 句柄值
	// it must be first message
	uboss_send(ctx, 0, handle_id, PTYPE_TAG_DONTCOPY,0, tmp, sz); // 发送消息到框架
	return 0;
}

// 创建
struct lua *
lua_create(void) {
	struct lua * l = uboss_malloc(sizeof(*l)); // 分配内存空间
	memset(l,0,sizeof(*l)); // 清空内存
	l->L = lua_newstate(uboss_lalloc, NULL); // 新建 luaVM 状态机
	return l;
}

// 释放
void
lua_release(struct lua *l) {
	lua_close(l->L); // 关闭 luaVM
	uboss_free(l); // 释放内存空间
}

// 信号
void
lua_signal(struct lua *l, int signal) {
	uboss_error(l->ctx, "recv a signal %d", signal);
#ifdef lua_checksig
	// If our lua support signal (modified lua version by uboss), trigger it.
	uboss_sig_L = l->L;
#endif
}
