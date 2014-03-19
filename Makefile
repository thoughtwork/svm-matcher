CONFIG             := build

CFLAGS             += -O2 -fPIC -w -std=gnu99 -g
DFLAGS             +=  
IFLAGS             += -I$(CONFIG)/inc
LDFLAGS            += '-rdynamic' '-Wl,--enable-new-dtags' '-Wl,-rpath,$$ORIGIN/'
LIBPATHS           += 
LIBS               += -lpthread

TARGETS            := $(CONFIG)/bin/matcher


all build compile: prep $(TARGETS)

.PHONY: prep

prep:
	@if [ "$(CONFIG)" = "" ] ; then echo WARNING: CONFIG not set ; exit 255 ; fi
	@[ ! -x $(CONFIG)/bin ] && mkdir -p $(CONFIG)/bin; true
	@[ ! -x $(CONFIG)/inc ] && mkdir -p $(CONFIG)/inc; true
	@[ ! -x $(CONFIG)/obj ] && mkdir -p $(CONFIG)/obj; true
	
clean:
	rm -fr "$(CONFIG)"

#
#   mpr.h
#
$(CONFIG)/inc/mpr.h: \
	src/deps/mpr/mpr.h $(DEPS_1)
	@echo '      [Copy] $(CONFIG)/inc/mpr.h'
	cp src/deps/mpr/mpr.h $(CONFIG)/inc/mpr.h
	
#
#   list.h
#
$(CONFIG)/inc/list.h: \
	src/deps/list/list.h $(DEPS_2)
	@echo '      [Copy] $(CONFIG)/inc/list.h'
	cp src/deps/list/list.h $(CONFIG)/inc/list.h
	
#
#   xml.h
#
$(CONFIG)/inc/xml.h: \
	src/deps/xml/xml.h $(DEPS_3)
	@echo '      [Copy] $(CONFIG)/inc/xml.h'
	cp src/deps/xml/xml.h $(CONFIG)/inc/xml.h
	
#
#   xml.o
#
DEPS_4 += $(CONFIG)/inc/mpr.h
DEPS_4 += $(CONFIG)/inc/list.h
DEPS_4 += $(CONFIG)/inc/xml.h

$(CONFIG)/obj/xml.o: \
    src/deps/xml/xml.c $(DEPS_4)
	@echo '   [Compile] $(CONFIG)/obj/xml.o'
	$(CC) -c -o $(CONFIG)/obj/xml.o $(CFLAGS) $(DFLAGS) "$(IFLAGS)" src/deps/xml/xml.c

#
#   config.h
#
$(CONFIG)/inc/config.h: \
	src/modules/config/config.h $(DEPS_5)
	@echo '      [Copy] $(CONFIG)/inc/config.h'
	cp src/modules/config/config.h $(CONFIG)/inc/config.h
	
#
#   config.o
#
DEPS_6 += $(CONFIG)/inc/mpr.h
DEPS_6 += $(CONFIG)/inc/list.h
DEPS_6 += $(CONFIG)/inc/xml.h
DEPS_6 += $(CONFIG)/inc/matcher.h

$(CONFIG)/obj/config.o: \
    src/modules/config/config.c $(DEPS_6)
	@echo '   [Compile] $(CONFIG)/obj/config.o'
	$(CC) -c -o $(CONFIG)/obj/config.o $(CFLAGS) $(DFLAGS) "$(IFLAGS)" src/modules/config/config.c
	
#
#   packet.h
#
$(CONFIG)/inc/packet.h: \
	src/modules/packet/packet.h $(DEPS_7)
	@echo '      [Copy] $(CONFIG)/inc/packet.h'
	cp src/modules/packet/packet.h $(CONFIG)/inc/packet.h
	
#
#   packet.o
#
DEPS_8 += $(CONFIG)/inc/mpr.h
DEPS_8 += $(CONFIG)/inc/list.h
DEPS_8 += $(CONFIG)/inc/xml.h
DEPS_8 += $(CONFIG)/inc/matcher.h
DEPS_8 += $(CONFIG)/inc/svmsys.h

