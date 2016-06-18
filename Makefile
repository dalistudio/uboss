####################
#
# uBoss 编译脚本
#
####################


# 包含平台参数文件
include platform.mk

# uBoss 生成路径
UBOSS_BUILD_PATH ?= .

# uBoss 模块的路径
MODULE_PATH ?= module

# Lua 库的路径
LUA_LIB_PATH ?= lib

# 定义标志
CFLAGS = -g -O2 -Wall -I$(LUA_INC) $(MYCFLAGS)
# CFLAGS += -DUSE_PTHREAD_LOCK


###
# 编译 lua
###

# lua静态库的路径
LUA_STATICLIB := 3rd/lua/src/liblua.a
LUA_LIB ?= $(LUA_STATICLIB)
LUA_INC ?= 3rd/lua/src

$(LUA_STATICLIB) :
	cd 3rd/lua && $(MAKE) CC='$(CC) -std=gnu99' $(PLAT)

###
# 编译 jemalloc 
###

JEMALLOC_STATICLIB := 3rd/jemalloc/lib/libjemalloc_pic.a
JEMALLOC_INC := 3rd/jemalloc/include/jemalloc

all : jemalloc
	
.PHONY : jemalloc update3rd

MALLOC_STATICLIB := $(JEMALLOC_STATICLIB)

$(JEMALLOC_STATICLIB) : 3rd/jemalloc/Makefile
	cd 3rd/jemalloc && $(MAKE) CC=$(CC) 

3rd/jemalloc/autogen.sh :
	git submodule update --init

3rd/jemalloc/Makefile : | 3rd/jemalloc/autogen.sh
	cd 3rd/jemalloc && ./autogen.sh --with-jemalloc-prefix=je_ --disable-valgrind

jemalloc : $(MALLOC_STATICLIB)

update3rd :
	rm -rf 3rd/jemalloc && git submodule update --init

###
# uboss
###

# uBoss 的模块
MODULE = luavm logger harbor monitor gate

# Lua 的库
LUA_CLIB = uboss socketdriver profile netpack stm bson crypt clientsocket \
  memory multicast mongo mysqlaux sharedata debugchannel sproto cluster \
  md5 lpeg

# uBoss 核心
UBOSS_CORE = uboss.c uboss_handle.c uboss_module.c uboss_mq.c \
  uboss_server.c uboss_start.c uboss_timer.c uboss_error.c \
  uboss_harbor.c uboss_env.c uboss_monitor.c uboss_socket.c socket_server.c \
  uboss_malloc.c uboss_daemon.c uboss_log.c uboss_command.c uboss_license.c

all : \
  $(UBOSS_BUILD_PATH)/uboss \
  $(foreach v, $(MODULE), $(MODULE_PATH)/$(v).so) \
  $(foreach v, $(LUA_CLIB), $(LUA_LIB_PATH)/$(v).so) 

$(UBOSS_BUILD_PATH)/uboss : $(foreach v, $(UBOSS_CORE), core/$(v)) $(LUA_LIB) $(MALLOC_STATICLIB)
	$(CC) $(CFLAGS) -o $@ $^ -Icore -I$(JEMALLOC_INC) $(LDFLAGS) $(EXPORT) $(UBOSS_LIBS) $(UBOSS_DEFINES)

$(LUA_LIB_PATH) :
	mkdir $(LUA_LIB_PATH)

$(MODULE_PATH) :
	mkdir $(MODULE_PATH)

###
# 编译 uBoss 模块
###
define MODULE_TEMP
  $$(MODULE_PATH)/$(1).so : module/$(1)/module_$(1).c | $$(MODULE_PATH)
	$$(CC) $$(CFLAGS) $$(SHARED) $$< -o $$@ -Icore
endef

$(foreach v, $(MODULE), $(eval $(call MODULE_TEMP,$(v))))

###
# 编译 lua 库
###
$(LUA_LIB_PATH)/uboss.so : lib/uboss/lua-uboss.c lib/uboss/lua-seri.c | $(LUA_LIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Icore -Imodule -Ilib

$(LUA_LIB_PATH)/socketdriver.so : lib/socket/lua-socket.c | $(LUA_LIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Icore -Imodule

$(LUA_LIB_PATH)/profile.so : lib/profile/lua-profile.c | $(LUA_LIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ 

$(LUA_LIB_PATH)/netpack.so : lib/netpack/lua-netpack.c | $(LUA_LIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Icore -Imodule -Ilib

$(LUA_LIB_PATH)/stm.so : lib/stm/lua-stm.c | $(LUA_LIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Icore -Imodule -Ilib

$(LUA_LIB_PATH)/bson.so : lib/bson/lua-bson.c | $(LUA_LIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Icore

$(LUA_LIB_PATH)/crypt.so : lib/crypt/lua-crypt.c lib/crypt/lsha1.c | $(LUA_LIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@

$(LUA_LIB_PATH)/cluster.so : lib/cluster/lua-cluster.c | $(LUA_LIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Icore -Imodule -Ilib

$(LUA_LIB_PATH)/clientsocket.so : lib/clientsocket/lua-clientsocket.c | $(LUA_LIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -lpthread

$(LUA_LIB_PATH)/memory.so : lib/memory/lua-memory.c | $(LUA_LIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Icore

$(LUA_LIB_PATH)/multicast.so : lib/multicast/lua-multicast.c | $(LUA_LIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Icore

$(LUA_LIB_PATH)/mongo.so : lib/mongo/lua-mongo.c | $(LUA_LIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Icore

$(LUA_LIB_PATH)/mysqlaux.so : lib/mysqlaux/lua-mysqlaux.c | $(LUA_LIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@

$(LUA_LIB_PATH)/sharedata.so : lib/sharedata/lua-sharedata.c | $(LUA_LIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Icore

$(LUA_LIB_PATH)/debugchannel.so : lib/debugchannel/lua-debugchannel.c | $(LUA_LIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Icore

$(LUA_LIB_PATH)/sproto.so : lib/sproto/sproto.c lib/sproto/lsproto.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Ilib/sproto

$(LUA_LIB_PATH)/md5.so : lib/md5/md5.c lib/md5/md5lib.c lib/md5/compat-5.2.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Ilib/md5

$(LUA_LIB_PATH)/lpeg.so : lib/lpeg/lpcap.c lib/lpeg/lpcode.c lib/lpeg/lpprint.c lib/lpeg/lptree.c lib/lpeg/lpvm.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Ilib/lpeg 

###
# 清理项目
###
clean :
	rm -f $(UBOSS_BUILD_PATH)/uboss $(MODULE_PATH)/*.so $(LUA_LIB_PATH)/*.so $(UBOSS_BUILD_PATH)/core/*.o

###
# 清理所有项目，包括 Lua 和 Jemalloc 项目
###
cleanall: clean
ifneq (,$(wildcard 3rd/jemalloc/Makefile))
	cd 3rd/jemalloc && $(MAKE) clean
endif
	cd 3rd/lua && $(MAKE) clean
	rm -f $(LUA_STATICLIB)

