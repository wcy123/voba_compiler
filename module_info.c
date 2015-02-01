#define EXEC_ONCE_TU_NAME "voba.compiler.module_info"
#include <exec_once.h>
#include <voba/value.h>
#include "module_info.h"
VOBA_DEF_CLS(sizeof(module_info_t),module_info);
voba_value_t module_info_id(voba_value_t info)    
{
    return MODULE_INFO(info)->syn_id;
}
voba_value_t module_info_name(voba_value_t info)    
{
    return MODULE_INFO(info)->syn_name;
}
voba_value_t module_info_symbols(voba_value_t info)    
{
     return MODULE_INFO(info)->a_syn_symbols;
}
voba_value_t make_module_info()
{
    voba_value_t ret = voba_make_user_data(voba_cls_module_info);
    MODULE_INFO(ret)->syn_name = VOBA_NIL;
    MODULE_INFO(ret)->syn_id = VOBA_NIL;
    MODULE_INFO(ret)->a_syn_symbols = voba_make_array_0();
    return ret;
}

