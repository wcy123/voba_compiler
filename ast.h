#pragma once
#define AST_TYPES(XX)                           \
XX(SET_TOP)                                     \
XX(CONSTANT)                                    \
XX(FUN)                                         \
XX(ARG)                                         \
XX(CLOSURE_VAR)                                 \
XX(TOP_VAR)                                     \

#define DECLARE_AST_TYPE_ENUM(X) X,
typedef enum ast_type_e {
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
} ast_fun_t;
typedef struct ast_arg_s {
    voba_value_t syn_s_name;
    uint32_t     index;
}ast_arg_t;
typedef struct ast_closure_var_s {
    voba_value_t syn_s_name;
    uint32_t     index;
} ast_closure_var_t;
typedef struct ast_top_var_s {
    voba_value_t syn_s_name;
} ast_top_var_t;
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
    } u;
} ast_t;
#define AST(s) VOBA_USER_DATA_AS(ast_t *,s)
extern voba_value_t voba_cls_ast;
voba_value_t compile_ast(voba_value_t syn,voba_value_t module, int * error);

