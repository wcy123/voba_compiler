#define EXEC_ONCE_TU_NAME "module_info"
#include <exec_once.h>
#include <voba/value.h>
#include "module_info.h"
VOBA_DEF_CLS(sizeof(module_info_t),module_info);
voba_value_t module_info_id(voba_value_t info)    
{
     return voba_make_string(MODULE_INFO(info)->id);
}
voba_value_t module_info_name(voba_value_t info)    
{
     return voba_make_string(MODULE_INFO(info)->name);
}
voba_value_t module_info_symbols(voba_value_t info)    
{
     return MODULE_INFO(info)->symbols;
}
voba_value_t make_module_info()
{
    voba_value_t ret = voba_make_user_data(voba_cls_module_info);
    MODULE_INFO(ret)->name = NULL;
    MODULE_INFO(ret)->id = NULL;
    MODULE_INFO(ret)->symbols = voba_make_array_0();
    return ret;
}

