// return an array of ast
#include "match.h"
static voba_value_t compile_exprs(voba_value_t la_syn_exprs, voba_value_t env, voba_value_t toplevel_env)
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
        ret = voba_array_push(ret,make_ast_constant(make_syn_nil()));
    }
    return ret;
}
static voba_value_t compile_expr(voba_value_t syn_expr,voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t expr = SYNTAX(syn_expr)->v;
    voba_value_t cls = voba_get_class(expr);
    if(cls == voba_cls_nil){
        ret = make_ast_constant(make_syn_nil());
    }
#define COMPILE_EXPR_SMALL_TYPES(tag,name,type)                         \
    else if(cls == voba_cls_##name){                                    \
        ret = make_ast_constant(syn_expr);                              \
    }
    VOBA_SMALL_TYPES(COMPILE_EXPR_SMALL_TYPES)
    else if(cls == voba_cls_str){
        ret = make_ast_constant(syn_expr);
    }
    else if(cls == voba_cls_symbol){
        ret = compile_symbol(syn_expr,env,toplevel_env);
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
        if(voba_is_symbol(f)){
            if(voba_eq(f, K(toplevel_env,quote))){
                report_error(VOBA_CONST_CHAR("not implemented for quote"),syn_f,toplevel_env);
            }else if(voba_eq(f, K(toplevel_env,if))){
                report_error(VOBA_CONST_CHAR("not implemented for if"),syn_f,toplevel_env);
            }else if(voba_eq(f, K(toplevel_env,import))){
                report_error(VOBA_CONST_CHAR("illegal form. import is the keyword"),syn_f,toplevel_env);
            }else if(voba_eq(f, K(toplevel_env,fun))){
                ret = compile_fun(syn_form, env, toplevel_env);
            }else if(voba_eq(f, K(toplevel_env,let))){
                ret = compile_let(syn_form, env, toplevel_env);
            }else if(voba_eq(f, K(toplevel_env,match))){
                ret = compile_match(syn_form, env, toplevel_env);
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
                assert(voba_is_array(ast_form));
                ret = make_ast_apply(ast_form);
            }
        }
    }else{
        report_error(VOBA_CONST_CHAR("illegal form. empty form"),syn_form,toplevel_env);
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
        if(voba_is_array(fun_args)){
            voba_value_t a_var_A = compile_arg_list(voba_la_from_array0(fun_args),toplevel_env);
            uint32_t offset = 2;// skip (fun (...) ...)
            voba_value_t la_syn_body = voba_la_from_array1(form,offset);
            voba_value_t fun = make_compiler_fun();
            COMPILER_FUN(fun)->a_var_A = a_var_A;
            COMPILER_FUN(fun)->a_var_C = voba_make_array_0();
            COMPILER_FUN(fun)->parent = env;
            voba_value_t a_ast_exprs = compile_exprs(la_syn_body,fun,toplevel_env);
            voba_value_t syn_s_name = voba_array_at(form,0); // function name is `fun`, the keyword
            ret = make_ast_fun(syn_s_name,COMPILER_FUN(fun),a_ast_exprs);
        }else{
            report_error(VOBA_CONST_CHAR("illegal form. argument list is not a list"),syn_fun_args,toplevel_env);
        }
    }
    }
    return ret;
}
static voba_value_t compile_arg_list(voba_value_t la_arg, voba_value_t toplevel_env)
{
    voba_value_t ret = voba_make_array_0(); // a_var_A;
    voba_value_t cur = la_arg;
    int32_t index = 0;
    while(!voba_la_is_nil(cur)){
        voba_value_t arg = voba_la_car(cur);
        voba_value_t arg_var = compile_arg(arg,index,toplevel_env);
        if(!voba_is_nil(arg_var)){
            voba_array_push(ret,arg_var);
        }
        cur = voba_la_cdr(cur);
        index ++;
    }
    return ret;
}
static inline voba_value_t compile_arg(voba_value_t a, int32_t index, voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    assert(voba_is_a(a,voba_cls_syn));
    voba_value_t v = SYNTAX(a)->v;
    if(voba_is_symbol(v)){
        ret = make_var(a,VAR_ARGUMENT);
        VAR(ret)->u.index = index;
    }else{
        report_error(VOBA_CONST_CHAR("not implemented"),a,toplevel_env);
    }
    return ret;
}
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
static inline voba_value_t compile_let_bindings(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env, voba_value_t a_la_syn_exprs);
static inline voba_value_t compile_let_body(voba_value_t syn_form, voba_value_t a_la_syn_exprs, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_let(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env)
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
static inline voba_value_t compile_match_value(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_match_match(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_match(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t ast_value = compile_match_value(syn_form,env,toplevel_env);
    if(!voba_is_nil(ast_value)){
        voba_value_t match = compile_match_match(syn_form,env,toplevel_env);
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
static inline voba_value_t compile_match_match(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    int64_t len = voba_array_len(form);
    voba_value_t a_rules = voba_make_array_0();
    for(int64_t i = 2; i < len; ++i){
        voba_value_t syn_rule = voba_array_at(form,i);
        voba_value_t rule = compile_match_rule(syn_rule, env, toplevel_env);
        if(!voba_is_nil(rule)){
            voba_array_push(a_rules,rule);
        }
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
static inline voba_value_t compile_match_pattern_apply(voba_value_t syn_pattern, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_match_pattern(voba_value_t syn_pattern, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t pattern = SYNTAX(syn_pattern)->v;
    voba_value_t cls = voba_get_class(pattern);
    if(cls == voba_cls_nil){
        ret = make_pattern_constant(make_syn_nil());
    }
#define COMPILE_MATCH_PATTERN_SMALL_TYPES(tag,name,type)                \
    else if(cls == voba_cls_##name){                                    \
        ret = make_pattern_constant(syn_pattern);                       \
    }
    VOBA_SMALL_TYPES(COMPILE_MATCH_PATTERN_SMALL_TYPES)
    else if(cls == voba_cls_str){
        ret = make_pattern_constant(syn_pattern);
    }
    else if(cls == voba_cls_symbol){
        ret = compile_match_pattern_var(syn_pattern, env,toplevel_env);
    }
    else if(cls == voba_cls_array){
        ret = compile_match_pattern_apply(syn_pattern, env,toplevel_env);
    }else{
        report_error(VOBA_CONST_CHAR("invalid pattern"),syn_pattern,toplevel_env);
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
static inline voba_value_t compile_match_pattern_apply(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    assert(voba_is_a(form,voba_cls_array));
    int64_t len = voba_array_len(form);
    if(len > 0 ){
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
    }else{
        report_error(VOBA_CONST_CHAR("bare pattern"),syn_form,toplevel_env);
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
/* Local Variables: */
/* mode:c */
/* coding: utf-8-unix */
/* End: */
