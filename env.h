#pragma once
// compilation environment for looking up a symbol
typedef struct env_s {
    voba_value_t a_var; // local variables or top level variables
    // parent:
    //   env for the enclosing lexical scope
    //   compiler_fun for the enclosing compiler_fun
    //   nil for the top level
    voba_value_t parent; 
}env_t;
#define ENV(s)  VOBA_USER_DATA_AS(env_t *,s)
extern voba_value_t voba_cls_env;
typedef struct compiler_fun_s {
    voba_value_t a_var_A; // arguments: array of var object
    voba_value_t a_var_C; // closure: array of var object
    // parent:
    //   env for the enclosing lexical scope
    //   compiler_fun for the enclosing compiler_fun
    //   nil for the top level
    voba_value_t parent;
    int  is_generator  ;
}compiler_fun_t;
#define COMPILER_FUN(s) VOBA_USER_DATA_AS(compiler_fun_t *,s)
extern voba_value_t voba_cls_compiler_fun;

voba_value_t make_env();
voba_value_t make_compiler_fun();
voba_value_t search_symbol(voba_value_t syn_symbol, voba_value_t env_or_fun);
void env_push_var(voba_value_t env, voba_value_t var);

