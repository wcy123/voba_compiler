#include <voba/include/value.h>
#include <exec_once.h>
#include "compiler.h"
#include "read_module_info.h"
static DEFINE_CLS(sizeof(module_info_t),module_info);
#include "read_module_info_lex.inc"

static inline int voba_is_module_info(voba_value_t v)
{
    return voba_get_class(v) == voba_cls_module_info;
}
EXEC_ONCE_PROGN {
    voba_symbol_set_value(s_read_module_info, voba_make_func(read_module_info));
}
VOBA_FUNC voba_value_t read_module_info(voba_value_t self, voba_value_t args)
{
    VOBA_DEF_ARG(content1, args, 0, voba_is_string);
    voba_str_t * content = voba_value_to_str(content1);
    voba_value_t ret = voba_make_user_data(voba_cls_module_info, sizeof(module_info_t));
    MODULE_INFO(ret)->name = NULL;
    MODULE_INFO(ret)->id = NULL;
    MODULE_INFO(ret)->symbols = voba_make_array_0();
    void * scanner;
    yylex_init(&scanner);
    yy_scan_bytes(content->data,content->len,scanner);
    module_lex(ret,content,scanner);
    yylex_destroy(scanner);
    if(MODULE_INFO(ret)->name == NULL){
        ret = VOBA_NIL;
        VOBA_THROW("read_module_info: VOBA_MODULE_NAME is not defined.");
    }
    if(MODULE_INFO(ret)->id == NULL){
        ret = VOBA_NIL;
        VOBA_THROW("read_module_info: VOBA_MODULE_ID is not defined.");
    }
    if(voba_array_len(MODULE_INFO(ret)->symbols) == 0){ 
        ret = VOBA_NIL;
        VOBA_THROW("read_module_info: no symbol is defined.");
    }
    return ret;
}
EXEC_ONCE_PROGN {
    voba_symbol_set_value(s_module_info_id, voba_make_func(module_info_id));
}
VOBA_FUNC voba_value_t module_info_id(voba_value_t self, voba_value_t args)
{
    VOBA_DEF_ARG(info, args, 0, voba_is_module_info);
    return voba_make_string(MODULE_INFO(info)->id);
}
EXEC_ONCE_PROGN {
    voba_symbol_set_value(s_module_info_name, voba_make_func(module_info_name));
}
VOBA_FUNC voba_value_t module_info_name(voba_value_t self, voba_value_t args)
{
    VOBA_DEF_ARG(info, args, 0, voba_is_module_info);
    return voba_make_string(MODULE_INFO(info)->name);
}
EXEC_ONCE_PROGN {
    voba_symbol_set_value(s_module_info_symbols, voba_make_func(module_info_symbols));
}
VOBA_FUNC voba_value_t module_info_symbols(voba_value_t self, voba_value_t args)
{
    VOBA_DEF_ARG(info, args, 0, voba_is_module_info);
    return MODULE_INFO(info)->symbols;
}

EXEC_ONCE_START
