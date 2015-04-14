#include <exec_once.h>
#include <voba/value.h>
#include "syn.h"
#include "ast.h"
#include "keywords.h"
#include "match.h"
#include "syn2ast_report.h"
#include "syn2ast_basic.h"
#include "syn2ast_match.h"
static inline voba_value_t compile_for_sub(ast_for_t* _for, voba_value_t la_syn_form, voba_value_t env, voba_value_t toplevel_env);
static inline int check_ast_for(ast_for_t* _for, voba_value_t syn, voba_value_t toplevel_env);
voba_value_t compile_for(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    int64_t len = voba_array_len(form);
    if(len != 1){
        voba_value_t ast_for = make_ast_for();
        ast_for_t * _for = &(AST(ast_for)->u._for);
        voba_value_t la_syn_form = voba_la_from_array1(form,1);
        while(!voba_is_nil(la_syn_form) && !voba_la_is_nil(la_syn_form)){
            la_syn_form = compile_for_sub(_for,la_syn_form,env,toplevel_env);
        }
        if( ! voba_is_nil(la_syn_form) ){
            int ok = check_ast_for(_for,syn_form,toplevel_env);
            if(ok){
                ret = ast_for;
            }
        }
    }else{
        report_error(
            VOBA_CONST_CHAR("bare `for'"),
            syn_form,toplevel_env);
    }
    return ret;
}
static inline voba_value_t for_init(ast_for_t* _for, voba_value_t la_syn_form, voba_value_t env, voba_value_t toplevel_env);
static inline voba_value_t for_each(ast_for_t* _for, voba_value_t la_syn_form, voba_value_t env, voba_value_t toplevel_env);
static inline voba_value_t for_do(ast_for_t* _for, voba_value_t la_syn_form, voba_value_t env, voba_value_t toplevel_env);
static inline voba_value_t for_if(ast_for_t* _for, voba_value_t la_syn_form, voba_value_t env, voba_value_t toplevel_env);
static inline voba_value_t for_accumulate(ast_for_t* _for, voba_value_t la_syn_form, voba_value_t env, voba_value_t toplevel_env);
static inline voba_value_t compile_for_sub(ast_for_t* _for, voba_value_t la_syn_form, voba_value_t env, voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t syn_key = voba_la_car(la_syn_form);
    voba_value_t key = SYNTAX(syn_key)->v;
    if(voba_eq(key, COLON(toplevel_env,each))){
        ret = for_each(_for,la_syn_form,env,toplevel_env);
    }else if(voba_eq(key, COLON(toplevel_env,if))){
        ret = for_if(_for,la_syn_form,env,toplevel_env);
    }else if(voba_eq(key, COLON(toplevel_env,accumulate))){
        ret = for_accumulate(_for,la_syn_form,env,toplevel_env);
    }else if(voba_eq(key, COLON(toplevel_env,do))){
        ret = for_do(_for,la_syn_form,env,toplevel_env);
    }else if(voba_eq(key, COLON(toplevel_env,init))){
        ret = for_init(_for,la_syn_form,env,toplevel_env);
    }else{
        report_error(VOBA_CONST_CHAR("unrecoganized keyword in `for' statement"),
                     syn_key,toplevel_env);
    }
    return ret;
}
static inline voba_value_t for_E(voba_value_t * E, voba_value_t la_syn_form, voba_value_t env, voba_value_t toplevel_env);
static inline voba_value_t for_each(ast_for_t* _for, voba_value_t la_syn_form, voba_value_t env, voba_value_t toplevel_env)
{
    return for_E(&_for->each,la_syn_form,env,toplevel_env);
}
static inline voba_value_t for_init(ast_for_t* _for, voba_value_t la_syn_form, voba_value_t env, voba_value_t toplevel_env)
{
    return for_E(&_for->init,la_syn_form,env,toplevel_env);
}
static inline voba_value_t for_accumulate(ast_for_t* _for, voba_value_t la_syn_form, voba_value_t env, voba_value_t toplevel_env)
{
    return for_E(&_for->accumulate,la_syn_form,env,toplevel_env);
}
static inline voba_value_t for_if(ast_for_t* _for, voba_value_t la_syn_form, voba_value_t env, voba_value_t toplevel_env)
{
    return for_E(&_for->_if,la_syn_form,env,toplevel_env);
}
static inline voba_value_t for_do(ast_for_t* _for, voba_value_t la_syn_form, voba_value_t env, voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    la_syn_form = voba_la_cdr(la_syn_form);
    _for->match = compile_match_match(la_syn_form,env,toplevel_env);
    if(!voba_is_nil(_for->match)){
        ret = la_syn_form;
    }
    return ret;
}
static inline voba_value_t for_E(voba_value_t * E, voba_value_t la_syn_form, voba_value_t env, voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t syn_key = voba_la_car(la_syn_form); /* skip :key */
    la_syn_form = voba_la_cdr(la_syn_form);
    if(!voba_la_is_nil(la_syn_form)){
        voba_value_t syn_expr = voba_la_car(la_syn_form);
        *E = compile_expr(syn_expr,env,toplevel_env);
        if(!voba_is_nil(*E)){
            ret = voba_la_cdr(la_syn_form);
        }
    }else{
        report_error(VOBA_CONST_CHAR("missing expression in `for' statement")
                     ,syn_key,toplevel_env);
    }
    return ret;
}
static inline int check_ast_for(ast_for_t* _for, voba_value_t syn, voba_value_t toplevel_env)
{
    int ret = 1;
    if(voba_is_nil(_for->each)){
        ret = 0;
        report_error(VOBA_CONST_CHAR("missing :each in `for' statement"),
                     syn,toplevel_env);
    }
    return ret;
}

voba_value_t compile_break(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    int64_t len = voba_array_len(form);
    voba_value_t ast_value = VOBA_NIL;
    if(len == 1){
        voba_value_t syn_break = voba_array_at(form,0);
        ast_value = make_ast_constant(MAKE_SYN_CONST(VOBA_NIL));
        ret = make_ast_break(ast_value,syn_break);
    }else if(len == 2){
        voba_value_t syn_expr = voba_array_at(form,1);
        ast_value = compile_expr(syn_expr,env,toplevel_env);
        if(!voba_is_nil(ast_value)){
            voba_value_t syn_break = voba_array_at(form,0);
            ret = make_ast_break(ast_value,syn_break);
        }
    }else{
        report_error(VOBA_CONST_CHAR("illegal form: (break [<E>]), len is not 2")
                     ,syn_form,toplevel_env);
    }
    return ret;
}
