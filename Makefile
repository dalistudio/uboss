.PHONY : all clean 

CC = gcc 
CFLAGS = -g -O2 -Wall -Wl,-E -fPIC -DNOUSE_JEMALLOC -I3rd/lua-5.3.2/src -Icore -Icore/net
LDFLAGS = -L3rd/lua-5.3.2/src -llua -lpthread -ldl -lm #lua调用了标准数学库 -lm


all : \
	uboss

uboss : \
	core/malloc_hook.c \
	core/uboss_daemon.c \
	core/uboss_env.c \
	core/uboss_error.c \
	core/uboss_handle.c \
	core/uboss_harbor.c \
	core/uboss_log.c \
	core/uboss_main.c \
	core/uboss_module.c \
	core/uboss_monitor.c \
	core/uboss_mq.c \
	core/uboss_start.c \
	core/uboss_server.c \
	core/uboss_timer.c
	$(CC) $(CFLAGS) -c $^
	$(CC) $(CFLAGS) -o $@ \
	*.o $(LDFLAGS)
	rm *.o

clean :
	rm *.o *.a uboss
