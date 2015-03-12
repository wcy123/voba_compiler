#include <string.h>
#include <errno.h>
#define EXEC_ONCE_TU_NAME "voba.compiler.ast"
#include <voba/value.h>
#include <exec_once.h>
#include "parser.syn.h"
#include "ast.h"
#include "read_module_info.h"
#include "keywords.h"
/* #define DECLARE_AST_TYPE_ARRAY(X) #X, */
/* const char * AST_TYPE_NAMES [] = { */
/*     AST_TYPES(DECLARE_AST_TYPE_ARRAY) */
/*     "# of ast type" */
/* }; */

/* #define AST_DECLARE_TYPE_PRINT_DISPATCHER(X)                            \ */
/*     static inline voba_str_t* print_ast_##X(ast_t * ast, int level);    \ */
/*     static voba_str_t* print_ast(ast_t * ast, int level); */

// AST_TYPES(AST_DECLARE_TYPE_PRINT_DISPATCHER);

// voba_cls_ast is defined here.
VOBA_DEF_CLS(sizeof(ast_t),ast);

/* VOBA_FUNC static voba_value_t str_ast(voba_value_t self, voba_value_t args); */
/* EXEC_ONCE_PROGN{ */
/*     voba_gf_add_class(voba_symbol_value(s_str), */
/*                       voba_cls_ast, */
/*                       voba_make_func(str_ast)); */
/* } */
int is_ast(voba_value_t x)
{
    return voba_get_class(x) == voba_cls_ast;
}
/* static inline voba_str_t* indent(int level) */
/* { */
/*     return voba_strcat(voba_str_fmt_int32_t(level,10), */
/*                        voba_str_from_char(' ',level * 4)); */
/* } */
/* static voba_str_t* print_ast_array(voba_value_t asts,int level) */
/* { */
/*     int64_t len = voba_array_len(asts); */
/*     voba_str_t * ret = voba_str_empty(); */
/*     for(int64_t i = 0 ; i < len ; ++i){ */
/*         ret = voba_strcat(ret, print_ast(AST(voba_array_at(asts,i)),level)); */
/*         //ret = voba_strcat(ret, VOBA_CONST_CHAR("\n")); */
/*     } */
/*     return ret; */
/* } */
/* static voba_str_t* print_ast_SET_TOP(ast_t * ast, int level) */
/* { */
/*     voba_value_t args[] = {1 , (SYNTAX(ast->u.set_top.syn_s_name)->v)}; */
/*     voba_value_t ss = voba_apply(voba_symbol_value(s_str), */
/*                                  voba_make_tuple(args)); */
/*     voba_str_t* ret = voba_str_empty(); */
/*     ret = voba_vstrcat */
/*         (ret, */
/*          indent(level), */
/*          VOBA_CONST_CHAR("ast "), */
/*          voba_str_from_cstr(AST_TYPE_NAMES[ast->type]), */
/*          VOBA_CONST_CHAR(" : name = "), */
/*          voba_value_to_str(ss), */
/*          VOBA_CONST_CHAR("\n"), */
/*          NULL); */
/*     voba_value_t exprs = ast->u.set_top.a_ast_exprs; */
/*     ret = voba_strcat(ret, print_ast_array(exprs,level+1)); */
/*     return ret; */
/* } */
/* static voba_str_t* print_ast_CONSTANT(ast_t * ast, int level) */
/* { */
/*     voba_str_t* ret = voba_str_empty(); */
/*     voba_value_t args[] = {1 , ast->u.constant.value}; */
/*     voba_value_t ss = voba_apply(voba_symbol_value(s_str), */
/*                                  voba_make_tuple(args)); */
/*     ret = voba_vstrcat */
/*         (ret, */
/*          indent(level), */
/*          VOBA_CONST_CHAR("ast "), */
/*          voba_str_from_cstr(AST_TYPE_NAMES[ast->type]), */
/*          VOBA_CONST_CHAR(" : value = "), */
/*          voba_value_to_str(ss), */
/*          NULL); */
/*     return ret; */
/* } */
/* static voba_str_t* print_ast_FUN(ast_t * ast, int level) */
/* { */
/*     voba_value_t a_ast_closure = ast->u.fun.a_ast_closure; */
/*     voba_value_t a_ast_exprs = ast->u.fun.a_ast_exprs; */
/*     voba_value_t args[] = {1 , (SYNTAX(ast->u.fun.syn_s_name)->v)}; */
/*     voba_value_t ss = voba_apply(voba_symbol_value(s_str), */
/*                                  voba_make_tuple(args)); */
/*     voba_str_t* ret = voba_str_empty(); */
/*     return voba_vstrcat */
/*         (ret, */
/*          indent(level), */
/*          VOBA_CONST_CHAR("ast "), */
/*          voba_str_from_cstr(AST_TYPE_NAMES[ast->type]), */
/*          VOBA_CONST_CHAR(" : name = "), */
/*          voba_value_to_str(ss), */
/*          VOBA_CONST_CHAR("\n"),  */
/*          indent(level), */
/*          (voba_array_len(a_ast_closure) > 1)? */
/*          VOBA_STRCAT( */
/*              VOBA_CONST_CHAR("fun closure = \n"), */
/*              print_ast_array(a_ast_closure,level+1)): VOBA_CONST_CHAR("no closure\n"), */
/*          indent(level), */
/*          VOBA_CONST_CHAR("fun body =\n"), */
/*          print_ast_array(a_ast_exprs,level+1), */
/*          VOBA_CONST_CHAR("\n"), */
/*          NULL); */
/* } */
/* static voba_str_t* print_ast_ARG(ast_t * ast, int level) */
/* { */
/*     voba_str_t* ret = voba_str_empty(); */
/*     voba_value_t args[] = {1 , ast->u.arg.syn_s_name }; */
/*     voba_value_t ss = voba_apply(voba_symbol_value(s_str), */
/*                                  voba_make_tuple(args)); */
/*     ret = voba_vstrcat */
/*         (ret, */
/*          indent(level), */
/*          VOBA_CONST_CHAR("ast "), */
/*          voba_str_from_cstr(AST_TYPE_NAMES[ast->type]), */
/*          VOBA_CONST_CHAR(": index = " ), */
/*          voba_str_fmt_uint32_t(ast->u.arg.index,10), */
/*          VOBA_CONST_CHAR(" , name = "), */
/*          voba_value_to_str(ss), */
/*          NULL); */
/*     return ret; */
/* } */
/* static voba_str_t* print_ast_CLOSURE_VAR(ast_t * ast, int level) */
/* { */
/*     return print_ast_ARG(ast,level); */
/* } */
/* static voba_str_t* TOP_VAR_FLAG(enum ast_top_var_flag flag) */
/* { */
/*     voba_str_t * ret = NULL; */
/*     switch(flag){ */
/*     case LOCAL_VAR: */
/*         ret = VOBA_CONST_CHAR("LOCAL"); break; */
/*     case MODULE_VAR: */
/*         ret = VOBA_CONST_CHAR("MODULE_VAR"); break; */
/*     case FOREIGN_VAR: */
/*         ret = VOBA_CONST_CHAR("FOREIGN_VAR"); break; */
/*     } */
/*     return ret; */
/* } */
/* static voba_str_t* TOP_VAR_module_id(voba_value_t id) */
/* { */
/*     voba_str_t * ret = NULL; */
/*     if(voba_is_nil(id)){ */
/*         ret = VOBA_CONST_CHAR("<local>"); */
/*     }else{ */
/*         ret = voba_value_to_str(id); */
/*     } */
/*     return ret; */
/* } */
/* static voba_str_t* print_ast_TOP_VAR(ast_t * ast, int level) */
/* { */
/*     voba_str_t* ret = voba_str_empty(); */
/*     voba_value_t args[] = {1 , ast->u.top_var.syn_s_name }; */
/*     voba_value_t ss = voba_apply(voba_symbol_value(s_str), */
/*                                  voba_make_tuple(args)); */
/*     ret = voba_vstrcat */
/*         (ret, */
/*          indent(level), */
/*          VOBA_CONST_CHAR("ast "), */
/*          voba_str_from_cstr(AST_TYPE_NAMES[ast->type]), */
/*          VOBA_CONST_CHAR(" : name = "), */
/*          voba_value_to_str(ss), */
/*          VOBA_CONST_CHAR(" flag =  "), */
/*          TOP_VAR_FLAG(ast->u.top_var.flag), */
/*          VOBA_CONST_CHAR(" module id =  "), */
/*          TOP_VAR_module_id(ast->u.top_var.module_id), */
/*          VOBA_CONST_CHAR("\n"), */
/*          NULL); */
/*     return ret; */
/* } */
/* static voba_str_t* print_ast_APPLY(ast_t * ast, int level) */
/* { */
/*     voba_str_t* ret = voba_str_empty(); */
/*     ret = voba_vstrcat */
/*         (ret, */
/*          indent(level), */
/*          VOBA_CONST_CHAR("ast "), */
/*          voba_str_from_cstr(AST_TYPE_NAMES[ast->type]), */
/*          VOBA_CONST_CHAR(" : "), */
/*          VOBA_CONST_CHAR("\n"), */
/*          NULL); */
/*     voba_value_t exprs = ast->u.apply.a_ast_exprs; */
/*     ret = voba_strcat(ret, print_ast_array(exprs,level+1)); */
/*     return ret; */
/* } */
/* static voba_str_t* print_ast(ast_t * ast, int level) */
/* { */
/* #define AST_TYPE_PRINT_DISPATCHER(X)            \ */
/*     case X:                                     \ */
/*         return print_ast_##X(ast,level); */
/*     switch(ast->type){ */
/*         AST_TYPES(AST_TYPE_PRINT_DISPATCHER) */
/*             ; */
/*     case N_OF_AST_TYPES: */
/*         return voba_str_empty(); */
/*     } */
/*     assert(0 && "never goes here"); */
/*     return NULL; */
/* } */
/* VOBA_FUNC static voba_value_t str_ast(voba_value_t self, voba_value_t args) */
/* { */
/*     VOBA_DEF_ARG(ast, args, 0, is_ast); */
/*     voba_str_t * x = voba_str_empty(); */
/*     x = voba_vstrcat */
/*         (x, */
/*          VOBA_CONST_CHAR("\n"), */
/*          print_ast(AST(ast),0), */
/*          NULL); */
/*     return voba_make_string(x); */
/* } */

