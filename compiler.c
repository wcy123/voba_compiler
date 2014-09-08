#include <stdlib.h>
#include <voba/include/value.h>
#include "voba_str.h"
#include "parser.syn.h"
#include "ast.h"
#include "exec_once.h"
#include <voba/core/builtin.h>
#include "compiler.h"

VOBA_FUNC static voba_value_t compile(voba_value_t self, voba_value_t args);
EXEC_ONCE_PROGN{voba_symbol_set_value(s_compile, voba_make_func(compile));}

//voba_value_t voba_cls_syn;
DEFINE_CLS(sizeof(syntax_t),syn)
VOBA_FUNC static voba_value_t to_string_syn(voba_value_t self, voba_value_t args);

EXEC_ONCE_PROGN{voba_gf_add_class(voba_symbol_value(s_to_string), voba_cls_syn, voba_make_func(to_string_syn));}



extern int yyparse();
extern voba_value_t yylval;
int yydebug = 0;
EXEC_ONCE_PROGN{yydebug = getenv("VOBA_YYDEBUG") != NULL;}
typedef void *yyscan_t ;
int z1lex_init ( yyscan_t * ptr_yy_globals ) ;
int z1lex_destroy ( yyscan_t yyscanner ) ;
static inline voba_str_t* indent(voba_str_t * s, int level)
{
    return voba_strcat(s,voba_str_from_char(' ',level * 4));
}
static voba_str_t* dump_location(voba_value_t syn, int level)
{
    if(level > 10){
        exit(1);
    }
    voba_str_t * ret = voba_str_empty();
    ret = indent(ret,level);
    uint32_t start_line;
    uint32_t end_line;
    uint32_t start_col;
    uint32_t end_col;
    syn_get_line_column(1,syn,&start_line,&start_col);
    syn_get_line_column(0,syn,&end_line,&end_col);    
    ret = voba_vstrcat(ret,
                       VOBA_CONST_CHAR("(L"),
                       voba_str_fmt_uint32_t(start_line,10),
                       VOBA_CONST_CHAR(",C"),
                       voba_str_fmt_uint32_t(start_col,10),
                       VOBA_CONST_CHAR(")-(L"),
                       voba_str_fmt_uint32_t(end_line,10),
                       VOBA_CONST_CHAR(",C"),
                       voba_str_fmt_uint32_t(end_col,10),
                       VOBA_CONST_CHAR("):"),NULL);
    if(voba_is_array(SYNTAX(syn)->v)){
        ret = voba_strcat(ret,VOBA_CONST_CHAR("[\n"));
        for(size_t n = 0 ; n < voba_array_len(SYNTAX(syn)->v); ++n){
            ret = voba_strcat(ret,dump_location(voba_array_at(SYNTAX(syn)->v,n), level+1));
        }
        ret = indent(ret,level);
        ret = voba_strcat(ret,VOBA_CONST_CHAR("]\n"));
    }else{
        voba_value_t args[] = {1, SYNTAX(syn)->v};
        ret = voba_strcat(ret, voba_value_to_str(voba_apply(voba_symbol_value(s_to_string),
                                                            voba_make_array(args))));
        ret = voba_strcat_data(ret,"\n",1);
    }
    return ret;
}

struct z1_buffer_state*  z1_scan_bytes (char *bytes,int len ,yyscan_t yyscanner );
static inline int voba_is_syn(voba_value_t x)
{
    return voba_get_class(x) == voba_cls_syn;
}
VOBA_FUNC static voba_value_t to_string_syn(voba_value_t self, voba_value_t args)
{
    VOBA_DEF_ARG(syn, args, 0, voba_is_syn);
    return voba_make_string(dump_location(syn,0));
}
VOBA_FUNC static voba_value_t compile(voba_value_t self, voba_value_t args)
{
    voba_value_t program =  VOBA_NIL;
    VOBA_DEF_ARG(content1, args, 0, voba_is_string);
    VOBA_DEF_ARG(filename, args, 1, voba_is_string);
    void * scanner;
    voba_value_t module = voba_make_symbol_table();
    voba_str_t * content = voba_value_to_str(content1);
    z1lex_init(&scanner);
    z1_scan_bytes(content->data,content->len,scanner);
    z1parse(scanner,&program, module);
    voba_value_t ast = VOBA_NIL;
    int error = 0;
    voba_value_t si = make_source_info(filename,content1);
    attach_source_info(program,si);
    ast = compile_ast(program,module,&error);
    z1lex_destroy(scanner);
    if(error){
        fprintf(stderr,__FILE__ ":%d:[%s] total %d errors \n", __LINE__, __FUNCTION__,error);

    }
    return ast;
}
voba_value_t voba_init(voba_value_t this_module)
{
    exec_once_run();
    return VOBA_NIL;
}
