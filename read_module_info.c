#include <voba/value.h>
#define EXEC_ONCE_TU_NAME "voba.compiler.read_module_info"
#include <exec_once.h>
#include "compiler.h"
#include "module_info.h"
#include "read_module_info.h"
#include "read_module_info_lex.inc"
voba_value_t read_module_info(voba_value_t content1)
{
    voba_str_t * content = voba_value_to_str(content1);
    voba_value_t ret = make_module_info();
    void * scanner;
    yylex_init(&scanner);
    yy_scan_bytes(content->data,content->len,scanner);
    YYLTYPE pos;
    YYSTYPE s;
    module_lex(ret,content,scanner,&s,&pos);
    yylex_destroy(scanner);
    if(voba_is_nil(MODULE_INFO(ret)->syn_name)){
        ret = voba_make_string(VOBA_CONST_CHAR("read_module_info: VOBA_MODULE_NAME is not defined."));
    }
    if(voba_is_nil(MODULE_INFO(ret)->syn_id)){
        ret = voba_make_string(VOBA_CONST_CHAR("read_module_info: VOBA_MODULE_ID is not defined."));
    }
    if(voba_array_len(MODULE_INFO(ret)->a_syn_symbols) == 0){ 
        ret = voba_make_string(VOBA_CONST_CHAR("read_module_info: no symbol is defined."));
    }
    return ret;
}
