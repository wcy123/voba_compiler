#define EXEC_ONCE_TU_NAME "syn2ast_for"
#include <exec_once.h>
#include <voba/include/value.h>
#include "syn.h"
#include "ast.h"
#include "match.h"
#include "syn2ast_report.h"
#include "syn2ast_basic.h"
#include "syn2ast_match.h"
voba_value_t compile_for(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    int64_t len = voba_array_len(form);
    if(len == 1){
        report_error(
            VOBA_CONST_CHAR("bare `for'"),
            syn_form,toplevel_env);
    }else if(len == 2){
        report_error(
            VOBA_CONST_CHAR("illegal form: no `for' body"),
            syn_form,toplevel_env);
    }else{
        voba_value_t syn_iter = voba_array_at(form,1);
        voba_value_t ast_iter = compile_expr(syn_iter,env,toplevel_env);
        if(!voba_is_nil(ast_iter)){
            uint32_t offset = 2; /*skip `for' and `iterat' */
            voba_value_t la_syn_form = voba_la_from_array1(form,offset);
            voba_value_t match = compile_match_match(la_syn_form,env,toplevel_env);
            if(!voba_is_nil(match)){
                ret = make_ast_for(ast_iter,match);
            }
        }
    }
    return ret;
}
