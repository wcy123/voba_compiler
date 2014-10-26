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

ifneq ($(CONFIG),release)
	CFLAGS += -ggdb -O0
	CXXFLAGS += -ggdb -O0
else
	CFLAGS += -O3 -DNDEBUG
	CXXFLAGS += -O3 -DNDEBUG
endif

CFLAGS += -std=c99
CFLAGS += $(FLAGS)
CFLAGS += -fPIC

LFLAGS += --noline
LEX = flex
YACC = bison

LDFLAGS += -L $(PREFIX) -lexec_once

all: install

install: libvoba_compiler.so
	install libvoba_compiler.so $(PREFIX)/voba/core
	install compiler.h $(PREFIX)/voba/core/

libvoba_compiler.so:  var.o env.o src.o syn.o ast.o c_backend.o module_info.o match.o src2syn.o syn2ast.o ast2c.o flex.o parser.o read_module_info.o compiler.o 
	$(CXX) -shared -Wl,-soname,$@  -o $@ $+ $(LDFLAGS) -lexec_once

flex.o: parser.o

ast2c.o: ast.h

clean:
	rm *.o *.so parser.c flex.c read_module_info_lex.inc

flex.c: flex.l
	flex -d -P z1 --noline -o $@ flex.l
parser.c: parser.y
	bison -p z1 parser.y

.PHONY: all clean


read_module_info.o: read_module_info.c read_module_info_lex.inc

read_module_info_lex.inc: read_module_info_lex.l
	flex --noline  read_module_info_lex.l
syn2ast.o: syn2ast_scan1.c syn2ast_scan1.h syn2ast_scan2.c syn2ast_scan2.h syn2ast_report.c

