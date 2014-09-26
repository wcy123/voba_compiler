#include <voba/include/value.h>
#include <exec_once.h>
#include "compiler.h"
#include "module_info.h"
#include "read_module_info.h"
#include "read_module_info_lex.inc"
VOBA_FUNC static voba_value_t v_read_module_info(voba_value_t self, voba_value_t args);
EXEC_ONCE_PROGN {
    voba_symbol_set_value(s_read_module_info, voba_make_func(v_read_module_info));
}
VOBA_FUNC static voba_value_t v_read_module_info(voba_value_t self, voba_value_t args)
{
    VOBA_DEF_ARG(content1, args, 0, voba_is_string);
    return read_module_info(content1);
}
voba_value_t read_module_info(voba_value_t content1)
{
    voba_str_t * content = voba_value_to_str(content1);
    voba_value_t ret = make_module_info();
    void * scanner;
    yylex_init(&scanner);
    yy_scan_bytes(content->data,content->len,scanner);
    module_lex(ret,content,scanner);
    yylex_destroy(scanner);
    if(MODULE_INFO(ret)->name == NULL){
        ret = voba_make_string(VOBA_CONST_CHAR("read_module_info: VOBA_MODULE_NAME is not defined."));
    }
    if(MODULE_INFO(ret)->id == NULL){
        ret = voba_make_string(VOBA_CONST_CHAR("read_module_info: VOBA_MODULE_ID is not defined."));
    }
    if(voba_array_len(MODULE_INFO(ret)->symbols) == 0){ 
        ret = voba_make_string(VOBA_CONST_CHAR("read_module_info: no symbol is defined."));
    }
    return ret;
}
static VOBA_FUNC voba_value_t v_module_info_id(voba_value_t self, voba_value_t args);
EXEC_ONCE_PROGN {
    voba_symbol_set_value(s_module_info_id, voba_make_func(v_module_info_id));
}
static VOBA_FUNC voba_value_t v_module_info_id(voba_value_t self, voba_value_t args)
{
    VOBA_DEF_ARG(info, args, 0, is_module_info);
    return module_info_id(info);
}
static VOBA_FUNC voba_value_t v_module_info_name(voba_value_t self, voba_value_t args);
EXEC_ONCE_PROGN {
    voba_symbol_set_value(s_module_info_name, voba_make_func(v_module_info_name));
}
static VOBA_FUNC voba_value_t v_module_info_name(voba_value_t self, voba_value_t args)
{
    VOBA_DEF_ARG(info, args, 0, is_module_info);
    return module_info_name(info);
}
static VOBA_FUNC voba_value_t v_module_info_symbols(voba_value_t self, voba_value_t args);
EXEC_ONCE_PROGN {
    voba_symbol_set_value(s_module_info_symbols, voba_make_func(v_module_info_symbols));
}
static VOBA_FUNC voba_value_t v_module_info_symbols(voba_value_t self, voba_value_t args)
{
    VOBA_DEF_ARG(info, args, 0, is_module_info);
    return module_info_symbols(info);
}
EXEC_ONCE_START
