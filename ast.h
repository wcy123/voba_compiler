#pragma once
#define AST_TYPES(XX)                           \
XX(SET_TOP)                                     \
XX(CONSTANT)                                    \
XX(FUN)                                    

/*

XX(ARG)                                         \
XX(SELF)                                        \
XX(CLOSURE)                                     \
XX(FUNCTION)                                    \
XX(TOP)                                         \
XX(MODULE)
*/
#define DECLARE_AST_TYPE_ENUM(X) X,
typedef enum ast_type_e {
    AST_TYPES(DECLARE_AST_TYPE_ENUM)
    N_OF_AST_TYPES
} ast_type_t;
/*

ast:

1. CONSTANT
  
   `value` is the constant value.

2. (arg n)

   `value` is the index `n`

3. (self n)

   `value` is the index `n`

4. (closure function(ast), a0(ast), a1(ast))

   `value` is the array (f a0 a1 a2 ....)

5. (top n)

   `value` is the index `n`

6. (function ast)

   `value` is the function address.

7. (moduel ast)

   `value` is an array of `top`

 */


typedef struct ast_set_top_s {
    voba_value_t name;  // a symbol
    voba_value_t exprs; // a list of ast
} ast_set_top_t;
typedef struct ast_constant_s {
    voba_value_t value;
} ast_constant_t;
typedef struct ast_fun_s {
    voba_value_t name; // NIL for annonymous fun.
    voba_value_t body; // an array of ast
} ast_fun_t;
typedef struct ast_s {
    //DEFINE_SOURCE_LOCATION
    ast_type_t  type;
    union {
        ast_set_top_t set_top;
        ast_constant_t constant;
        ast_fun_t fun;
    } u;
} ast_t;
#define AST(s) VOBA_USER_DATA_AS(ast_t *,s)
extern voba_value_t voba_cls_ast;
voba_value_t compile_ast(voba_value_t syn,voba_value_t module, int * error);

