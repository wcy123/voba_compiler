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

C_SRCS += env.c
C_SRCS += src.c
C_SRCS += syn.c
C_SRCS += ast.c
C_SRCS += c_backend.c
C_SRCS += module_info.c
C_SRCS += match.c
C_SRCS += src2syn.c
C_SRCS += syn2ast.c
C_SRCS += syn2ast_basic.c
C_SRCS += syn2ast_decl_top_var.c
C_SRCS += syn2ast_for.c
C_SRCS += syn2ast_if.c
C_SRCS += syn2ast_let.c
C_SRCS += syn2ast_match.c
C_SRCS += syn2ast_other.c
C_SRCS += syn2ast_report.c
C_SRCS += ast2c.c
C_SRCS += read_module_info.c
C_SRCS += compiler.c


OBJS += flex.o
OBJS += parser.o
OBJS += $(patsubst %.c,%.o,$(C_SRCS))

libvoba_compiler.so:  $(OBJS)
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

.PHONY: depend
depend: 
	for i in $(C_SRCS); do $(CC) -MM $(CFLAGS) $$i; done > $@

include depend

.DELETE_ON_ERROR:

