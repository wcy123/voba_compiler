#include <voba/include/value.h>
#include <exec_once.h>
#include <voba/core/builtin.h>
#include "parser.syn.h"
#include "ast.h"

static inline int is_ast(voba_value_t x);
static inline int is_fun(voba_value_t x);
static inline voba_value_t fun_name(voba_value_t fun);
static inline voba_value_t fun_args(voba_value_t fun);
static inline voba_value_t fun_body(voba_value_t fun);
static inline voba_value_t fun_parent(voba_value_t fun);
static inline voba_value_t fun_capture(voba_value_t fun);
static inline voba_value_t make_ast_fun(voba_value_t name, voba_value_t body, voba_value_t closure);
static inline voba_value_t make_ast_constant(voba_value_t value);
static inline voba_value_t make_ast_arg(voba_value_t value,uint32_t n);
static inline voba_value_t make_ast_closure_var(voba_value_t value, uint32_t n);
static inline voba_value_t make_ast_top_var(voba_value_t value);
static inline voba_value_t search_fun_args(voba_value_t syn_symbol, voba_value_t fun);
static inline voba_value_t search_fun_closure(voba_value_t syn_symbol, voba_value_t fun);
static inline voba_value_t search_fun_in_parent(voba_value_t syn_symbol, voba_value_t fun);
static inline voba_value_t search_fun(voba_value_t syn_symbol, voba_value_t fun);
VOBA_FUNC static voba_value_t compile_fun(voba_value_t self, voba_value_t args);
static inline voba_value_t compile_exprs(voba_value_t la_syn_exprs, voba_value_t fun, voba_value_t top_level);
static int ok(voba_value_t any) {return 1;}
typedef struct top_level_s {
    uint32_t n_of_errors;
    uint32_t n_of_warnings;
    voba_value_t keywords; // an array of keywords, e.g. `def`
    voba_value_t module; // 
    voba_value_t next; // closure to do next after scan the top level names;
    voba_value_t a_ast_names;
} top_level_t;
#define TOP_LEVEL(s) VOBA_USER_DATA_AS(top_level_t *,s)


#define VOBA_KEYWORDS(XX)                       \
    XX(def)                                     \
    XX(fun)                                     \
    XX(quote)                                   \
    XX(import)                                  \
    XX(if)                                      \
    XX(cond)                                    \
    
#define VOBA_DECLARE_KEYWORD(key) k_##key,
enum voba_keyword_e {
    VOBA_KEYWORDS(VOBA_DECLARE_KEYWORD)
    K_N_OF_KEYWORDS
};
#define K(top_level,key) voba_array_at(TOP_LEVEL(top_level)->keywords,k_##key)

static inline
voba_value_t create_top_level(voba_value_t module)
{
    voba_value_t r = voba_make_user_data(VOBA_NIL, sizeof(top_level_t));
    TOP_LEVEL(r)->n_of_errors = 0;
    TOP_LEVEL(r)->n_of_warnings = 0;
    TOP_LEVEL(r)->module = module;
    TOP_LEVEL(r)->keywords = voba_make_array_0();
    TOP_LEVEL(r)->a_ast_names = voba_make_array_0();
    TOP_LEVEL(r)->next = voba_make_array_0();
#   define VOBA_DEFINE_KEYWORD(key) voba_array_push(TOP_LEVEL(r)->keywords, VOBA_SYMBOL(key,module));
    VOBA_KEYWORDS(VOBA_DEFINE_KEYWORD);
    return r;
}


#define DECLARE_AST_TYPE_ARRAY(X) #X,
const char * AST_TYPE_NAMES [] = {
    AST_TYPES(DECLARE_AST_TYPE_ARRAY)
    "# of ast type"
};

#define AST_DECLARE_TYPE_PRINT_DISPATCHER(X)                            \
    static inline voba_str_t* print_ast_##X(ast_t * ast, int level);    \
    static voba_str_t* print_ast(ast_t * ast, int level);

AST_TYPES(AST_DECLARE_TYPE_PRINT_DISPATCHER);
    


DEFINE_CLS(sizeof(ast_t),ast);
VOBA_FUNC static voba_value_t to_string_ast(voba_value_t self, voba_value_t args);
EXEC_ONCE_PROGN{
    voba_gf_add_class(voba_symbol_value(s_to_string), voba_cls_ast,
                      voba_make_func(to_string_ast));
}

