#include <stdlib.h>
#include <voba/include/value.h>
#include "voba_str.h"
#include "parser.syn.h"
#include "ast.h"
#include "exec_once.h"
#include <voba/core/builtin.h>
#include "compiler.h"

VOBA_FUNC static voba_value_t compile(voba_value_t self, voba_value_t args);
EXEC_ONCE_DO(voba_symbol_set_value(s_compile, voba_make_func(compile));)



extern int yyparse();
extern voba_value_t yylval;
int yydebug = 0;
EXEC_ONCE_DO(yydebug = getenv("VOBA_YYDEBUG") != NULL;)
typedef void *yyscan_t ;
int z1lex_init ( yyscan_t * ptr_yy_globals ) ;
int z1lex_destroy ( yyscan_t yyscanner ) ;
static void indent(int level)
{
    while(level-- > 0) fputs("      ",stdout);
}
static void dump_location(voba_value_t syn, int level)
{
    if(level > 10){
        exit(1);
    }
    indent(level);
    printf("(L%d,C%d)-(L%d,C%d):",SYNTAX(syn)->first_line,SYNTAX(syn)->first_column,
           SYNTAX(syn)->last_line, SYNTAX(syn)->last_column);
    if(voba_is_array(SYNTAX(syn)->v)){
        printf("[\n");
        for(size_t n = 0 ; n < voba_array_len(SYNTAX(syn)->v); ++n){
            dump_location(voba_array_at(SYNTAX(syn)->v,n), level+1);
        }
        indent(level);printf("]\n");
    }else{
        voba_value_t args[] = {1, SYNTAX(syn)->v};
        voba_apply(voba_symbol_value(s_print),voba_make_array(args));
    }
    return;
}

struct z1_buffer_state*  z1_scan_bytes (char *bytes,int len ,yyscan_t yyscanner );

VOBA_FUNC static voba_value_t compile(voba_value_t self, voba_value_t args)
{
    VOBA_DEF_ARG( content1, args, 0, voba_is_string);
    voba_value_t yy_program = VOBA_NIL;
    void * scanner;
    voba_str_t * content = voba_value_to_str(content1);
    fprintf(stderr,__FILE__ ":%d:[%s] content %p %d %d\n", __LINE__, __FUNCTION__,
            content->data,content->len,content->capacity);

    z1lex_init(&scanner);
    z1_scan_bytes(content->data,content->len,scanner);
    int r = z1parse(scanner,&yy_program);
    if(r ==0){
        fprintf(stderr,__FILE__ ":%d:[%s] OK\n", __LINE__, __FUNCTION__);
        dump_location(yy_program,0);
    }else{
        fprintf(stderr,__FILE__ ":%d:[%s] FAIL.", __LINE__, __FUNCTION__);
    }
    voba_value_t ast = VOBA_NIL;
    //ast = compile_ast(z1_program);
    z1lex_destroy(scanner);
    return ast;
}
voba_value_t voba_init(voba_value_t this_module)
{
    exec_once_run();
    return VOBA_NIL;
}
