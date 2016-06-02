include platform.mk

LUA_CLIB_PATH ?= lib
MODULE_PATH ?= module

UBOSS_BUILD_PATH ?= .

CFLAGS = -g -O2 -Wall -I$(LUA_INC) $(MYCFLAGS)
# CFLAGS += -DUSE_PTHREAD_LOCK

# lua

LUA_STATICLIB := 3rd/lua/src/liblua.a
LUA_LIB ?= $(LUA_STATICLIB)
LUA_INC ?= 3rd/lua/src

$(LUA_STATICLIB) :
	cd 3rd/lua && $(MAKE) CC='$(CC) -std=gnu99' $(PLAT)

# jemalloc 

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

# uboss

MODULE = lua logger
LUA_CLIB = uboss 

UBOSS_SRC = uboss.c uboss_handle.c uboss_module.c uboss_mq.c \
  uboss_server.c uboss_start.c uboss_timer.c uboss_error.c \
  uboss_harbor.c uboss_env.c uboss_monitor.c uboss_socket.c socket_server.c \
  uboss_malloc.c uboss_daemon.c uboss_log.c uboss_command.c

all : \
  $(UBOSS_BUILD_PATH)/uboss \
  $(foreach v, $(MODULE), $(MODULE_PATH)/$(v).so) \
  $(foreach v, $(LUA_CLIB), $(LUA_CLIB_PATH)/$(v).so) 

$(UBOSS_BUILD_PATH)/uboss : $(foreach v, $(UBOSS_SRC), core/$(v)) $(LUA_LIB) $(MALLOC_STATICLIB)
	$(CC) $(CFLAGS) -o $@ $^ -Icore -I$(JEMALLOC_INC) $(LDFLAGS) $(EXPORT) $(UBOSS_LIBS) $(UBOSS_DEFINES)

$(LUA_CLIB_PATH) :
	mkdir $(LUA_CLIB_PATH)

$(MODULE_PATH) :
	mkdir $(MODULE_PATH)

define MODULE_TEMP
  $$(MODULE_PATH)/$(1).so : module/$(1)/module_$(1).c | $$(MODULE_PATH)
	$$(CC) $$(CFLAGS) $$(SHARED) $$< -o $$@ -Icore
endef

$(foreach v, $(MODULE), $(eval $(call MODULE_TEMP,$(v))))

# lua Lib
$(LUA_CLIB_PATH)/uboss.so : lib/uboss/lua-uboss.c lib/uboss/lua-seri.c | $(LUA_CLIB_PATH)
	$(CC) $(CFLAGS) $(SHARED) $^ -o $@ -Icore -Imodule -Ilib



clean :
	rm -f $(UBOSS_BUILD_PATH)/uboss $(MODULE_PATH)/*.so $(LUA_CLIB_PATH)/*.so

cleanall: clean
ifneq (,$(wildcard 3rd/jemalloc/Makefile))
	cd 3rd/jemalloc && $(MAKE) clean
endif
	cd 3rd/lua && $(MAKE) clean
	rm -f $(LUA_STATICLIB)

