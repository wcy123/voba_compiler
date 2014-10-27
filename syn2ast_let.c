#define EXEC_ONCE_TU_NAME "syn2ast_let"
#include <exec_once.h>
#include <voba/include/value.h>
#include "ast.h"
#include "syn.h"
#include "syn2ast_report.h"
#include "syn2ast_basic.h"
#include "syn2ast_let.h"

static inline voba_value_t compile_let_bindings(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env, voba_value_t a_la_syn_exprs);
static inline voba_value_t compile_let_body(voba_value_t syn_form, voba_value_t a_la_syn_exprs, voba_value_t env,voba_value_t toplevel_env);
voba_value_t compile_let(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    assert(voba_is_a(form,voba_cls_array));
    int64_t len = voba_array_len(form);
    if(len == 0){
        assert(0&& "logical error");
    }else if (len == 1){
        report_error(VOBA_CONST_CHAR("bare let"), voba_array_at(form,0),toplevel_env);
    }else{
        voba_value_t syn_let_1 = voba_array_at(form,1);
        voba_value_t a_la_syn_exprs = voba_make_array_0();
        voba_value_t let_env = compile_let_bindings(syn_let_1,env,toplevel_env,a_la_syn_exprs);
        uint32_t offset = 2; // (let (<E>) <B>) ; skip `let` and `(<E>)`;
        voba_value_t la_syn_body = voba_la_from_array1(form, offset);
        voba_value_t a_ast_exprs = compile_let_body(la_syn_body,a_la_syn_exprs,let_env,toplevel_env);
        env_t * p_let_env = ENV(let_env);
        ret = make_ast_let(p_let_env,a_ast_exprs);
    }
    return ret;
}
// return an environment of the let lexical scope
static inline voba_value_t compile_let_binding(voba_value_t syn_let_pair, voba_value_t toplevel_env, voba_value_t a_la_syn_exprs);
static inline voba_value_t compile_let_bindings(voba_value_t syn_let_pair, voba_value_t env,voba_value_t toplevel_env, voba_value_t a_la_syn_exprs)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t let_pair = SYNTAX(syn_let_pair)->v;
    if(voba_is_a(let_pair, voba_cls_array)){
        voba_value_t cur = voba_la_from_array0(let_pair);
        ret = make_env();
        env_t * p_env = ENV(ret);
        p_env->parent = env;
        p_env->a_var = voba_make_array_0();
        while(!voba_la_is_nil(cur)){
            voba_value_t var = compile_let_binding(voba_la_car(cur),toplevel_env,a_la_syn_exprs);
            if(!voba_is_nil(var)){ // no need to report error, it is done in compile_let_binding.
                assert(voba_is_a(var, voba_cls_var));
                voba_array_push(p_env->a_var, var);
            }
            cur = voba_la_cdr(cur);
        }
    }else{
        report_error(VOBA_CONST_CHAR("illegal form. (let <E> ...) <E> must be an array"),
                     syn_let_pair,toplevel_env);
    }
    return ret;
}
static inline voba_value_t compile_let_binding_uninitialized_var(voba_value_t syn_s_name, voba_value_t toplevel_env, voba_value_t a_la_syn_exprs);
static inline voba_value_t compile_let_binding_var(voba_value_t syn_let_binding, voba_value_t toplevel_env, voba_value_t a_la_syn_exprs);
// return a var object
static inline voba_value_t compile_let_binding(voba_value_t syn_let_binding, voba_value_t toplevel_env, voba_value_t a_la_syn_exprs)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t let_binding = SYNTAX(syn_let_binding)->v;
    if(voba_is_a(let_binding, voba_cls_array)){
        ret = compile_let_binding_var(syn_let_binding,toplevel_env,a_la_syn_exprs);
    }else{
        voba_value_t syn_s_name = syn_let_binding;
        ret = compile_let_binding_uninitialized_var(syn_s_name,toplevel_env,a_la_syn_exprs);
    }
    return ret;
}
static inline voba_value_t compile_let_binding_uninitialized_var(voba_value_t syn_s_name, voba_value_t toplevel_env, voba_value_t a_la_syn_exprs)
{
    voba_value_t ret = make_var(syn_s_name, VAR_LOCAL);
    voba_value_t s_name = SYNTAX(syn_s_name)->v;
    if(voba_is_a(s_name,voba_cls_symbol)){
        report_warn(VOBA_CONST_CHAR("var is not initialized"),syn_s_name, toplevel_env);
        voba_array_push(a_la_syn_exprs, voba_la_from_array0(voba_make_array_0()));
        ret = make_var(syn_s_name,VAR_LOCAL);
    }else{
        ret = VOBA_NIL;
        report_error(VOBA_CONST_CHAR("illegal form. (let (<E>)) or (let ((<E>)), where <E> is not a symbol"), syn_s_name, toplevel_env);                
    }
    return ret;
}
static inline voba_value_t compile_let_binding_with_value(voba_value_t syn_let_binding, voba_value_t toplevel_env, voba_value_t a_la_syn_exprs);
static inline voba_value_t compile_let_binding_var(voba_value_t syn_let_binding, voba_value_t toplevel_env, voba_value_t a_la_syn_exprs)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t let_binding = SYNTAX(syn_let_binding)->v;
    int64_t len = voba_array_len(let_binding);
    if(len != 0){
        voba_value_t syn_s_name = voba_array_at(let_binding,0);
        if(len == 1){
            ret = compile_let_binding_uninitialized_var(syn_s_name,toplevel_env,a_la_syn_exprs);
        }else{
            ret = compile_let_binding_with_value(syn_let_binding,toplevel_env,a_la_syn_exprs);
        }
    }else{
        report_error(VOBA_CONST_CHAR("illegal form. (let (<X>...)), where <X> is an empty list, e.g. (let (()) nil)"), syn_let_binding, toplevel_env);
        ret = VOBA_NIL;
    }
    return ret;
}
static inline voba_value_t compile_let_binding_with_value(voba_value_t syn_let_binding, voba_value_t toplevel_env, voba_value_t a_la_syn_exprs)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t let_binding = SYNTAX(syn_let_binding)->v;
    voba_value_t syn_s_name = voba_array_at(let_binding,0);
    voba_value_t s_name = SYNTAX(syn_s_name)->v;
    if(voba_is_a(s_name,voba_cls_symbol)){
        ret = make_var(syn_s_name,VAR_LOCAL);
        uint32_t offset = 1;// (var value ....) skip var
        voba_array_push(a_la_syn_exprs, voba_la_from_array1(let_binding,offset));
    }else{
        ret = VOBA_NIL;
        report_error(VOBA_CONST_CHAR("illegal form. (let (<E>)) or (let ((<E>)), where <E> is not a symbol"), syn_s_name, toplevel_env);                
    }
    return ret;
}
static inline voba_value_t compile_let_body_init(voba_value_t a_la_syn_exprs, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_let_body(voba_value_t la_syn_body, voba_value_t a_la_syn_exprs, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t a_ast_init_exprs = compile_let_body_init(a_la_syn_exprs,env,toplevel_env);
    ret = a_ast_init_exprs;
    assert(voba_is_a(a_ast_init_exprs,voba_cls_array));
    voba_value_t a_ast_body_exprs = compile_exprs(la_syn_body,env,toplevel_env);
    assert(voba_is_a(a_ast_body_exprs,voba_cls_array));
    ret = voba_array_concat(a_ast_init_exprs,a_ast_body_exprs);
    return ret;
}
static inline voba_value_t compile_let_body_init_var(voba_value_t var, voba_value_t la_syn_exprs, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_let_body_init(voba_value_t a_la_syn_exprs, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    env_t * p_env = ENV(env);
    int64_t len_var = voba_array_len(p_env->a_var);
    int64_t len_expr = voba_array_len(a_la_syn_exprs);
    assert(voba_is_a(env,voba_cls_env));
    if(len_var == len_expr){
        ret = voba_make_array_0();
        for(int64_t i = 0; i < len_var ; ++i){
            voba_value_t la_syn_exprs = voba_array_at(a_la_syn_exprs,i);
            voba_value_t var = voba_array_at(p_env->a_var,i);
            voba_value_t ast_set_var = compile_let_body_init_var(var,la_syn_exprs,env,toplevel_env);
            if(!voba_is_nil(ast_set_var)){
                voba_array_push(ret, ast_set_var);
            }
        }
    }else{
        assert(0&&"length of let var and let expr are not same.");
    }
    return ret;
}
static inline voba_value_t compile_let_body_init_var(voba_value_t var, voba_value_t la_syn_exprs, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t a_ast_exprs = compile_exprs(la_syn_exprs,env,toplevel_env);
    if(!voba_is_nil(a_ast_exprs)){
        ret = make_ast_set_var(var,a_ast_exprs);
    }
    return ret;
}

