#define EXEC_ONCE_TU_NAME "syn2ast_match"
#include <exec_once.h>
#include <voba/include/value.h>
#include "ast.h"
#include "syn.h"
#include "match.h"
#include "keywords.h"
#include "syn2ast_report.h"
#include "syn2ast_basic.h"
#include "syn2ast_match.h"

static inline voba_value_t compile_match_value(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env);
voba_value_t compile_match(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t ast_value = compile_match_value(syn_form,env,toplevel_env);
    if(!voba_is_nil(ast_value)){
        voba_value_t form = SYNTAX(syn_form)->v;
        uint32_t offset = 2; /* skip `match` and `value' */
        voba_value_t la_syn_form = voba_la_from_array1(form,offset);
        voba_value_t match = compile_match_match(la_syn_form,env,toplevel_env);
        if(!voba_is_nil(match)){
            ret = make_ast_match(ast_value,match);
        }
    }
    return ret;
}
static inline voba_value_t compile_match_value(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    int64_t len = voba_array_len(form);
    assert(len >= 1);
    voba_value_t syn_match = voba_array_at(form,0);
    if(len >= 2){
        voba_value_t syn_expr = voba_array_at(form,1);
        ret = compile_expr(syn_expr,env,toplevel_env);
    }else{
        report_error(VOBA_CONST_CHAR("bare match"),syn_match,toplevel_env);
    }
    return ret;
}
static inline voba_value_t compile_match_rule(voba_value_t syn_rule, voba_value_t env,voba_value_t toplevel_env);
voba_value_t compile_match_match(voba_value_t la_syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t a_rules = voba_make_array_0();
    while(!voba_la_is_nil(la_syn_form)){
        voba_value_t syn_rule = voba_la_car(la_syn_form);
        voba_value_t rule = compile_match_rule(syn_rule, env, toplevel_env);
        if(!voba_is_nil(rule)){
            voba_array_push(a_rules,rule);
        }
        la_syn_form = voba_la_cdr(la_syn_form);
    }
    ret = make_match(a_rules);
    return ret;
}
static inline voba_value_t compile_match_pattern(voba_value_t syn_rule, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_match_action(voba_value_t syn_rule, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_match_rule(voba_value_t syn_rule, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t rule = SYNTAX(syn_rule)->v;
    int64_t len = voba_array_len(rule);
    if(len >= 1){
        voba_value_t syn_pattern = voba_array_at(rule,0);
        voba_value_t pattern = compile_match_pattern(syn_pattern,env,toplevel_env);
        if(!voba_is_nil(pattern)){
            voba_value_t new_env = calculate_pattern_env(pattern,env);
            voba_value_t a_ast_action = compile_match_action(syn_rule,new_env,toplevel_env);
            if(!voba_is_nil(a_ast_action)){
                ret = make_rule(pattern,a_ast_action,new_env);
            }
        }
    }else{
        report_error(VOBA_CONST_CHAR("empty list is not a valid rule"), syn_rule,toplevel_env);
    }
    return ret;
}
static inline voba_value_t compile_match_pattern_var(voba_value_t syn_pattern, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_match_pattern_array(voba_value_t syn_pattern, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_match_pattern_else(voba_value_t syn_pattern, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_match_pattern(voba_value_t syn_pattern, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t pattern = SYNTAX(syn_pattern)->v;
    voba_value_t cls = voba_get_class(pattern);
    if(cls == voba_cls_symbol){
        ret = compile_match_pattern_var(syn_pattern, env,toplevel_env);
    }
    else if(cls == voba_cls_array){
        ret = compile_match_pattern_array(syn_pattern, env,toplevel_env);
    }else{
        ret = compile_match_pattern_else(syn_pattern,env,toplevel_env);
    }
    return ret;
}
static inline voba_value_t compile_match_pattern_var(voba_value_t syn_s_name, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
#ifndef NDEBUG    
    voba_value_t s_name = SYNTAX(syn_s_name)->v;
#endif
    assert(voba_is_a(s_name,voba_cls_symbol));
    voba_value_t var = make_var(syn_s_name,VAR_LOCAL);
    ret = make_pattern_var(var);
    return ret;
}
static inline voba_value_t compile_match_pattern_apply(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_match_pattern_value(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_match_pattern_array(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    assert(voba_is_a(form,voba_cls_array));
    int64_t len = voba_array_len(form);
    if(len > 0 ){
        voba_value_t syn_first = voba_array_at(form,0);
        voba_value_t first = SYNTAX(syn_first)->v;
        if(voba_eq(first, K(toplevel_env,value))){
            ret = compile_match_pattern_value(syn_form, env,toplevel_env);
        }else{
            ret = compile_match_pattern_apply(syn_form, env,toplevel_env);
        }
    }else{
        report_error(VOBA_CONST_CHAR("bare pattern"),syn_form,toplevel_env);
    }
    return ret;
}
static inline voba_value_t compile_match_pattern_value(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    uint32_t offset = 1; /* skip the value keyword */
    voba_value_t la_form = voba_la_from_array1(form,offset);
    voba_value_t a_ast_form = compile_exprs(la_form,env,toplevel_env);
    if(a_ast_form){
        ret = make_pattern_value(a_ast_form);
    }
    return ret;
}
static inline voba_value_t compile_match_pattern_else(voba_value_t syn_pattern, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t ast_value = compile_expr(syn_pattern, env,toplevel_env);
    if(ast_value){
        voba_value_t a_ast_form = voba_make_array_1(ast_value);
        ret = make_pattern_value(a_ast_form);
    }
    return ret;
}
static inline voba_value_t compile_match_pattern_apply(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    assert(voba_is_a(form,voba_cls_array));
    int64_t len = voba_array_len(form);
    voba_value_t syn_cls = voba_array_at(form,0);
    voba_value_t ast_cls = compile_expr(syn_cls,env,toplevel_env);
    if(!voba_is_nil(ast_cls)){
        voba_value_t subpatterns = voba_make_array_0();
        for(int64_t i = 1; i < len; ++i){
            voba_value_t syn_subpattern = voba_array_at(form,i);
            voba_value_t subpattern = compile_match_pattern(syn_subpattern,env,toplevel_env);
            if(!voba_is_nil(subpattern)){
                voba_array_push(subpatterns,subpattern);
            }
        }
        int64_t len_sub_patterns = voba_array_len(subpatterns);
        int subpattern_ok = (len_sub_patterns == (len -1));
        if(subpattern_ok){
            ret = make_pattern_apply(ast_cls,subpatterns);
        }
    }
    return ret;
}
static inline voba_value_t compile_match_action(voba_value_t syn_rule, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t rule = SYNTAX(syn_rule)->v;
#ifndef NDEBUG
    int64_t len = voba_array_len(rule);
#endif
    assert(len >= 1);
    uint32_t offset = 1; // (pattern action ...) skip pattern
    voba_value_t la_syn_exprs = voba_la_from_array1(rule,offset);
    ret = compile_exprs(la_syn_exprs, env, toplevel_env);
    return ret;
}

