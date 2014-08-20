#pragma once
#ifndef MODULE_NAME
#define MODULE_NAME "compiler"
#endif
#define SYMBOL_TABLE(XX)                        \
    XX(compile)                                 \

#define IMP "./libvoba_compiler.so"

#include <voba/include/module_end.h>

