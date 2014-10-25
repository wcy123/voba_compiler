#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <voba/include/value.h>
#include <exec_once.h>
#include "ast.h"
#include "src2syn.h"
#include "syn.h"
#include "parser.h"

typedef void *yyscan_t ;
int z1lex_init ( yyscan_t * ptr_yy_globals ) ;
int z1lex_destroy ( yyscan_t yyscanner ) ;
void z1set_debug (int  bdebug , yyscan_t yyscanner);
struct z1_buffer_state*  z1_scan_bytes (char *bytes,int len ,yyscan_t yyscanner );
voba_value_t src2syn(voba_value_t content1, voba_value_t filename, voba_value_t * module, int * error)
{
    voba_value_t ret =  VOBA_NIL;
    void * scanner;
    *module = voba_make_symbol_table();
    voba_str_t * content = voba_value_to_str(content1);
    z1lex_init(&scanner);
    if(getenv("YY_FLEX_DEBUG")){
        int yy_flex_debug = atoi(getenv("YY_FLEX_DEBUG"));
        z1set_debug(yy_flex_debug,scanner);
    }
    z1_scan_bytes(content->data,content->len,scanner);
    int r = z1parse(scanner,&ret, *module);
    if(!voba_is_nil(ret)){
        voba_value_t si = make_source_info(filename,content1);
        attach_source_info(ret,si);
        if(r == 0){
            *error = 0;
        }else{
            *error = 1;
        }
    }else{
        *error = 1;
    }
    z1lex_destroy(scanner);
    return ret;
}
