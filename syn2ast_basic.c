#include <exec_once.h>
#include <voba/value.h>
#include "ast.h"
#include "syn.h"
#include "keywords.h"
#include "syn2ast_report.h"
#include "syn2ast_basic.h"
#include "syn2ast_let.h"
#include "syn2ast_match.h"
#include "syn2ast_for.h"
#include "syn2ast_if.h"
#include "syn2ast_quote.h"
#include "syn2ast_and_or.h"
#include "syn2ast_yield.h"
static inline voba_value_t compile_fun(voba_value_t syn_form, voba_value_t env, voba_value_t toplevel_env);
//static inline voba_value_t compile_arg_list(voba_value_t la_arg, voba_value_t toplevel_env);
//static inline voba_value_t compile_arg(voba_value_t a, int32_t index, voba_value_t toplevel_env);
static inline voba_value_t compile_array(voba_value_t syn_form, voba_value_t env, voba_value_t toplevel_env);
static inline voba_value_t compile_symbol(voba_value_t syn_symbol, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_it(voba_value_t syn_symbol, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_args(voba_value_t syn_symbol, voba_value_t env,voba_value_t toplevel_env);
voba_value_t compile_exprs(voba_value_t la_syn_exprs, voba_value_t env, voba_value_t toplevel_env)
{
    voba_value_t cur = voba_la_copy(la_syn_exprs);
    voba_value_t ret = voba_make_array_0();
    if(!voba_la_is_nil(cur)){
        while(!voba_la_is_nil(cur)){
            voba_value_t syn_exprs = voba_la_car(cur);
            voba_value_t ast_expr = compile_expr(syn_exprs,env,toplevel_env);
            if(!voba_is_nil(ast_expr)){
                ret = voba_array_push(ret,ast_expr);
            }
            cur = voba_la_cdr(cur);
        }
    }else {
        ret = voba_array_push(ret,make_ast_constant(MAKE_SYN_CONST(VOBA_NIL)));
    }
    return ret;
}
voba_value_t compile_expr(voba_value_t syn_expr,voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t expr = SYNTAX(syn_expr)->v;
    voba_value_t cls = voba_get_class(expr);
    if(cls == voba_cls_nil){
        ret = make_ast_constant(MAKE_SYN_CONST(VOBA_NIL));
    }
    else if(cls == voba_cls_i32){
        ret = make_ast_constant(syn_expr);
    }
    else if(cls == voba_cls_bool){
        ret = make_ast_constant(syn_expr);
    }
    else if(cls == voba_cls_str){
        ret = make_ast_constant(syn_expr);
    }
    else if(cls == voba_cls_symbol){
        voba_value_t expr = SYNTAX(syn_expr)->v;
        if( voba_eq(expr, K(toplevel_env, it)) ){
            ret = compile_it(syn_expr,env,toplevel_env);
        }else if(voba_eq(expr, K(toplevel_env,args))){
	    ret = compile_args(syn_expr, env, toplevel_env);
	}else{
            ret = compile_symbol(syn_expr,env,toplevel_env);
        }
    }
    else if(cls == voba_cls_array){
        ret = compile_array(syn_expr,env,toplevel_env);
    }else{
        report_error(VOBA_CONST_CHAR("invalid expression"),syn_expr,toplevel_env);
    }
    return ret;
}
static inline voba_value_t compile_array(voba_value_t syn_form, voba_value_t env, voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    int64_t len = voba_array_len(form);
    if(len != 0){
        voba_value_t syn_f = voba_array_at(form,0);
        voba_value_t f = SYNTAX(syn_f)->v;
        if(voba_is_a(f,voba_cls_symbol)){
            if(voba_eq(f, K(toplevel_env,quote))){
                ret = compile_quote(syn_form,env,toplevel_env);
            }else if(voba_eq(f, K(toplevel_env,import))){
                report_error(VOBA_CONST_CHAR("illegal form. import is the keyword"),syn_f,toplevel_env);
            }else if(voba_eq(f, K(toplevel_env,fun))){
                ret = compile_fun(syn_form, env, toplevel_env);
            }else if(voba_eq(f, K(toplevel_env,let))){
                ret = compile_let(syn_form, env, toplevel_env);
            }else if(voba_eq(f, K(toplevel_env,match))){
                ret = compile_match(syn_form, env, toplevel_env);
            }else if(voba_eq(f, K(toplevel_env,if))){
                ret = compile_if(syn_form, env, toplevel_env);
            }else if(voba_eq(f, K(toplevel_env,for))){
                ret = compile_for(syn_form, env, toplevel_env);
            }else if(voba_eq(f, K(toplevel_env,break))){
                ret = compile_break(syn_form, env, toplevel_env);
            }else if(voba_eq(f, K(toplevel_env,and))){
                ret = compile_and(syn_form, env, toplevel_env);
            }else if(voba_eq(f, K(toplevel_env,or))){
                ret = compile_or(syn_form, env, toplevel_env);
            }else if(voba_eq(f, K(toplevel_env,yield))){
                ret = compile_yield(syn_form, env, toplevel_env);
            }else{
                // if the first s-exp is a symbol but not a keyword,
                // compile it as same as default behaviour,
                // i.e. return an ``apply'' form.
                goto label_default;
            }
        }else{
        label_default:
            ;
            voba_value_t ast_form = compile_exprs(voba_la_from_array0(form),env,toplevel_env);
            if(!voba_is_nil(ast_form)){
                assert(voba_is_a(ast_form,voba_cls_array));
                ret = make_ast_apply(ast_form);
            }
        }
    }else{
        report_error(VOBA_CONST_CHAR("illegal form. empty form"),syn_form,toplevel_env);
    }
    return ret;
}
static inline voba_value_t compile_function_1(voba_value_t syn_s_name,
					      voba_value_t a_la_syn_args,
					      voba_value_t a_la_syn_body,
					      voba_value_t env,
					      voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    // '(match args ((match-tuple <la_syn_args>) <la_syn_body>))
    voba_value_t a_rules = voba_make_array_0();
    int64_t len = voba_array_len(a_la_syn_args);
    assert( len == voba_array_len(a_la_syn_body));
    for(int64_t i = 0; i < len; ++i){
	voba_value_t la_syn_args = voba_array_at(a_la_syn_args,i);
	voba_value_t la_syn_body = voba_array_at(a_la_syn_body,i);
	a_rules = voba_array_push(a_rules,
				  syn_backquote(2,
						syn_backquote(2,
							      SYN_SYMBOL(match-tuple),
							      voba_la_to_array(la_syn_args)),
						voba_la_to_array(la_syn_body)));
    }
    voba_value_t new_form =
	syn_backquote(3,
		      SYN_SYMBOL(match),
		      SYN_SYMBOL(args),
		      a_rules);
    voba_value_t fun = make_compiler_fun();
    COMPILER_FUN(fun)->a_var_A = voba_make_array_0();
    COMPILER_FUN(fun)->parent = env;
    voba_value_t ast_expr = compile_expr(new_form,fun,toplevel_env);
    if(ast_expr){
	voba_value_t ast_fun = make_ast_fun(syn_s_name,COMPILER_FUN(fun),
					    voba_make_array_1(ast_expr));
	ret = ast_fun;
    }
    return ret;
}
voba_value_t compile_defun(voba_value_t top_var, voba_value_t a_syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    assert(voba_array_len(a_syn_form)>=1);
    voba_value_t syn_form = voba_array_at(a_syn_form,0);
    voba_value_t form = SYNTAX(syn_form)->v;
    voba_value_t syn_s_name = voba_array_at(form,0);
    assert(voba_is_a(SYNTAX(syn_s_name)->v,voba_cls_symbol));
    voba_value_t a_la_syn_args = voba_make_array_0();
    voba_value_t a_la_syn_body = voba_make_array_0();
    int64_t len = voba_array_len(a_syn_form);
    int64_t i = 0;
    for( i = 0; i < len; ++i){
	voba_value_t syn_tmp_form = voba_array_at(a_syn_form,i);
	voba_value_t tmp_form = SYNTAX(syn_tmp_form)->v;
	voba_value_t syn_tmp_s_name = voba_array_at(tmp_form,0);
	if(! voba_eq(SYNTAX(syn_s_name)->v,
		     SYNTAX(syn_tmp_s_name)->v) ){
	    assert(0&&"all function segment must have the same name");
	}
	assert(voba_array_len(tmp_form) >= 2);
	// extract args
	voba_value_t syn_f_args = voba_array_at(tmp_form,1);
	voba_value_t a_f_args = SYNTAX(syn_f_args)->v;
	voba_value_t la_f_args = voba_la_from_array0(a_f_args);
	assert(voba_la_len(la_f_args) >= 1);
	voba_value_t la_syn_args = voba_la_cdr(la_f_args); // skip f;
	a_la_syn_args = voba_array_push(a_la_syn_args,la_syn_args);
	// extract body
	uint32_t offset = 2; // skip def, args
	voba_value_t la_syn_body = voba_la_from_array1(tmp_form,offset);
	a_la_syn_body = voba_array_push(a_la_syn_body,la_syn_body);
    }
    voba_value_t ast_fun = compile_function_1(syn_s_name,
					      a_la_syn_args,
					      a_la_syn_body,
					      env,
					      toplevel_env);
    if(ast_fun){
	voba_value_t ast_exprs = voba_make_array_1(ast_fun);
	ret = make_ast_set_var(top_var, ast_exprs);
    }
    return ret;
}
static inline voba_value_t compile_fun(voba_value_t syn_form, voba_value_t env, voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    int64_t len = voba_array_len(form);
    // here we don't know the ``fun'' capture any variable
    // or not, ``compile_fun'' try to compile the body of
    // the ``fun'', and create a capture if necessary.
    switch(len){
    case 1:
        report_error(VOBA_CONST_CHAR("illegal form. bare fun is not fun"),syn_form,toplevel_env);
        break;
    case 2:
        report_error(VOBA_CONST_CHAR("illegal form. empty body is not fun"),syn_form,toplevel_env);
        break;
    default: {
        voba_value_t syn_fun_args = voba_array_at(form,1);
        voba_value_t fun_args = SYNTAX(syn_fun_args)->v;
        if(voba_is_a(fun_args,voba_cls_array)){
	    voba_value_t la_syn_args = voba_la_from_array0(fun_args);
            uint32_t offset = 2;// skip (fun (...) ...)
            voba_value_t la_syn_body = voba_la_from_array1(form,offset);
            voba_value_t syn_s_name = voba_array_at(form,0); // function name is `fun`, the keyword
            ret = compile_function_1(syn_s_name,
				     voba_make_array_1(la_syn_args),
				     voba_make_array_1(la_syn_body),
				     env,
				     toplevel_env);
        }else{
            report_error(VOBA_CONST_CHAR("illegal form. argument list is not a list"),syn_fun_args,toplevel_env);
        }
    }
    }
    return ret;
}
/* static voba_value_t compile_arg_list(voba_value_t la_arg, voba_value_t toplevel_env) */
/* { */
/*     voba_value_t ret = voba_make_array_0(); // a_var_A; */
/*     voba_value_t cur = la_arg; */
/*     int32_t index = 0; */
/*     while(!voba_la_is_nil(cur)){ */
/*         voba_value_t arg = voba_la_car(cur); */
/*         voba_value_t arg_var = compile_arg(arg,index,toplevel_env); */
/*         if(!voba_is_nil(arg_var)){ */
/*             voba_array_push(ret,arg_var); */
/*         } */
/*         cur = voba_la_cdr(cur); */
/*         index ++; */
/*     } */
/*     return ret; */
/* } */
/* static inline voba_value_t compile_arg(voba_value_t a, int32_t index, voba_value_t toplevel_env) */
/* { */
/*     voba_value_t ret = VOBA_NIL; */
/*     assert(voba_is_a(a,voba_cls_syn)); */
/*     voba_value_t v = SYNTAX(a)->v; */
/*     if(voba_is_a(v,voba_cls_symbol)){ */
/*         ret = make_var(a,VAR_ARGUMENT); */
/*         VAR(ret)->u.index = index; */
/*     }else{ */
/*         report_error(VOBA_CONST_CHAR("not implemented"),a,toplevel_env); */
/*     } */
/*     return ret; */
/* } */
static inline voba_value_t compile_symbol(voba_value_t syn_symbol, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t var = search_symbol(syn_symbol,env);
    if(!voba_is_nil(var)){
        ret = make_ast_var(var);
    }else{
        report_error(VOBA_CONST_CHAR("cannot find symbol definition"),syn_symbol,toplevel_env);
    }
    return ret;
}
static inline voba_value_t compile_it(voba_value_t syn_symbol, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = make_ast_it(syn_symbol);
    return ret;
}
static inline voba_value_t compile_args(voba_value_t syn_symbol, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = make_ast_args(syn_symbol);
    return ret;
}
