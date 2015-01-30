#define EXEC_ONCE_TU_NAME "voba.compiler.syn2ast_yield"
#include <exec_once.h>
#include <voba/value.h>
#include "syn2ast_yield.h"
#include "ast.h"
#include "syn.h"
#include "match.h"
#include "syn2ast_report.h"
#include "syn2ast_basic.h"
static voba_value_t find_enclosing_fun(voba_value_t env);
voba_value_t compile_yield(voba_value_t syn_form, voba_value_t env, voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t enc_fun = find_enclosing_fun(env);
    if(!voba_is_a(enc_fun, voba_cls_compiler_fun)){
        report_error(VOBA_CONST_CHAR("yield keyword is only allowed in a function body"), syn_form,toplevel_env);
    }else{
        COMPILER_FUN(enc_fun)->is_generator = 1;
        voba_value_t form = SYNTAX(syn_form)->v;
        int64_t len = voba_array_len(form);
        if(len == 0){
            assert(0 && "never goes here");
        }else if(len == 1){
            report_error(VOBA_CONST_CHAR("yield no value")
                         , syn_form,toplevel_env);            
        }else if(len != 2){
            report_error(VOBA_CONST_CHAR("yield more than more value")
                         , syn_form,toplevel_env);            
        }else{
            voba_value_t syn_expr = voba_array_at(form,1);
            voba_value_t ast_expr = compile_expr(syn_expr, env,toplevel_env);
            ret = make_ast_yield(ast_expr);
        }
    }
    return ret;
}
static voba_value_t find_enclosing_fun(voba_value_t env_or_fun)
{
    voba_value_t ret = VOBA_NIL;
    if(voba_is_nil(env_or_fun)){
        ret = VOBA_NIL; // reach the top level, cannot find the symbol
    }else if(voba_is_a(env_or_fun, voba_cls_env)){
        ret = find_enclosing_fun(ENV(env_or_fun)->parent);
    }else if(voba_is_a(env_or_fun, voba_cls_compiler_fun)){
        ret = env_or_fun;
    }else{
        assert(0&&"never goes here");
    }
    return ret;
}
