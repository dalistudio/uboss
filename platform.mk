PLAT ?= none
PLATS = linux freebsd macosx

CC ?= gcc

.PHONY : none $(PLATS) clean all cleanall

#ifneq ($(PLAT), none)

.PHONY : default

default :
	$(MAKE) $(PLAT)

#endif

none :
	@echo "Please do 'make PLATFORM' where PLATFORM is one of these:"
	@echo "   $(PLATS)"

UBOSS_LIBS := -lpthread -lm
SHARED := -fPIC --shared
EXPORT := -Wl,-E

linux : PLAT = linux
macosx : PLAT = macosx
freebsd : PLAT = freebsd

macosx : SHARED := -fPIC -dynamiclib -Wl,-undefined,dynamic_lookup
macosx : EXPORT :=
macosx linux : UBOSS_LIBS += -ldl
linux freebsd : UBOSS_LIBS += -lrt

# 启用下面两行定义，将禁止 Jemalloc 库的使用
#linux : MALLOC_STATICLIB :=
#linux : UBOSS_DEFINES :=-DNOUSE_JEMALLOC

# Turn off jemalloc and malloc hook on macosx

macosx : MALLOC_STATICLIB :=
macosx : UBOSS_DEFINES :=-DNOUSE_JEMALLOC

linux macosx freebsd :
	$(MAKE) all PLAT=$@ UBOSS_LIBS="$(UBOSS_LIBS)" SHARED="$(SHARED)" EXPORT="$(EXPORT)" MALLOC_STATICLIB="$(MALLOC_STATICLIB)" UBOSS_DEFINES="$(UBOSS_DEFINES)"
