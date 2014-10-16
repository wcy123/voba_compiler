#include <string.h>
#include <errno.h>
#include <voba/include/value.h>
#include <exec_once.h>
#include <voba/core/builtin.h>
#include "parser.syn.h"
#include "ast.h"
#include "module_info.h"
#include "read_module_info.h"

#include "syn2ast_report.h"
#include "syn2ast_scan1.h"
#include "syn2ast_scan2.h"
#include "env.h"
#include "var.h"

#include "syn2ast_report.c"
#include "syn2ast_other.c"
#include "syn2ast_scan1.c"
#include "syn2ast_scan2.c"
// `syn` is the syntax object to be compiled into an ast
// `module` is the symbol table for all symbols.
voba_value_t syn2ast(voba_value_t syn,voba_value_t module, int * error)
{
    voba_value_t asts = VOBA_NIL;
    voba_value_t toplevel_env = create_toplevel_env(module);
    TOPLEVEL_ENV(toplevel_env)->env = make_env();
    if(!voba_is_array(SYNTAX(syn)->v)){
        report_error(VOBA_CONST_CHAR("expr must be an array."),syn,toplevel_env);
        return VOBA_NIL;
    }
    compile_toplevel_exprs(voba_la_from_array1(SYNTAX(syn)->v,0),toplevel_env);
    voba_value_t next = TOPLEVEL_ENV(toplevel_env)->next;
    int64_t len =  voba_array_len(next);
    asts = voba_make_array_0();
    voba_value_t args [] = {1, toplevel_env};
    for(int64_t i = 0 ; i < len ; ++i) {
        voba_value_t ast = voba_apply(voba_array_at(next,i),voba_make_array(args));
        if(!voba_is_nil(ast)){
            voba_array_push(asts,ast);
        }else{
            ++ *error;
        }
    }
    *error = TOPLEVEL_ENV(toplevel_env)->n_of_errors;
    TOPLEVEL_ENV(toplevel_env)->a_asts = asts;
    return toplevel_env;
}
EXEC_ONCE_START;