$(CONFIG)/obj/packet.o: \
    src/modules/packet/packet.c $(DEPS_8)
	@echo '   [Compile] $(CONFIG)/obj/packet.o'
	$(CC) -c -o $(CONFIG)/obj/packet.o $(CFLAGS) $(DFLAGS) "$(IFLAGS)" src/modules/packet/packet.c
	
#
#   svmsys.h
#
$(CONFIG)/inc/svmsys.h: \
	src/modules/svmsys/svmsys.h $(DEPS_9)
	@echo '      [Copy] $(CONFIG)/inc/svmsys.h'
	cp src/modules/svmsys/svmsys.h $(CONFIG)/inc/svmsys.h
	
#
#   svmsig.o
#
DEPS_10 += $(CONFIG)/inc/mpr.h
DEPS_10 += $(CONFIG)/inc/list.h
DEPS_10 += $(CONFIG)/inc/xml.h
DEPS_10 += $(CONFIG)/inc/matcher.h

$(CONFIG)/obj/svmsig.o: \
    src/modules/svmsys/svmsig.c $(DEPS_10)
	@echo '   [Compile] $(CONFIG)/obj/svmsig.o'
	$(CC) -c -o $(CONFIG)/obj/svmsig.o $(CFLAGS) $(DFLAGS) "$(IFLAGS)" src/modules/svmsys/svmsig.c
	
#
#   svmvoc.o
#
DEPS_11 += $(CONFIG)/inc/mpr.h
DEPS_11 += $(CONFIG)/inc/list.h
DEPS_11 += $(CONFIG)/inc/xml.h
DEPS_11 += $(CONFIG)/inc/matcher.h

$(CONFIG)/obj/svmvoc.o: \
    src/modules/svmsys/svmvoc.c $(DEPS_11)
	@echo '   [Compile] $(CONFIG)/obj/svmvoc.o'
	$(CC) -c -o $(CONFIG)/obj/svmvoc.o $(CFLAGS) $(DFLAGS) "$(IFLAGS)" src/modules/svmsys/svmvoc.c
	
#
#   matcher.h
#
$(CONFIG)/inc/matcher.h: \
	src/matcher.h $(DEPS_12)
	@echo '      [Copy] $(CONFIG)/inc/matcher.h'
	cp src/matcher.h $(CONFIG)/inc/matcher.h
	
#
#   matcher.o
#
DEPS_13 += $(CONFIG)/inc/matcher.h

$(CONFIG)/obj/matcher.o: \
    src/matcher.c $(DEPS_13)
	@echo '   [Compile] $(CONFIG)/obj/matcher.o'
	$(CC) -c -o $(CONFIG)/obj/matcher.o $(CFLAGS) $(DFLAGS) "$(IFLAGS)" src/matcher.c

#
#   matcher
#
DEPS_83 += $(CONFIG)/inc/mpr.h
DEPS_83 += $(CONFIG)/inc/list.h
DEPS_83 += $(CONFIG)/inc/xml.h
DEPS_83 += $(CONFIG)/obj/xml.o
DEPS_83 += $(CONFIG)/inc/config.h
DEPS_83 += $(CONFIG)/obj/config.o
DEPS_83 += $(CONFIG)/inc/packet.h
DEPS_83 += $(CONFIG)/obj/packet.o
DEPS_83 += $(CONFIG)/inc/svmsys.h
DEPS_83 += $(CONFIG)/obj/svmsig.o
DEPS_83 += $(CONFIG)/obj/svmvoc.o
DEPS_83 += $(CONFIG)/inc/matcher.h
DEPS_83 += $(CONFIG)/obj/matcher.o

OBJS_83 += $(CONFIG)/obj/xml.o
OBJS_83 += $(CONFIG)/obj/config.o
OBJS_83 += $(CONFIG)/obj/packet.o
OBJS_83 += $(CONFIG)/obj/svmsig.o
OBJS_83 += $(CONFIG)/obj/svmvoc.o

$(CONFIG)/bin/matcher: $(DEPS_83)
	@echo '      [Link] $(CONFIG)/bin/matcher'
	$(CC) -o $(CONFIG)/bin/matcher $(LDFLAGS) $(LIBPATHS)  "$(CONFIG)/obj/matcher.o" $(OBJS_83) $(LIBS) $(LIBS) 

