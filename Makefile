PREFIX  = ../build
INCLUDE += -I .
INCLUDE += -I ../exec_once
INCLUDE += -I ../voba_str
INCLUDE += -I ../../vhash
INCLUDE += -I ~/d/other-working/GC/bdwgc/include
INCLUDE += -I $(PREFIX)

CFLAGS   += $(INCLUDE)
CXXFLAGS += $(INCLUDE)

FLAGS += -Wall -Werror
FLAGS += -fPIC

CFLAGS += -ggdb -O0
CFLAGS += -std=c99
CFLAGS += $(FLAGS)
CFLAGS += -fPIC

LFLAGS += --noline
LEX = flex
YACC = bison

all: install

install: libvoba_compiler.so
	install libvoba_compiler.so $(PREFIX)/voba/core
	install compiler.h $(PREFIX)/voba/core

libvoba_compiler.so: voba_compiler_module.o 
	$(CXX) -shared -Wl,-soname,$@  -o $@ voba_compiler_module.o flex.o parser.o

libvoba_compiler.so:  flex.o parser.o


flex.o: parser.o

clean:
	rm *.o *.so parser.c flex.c
flex.c: flex.l
	flex -P z1 --noline -o $@ flex.l
parser.c: parser.y
	bison -p z1 parser.y

.PHONY: all clean