static inline voba_value_t make_ast_set_top(voba_value_t syn_name /*symbol*/,
                                            voba_value_t exprs /* array of ast */)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast,sizeof(ast_t));
    AST(r)->type = SET_TOP;
    AST(r)->u.set_top.syn_s_name = syn_name;
    AST(r)->u.set_top.a_ast_exprs = exprs;
    return r;
}   
static inline voba_value_t make_ast_fun(voba_value_t name, voba_value_t body, voba_value_t closure)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast,sizeof(ast_t));
    AST(r)->type = FUN;
    AST(r)->u.fun.syn_s_name = name;
    AST(r)->u.fun.a_ast_exprs = body;
    AST(r)->u.fun.a_ast_closure = closure;
    return r;
}   
static inline voba_value_t make_ast_constant(voba_value_t value)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast,sizeof(ast_t));
    AST(r)->type = CONSTANT;
    AST(r)->u.constant.value = value;
    return r;
}
static inline voba_value_t make_ast_arg(voba_value_t value,uint32_t n)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast,sizeof(ast_t));
    AST(r)->type = ARG;
    AST(r)->u.arg.syn_s_name = value;
    AST(r)->u.arg.index = n;
    return r;
}
static inline voba_value_t make_ast_closure_var(voba_value_t value, uint32_t n)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast,sizeof(ast_t));
    AST(r)->type = CLOSURE_VAR;
    AST(r)->u.closure_var.syn_s_name = value;
    AST(r)->u.closure_var.index = n;
    return r;
}
static inline voba_value_t make_ast_top_var(voba_value_t value)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast,sizeof(ast_t));
    AST(r)->type = TOP_VAR;
    AST(r)->u.top_var.syn_s_name = value;
    return r;
}
static inline voba_value_t make_ast_apply(voba_value_t value)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast,sizeof(ast_t));
    AST(r)->type = APPLY;
    AST(r)->u.apply.a_ast_exprs = value;
    return r;
}


#define LEVEL_ERROR 1
#define LEVEL_WARNING 2

inline static void report(int level, voba_str_t * msg,voba_value_t syn,voba_value_t top_level)
{
    switch(level){
    case LEVEL_ERROR:
        TOP_LEVEL(top_level)->n_of_errors ++ ; break;
    case LEVEL_WARNING:
        TOP_LEVEL(top_level)->n_of_warnings ++ ; break;
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
inline static void report_error(voba_str_t * msg,voba_value_t syn,voba_value_t top_level)
{
    report(LEVEL_ERROR,msg,syn,top_level);
}
inline static void report_warn(voba_str_t * msg,voba_value_t syn,voba_value_t top_level)
{
    report(LEVEL_WARNING,msg,syn,top_level);
}
static inline int is_keyword(voba_value_t top_level, voba_value_t x)
{
    voba_value_t keywords = TOP_LEVEL(top_level)->keywords;
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
static inline voba_value_t search_in_top_level(voba_value_t syn_symbol, voba_value_t top_level)
{
    assert(voba_get_class(syn_symbol) == voba_cls_syn);
    int64_t n = 0;
    voba_value_t a_ast_names = TOP_LEVEL(top_level)->a_ast_names;
    int64_t len = voba_array_len(a_ast_names);
    voba_value_t symbol = SYNTAX(syn_symbol)->v;
    voba_value_t ret = VOBA_NIL;
    for(n = 0; n < len; ++n){
        voba_value_t ast_name = voba_array_at(a_ast_names,n);
        voba_value_t syn_s_name = AST(ast_name)->u.top_var.syn_s_name;
        voba_value_t s_name = SYNTAX(syn_s_name)->v;
        if(voba_eq(s_name,symbol)){
            ret = make_ast_top_var(syn_symbol);break;
        }
    }
    return ret;
}
static inline voba_value_t compile_symbol(
    voba_value_t syn_symbol, voba_value_t fun, voba_value_t top_level
    )
{
    voba_value_t ret = search_fun(syn_symbol,fun);
    if(voba_is_nil(ret)){
        ret = search_in_top_level(syn_symbol,top_level);
    }
    if(voba_is_nil(ret)){
        report_error(VOBA_CONST_CHAR("cannot find symbol definition"),syn_symbol,top_level);
    }
    return ret;
}
static inline voba_value_t compile_array(voba_value_t syn_form,voba_value_t fun,voba_value_t top_level)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t form = SYNTAX(syn_form)->v;
    int64_t len = voba_array_len(form);
    if(len != 0){
        voba_value_t syn_f = voba_array_at(form,0);
        voba_value_t f = SYNTAX(syn_f)->v;
        if(voba_is_symbol(f)){
            if(voba_eq(f, K(top_level,quote))){
                report_error(VOBA_CONST_CHAR("not implemented for quote"),syn_f,top_level);
            }else if(voba_eq(f, K(top_level,if))){
                report_error(VOBA_CONST_CHAR("not implemented for if"),syn_f,top_level);
            }else if(voba_eq(f, K(top_level,import))){
                report_error(VOBA_CONST_CHAR("illegal form. import is the keyword"),syn_f,top_level);
            }else if(voba_eq(f, K(top_level,fun))){
                switch(len){
                case 1:
                    report_error(VOBA_CONST_CHAR("illegal form. bare fun is not fun"),syn_f,top_level);
                    break;
                case 2:
                    report_error(VOBA_CONST_CHAR("illegal form. empty body is not fun"),syn_f,top_level);
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
                        voba_value_t args[] = {1,top_level};
                        voba_value_t ast_expr = compile_fun(voba_make_array(self),
                                                            voba_make_array(args));
                        ret = ast_expr;
                    }else{
                        report_error(VOBA_CONST_CHAR("illegal form. argument list is not a list"),syn_fun_args,top_level);
                    }
                }
                }
            }else{
                goto label_default;
            }
        }else{
        label_default:
            ;
            voba_value_t ast_form = compile_exprs(voba_la_from_array0(form),fun,top_level);
            if(!voba_is_nil(ast_form)){
                assert(voba_is_array(ast_form));
                ret = make_ast_apply(ast_form);
            }
        }
    }else{
        report_error(VOBA_CONST_CHAR("illegal form. empty form"),syn_form,top_level);
    }
    return ret;
}
static inline voba_value_t compile_expr(voba_value_t syn_expr,voba_value_t fun,voba_value_t top_level)
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
        ret = compile_symbol(syn_expr,fun,top_level);
    }
    else if(cls == voba_cls_array){
        ret = compile_array(syn_expr,fun,top_level);
    }else{
        report_error(VOBA_CONST_CHAR("invalid expression"),syn_expr,top_level);
    }
    return ret;
}
    
