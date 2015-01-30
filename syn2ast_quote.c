#include <exec_once.h>
#include <voba/value.h>
#include "ast.h"
#include "syn.h"
#include "syn2ast_quote.h"
#include "syn2ast_report.h"
voba_value_t compile_quote(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    int64_t len = voba_array_len(form);
    if(len == 2){
        ret = make_ast_constant(voba_array_at(form,1));
    }else{
        report_error(
            VOBA_CONST_CHAR("illegal form for quote"),
            syn_form, toplevel_env
            );
    }
    return ret;
}

