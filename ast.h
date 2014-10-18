#pragma once
#include "var.h"
#include "env.h"
#define VOBA_KEYWORDS(XX)                       \
    XX(def)                                     \
    XX(fun)                                     \
    XX(quote)                                   \
    XX(import)                                  \
    XX(if)                                      \
    XX(cond)                                    \
    XX(let)                                     \

typedef struct toplevel_env_s {
    uint32_t n_of_errors;
    uint32_t n_of_warnings;
    voba_value_t keywords; // an array of keywords, e.g. `def`
    voba_value_t module; // container for symbols
    voba_value_t next; // closure to do next after scan the top level names;
    voba_value_t env; // top level env
    voba_value_t a_modules; // imported modules.
    voba_value_t a_asts;
} toplevel_env_t;
#define TOPLEVEL_ENV(s) VOBA_USER_DATA_AS(toplevel_env_t *,s)

#define AST_TYPES(XX)                           \
    XX(SET_VAR)                                 \
    XX(VAR)                                     \
    XX(CONSTANT)                                \
    XX(FUN)                                     \
    XX(APPLY)                                   \
    XX(LET)                                     \

typedef enum ast_type_e {
#define DECLARE_AST_TYPE_ENUM(X) X,
    AST_TYPES(DECLARE_AST_TYPE_ENUM)
    N_OF_AST_TYPES
} ast_type_t;
typedef struct ast_set_var_s {
    var_t* var;  // a symbol
    voba_value_t a_ast_exprs; // a list of ast
} ast_set_var_t;
typedef struct ast_constant_s {
    voba_value_t value;
} ast_constant_t;
typedef struct ast_fun_s {
    voba_value_t syn_s_name; // NIL for annonymous fun.
    compiler_fun_t * f;
    voba_value_t a_ast_exprs; // an array of ast
} ast_fun_t;
typedef struct ast_var_s {
    var_t* var;
} ast_var_t;
typedef struct ast_apply_s {
    voba_value_t a_ast_exprs; // a list of ast
} ast_apply_t;
typedef struct ast_let_s {
    env_t* env;
    voba_value_t a_ast_exprs; // a list of ast
} ast_let_t;
typedef struct ast_s {
    //DEFINE_SOURCE_LOCATION
    ast_type_t  type;
    union {
        ast_set_var_t set_var;
        ast_constant_t constant;
        ast_fun_t fun;
        ast_var_t var;
        ast_apply_t apply;
        ast_let_t let;
    } u;
} ast_t;
#define AST(s) VOBA_USER_DATA_AS(ast_t *,s)
extern voba_value_t voba_cls_ast;
int is_ast(voba_value_t x);
voba_value_t make_ast_apply(voba_value_t value);
voba_value_t make_ast_fun(voba_value_t syn_s_name, compiler_fun_t* f, voba_value_t a_ast_exprs);
voba_value_t make_ast_constant(voba_value_t value);
voba_value_t make_ast_var(voba_value_t var);
voba_value_t make_ast_set_var(voba_value_t var, voba_value_t exprs);
voba_value_t make_ast_let(env_t * p_env, voba_value_t a_ast_exprs);
voba_value_t create_toplevel_env(voba_value_t module);



