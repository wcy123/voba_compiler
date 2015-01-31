#include <exec_once.h>
#include <voba/value.h>
#include "syn2ast_and_or.h"
#include "ast.h"
#include "syn.h"
#include "match.h"
#include "syn2ast_report.h"
#include "syn2ast_basic.h"
static inline voba_value_t compile_and_or(voba_value_t syn_form, voba_value_t env, voba_value_t toplevel_env);
voba_value_t compile_and(voba_value_t syn_form, voba_value_t env, voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t a_ast_exprs = compile_and_or(syn_form,env,toplevel_env);
    if(!voba_is_nil(a_ast_exprs)){
        ret = make_ast_and(a_ast_exprs);
    }
    return ret;
}
voba_value_t compile_or(voba_value_t syn_form, voba_value_t env, voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t a_ast_exprs = compile_and_or(syn_form,env,toplevel_env);
    if(!voba_is_nil(a_ast_exprs)){
        ret = make_ast_or(a_ast_exprs);
    }
    return ret;
}
static inline voba_value_t compile_and_or(voba_value_t syn_form, voba_value_t env, voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    assert(voba_is_a(form,voba_cls_array));
    uint32_t offset = 1;        /* skip `and' or `or' */
    voba_value_t la_syn_exprs = voba_la_from_array1(form,offset);
    voba_value_t a_ast_exprs = compile_exprs(la_syn_exprs,env,toplevel_env);
    if(!voba_is_nil(a_ast_exprs)){
        ret = a_ast_exprs;
    }
    return ret;
}
