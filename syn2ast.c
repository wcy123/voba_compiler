#include <string.h>
#include <errno.h>
#include <voba/include/value.h>
#include <exec_once.h>
#include <voba/core/builtin.h>
#include "parser.syn.h"
#include "ast.h"
#include "module_info.h"
#include "read_module_info.h"
static inline int is_fun(voba_value_t x);
static inline voba_value_t fun_name(voba_value_t fun);
static inline voba_value_t fun_args(voba_value_t fun);
static inline voba_value_t fun_body(voba_value_t fun);
static inline voba_value_t fun_parent(voba_value_t fun);
static inline voba_value_t fun_capture(voba_value_t fun);
static inline voba_value_t search_fun_args(voba_value_t syn_symbol, voba_value_t fun);
static inline voba_value_t search_fun_closure(voba_value_t syn_symbol, voba_value_t fun);
static inline voba_value_t search_fun_in_parent(voba_value_t syn_symbol, voba_value_t fun);
static inline voba_value_t search_fun(voba_value_t syn_symbol, voba_value_t fun);
VOBA_FUNC static voba_value_t compile_fun(voba_value_t self, voba_value_t args);
// syn_expr is a syntax object for an s-expr
// fun is a function object, which could be NIL, means, top level environment.
// fun is an array of : syn_f, la_syn_args, la_ast_exprs, fun_parent, a_ast_capture
// fun_args, fun_body, fun_parent, and fun_capture return individual objects.
//  syn_f:  the name of the function,
//  la_syn_args:  the argument list of the function
//  la_ast_exprs: the body of the function
//  fun_parent:  another fun object, the parent of the current function, could be NIL.
//  a_ast_capture: closure enviroment of the current function, ast object in the parent object.
//  
static inline voba_value_t compile_expr(voba_value_t syn_expr,voba_value_t fun,voba_value_t toplevel_env);
static inline voba_value_t compile_exprs(voba_value_t la_syn_exprs, voba_value_t fun, voba_value_t toplevel_env);
static int ok(voba_value_t any) {return 1;}
    
#define VOBA_DECLARE_KEYWORD(key) k_##key,
enum voba_keyword_e {
    VOBA_KEYWORDS(VOBA_DECLARE_KEYWORD)
    K_N_OF_KEYWORDS
};
#define K(toplevel_env,key) voba_array_at(TOPLEVEL_ENV(toplevel_env)->keywords,k_##key)

#define LEVEL_ERROR 1
#define LEVEL_WARNING 2

