#pragma once
#define VOBA_MODULE_ID "3d57cd44-3737-11e4-9001-08002796644a"

#define VOBA_SYMBOL_TABLE(XX)                   \
    XX(compile)                                 \
    XX(read_module_info)                        \
    XX(module_info_id)                          \
    XX(module_info_name)                        \
    XX(module_info_symbols)
    
#define VOBA_MODULE_NAME "compiler"


#include <voba/module_end.h>

