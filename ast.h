#pragma once

typedef enum ast_type_e {
    CONSTANT,
    ARG,
    SELF,
    CLOSURE,
    FUNCTION,
    TOP,
    MODULE
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

typedef struct ast_s {
    ast_type_t  type;
} ast_t;
#define AST(s) VOBA_USER_DATA_AS(ast_t *,s)

typedef struct ast_function_s {
    ast_type_t  type;
    voba_value_t f_name;
    voba_value_t f_args;
    voba_value_t f_body;
} ast_fun_t;
#define AST(s) VOBA_USER_DATA_AS(ast_t *,s)


voba_value_t compile_module(voba_value_t syn);
voba_value_t compile_build_top_level_list(voba_value_t syns);