inline static void report(int level, voba_str_t * msg,voba_value_t syn,voba_value_t toplevel_env)
{
    switch(level){
    case LEVEL_ERROR:
        TOPLEVEL_ENV(toplevel_env)->n_of_errors ++ ; break;
    case LEVEL_WARNING:
        TOPLEVEL_ENV(toplevel_env)->n_of_warnings ++ ; break;
    default:
        assert(0);
    }
    uint32_t start_line;
    uint32_t end_line;
    uint32_t start_col;
    uint32_t end_col;
    syn_get_line_column(1,syn,&start_line,&start_col);
    syn_get_line_column(0,syn,&end_line,&end_col);
    voba_str_t* s = voba_str_empty();
    s = voba_vstrcat
        (s,
         voba_value_to_str(syntax_source_filename(syn)),
         VOBA_CONST_CHAR(":"),
         voba_str_fmt_uint32_t(start_line,10),
         VOBA_CONST_CHAR(":"),
         voba_str_fmt_uint32_t(start_col,10),
         VOBA_CONST_CHAR(" - "),
         voba_str_fmt_uint32_t(end_line,10),
         VOBA_CONST_CHAR(":"),
         voba_str_fmt_uint32_t(end_col,10),
         VOBA_CONST_CHAR(" error: "),
         msg,
         //VOBA_CONST_CHAR("\n"),
         NULL);
    fwrite(s->data,s->len,1,stderr);
    fprintf(stderr,"    ");
    voba_str_t* c = voba_value_to_str(syntax_source_content(syn));
    uint32_t pos1 = SYNTAX(syn)->start_pos;
    uint32_t pos2 = SYNTAX(syn)->end_pos;
    uint32_t i = pos1;
    for(i = pos1; i != 0; i--){
        if(c->data[i] == '\n') break;
    }
    if(i==0) {fprintf(stderr,"\n");}
    for(;pos2 < c->len; pos2++){
        if(c->data[pos2] == '\n') break;
    }
    fwrite(c->data + i, pos2 - i, 1, stderr);
    if(pos2 == c->len) fprintf(stderr,"\n");
    fprintf(stderr,"\n");
    for(uint32_t n = i==0?0:1; n + i < pos1; ++n){
        fputs(" ",stderr);
    }
    fprintf(stderr,"^\n");
    return;
}
inline static void report_error(voba_str_t * msg,voba_value_t syn,voba_value_t toplevel_env)
{
    report(LEVEL_ERROR,msg,syn,toplevel_env);
}
inline static void report_warn(voba_str_t * msg,voba_value_t syn,voba_value_t toplevel_env)
{
    report(LEVEL_WARNING,msg,syn,toplevel_env);
}
static inline int is_keyword(voba_value_t toplevel_env, voba_value_t x)
{
    voba_value_t keywords = TOPLEVEL_ENV(toplevel_env)->keywords;
    for(int i = 0; i < K_N_OF_KEYWORDS; ++i){
        if(voba_eq(x,voba_array_at(keywords,i))){
            return 1;
        }
    }
    return 0;
}
static voba_value_t make_syn_nil()
{
    static YYLTYPE s = { 0,0};
    static voba_value_t si = VOBA_NIL;
    static voba_value_t ret = VOBA_NIL;
    if(voba_is_nil(si)){
        si = make_source_info(
            voba_make_string(VOBA_CONST_CHAR(__FILE__)),
            voba_make_string(VOBA_CONST_CHAR("nil")));
        ret = make_syntax(VOBA_NIL,&s);
        attach_source_info(ret,si);
    }
    return ret;
}

