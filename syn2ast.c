#include <string.h>
#include <errno.h>
#include <voba/value.h>
#define EXEC_ONCE_TU_NAME "voba.compiler.syn2ast"
#include <exec_once.h>
#include <voba/core/builtin.h>
#include "ast.h"
#include "syn.h"
#include "syn2ast_report.h"
#include "syn2ast_decl_top_var.h"
// `syn` is the syntax object to be compiled into an ast
// `module` is the symbol table for all symbols.
voba_value_t syn2ast(voba_value_t syn,voba_value_t module)
{
    voba_value_t asts = VOBA_NIL;
    voba_value_t toplevel_env = create_toplevel_env(module);
    TOPLEVEL_ENV(toplevel_env)->env = make_env();
    if(!voba_is_a(SYNTAX(syn)->v,voba_cls_array)){
        report_error(VOBA_CONST_CHAR("expr must be an array."),syn,toplevel_env);
        return VOBA_NIL;
    }
    compile_toplevel_exprs(voba_la_from_array1(SYNTAX(syn)->v,0),toplevel_env);
    voba_value_t next = TOPLEVEL_ENV(toplevel_env)->next;
    int64_t len =  voba_array_len(next);
    asts = voba_make_array_0();
    voba_value_t args [] = {1, toplevel_env};
    for(int64_t i = 0 ; i < len ; ++i) {
        voba_value_t ast = voba_apply(voba_array_at(next,i),voba_make_tuple(args));
        if(!voba_is_nil(ast)){
            voba_array_push(asts,ast);
        }
    }
    TOPLEVEL_ENV(toplevel_env)->a_asts = asts;
    return toplevel_env;
}

