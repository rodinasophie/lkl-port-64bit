#uncoment the next 2 lines to get LKL's file APIs. This currently works on linux only
.PHONY=clean
LKL_DEFINES+=-Iinclude
LKL=$(CROSS)lkl/lkl.a

#APR_WIN_INCLUDE=-Iapr_win/include/

#APR_LIN_LIB=-lapr-1 -L/home/gringo/apr12x/.libs/libapr-1.so.0.2.12
#APR_WIN_LIB=apr_win/Debug/libapr-1.lib

CFLAGS_LIN=$(shell --includes --cppflags)
#CFLAGS_WIN=$(APR_WIN_INCLUDE)

#select here between Linux and Windows
#CFLAGS_OS=$(CFLAGS_WIN)
#CFLAGS_OS=$(CFLAGS_LIN)

HERE=$(PWD)
LINUX=$(HERE)/../lkl-2.6

SRCS=$(shell ls *.c)
OBJS=$(patsubst %.c,%.o,$(SRCS))
DEPS=$(patsubst %.c,.deps/%.d,$(SRCS))

MKDIR=mkdir -p

all: imgwrt.out 

include/asm-generic:
	-$(MKDIR) `dirname $@`
	ln -s $(LINUX)/include/asm-generic include/asm-generic

include/asm:
	-$(MKDIR) `dirname $@`
	ln -s  $(LINUX)/arch/lkl/include/asm include/asm

include/x86:
	-$(MKDIR) `dirname $@`
	ln -s $(LINUX)/arch/x86 include/x86

include/linux:
	-$(MKDIR) `dirname $@`
	ln -s $(LINUX)/include/linux include/linux

INC=include/asm-generic include/asm include/x86 include/linux 

$(CROSS)lkl/.config: .config 
	mkdir -p $(CROSS)lkl && \
	cp $< $@

$(CROSS)lkl/lkl.a: $(CROSS)lkl/.config 
	cd $(LINUX) && \
	$(MAKE) O=$(HERE)/$(CROSS)lkl ARCH=lkl \
	CROSS_COMPILE=$(CROSS) LKLENV_CFLAGS="$(CFLAGS_OS)" \
	lkl.a

CFLAGS=-Wall -g $(CFLAGS_OS) $(LKL_DEFINES)

%.o: %.c $(INC)
	$(CC) -c $(CFLAGS) $< 

AOUT=$(OBJS) $(LKL) 
AEXE=$(OBJS) $(LKL)

clean:
	-rm -rf imgwrt.out imgwrt.exe include *.o .deps/ *~

clean-all: clean
	-rm -rf lkl lkl-nt

TAGS:  *.c *.h
	etags $^

imgwrt.out: $(AOUT) $(INC) 
	$(CC) $(AOUT) -pthread -o $@ 

imgwrt.exe: CROSS=i586-mingw32msvc-

imgwrt.exe: $(AEXE) $(INC) 
	$(CROSS)gcc $(AEXE) -o $@

.deps/%.d: %.c
	mkdir -p .deps/$(dir $<)
	$(CC) $(CFLAGS) -MM -MT $(patsubst %.c,%.o,$<) $< > $@.$$$$; \
	sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

include $(DEPS)