// return an array of ast
static inline voba_value_t compile_exprs(voba_value_t la_syn_exprs,
                                         voba_value_t fun,
                                         voba_value_t top_level)
{
    // exprs: (....)
    //        ^la_syn_exprs
    voba_value_t cur = voba_la_copy(la_syn_exprs);
    voba_value_t ret = voba_make_array_0();
    voba_value_t cur_ret = voba_la_from_array1(ret,0);
    if(!voba_la_is_nil(cur)){
        while(!voba_la_is_nil(cur)){
            voba_value_t syn_exprs = voba_la_car(cur);
            voba_value_t ast_expr = compile_expr(syn_exprs,fun,top_level);
            if(!voba_is_nil(ast_expr)){
                cur_ret = voba_la_cons(cur_ret,ast_expr);
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
    VOBA_DEF_ARG(top_level, args, 0, ok);
    if(0)fprintf(stderr,__FILE__ ":%d:[%s] \n", __LINE__, __FUNCTION__);
    voba_value_t exprs = compile_exprs(la_syn_exprs,VOBA_NIL,top_level);
    voba_value_t ret = VOBA_NIL;
    if(!voba_is_nil(exprs)){
        ret = make_ast_set_top(syn_name, exprs);
    }
    if(0) fprintf(stderr,__FILE__ ":%d:[%s] v = 0x%lx type = 0x%lx\n", __LINE__, __FUNCTION__,
            ret, voba_get_class(ret));
    return ret;

}
static inline voba_value_t compile_top_expr_def_name(voba_value_t syn_name, voba_value_t la_syn_exprs,voba_value_t top_level)
{
    //  (def name ...)
    //       ^^^^^^^^------------------  la_syn_exprs
    //       ^^^ ----------------------  syn_name
    voba_value_t closure = voba_make_closure_2
        (compile_top_expr_def_name_next,syn_name,la_syn_exprs);
    voba_array_push(TOP_LEVEL(top_level)->next, closure);
    return make_ast_top_var(syn_name);
}
VOBA_FUNC static voba_value_t compile_fun(voba_value_t self, voba_value_t args)
{
    VOBA_DEF_CVAR(syn_fname,self,0);
    //VOBA_DEF_CVAR(la_syn_args,self,1);
    VOBA_DEF_CVAR(la_ast_exprs,self,2);
    /* VOBA_DEF_CVAR(parent,self,3); */
    VOBA_DEF_CVAR(a_ast_capture,self,4);
    VOBA_DEF_ARG(top_level, args, 0, ok);
    return make_ast_fun
        (syn_fname, compile_exprs(la_ast_exprs,self,top_level), a_ast_capture);
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
    VOBA_DEF_ARG(top_level, args, 0, ok);
    voba_value_t syn_f = fun_name(voba_closure_array(fun));
    voba_value_t ast_expr = voba_closure_func(fun)(voba_closure_array(fun), args);
    return make_ast_set_top(syn_f, voba_make_array_1(ast_expr));
}
static inline voba_value_t compile_top_expr_def_fun(
    voba_value_t syn_def,
    voba_value_t syn_var_form,
    voba_value_t la_ast_exprs_form,
    voba_value_t top_level)
{
    //  (def ( ... ) ...)
    //   ^------------------------  syn_def
    //       ^^^^^^^--------------- syn_var_form
    //               ^^^^---------  la_body_form
    voba_value_t ret = VOBA_NIL;
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
            ret = make_ast_top_var(syn_f);
            voba_value_t fun_parent = VOBA_NIL;
            voba_value_t a_ast_capture = voba_make_array_0();
            voba_value_t fun = make_fun
                (syn_f, voba_la_cdr(la_syn_var_form), la_ast_exprs_form, fun_parent, a_ast_capture);
            voba_value_t next = voba_make_closure_1(compile_top_expr_def_fun_next,fun);
            voba_array_push(TOP_LEVEL(top_level)->next,next);
        }else{
            report_error(VOBA_CONST_CHAR("illegal form. function name must be a symbol."),
                         syn_f,top_level);
        }
    }else{
        report_error(VOBA_CONST_CHAR("illegal form. no function name"),
                     syn_var_form,top_level);
    }
    return ret;
}

static inline voba_value_t compile_top_expr_def(voba_value_t syn_def, voba_value_t la_syn_top_expr,voba_value_t top_level)
{
    //  (def ...)
    //       ^la_syn_top_expr
    voba_value_t ret = VOBA_NIL;
    if(!voba_la_is_nil(la_syn_top_expr)){
        voba_value_t syn_var_form = voba_la_car(la_syn_top_expr);
        voba_value_t var_form = SYNTAX(syn_var_form)->v;
        if(voba_is_symbol(var_form)){
            //  (def var ...)
            //   ^^^---------------------------  syn_def
            //       ^^^^^^^-------------------  la_syn_top_expr
            //       ^^^ ----------------------  syn_var_form
            if(!is_keyword(top_level,var_form)){
                ret = compile_top_expr_def_name(syn_var_form, voba_la_cdr(la_syn_top_expr),top_level);
            }else{
                report_error(VOBA_CONST_CHAR("redefine keyword")
                             ,var_form
                             ,top_level);
            }
        }else if(voba_is_array(var_form)){
            //  (def ( ... ) ...)
            //   ^------------------------------  syn_def
            //       ^^^^^^^^^^^----------------- la_syn_top_expr
            //       ^^^^^^ --------------------  syn_var_form
            ret = compile_top_expr_def_fun(syn_def,
                                           syn_var_form,
                                           voba_la_cdr(la_syn_top_expr),
                                           top_level);
        }else{
            report_error(VOBA_CONST_CHAR("(def x), x must be a symbol or list")
                         ,syn_var_form
                         ,top_level);
        }
    }else{
        report_error(VOBA_CONST_CHAR("bare def"),syn_def,top_level);
    }
    assert(voba_is_nil(ret) || voba_get_class(ret) == voba_cls_ast);
    return ret;
}
static inline voba_value_t compile_top_expr(voba_value_t syn_top_expr,
                                            voba_value_t top_level)

{
    voba_value_t ret = VOBA_NIL;
    voba_value_t top_expr = SYNTAX(syn_top_expr)->v;
    if(voba_is_array(top_expr)){
        int64_t len =  voba_array_len(top_expr);
        if(len > 0){
            voba_value_t cur = voba_la_from_array1(top_expr,0);
            voba_value_t syn_key_word = voba_la_car(cur);
            voba_value_t key_word = SYNTAX(syn_key_word)->v;
            if(voba_eq(key_word, K(top_level,def))){
                ret = compile_top_expr_def(syn_key_word,voba_la_cdr(cur),top_level);
            }else{
                report_error(VOBA_CONST_CHAR("unrecognised keyword"),syn_key_word,top_level);
            }
        }else{
            report_warn(VOBA_CONST_CHAR("top expr is an empty list"), syn_top_expr,top_level);
        }
    }else{
        report_error(VOBA_CONST_CHAR("unrecognised form"),syn_top_expr,top_level);
    }
    assert(voba_is_nil(ret) || voba_get_class(ret) == voba_cls_ast);
    return ret;
}
static inline voba_value_t compile_top_level_list(voba_value_t la_syn_top_exprs,
                                                  voba_value_t top_level)
{
    voba_value_t cur = la_syn_top_exprs;
    while(!voba_la_is_nil(cur)){
        voba_value_t syn_top_expr = voba_la_car(cur);
        voba_value_t top_name     = compile_top_expr(syn_top_expr,top_level);
        assert(voba_is_nil(top_name) || voba_get_class(top_name) == voba_cls_ast);
        if(!voba_is_nil(top_name)) {
            voba_array_push(TOP_LEVEL(top_level)->a_ast_names,top_name);
        }
        cur = voba_la_cdr(cur);
    }
    return TOP_LEVEL(top_level)->a_ast_names;
}
voba_value_t compile_ast(voba_value_t syn,voba_value_t module, int * error)
{
    voba_value_t asts = VOBA_NIL;
    voba_value_t top_level = create_top_level(module);
    if(!voba_is_array(SYNTAX(syn)->v)){
        report_error(VOBA_CONST_CHAR("module must be an array."),syn,top_level);
        return VOBA_NIL;
    }
    compile_top_level_list(voba_la_from_array1(SYNTAX(syn)->v,0),top_level);
    voba_value_t next = TOP_LEVEL(top_level)->next;
    int64_t len =  voba_array_len(next);
    asts = voba_make_array_0();
    voba_value_t args [] = {1, top_level};
    for(int64_t i = 0 ; i < len ; ++i) {
        voba_value_t ast = voba_apply(voba_array_at(next,i),voba_make_array(args));
        if(!voba_is_nil(ast)){
            voba_array_push(asts,ast);
        }else{
            ++ *error;
        }
    }
    *error = TOP_LEVEL(top_level)->n_of_errors;
    return asts;
}

static inline int is_ast(voba_value_t x)
{
    return voba_get_class(x) == voba_cls_ast;
}
static inline voba_str_t* indent(int level)
{
    return voba_strcat(voba_str_fmt_int32_t(level,10),
                       voba_str_from_char(' ',level * 4));
}
static voba_str_t* print_ast_array(voba_value_t asts,int level)
{
    int64_t len = voba_array_len(asts);
    voba_str_t * ret = voba_str_empty();
    for(int64_t i = 0 ; i < len ; ++i){
        ret = voba_strcat(ret, print_ast(AST(voba_array_at(asts,i)),level));
        ret = voba_strcat(ret, VOBA_CONST_CHAR("\n"));
    }
    return ret;
}
static voba_str_t* print_ast_SET_TOP(ast_t * ast, int level)
{
    voba_value_t args[] = {1 , (SYNTAX(ast->u.set_top.syn_s_name)->v)};
    voba_value_t ss = voba_apply(voba_symbol_value(s_to_string),
                                 voba_make_array(args));
    voba_str_t* ret = voba_str_empty();
    ret = voba_vstrcat
        (ret,
         indent(level),
         VOBA_CONST_CHAR("ast "),
         voba_str_from_cstr(AST_TYPE_NAMES[ast->type]),
         VOBA_CONST_CHAR(" : name = "),
         voba_value_to_str(ss),
         VOBA_CONST_CHAR("\n"),
         NULL);
    voba_value_t exprs = ast->u.set_top.a_ast_exprs;
    ret = voba_strcat(ret, print_ast_array(exprs,level+1));
    return ret;
}
static voba_str_t* print_ast_CONSTANT(ast_t * ast, int level)
{
    voba_str_t* ret = voba_str_empty();
    voba_value_t args[] = {1 , ast->u.constant.value};
    voba_value_t ss = voba_apply(voba_symbol_value(s_to_string),
                                 voba_make_array(args));
    ret = voba_vstrcat
        (ret,
         indent(level),
         VOBA_CONST_CHAR("ast "),
         voba_str_from_cstr(AST_TYPE_NAMES[ast->type]),
         VOBA_CONST_CHAR(" : value = "),
         voba_value_to_str(ss),
         NULL);
    return ret;
}
static voba_str_t* print_ast_FUN(ast_t * ast, int level)
{
    voba_value_t a_ast_closure = ast->u.fun.a_ast_closure;
    voba_value_t a_ast_exprs = ast->u.fun.a_ast_exprs;
    voba_value_t args[] = {1 , (SYNTAX(ast->u.fun.syn_s_name)->v)};
    voba_value_t ss = voba_apply(voba_symbol_value(s_to_string),
                                 voba_make_array(args));
    voba_str_t* ret = voba_str_empty();
    return voba_vstrcat
        (ret,
         indent(level),
         VOBA_CONST_CHAR("ast "),
         voba_str_from_cstr(AST_TYPE_NAMES[ast->type]),
         VOBA_CONST_CHAR(" : name = "),
         voba_value_to_str(ss),
         VOBA_CONST_CHAR("\n"),
         indent(level),
         VOBA_CONST_CHAR("closure:\n"),
         print_ast_array(a_ast_closure,level+1),
         indent(level),
         VOBA_CONST_CHAR("body:\n"),
         print_ast_array(a_ast_exprs,level+1),
         VOBA_CONST_CHAR("\n"),
         NULL);
}
static voba_str_t* print_ast_ARG(ast_t * ast, int level)
{
    voba_str_t* ret = voba_str_empty();
    voba_value_t args[] = {1 , ast->u.arg.syn_s_name };
    voba_value_t ss = voba_apply(voba_symbol_value(s_to_string),
                                 voba_make_array(args));
    ret = voba_vstrcat
        (ret,
         indent(level),
         VOBA_CONST_CHAR("ast "),
         voba_str_from_cstr(AST_TYPE_NAMES[ast->type]),
         VOBA_CONST_CHAR(": index = " ),
         voba_str_fmt_uint32_t(ast->u.arg.index,10),
         VOBA_CONST_CHAR(" , name = "),
         voba_value_to_str(ss),
         NULL);
    return ret;
}
static voba_str_t* print_ast_CLOSURE_VAR(ast_t * ast, int level)
{
    return print_ast_ARG(ast,level);
}
static voba_str_t* print_ast_TOP_VAR(ast_t * ast, int level)
{
    voba_str_t* ret = voba_str_empty();
    voba_value_t args[] = {1 , ast->u.top_var.syn_s_name };
    voba_value_t ss = voba_apply(voba_symbol_value(s_to_string),
                                 voba_make_array(args));
    ret = voba_vstrcat
        (ret,
         indent(level),
         VOBA_CONST_CHAR("ast "),
         voba_str_from_cstr(AST_TYPE_NAMES[ast->type]),
         VOBA_CONST_CHAR(" : name = "),
         voba_value_to_str(ss),
         NULL);
    return ret;
}
static voba_str_t* print_ast_APPLY(ast_t * ast, int level)
{
    voba_str_t* ret = voba_str_empty();
    ret = voba_vstrcat
        (ret,
         indent(level),
         VOBA_CONST_CHAR("ast "),
         voba_str_from_cstr(AST_TYPE_NAMES[ast->type]),
         VOBA_CONST_CHAR(" : "),
         VOBA_CONST_CHAR("\n"),
         NULL);
    voba_value_t exprs = ast->u.apply.a_ast_exprs;
    ret = voba_strcat(ret, print_ast_array(exprs,level+1));
    return ret;
}
static voba_str_t* print_ast(ast_t * ast, int level)
{
#define AST_TYPE_PRINT_DISPATCHER(X)            \
case X:                                         \
return print_ast_##X(ast,level);
    switch(ast->type){
        AST_TYPES(AST_TYPE_PRINT_DISPATCHER)
            ;
    case N_OF_AST_TYPES:
        return voba_str_empty();
    }
    assert(0 && "never goes here");
    return NULL;
}
VOBA_FUNC static voba_value_t to_string_ast(voba_value_t self, voba_value_t args)
{
    VOBA_DEF_ARG(ast, args, 0, is_ast);
    voba_str_t * x = voba_str_empty();
    x = voba_vstrcat
        (x,
         VOBA_CONST_CHAR("\n"),
         print_ast(AST(ast),0),
         NULL);
    return voba_make_string(x);
}

EXEC_ONCE_START;
