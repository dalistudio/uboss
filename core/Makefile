# Makefile for building uBoss
# See ../doc/readme.html for installation and customization instructions.

# == CHANGE THE SETTINGS BELOW TO SUIT YOUR ENVIRONMENT =======================

# Your platform. See PLATS for possible values.
PLAT= none

CC= gcc -std=gnu99
CFLAGS= -O2 -fPIC -Wall -Wl,-E -DNOUSE_JEMALLOC $(SYSCFLAGS) $(MYCFLAGS)
LDFLAGS= $(SYSLDFLAGS) $(MYLDFLAGS)
LIBS= ../3rd/lua-5.3.2/src/liblua.a -lpthread -ldl -lm $(SYSLIBS) $(MYLIBS)

SYSCFLAGS=
SYSLDFLAGS=
SYSLIBS=

MYCFLAGS=  -I../3rd/lua-5.3.2/src
MYLDFLAGS= 
MYLIBS=
MYOBJS=

# == END OF USER SETTINGS -- NO NEED TO CHANGE ANYTHING BELOW THIS LINE =======

PLATS= linux macosx freebsd

CORE_O=	uboss_malloc.o uboss_start.o uboss_handle.o uboss_server.o \
	uboss_module.o uboss_mq.o uboss_monitor.o uboss_harbor.o \
	uboss_error.o uboss_log.o uboss_timer.o uboss_daemon.o uboss_env.o
	
BASE_O= $(CORE_O) $(MYOBJS)

UBOSS_T= uboss
UBOSS_O= uboss.o

ALL_O= $(BASE_O) $(UBOSS_O)
ALL_T= $(UBOSS_T)

# Targets start here.
default: $(PLAT)

all:	$(ALL_T)

o:	$(ALL_O)

$(UBOSS_T): $(UBOSS_O) $(BASE_O)
	$(CC) $(CFLAGS) -o $@ $(LDFLAGS) $(UBOSS_O) $(BASE_O) $(LIBS)

clean:
	$(RM) $(ALL_T) $(ALL_O)

echo:
	@echo "PLAT= $(PLAT)"
	@echo "CC= $(CC)"
	@echo "CFLAGS= $(CFLAGS)"
	@echo "LDFLAGS= $(SYSLDFLAGS)"
	@echo "LIBS= $(LIBS)"

# Convenience targets for popular platforms
ALL= all

none:
	@echo "Please do 'make PLATFORM' where PLATFORM is one of these:"
	@echo "   $(PLATS)"

linux:
	$(MAKE) $(ALL) SYSLIBS="-Wl,-E -ldl"

macosx:
	$(MAKE) $(ALL) CC=cc

freebsd:
	$(MAKE) $(ALL) SYSLIBS="-Wl,-E"


# list targets that do not create files (but not all makes understand .PHONY)
.PHONY: all $(PLATS) default o a clean depend echo none

# DO NOT DELETE

uboss.o: uboss.c
uboss_malloc.o: uboss_malloc.h uboss_malloc.c
uboss_env.o: uboss_env.h uboss_env.c
uboss_start.o: uboss_start.c
uboss_handle.o: uboss_handle.h uboss_handle.c
uboss_module.o: uboss_module.h uboss_module.c
uboss_mq.o: uboss_mq.h uboss_mq.c
uboss_server.o: uboss_server.h uboss_server.c
uboss_monitor.o: uboss_monitor.h uboss_monitor.c
uboss_harbor.o: uboss_harbor.h uboss_harbor.c
uboss_error.o: uboss_error.c
uboss_log.o: uboss_log.h uboss_log.c
uboss_timer.o: uboss_timer.h uboss_timer.c
uboss_daemon.o: uboss_daemon.h uboss_daemon.c

# (end of Makefile)
