#
# Makefile
#
# See the README file for copyright information and how to reach the author.
#

LIBTARGET = libhorchi
HLIB      = -L. -lhorchi

DEMO = demo
TEST = tst

LIBOBJS = common.o curl.o imgtools.o config.o db.o tabledef.o dbdict.o

CC        = g++
doCompile = $(CC) -c $(CFLAGS) $(DEFINES)
doLink    = $(CC) $(LFLAGS)
doLib     = ar -rs

BASELIBS = -lrt -lz -lmysqlclient -lcurl -luuid -lcrypto
BASELIBS += $(shell mysql_config --libs)

DEBUG = 1

ifdef DEBUG
  CFLAGS += -ggdb -O0
endif

CFLAGS += -fPIC -Wreturn-type -Wall -Wno-parentheses -Wformat -pedantic -Wunused-variable -Wunused-label \
          -Wunused-value -Wunused-function \
          -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64

CFLAGS   += $(shell mysql_config --include)

DEFINES = -DPLGDIR='"."' -DUSEUUID -DUSEMD5

all: lib $(TEST) $(DEMO)
lib: $(LIBTARGET).a

$(LIBTARGET).a : $(LIBOBJS)
	@echo Building Lib ...
	$(doLib) $@ $(LIBOBJS)

tst: test.o
	$(doLink) test.o $(HLIB) $(BASELIBS) -o $@

demo: demo.o
	$(doLink) demo.o $(HLIB) $(BASELIBS) -o $@

clean:
	rm -f *.o *~ core $(TEST) $(DEMO) $(LIBTARGET).a

cppchk:
	cppcheck --template="{file}:{line}:{severity}:{message}" --quiet --force *.c *.h 

%.o: %.c
	@echo Compile "$(*F)" ...
	$(doCompile) $(*F).c -o $@

#--------------------------------------------------------
# dependencies
#--------------------------------------------------------

HEADER = db.h common.h config.h

common.o    :  common.c      $(HEADER) common.h
curl.o      :  curl.c        $(HEADER)
imgtools.o  :  imgtools.c    $(HEADER) imgtools.h
config.o    :  config.c      $(HEADER) config.h
db.o        :  db.c          $(HEADER) db.h
tabledef.o  :  tabledef.c    $(HEADER) tabledef.h
dbdict.o    :  dbdict.c      $(HEADER) dbdict.h

demo.o      :  demo.c        $(HEADER) 
test.o      :  test.c        $(HEADER) 
