#define EXEC_ONCE_TU_NAME "env"
#include <voba/include/value.h>
#include <voba/core/builtin.h>
#include "env.h"
#include "syn.h"
#include "var.h"
#include "ast.h"

DEFINE_CLS(sizeof(env_t),env);
DEFINE_CLS(sizeof(compiler_fun_t),compiler_fun);

voba_value_t make_env()
{
    voba_value_t ret = voba_make_user_data(voba_cls_env);
    ENV(ret)->a_var = voba_make_array_0();
    ENV(ret)->parent = VOBA_NIL;
    return ret;
}
voba_value_t make_compiler_fun()
{
    voba_value_t ret = voba_make_user_data(voba_cls_compiler_fun);
    COMPILER_FUN(ret)->a_var_A = voba_make_array_0();
    COMPILER_FUN(ret)->a_var_C = voba_make_array_0();
    COMPILER_FUN(ret)->parent = VOBA_NIL;
    return ret;
}
// return an index to the array `a`
int32_t search_symbol_in_array(voba_value_t syn_symbol, voba_value_t a)
{
    int32_t ret = -1;
    int64_t n = 0;
    int64_t len = voba_array_len(a);
    assert(voba_is_a(syn_symbol,voba_cls_syn));
    voba_value_t symbol = SYNTAX(syn_symbol)->v;
    if(0) fprintf(stderr,__FILE__ ":%d:[%s] searching for 0x%lx %s\n", __LINE__, __FUNCTION__,
            symbol, voba_value_to_str(voba_symbol_name(symbol))->data);

    for(n = 0; n < len; ++n){
        voba_value_t var = voba_array_at(a,n);
        voba_value_t v = SYNTAX(VAR(var)->syn_s_name)->v;
        if(0){
            fprintf(stderr,__FILE__ ":%d:[%s] compare with var 0x%lx\n", __LINE__, __FUNCTION__,
                    v);//, voba_value_to_str(voba_symbol_name(v))->data);
        }
        if(voba_eq(SYNTAX(VAR(var)->syn_s_name)->v, symbol)){
            ret = (int32_t)n;
            break;
        }
    }
    return ret;
}
// return a var object if found, otherwise VOBA_NIL
static inline voba_value_t search_symbol_env(voba_value_t syn_symbol, voba_value_t env);
static inline voba_value_t search_symbol_fun(voba_value_t syn_symbol, voba_value_t fun);
static inline voba_value_t search_symbol_fun_arg(voba_value_t syn_symbol, voba_value_t fun);
static inline voba_value_t search_symbol_fun_closure(voba_value_t syn_symbol, voba_value_t voba_fun);
voba_value_t search_symbol(voba_value_t syn_symbol, voba_value_t env_or_fun)
{
    voba_value_t ret = VOBA_NIL;
    if(voba_is_nil(env_or_fun)){
        ret = VOBA_NIL; // reach the top level, cannot find the symbol
    }else if(voba_is_a(env_or_fun, voba_cls_env)){
        ret = search_symbol_env(syn_symbol,env_or_fun);
    }else if(voba_is_a(env_or_fun, voba_cls_compiler_fun)){
        ret = search_symbol_fun(syn_symbol,env_or_fun);
    }else{
        assert(0&&"never goes here");
    }
    return ret;
}
static inline voba_value_t search_symbol_env(voba_value_t syn_symbol, voba_value_t v_env)
{
    voba_value_t ret = VOBA_NIL;
    env_t * env = ENV(v_env);
    int32_t n = search_symbol_in_array(syn_symbol,env->a_var);
    if(n != -1){
        voba_value_t syn_s_name = VAR(voba_array_at(env->a_var, n))->syn_s_name;
        if(voba_is_nil(env->parent)){ // top level enviroment
            ret = voba_array_at(env->a_var, n);
            assert(voba_is_a(ret,voba_cls_var)&&var_is_top(VAR(ret)));
        }else{
            ret = make_var(syn_s_name,VAR_LOCAL);
        }
    }else{
        ret = search_symbol(syn_symbol, env->parent);
    }
    return ret;
}
static inline voba_value_t search_symbol_fun(voba_value_t syn_symbol, voba_value_t voba_fun)
{
    voba_value_t ret = VOBA_NIL;
    ret = search_symbol_fun_arg(syn_symbol,voba_fun);
    if(voba_is_nil(ret)){
        ret = search_symbol_fun_closure(syn_symbol,voba_fun);
    }
    return ret;
}
static inline voba_value_t search_symbol_fun_arg(voba_value_t syn_symbol, voba_value_t voba_fun)
{
    voba_value_t ret = VOBA_NIL;
    compiler_fun_t* fun = COMPILER_FUN(voba_fun);
    int32_t n = search_symbol_in_array(syn_symbol,fun->a_var_A);
    if(n != -1){
        var_t * var = VAR(voba_array_at(fun->a_var_A, n));
        voba_value_t syn_s_name = var->syn_s_name;
        ret = make_var(syn_s_name, VAR_ARGUMENT);
        VAR(ret)->u.index = n;
        assert(n == var->u.index);
    }else{
        ret = VOBA_NIL;// cannot find it.
    }
    return ret;
}
static inline voba_value_t search_symbol_fun_closure(voba_value_t syn_symbol, voba_value_t voba_fun)
{
    voba_value_t ret = VOBA_NIL;
    compiler_fun_t* fun = COMPILER_FUN(voba_fun);
    int32_t n = search_symbol_in_array(syn_symbol,fun->a_var_C);
    if( n != -1){
        voba_value_t vvar = voba_array_at(fun->a_var_C, n);
        var_t * var = VAR(vvar);
        // var is in the context of the parent function
        if(!var_is_top(var)){
            voba_value_t syn_s_name = var->syn_s_name;
            ret = make_var(syn_s_name, VAR_CLOSURE);
            VAR(ret)->u.index = n;
        }else{
            ret = vvar; // direct access top level variable.
        }
    }else{
        // cannot found the variable definition in the current closure
        // try to capture new variable in the parent function context.
        voba_value_t vvar = search_symbol(syn_symbol, fun->parent);
        if( vvar != VOBA_NIL){
            assert(voba_is_a(vvar, voba_cls_var));
            var_t* var =  VAR(vvar);
            // var is in the context of the parent function
            if(var_is_top(var)){ // top level variable.
                ret = vvar;
            }else{
                // here the var is a bridge between parent
                // function and current function.
                // c is the variable in the context of parent function
                // ret is the variable in the context of current function.
                int32_t index = (int32_t)voba_array_len(fun->a_var_C);
                voba_value_t syn_s_name = var->syn_s_name;
                voba_array_push(fun->a_var_C, vvar);
                ret = make_var(syn_s_name, VAR_CLOSURE);
                VAR(ret)->u.index = index;
            }
        }else{
            ret = VOBA_NIL;// cannot find it.
        }
    }
    return ret;
}
void env_push_var(voba_value_t env, voba_value_t var)
{
    assert(voba_is_a(env,voba_cls_env));
    assert(voba_is_a(var,voba_cls_var));
    voba_array_push(ENV(env)->a_var,var);
}
VOBA_FUNC static voba_value_t to_string_env(voba_value_t self,voba_value_t vs);
EXEC_ONCE_PROGN{voba_gf_add_class(voba_symbol_value(s_to_string),voba_cls_env,voba_make_func(to_string_env));}
VOBA_FUNC static voba_value_t to_string_env(voba_value_t self,voba_value_t vs)
{
    voba_value_t v = voba_array_at(vs,0);
    voba_str_t* ret = voba_str_empty();
    env_t* env = ENV(v);
    voba_value_t args[] = {1, env->a_var};
    voba_value_t a = voba_apply(voba_symbol_value(s_to_string), voba_make_array(args));
    args[1] = env->parent;
    voba_value_t parent = voba_apply(voba_symbol_value(s_to_string), voba_make_array(args));
    ret = VOBA_STRCAT(ret,
                      VOBA_CONST_CHAR("<ENV"),
                      VOBA_CONST_CHAR(" vars="), voba_value_to_str(a),
                      VOBA_CONST_CHAR(" parent="), voba_value_to_str(parent),
                      VOBA_CONST_CHAR(">\n"));
    return voba_make_string(ret);
    
}

