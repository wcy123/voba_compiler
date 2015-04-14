#include <exec_once.h>
#include <voba/value.h>
#include "ast.h"
#include "syn.h"
#include "match.h"
#include "syn2ast_report.h"
#include "syn2ast_basic.h"

voba_value_t compile_if(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    int64_t len = voba_array_len(form);
    if(len >= 3){
        voba_value_t syn_cond = voba_array_at(form,1);
        voba_value_t ast_cond = compile_expr(syn_cond,env,toplevel_env);
        if(!voba_is_nil(ast_cond)){
            voba_value_t syn_then  = voba_array_at(form,2);
            voba_value_t ast_then = compile_expr(syn_then,env,toplevel_env);
            if(ast_then){
                uint32_t offset = 3;/* skip if, cond, then */
                voba_value_t la_syn_else = voba_la_from_array1(form,offset);
                voba_value_t a_ast_else = compile_exprs(la_syn_else,env,toplevel_env);
                if(a_ast_else){
                    voba_value_t new_env = make_env();
                    ENV(new_env)->parent = env;
                    
                    voba_value_t a_ast_then = voba_make_array_1(ast_then);
                    voba_value_t pat_then = make_pattern_else(); // match everything except FALSE
                    voba_value_t rule_then = make_rule(pat_then, a_ast_then, new_env);

                    voba_value_t syn_false = MAKE_SYN_CONST(VOBA_FALSE);
                    voba_value_t ast_false = compile_expr(syn_false,new_env,toplevel_env);
                    voba_value_t a_ast_false = voba_make_array_1(ast_false);
                    voba_value_t pat_false = make_pattern_value(a_ast_false); // match everything except FALSE
                    voba_value_t rule_false = make_rule(pat_false, a_ast_else,new_env);
                        
                    voba_value_t a_rules  = voba_make_array_2(rule_false, rule_then);
                    voba_value_t match  = make_match(a_rules);

                    ret = make_ast_match(ast_cond,match);
                }
            }
        }
    }else{
        report_error(VOBA_STRCAT(
                         VOBA_CONST_CHAR("illegal form. (if <COND> <THEN> <ELSE>...) len == "),
                         voba_str_fmt_int64_t(len,10)),
                     syn_form,toplevel_env);
    }
    return ret;
}