static inline voba_value_t search_fun_args(voba_value_t syn_symbol, voba_value_t fun)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t la_syn_args = voba_la_copy(fun_args(fun));
    voba_value_t symbol = SYNTAX(syn_symbol)->v;
    int n = 0;
    while(!voba_la_is_nil(la_syn_args)){
        voba_value_t syn_arg = voba_la_car(la_syn_args);
        if(voba_eq(SYNTAX(syn_arg)->v, symbol)){
            ret = make_ast_arg(syn_symbol,n);
            break;
        }
        ++n;
        la_syn_args = voba_la_cdr(la_syn_args);
    }
    return ret;
}
static inline voba_value_t search_fun_closure(voba_value_t syn_symbol, voba_value_t fun)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t a_ast_capture = fun_capture(fun);
    voba_value_t symbol = SYNTAX(syn_symbol)->v;
    int64_t n = 0;
    int64_t len = voba_array_len(a_ast_capture);
    for(n = 0; n < len; ++n){
        voba_value_t ast_closure_var = voba_array_at(a_ast_capture,n);
        voba_value_t syn_s_name = AST(ast_closure_var)->u.closure_var.syn_s_name;
        if(voba_eq(SYNTAX(syn_s_name)->v, symbol)){
            ret = make_ast_closure_var(syn_symbol,n);
            break;
        }
    }
    return ret;
}
static inline voba_value_t search_fun_in_parent(voba_value_t syn_symbol, voba_value_t fun)
{
    voba_value_t parent = fun_parent(fun);
    voba_value_t a_ast_capture = fun_capture(fun);
    voba_value_t ast_in_parent =  search_fun(syn_symbol, parent);
    int64_t len = voba_array_len(a_ast_capture);
    voba_value_t ret = VOBA_NIL;
    if(ast_in_parent){
        voba_array_push(a_ast_capture, ast_in_parent);
        ret = make_ast_closure_var(syn_symbol,len);
    }
    return ret;
}
static inline voba_value_t search_fun(voba_value_t syn_symbol, voba_value_t fun)
{
    voba_value_t ret = VOBA_NIL;
    if(!voba_is_nil(fun)){
        assert(voba_is_array(fun));
        ret = search_fun_args(syn_symbol,fun);
        if(voba_is_nil(ret)){
            ret = search_fun_closure(syn_symbol,fun);
        }
        if(voba_is_nil(ret)){
            // with side effect
            ret = search_fun_in_parent(syn_symbol,fun);
        }
    }
    return ret;
}
static inline voba_value_t search_in_toplevel_env(voba_value_t symbol, voba_value_t toplevel_env)
{
    assert(voba_get_class(symbol) == voba_cls_symbol);
    int64_t n = 0;
    voba_value_t a_ast_toplevel_vars = TOPLEVEL_ENV(toplevel_env)->a_ast_toplevel_vars;
    int64_t len = voba_array_len(a_ast_toplevel_vars);
    voba_value_t ret = VOBA_NIL;
    for(n = 0; n < len; ++n){
        voba_value_t ast_name = voba_array_at(a_ast_toplevel_vars,n);
        voba_value_t syn_s_name = AST(ast_name)->u.top_var.syn_s_name;
        voba_value_t s_name = SYNTAX(syn_s_name)->v;
        if(voba_eq(s_name,symbol)){
            ret = ast_name;break;
        }
    }
    return ret;
}
// for import, try to find the var, 
//     no var existed, it is a foreign var.
//     if a local var, var becomes a module var
//     if a module var, throw an error, name conflicts
//     if a foreign var, throw an error, name conficts again.
static inline void create_topleve_var_for_import(voba_value_t syn_symbol, voba_value_t module_id, voba_value_t toplevel_env)
{

    voba_value_t symbol = SYNTAX(syn_symbol)->v;
    voba_value_t top_var = search_in_toplevel_env(symbol,toplevel_env);
    if(voba_is_nil(top_var)){
        voba_value_t top_name = make_ast_top_var(syn_symbol,module_id,FOREIGN_VAR);
        voba_array_push(TOPLEVEL_ENV(toplevel_env)->a_ast_toplevel_vars,top_name);
    }else{
        assert(is_ast(top_var));
        assert(AST(top_var)->type == TOP_VAR);
        switch(AST(top_var)->u.top_var.flag){
        case LOCAL_VAR:
            AST(top_var)->u.top_var.flag = MODULE_VAR;
            break;
        case MODULE_VAR:
        case FOREIGN_VAR:
            report_error(
                VOBA_STRCAT(VOBA_CONST_CHAR("import name conflicts. symbol = "),
                            voba_symbol_name(symbol)),
                // TODO: report previous definitions position.
                syn_symbol,
                toplevel_env
                );
            break;
        default:
            assert(0);
        }
    }
    return;
}
// 
// for def, try to find the var
//     no var existed, it is a local var.
//     if a local var, warning, redefined a local var.
//     if a module var, warning, redefined a module var.
//     if a foreign var, var becomes a module var
static inline void create_topleve_var_for_def(voba_value_t syn_symbol, voba_value_t toplevel_env)
{
    voba_value_t symbol = SYNTAX(syn_symbol)->v;
    voba_value_t top_var = search_in_toplevel_env(symbol,toplevel_env);
    if(voba_is_nil(top_var)){
        /* fprintf(stderr,__FILE__ ":%d:[%s] debug cannot found symbol %s\n ", __LINE__, __FUNCTION__, */
        /*         voba_str_to_cstr(voba_value_to_str(voba_symbol_name(symbol)))); */
        voba_value_t top_name = make_ast_top_var(syn_symbol,VOBA_NIL,LOCAL_VAR);
        voba_array_push(TOPLEVEL_ENV(toplevel_env)->a_ast_toplevel_vars,top_name);
    }else{
        assert(is_ast(top_var));
        assert(AST(top_var)->type == TOP_VAR);
        switch(AST(top_var)->u.top_var.flag){
        case LOCAL_VAR:
        case MODULE_VAR:
            report_error(
                VOBA_STRCAT(VOBA_CONST_CHAR("redefine a symbol. symbol = "),
                            voba_value_to_str(voba_symbol_name(symbol))),
                // TODO: report previous definitions position.
                syn_symbol,
                toplevel_env
                );
            break;
        case FOREIGN_VAR:
            AST(top_var)->u.top_var.flag = MODULE_VAR;
            fprintf(stderr,__FILE__ ":%d:[%s] %s %s\n", __LINE__, __FUNCTION__,
                    voba_str_to_cstr(voba_value_to_str(voba_symbol_name(symbol))),
                    voba_is_nil(AST(top_var)->u.top_var.module_id)?"NIL":voba_str_to_cstr(voba_value_to_str(AST(top_var)->u.top_var.module_id)));
            break;
        default:
            assert(0);
        }
    }
    return;
}
static inline voba_value_t compile_symbol(
    voba_value_t syn_symbol, voba_value_t fun, voba_value_t toplevel_env
    )
{
    voba_value_t ret = search_fun(syn_symbol,fun);
    if(voba_is_nil(ret)){
        voba_value_t top_var = search_in_toplevel_env(SYNTAX(syn_symbol)->v,toplevel_env);
        if(!voba_is_nil(top_var)){
            ret = make_ast_top_var(syn_symbol,AST(top_var)->u.top_var.module_id,AST(top_var)->u.top_var.flag);
        }
    }
    if(voba_is_nil(ret)){
        report_error(VOBA_CONST_CHAR("cannot find symbol definition"),syn_symbol,toplevel_env);
    }
    return ret;
}
static inline voba_value_t compile_array(voba_value_t syn_form,voba_value_t fun,voba_value_t toplevel_env)
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
                switch(len){
                case 1:
                    report_error(VOBA_CONST_CHAR("illegal form. bare fun is not fun"),syn_f,toplevel_env);
                    break;
                case 2:
                    report_error(VOBA_CONST_CHAR("illegal form. empty body is not fun"),syn_f,toplevel_env);
                    break;
                default: 
                {
                    voba_value_t syn_fun_args = voba_array_at(form,1);
                    voba_value_t fun_args = SYNTAX(syn_fun_args)->v;
                    if(voba_is_array(fun_args)){
                        voba_value_t la_syn_body = voba_la_from_array1(form,2);
                        voba_value_t self[] = {5, make_syn_nil(), // no name
                                               voba_la_from_array0(fun_args),
                                               la_syn_body,
                                               fun,
                                               voba_make_array_0() };
                        voba_value_t args[] = {1,toplevel_env};
                        voba_value_t ast_expr = compile_fun(voba_make_array(self),
                                                            voba_make_array(args));
                        ret = ast_expr;
                    }else{
                        report_error(VOBA_CONST_CHAR("illegal form. argument list is not a list"),syn_fun_args,toplevel_env);
                    }
                }
                }
            }else{
                goto label_default;
            }
        }else{
        label_default:
            ;
            voba_value_t ast_form = compile_exprs(voba_la_from_array0(form),fun,toplevel_env);
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
static inline voba_value_t compile_expr(voba_value_t syn_expr,voba_value_t fun,voba_value_t toplevel_env)
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
        ret = compile_symbol(syn_expr,fun,toplevel_env);
    }
    else if(cls == voba_cls_array){
        ret = compile_array(syn_expr,fun,toplevel_env);
    }else{
        report_error(VOBA_CONST_CHAR("invalid expression"),syn_expr,toplevel_env);
    }
    return ret;
}
    
