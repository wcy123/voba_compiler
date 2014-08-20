#pragma once
#ifndef MODULE_NAME
#define MODULE_NAME "voba_compiler_module"
#endif
#define SYMBOL_TABLE(XX)                        \
    XX(compile)                                 \

#define IMP "./libvoba_compiler.so"

#include "voba_module_end.h"