static
VOBA_DEF_CLS(sizeof(toplevel_env_t),toplevel_env);
voba_value_t create_toplevel_env(voba_value_t module)
{
    voba_value_t r = voba_make_user_data(voba_cls_toplevel_env);
    TOPLEVEL_ENV(r)->n_of_errors = 0;
    TOPLEVEL_ENV(r)->n_of_warnings = 0;
    TOPLEVEL_ENV(r)->file_dirname = VOBA_NIL;
    TOPLEVEL_ENV(r)->module = module;
    TOPLEVEL_ENV(r)->keywords = voba_make_array_0();
    TOPLEVEL_ENV(r)->env = VOBA_NIL; //make_env();
    TOPLEVEL_ENV(r)->a_modules = voba_make_array_0();
    TOPLEVEL_ENV(r)->next = voba_make_array_0();
    TOPLEVEL_ENV(r)->a_asts = VOBA_NIL;
#   define VOBA_DEFINE_KEYWORD(key) voba_array_push(TOPLEVEL_ENV(r)->keywords, VOBA_SYMBOL(key,module));
    VOBA_KEYWORDS(VOBA_DEFINE_KEYWORD);
#   define VOBA_DEFINE_COLON_KEYWORD(key) voba_array_push(TOPLEVEL_ENV(r)->keywords, voba_make_symbol_cstr(":" #key,module));
    VOBA_COLON_KEYWORDS(VOBA_DEFINE_COLON_KEYWORD);
    voba_array_push(TOPLEVEL_ENV(r)->keywords, voba_make_symbol_cstr("|",module)); /* vbar */
    return r;
}
voba_value_t make_ast_fun(voba_value_t syn_s_name, compiler_fun_t* f, voba_value_t a_ast_exprs)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast);
    AST(r)->type = FUN;
    AST(r)->u.fun.syn_s_name = syn_s_name;
    AST(r)->u.fun.f = f;
    AST(r)->u.fun.a_ast_exprs = a_ast_exprs;
    return r;
}   
voba_value_t make_ast_constant(voba_value_t syn_value)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast);
    AST(r)->type = CONSTANT;
    AST(r)->u.constant.syn_value = syn_value;
    return r;
}
voba_value_t make_ast_var(voba_value_t var)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast);
    AST(r)->type = VAR;
    AST(r)->u.var.var = VAR(var);
    return r;
}
voba_value_t make_ast_set_var(voba_value_t var, voba_value_t a_ast_exprs)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast);
    assert(voba_is_a(var,voba_cls_var));
    AST(r)->type = SET_VAR;
    AST(r)->u.set_var.var = VAR(var);
    AST(r)->u.set_var.a_ast_exprs = a_ast_exprs;
    return r;
}   
voba_value_t make_ast_apply(voba_value_t value)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast);
    AST(r)->type = APPLY;
    AST(r)->u.apply.a_ast_exprs = value;
    return r;
}
voba_value_t make_ast_let(env_t * p_env, voba_value_t a_ast_exprs)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast);
    AST(r)->type = LET;
    AST(r)->u.let.env = p_env;    
    AST(r)->u.let.a_ast_exprs = a_ast_exprs;
    return r;
}
voba_value_t make_ast_match(voba_value_t ast_value, voba_value_t match)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast);
    AST(r)->type = MATCH;
    AST(r)->u.match.ast_value = ast_value;
    AST(r)->u.match.match = match;
    return r;
}
voba_value_t make_ast_for()
{
    voba_value_t r = voba_make_user_data(voba_cls_ast);
    AST(r)->type = FOR;
    AST(r)->u._for.each = VOBA_NIL;      /* :each, invoke repeatly until return VOBA_UNDEF */
    AST(r)->u._for.match = VOBA_NIL;     /* :do, for body */
    AST(r)->u._for._if = VOBA_NIL ;      /* :if */
    AST(r)->u._for.accumulate = VOBA_NIL;/* :accumulator */
    AST(r)->u._for.init = VOBA_NIL;      /* :init */
    return r;

}
voba_value_t make_ast_it(voba_value_t syn_it)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast);
    AST(r)->type = IT;
    AST(r)->u.it.syn_it = syn_it;
    return r;
}
voba_value_t make_ast_break(voba_value_t ast_value, voba_value_t syn_break)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast);
    AST(r)->type = BREAK;
    AST(r)->u._break.ast_value = ast_value;
    AST(r)->u._break.syn_break = syn_break;
    return r;
}
voba_value_t make_ast_and(voba_value_t a_ast_exprs)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast);
    AST(r)->type = AND;
    AST(r)->u.and.a_ast_exprs = a_ast_exprs;
    return r;
}
voba_value_t make_ast_or(voba_value_t a_ast_exprs)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast);
    AST(r)->type = OR;
    AST(r)->u.or.a_ast_exprs = a_ast_exprs;
    return r;
}
voba_value_t make_ast_yield(voba_value_t ast_expr)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast);
    AST(r)->type = YIELD;
    AST(r)->u.yield.ast_expr = ast_expr;
    return r;
}