// return an array of ast
static inline voba_value_t compile_exprs(voba_value_t la_syn_exprs,
                                         voba_value_t fun,
                                         voba_value_t toplevel_env)
{
    // exprs: (....)
    //        ^la_syn_exprs
    voba_value_t cur = voba_la_copy(la_syn_exprs);
    voba_value_t ret = voba_make_array_0();
    if(!voba_la_is_nil(cur)){
        while(!voba_la_is_nil(cur)){
            voba_value_t syn_exprs = voba_la_car(cur);
            voba_value_t ast_expr = compile_expr(syn_exprs,fun,toplevel_env);
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

VOBA_FUNC
static voba_value_t compile_top_expr_def_name_next(voba_value_t self, voba_value_t args)
{
    //  (def name ...)
    //       ^syn_name
    //            ^la_syn_exprs
    VOBA_DEF_CVAR(syn_name,self,0);
    VOBA_DEF_CVAR(la_syn_exprs,self,1);
    VOBA_DEF_ARG(toplevel_env, args, 0, ok);
    if(0)fprintf(stderr,__FILE__ ":%d:[%s] \n", __LINE__, __FUNCTION__);
    voba_value_t exprs = compile_exprs(la_syn_exprs,VOBA_NIL,toplevel_env);
    voba_value_t ret = VOBA_NIL;
    if(!voba_is_nil(exprs)){
        ret = make_ast_set_top(syn_name, exprs);
    }
    if(0) fprintf(stderr,__FILE__ ":%d:[%s] v = 0x%lx type = 0x%lx\n", __LINE__, __FUNCTION__,
            ret, voba_get_class(ret));
    return ret;

}
static inline void compile_top_expr_def_name(voba_value_t syn_name, voba_value_t la_syn_exprs,voba_value_t toplevel_env)
{
    //  (def name ...)
    //       ^^^^^^^^------------------  la_syn_exprs
    //       ^^^ ----------------------  syn_name
    voba_value_t closure = voba_make_closure_2
        (compile_top_expr_def_name_next,syn_name,la_syn_exprs);
    voba_array_push(TOPLEVEL_ENV(toplevel_env)->next, closure);
    create_topleve_var_for_def(syn_name,toplevel_env);
    return;
}
VOBA_FUNC static voba_value_t compile_fun(voba_value_t self, voba_value_t args)
{
    VOBA_DEF_CVAR(syn_fname,self,0);
    //VOBA_DEF_CVAR(la_syn_args,self,1);
    VOBA_DEF_CVAR(la_ast_exprs,self,2);
    /* VOBA_DEF_CVAR(parent,self,3); */
    VOBA_DEF_CVAR(a_ast_capture,self,4);
    VOBA_DEF_ARG(toplevel_env, args, 0, ok);
    return make_ast_fun
        (syn_fname, compile_exprs(la_ast_exprs,self,toplevel_env), a_ast_capture);
}
static inline voba_value_t make_fun(
    voba_value_t syn_f,
    voba_value_t la_syn_args,
    voba_value_t la_ast_exprs,
    voba_value_t fun_parent,
    voba_value_t a_ast_capture
    )
{
    return voba_make_closure_5
        (compile_fun, syn_f, la_syn_args, la_ast_exprs, fun_parent, a_ast_capture);
}
__attribute__((used))
static inline int is_fun(voba_value_t x)
{
    return voba_is_closure(x) && voba_closure_func(x) == compile_fun;
}
__attribute__((used))
static inline voba_value_t fun_name(voba_value_t fun)
{
    assert(voba_is_array(fun));
    return voba_array_at(fun,0);
}
static inline voba_value_t fun_args(voba_value_t fun)
{
    assert(voba_is_array(fun));
    return voba_array_at(fun,1);
}
__attribute__((used))
static inline voba_value_t fun_body(voba_value_t fun)
{
    assert(voba_is_array(fun));
    return voba_array_at(fun,2);
}
static inline voba_value_t fun_parent(voba_value_t fun)
{
    assert(voba_is_array(fun));
    return voba_array_at(fun,3);
}
static inline voba_value_t fun_capture(voba_value_t fun)
{
    assert(voba_is_array(fun));
    return voba_array_at(fun,4);
}
VOBA_FUNC
static voba_value_t compile_top_expr_def_fun_next(voba_value_t self, voba_value_t args) 
{
    VOBA_DEF_CVAR(fun,self,0);
    VOBA_DEF_ARG(toplevel_env, args, 0, ok);
    voba_value_t syn_f = fun_name(voba_closure_array(fun));
    voba_value_t ast_expr = voba_closure_func(fun)(voba_closure_array(fun), args);
    return make_ast_set_top(syn_f, voba_make_array_1(ast_expr));
}
static inline void compile_top_expr_def_fun(
    voba_value_t syn_def,
    voba_value_t syn_var_form,
    voba_value_t la_ast_exprs_form,
    voba_value_t toplevel_env)
{
    //  (def ( ... ) ...)
    //   ^------------------------  syn_def
    //       ^^^^^^^--------------- syn_var_form
    //               ^^^^---------  la_ast_exprs_form

    voba_value_t var_form = SYNTAX(syn_var_form)->v;
    voba_value_t la_syn_var_form = voba_la_from_array0(var_form);
    uint32_t len = voba_la_len(la_syn_var_form);
    if(len >= 1){
        voba_value_t syn_f = voba_la_car(la_syn_var_form);
        voba_value_t f = SYNTAX(syn_f)->v;
        if(voba_is_symbol(f)){
            //  (def (f ...) ...)
            //   ^------------------------  syn_def
            //       ^^^^^^^--------------- syn_var_form
            //               ^^^^---------  la_body_form
            create_topleve_var_for_def(syn_f,toplevel_env);
            voba_value_t fun_parent = VOBA_NIL;
            voba_value_t a_ast_capture = voba_make_array_0();
            voba_value_t fun = make_fun
                (syn_f, voba_la_cdr(la_syn_var_form), la_ast_exprs_form, fun_parent, a_ast_capture);
            voba_value_t next = voba_make_closure_1(compile_top_expr_def_fun_next,fun);
            voba_array_push(TOPLEVEL_ENV(toplevel_env)->next,next);
        }else{
            report_error(VOBA_CONST_CHAR("illegal form. function name must be a symbol."),
                         syn_f,toplevel_env);
        }
    }else{
        report_error(VOBA_CONST_CHAR("illegal form. no function name"),
                     syn_var_form,toplevel_env);
    }
    return;
}

static inline void compile_top_expr_def(voba_value_t syn_def, voba_value_t la_syn_top_expr,voba_value_t toplevel_env)
{
    //  (def ...)
    //       ^la_syn_top_expr
    if(!voba_la_is_nil(la_syn_top_expr)){
        voba_value_t syn_var_form = voba_la_car(la_syn_top_expr);
        voba_value_t var_form = SYNTAX(syn_var_form)->v;
        if(voba_is_symbol(var_form)){
            //  (def var ...)
            //   ^^^---------------------------  syn_def
            //       ^^^^^^^-------------------  la_syn_top_expr
            //       ^^^ ----------------------  syn_var_form
            if(!is_keyword(toplevel_env,var_form)){
                compile_top_expr_def_name(syn_var_form, voba_la_cdr(la_syn_top_expr),toplevel_env);
            }else{
                report_error(VOBA_CONST_CHAR("redefine keyword")
                             ,var_form
                             ,toplevel_env);
            }
        }else if(voba_is_array(var_form)){
            //  (def ( ... ) ...)
            //   ^------------------------------  syn_def
            //       ^^^^^^^^^^^----------------- la_syn_top_expr
            //       ^^^^^^ --------------------  syn_var_form
            compile_top_expr_def_fun(syn_def,
                                     syn_var_form,
                                     voba_la_cdr(la_syn_top_expr),
                                     toplevel_env);
        }else{
            report_error(VOBA_CONST_CHAR("(def x), x must be a symbol or list")
                         ,syn_var_form
                         ,toplevel_env);
        }
    }else{
        report_error(VOBA_CONST_CHAR("bare def"),syn_def,toplevel_env);
    }
    return;
}
static voba_value_t voba_include_path = VOBA_NIL;
EXEC_ONCE_PROGN{
    voba_include_path = voba_make_array_0();
}
static inline voba_value_t search_module_header_file(voba_value_t module_name)
{
    return voba_make_string(VOBA_CONST_CHAR("/home/chunywan/d/working/voba2/builtin/builtin.h"));
}
static inline voba_value_t read_module_header_file(voba_value_t header_file,voba_value_t syn_v,voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    FILE * fp = fopen(voba_str_to_cstr(voba_value_to_str(header_file)),"r");
    if(fp != NULL){
        voba_str_t * ss = voba_str_from_FILE(fp);
        if(ss != NULL){
            ret = voba_make_string(ss);
        }else{
            report_error(VOBA_STRCAT(VOBA_CONST_CHAR("cannot read module header file. filename = "),
                                     voba_value_to_str(header_file),
                                     VOBA_CONST_CHAR("; errro = "),
                                     voba_strdup(voba_str_from_cstr(strerror(errno))))
                         ,syn_v
                         ,toplevel_env);
        }
    }else{
        report_error(VOBA_STRCAT(VOBA_CONST_CHAR("cannot open module header file. filename = "),
                                 voba_value_to_str(header_file),
                                 VOBA_CONST_CHAR("; errro = "),
                                 voba_strdup(voba_str_from_cstr(strerror(errno))))
                     ,syn_v
                     ,toplevel_env);
    }
    return ret;
}
static inline void compile_top_expr_import_module_info(voba_value_t info,
                                                       voba_value_t filename,
                                                       voba_value_t content,
                                                       voba_value_t syn_module_name,voba_value_t toplevel_env)
{
    struct YYLTYPE yyloc = {1,1};
    voba_value_t symbol_names = module_info_symbols(info);
    voba_value_t module_id = module_info_id(info);
    int64_t len = voba_array_len(symbol_names);
    voba_array_push(TOPLEVEL_ENV(toplevel_env)->a_modules,info);
    for(int64_t i = 0; i < len ; i ++){
        voba_value_t symbol_name = voba_array_at(symbol_names,i);
        voba_value_t symbol = voba_make_symbol(voba_value_to_str(symbol_name),
                                               TOPLEVEL_ENV(toplevel_env)->module);
        voba_value_t syn_module_var_name = make_syntax(symbol,&yyloc);
        attach_source_info(syn_module_var_name,make_source_info(filename,content));
        create_topleve_var_for_import(syn_module_var_name,module_id,toplevel_env);
    }
}
static inline void compile_top_expr_import_name(voba_value_t syn_import, voba_value_t syn_module_name, voba_value_t la_syn_top_expr,voba_value_t toplevel_env)
{
    voba_value_t module_name = SYNTAX(syn_module_name)->v;
    voba_value_t module_header_file = search_module_header_file(module_name);
    if(!voba_is_nil(module_header_file)){
        voba_value_t module_header_content = read_module_header_file(module_header_file,syn_module_name,toplevel_env);
        if(!voba_is_nil(module_header_content)) {
            voba_value_t module_info = read_module_info(module_header_content);
            if(is_module_info(module_info)){
                compile_top_expr_import_module_info(module_info,module_header_file, module_header_content, syn_module_name,toplevel_env);
            }else{
                report_error(VOBA_STRCAT(
                                 VOBA_CONST_CHAR("invalid module info, error = "),
                                 voba_value_to_str(module_info),
                                 VOBA_CONST_CHAR("source = "),
                                 voba_value_to_str(module_header_content)),
                             syn_module_name,
                             toplevel_env);
            }
        }else{
            // do nothing, error is already reported.
        }
    }else{
        // TODO: report the search list.
        report_error(VOBA_STRCAT(VOBA_CONST_CHAR("cannot find module header file. module name = "),
                                 voba_value_to_str(module_name)),
                     syn_module_name,toplevel_env);
    }
}
static inline void compile_top_expr_import(voba_value_t syn_import, voba_value_t la_syn_top_expr,voba_value_t toplevel_env)
{
    //  (import ...)
    //          ^la_syn_top_expr
    if(!voba_la_is_nil(la_syn_top_expr)){
        voba_value_t syn_module_name = voba_la_car(la_syn_top_expr);
        voba_value_t module_name = SYNTAX(syn_module_name)->v;
        if(voba_is_symbol(module_name)){
            //  (import name ...)
            //   ^^^---------------------------  syn_import
            //          ^^^^-------------------  la_syn_top_expr
            //          ^^^ ----------------------  syn_module_name
            compile_top_expr_import_name(syn_import,syn_module_name,la_syn_top_expr,toplevel_env);
        }else{
            report_error(VOBA_CONST_CHAR("(import x), x must be a symbol")
                         ,syn_module_name
                         ,toplevel_env);
        }
    }else{
        report_error(VOBA_CONST_CHAR("bare import"),syn_import,toplevel_env);
    }
    return;
}
VOBA_FUNC static voba_value_t compile_top_expr_any_next(voba_value_t self, voba_value_t args)
{
    VOBA_DEF_CVAR(syn_top_expr,self,0);
    VOBA_DEF_ARG(toplevel_env, args, 0, ok);
    voba_value_t fun = VOBA_NIL;
    return compile_expr(syn_top_expr, fun, toplevel_env);
}
static inline void compile_top_expr_any(voba_value_t syn_top_expr, voba_value_t toplevel_env)
{
    voba_value_t closure = voba_make_closure_2
        (compile_top_expr_any_next, syn_top_expr, toplevel_env);
    voba_array_push(TOPLEVEL_ENV(toplevel_env)->next, closure);
}
static inline void compile_top_expr(voba_value_t syn_top_expr, voba_value_t toplevel_env)

{
    voba_value_t top_expr = SYNTAX(syn_top_expr)->v;
    if(voba_is_array(top_expr)){
        int64_t len =  voba_array_len(top_expr);
        if(len > 0){
            voba_value_t cur = voba_la_from_array1(top_expr,0);
            voba_value_t syn_key_word = voba_la_car(cur);
            voba_value_t key_word = SYNTAX(syn_key_word)->v;
            if(voba_eq(key_word, K(toplevel_env,def))){
                compile_top_expr_def(syn_key_word,voba_la_cdr(cur),toplevel_env);
            }else if(voba_eq(key_word, K(toplevel_env,import))){
                compile_top_expr_import(syn_key_word,voba_la_cdr(cur),toplevel_env);
            }else {
                compile_top_expr_any(syn_top_expr, toplevel_env);
            }
        }else{
            report_warn(VOBA_CONST_CHAR("top expr is an empty list"), syn_top_expr,toplevel_env);
        }
    }else{
        report_error(VOBA_CONST_CHAR("unrecognised form"),syn_top_expr,toplevel_env);
    }
    return;
}
static inline void compile_toplevel_exprs(voba_value_t la_syn_top_exprs,
                                          voba_value_t toplevel_env)
{
    voba_value_t cur = la_syn_top_exprs;
    while(!voba_la_is_nil(cur)){
        voba_value_t syn_top_expr = voba_la_car(cur);
        compile_top_expr(syn_top_expr,toplevel_env);
        cur = voba_la_cdr(cur);
    }
    return;
}
// `syn` is the syntax object to be compiled into an ast
// `module` is the symbol table for all symbols.
voba_value_t syn2ast(voba_value_t syn,voba_value_t module, int * error)
{
    voba_value_t asts = VOBA_NIL;
    voba_value_t toplevel_env = create_toplevel_env(module);
    if(!voba_is_array(SYNTAX(syn)->v)){
        report_error(VOBA_CONST_CHAR("expr must be an array."),syn,toplevel_env);
        return VOBA_NIL;
    }
    compile_toplevel_exprs(voba_la_from_array1(SYNTAX(syn)->v,0),toplevel_env);
    voba_value_t next = TOPLEVEL_ENV(toplevel_env)->next;
    int64_t len =  voba_array_len(next);
    asts = voba_make_array_0();
    voba_value_t args [] = {1, toplevel_env};
    for(int64_t i = 0 ; i < len ; ++i) {
        voba_value_t ast = voba_apply(voba_array_at(next,i),voba_make_array(args));
        if(!voba_is_nil(ast)){
            voba_array_push(asts,ast);
        }else{
            ++ *error;
        }
    }
    *error = TOPLEVEL_ENV(toplevel_env)->n_of_errors;
    TOPLEVEL_ENV(toplevel_env)->a_asts = asts;
    return toplevel_env;
}


EXEC_ONCE_START;
