.PHONY : all clean 

CFLAGS = -g -O2 -Wall 
LDFLAGS = -lpthread -llua -ldl -lm

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
ifeq ($(uname_S), Darwin)
	SHARED = -fPIC -dynamiclib -Wl,-undefined,dynamic_lookup
else
	LDFLAGS += -ldl -lrt -Wl,-E
	SHARED = -fPIC --shared
endif

all : \
	lua.so


lua.so : service_lua.c
	gcc $(CFLAGS) $(SHARED) -I../../core -I../../3rd/lua-5.3.2/src $^ -o $@ 