VOBA_FUNC static voba_value_t to_string_compiler_fun(voba_value_t self,voba_value_t vs);
EXEC_ONCE_PROGN{voba_gf_add_class(voba_symbol_value(s_to_string),voba_cls_compiler_fun,voba_make_func(to_string_compiler_fun));}
VOBA_FUNC static voba_value_t to_string_compiler_fun(voba_value_t self,voba_value_t vs)
{
    voba_value_t v = voba_array_at(vs,0);
    voba_str_t* ret = voba_str_empty();
    compiler_fun_t* fun = COMPILER_FUN(v);
    voba_value_t args[] = {1, fun->a_var_A};
    voba_value_t a = voba_apply(voba_symbol_value(s_to_string), voba_make_array(args));
    args[1] = fun->a_var_C;
    voba_value_t c = voba_apply(voba_symbol_value(s_to_string), voba_make_array(args));
    args[1] = fun->parent;
    voba_value_t parent = voba_apply(voba_symbol_value(s_to_string), voba_make_array(args));
    ret = VOBA_STRCAT(ret,
                      VOBA_CONST_CHAR("<FUN"),
                      VOBA_CONST_CHAR(" args="), voba_value_to_str(a),
                      VOBA_CONST_CHAR(" closure="), voba_value_to_str(c),
                      VOBA_CONST_CHAR(" parent="), voba_value_to_str(parent),
                      VOBA_CONST_CHAR(">\n"));
    return voba_make_string(ret);
    
}

