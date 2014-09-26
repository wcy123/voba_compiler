#pragma once
#define AST_TYPES(XX)                           \
    XX(SET_TOP)                                 \
    XX(CONSTANT)                                \
    XX(FUN)                                     \
    XX(ARG)                                     \
    XX(CLOSURE_VAR)                             \
    XX(TOP_VAR)                                 \
    XX(APPLY)                                   \


#define VOBA_KEYWORDS(XX)                       \
    XX(def)                                     \
    XX(fun)                                     \
    XX(quote)                                   \
    XX(import)                                  \
    XX(if)                                      \
    XX(cond)                                    \



typedef struct toplevel_env_s {
    uint32_t n_of_errors;
    uint32_t n_of_warnings;
    voba_value_t keywords; // an array of keywords, e.g. `def`
    voba_value_t module; // container for symbols
    voba_value_t next; // closure to do next after scan the top level names;
    voba_value_t a_ast_toplevel_vars;
    voba_value_t a_modules; // imported modules.
    voba_value_t a_asts;
} toplevel_env_t;
#define TOPLEVEL_ENV(s) VOBA_USER_DATA_AS(toplevel_env_t *,s)

typedef enum ast_type_e {
#define DECLARE_AST_TYPE_ENUM(X) X,
    AST_TYPES(DECLARE_AST_TYPE_ENUM)
    N_OF_AST_TYPES
} ast_type_t;
typedef struct ast_set_top_s {
    voba_value_t syn_s_name;  // a symbol
    voba_value_t a_ast_exprs; // a list of ast
} ast_set_top_t;
typedef struct ast_constant_s {
    voba_value_t value;
} ast_constant_t;
typedef struct ast_fun_s {
    voba_value_t syn_s_name; // NIL for annonymous fun.
    voba_value_t a_ast_exprs; // an array of ast
    voba_value_t a_ast_closure; // an array of ast
} ast_fun_t;
typedef struct ast_arg_s {
    voba_value_t syn_s_name;
    uint32_t     index;
}ast_arg_t;
typedef struct ast_closure_var_s {
    voba_value_t syn_s_name;
    uint32_t     index;
} ast_closure_var_t;
enum ast_top_var_flag {
    LOCAL_VAR, // local variable
    MODULE_VAR, // public, module variable, defined in this module
    FOREIGN_VAR // imported variable from other module, not defined in this module
};
typedef struct ast_top_var_s {
    voba_value_t syn_s_name;
    enum ast_top_var_flag flag;
    voba_value_t module_id;
} ast_top_var_t;
typedef struct ast_apply_s {
    voba_value_t a_ast_exprs; // a list of ast
} ast_apply_t;
typedef struct ast_s {
    //DEFINE_SOURCE_LOCATION
    ast_type_t  type;
    union {
        ast_set_top_t set_top;
        ast_constant_t constant;
        ast_fun_t fun;
        ast_arg_t arg;
        ast_closure_var_t closure_var;
        ast_top_var_t top_var;
        ast_apply_t apply;
    } u;
} ast_t;
#define AST(s) VOBA_USER_DATA_AS(ast_t *,s)
extern voba_value_t voba_cls_ast;
int is_ast(voba_value_t x);
voba_value_t make_ast_apply(voba_value_t value);
voba_value_t make_ast_fun(voba_value_t name, voba_value_t body, voba_value_t closure);
voba_value_t make_ast_constant(voba_value_t value);
voba_value_t make_ast_arg(voba_value_t value,uint32_t n);
voba_value_t make_ast_closure_var(voba_value_t value, uint32_t n);
voba_value_t make_ast_top_var(voba_value_t value,voba_value_t module_id, enum ast_top_var_flag flag);
voba_value_t make_ast_set_top(voba_value_t syn_name /*symbol*/,
                              voba_value_t exprs /* array of ast */);
voba_value_t create_toplevel_env(voba_value_t module);



