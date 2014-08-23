#include <voba/include/value.h>
#include <exec_once.h>
#include <voba/core/builtin.h>
#include "ast.h"
#define DECLARE_AST_TYPE_ARRAY(X) #X,
const char * AST_TYPE_NAMES [] = {
    AST_TYPES(DECLARE_AST_TYPE_ARRAY)
    "# of ast type"
};
#define AST_TYPE_PRINT_DISPATCHER(X)            \
case X:                                         \
return print_ast_##X(ast,level);

#define AST_DECLARE_TYPE_PRINT_DISPATCHER(X)                            \
    static inline voba_str_t* print_ast_##X(ast_t * ast, int level);    
static voba_str_t* print_ast(ast_t * ast, int level);

AST_TYPES(AST_DECLARE_TYPE_PRINT_DISPATCHER);
    
#include "parser.syn.h"
#define VOBA_KEYWORDS(XX)                       \
    XX(def)
#define VOBA_DECLARE_KEYWORD(key) k_##key,
#define VOBA_DEFINE_KEYWORD(key)  VOBA_SYMBOL(key,module),
#define K(key) keywords[k_##key]
enum voba_keyword_e {
    VOBA_KEYWORDS(VOBA_DECLARE_KEYWORD)
    K_N_OF_KEYWORDS
};
DEFINE_CLS(sizeof(ast_t),ast);
VOBA_FUNC static voba_value_t to_string_ast(voba_value_t self, voba_value_t args);
EXEC_ONCE_DO(voba_gf_add_class(voba_symbol_value(s_to_string), voba_cls_ast,
                               voba_make_func(to_string_ast));)

static inline
voba_value_t make_ast_set_top(voba_value_t name /*symbol*/,
                              voba_value_t exprs /* array of ast */)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast,sizeof(ast_t));
    AST(r)->type = SET_TOP;
    AST(r)->u.set_top.name = name;
    AST(r)->u.set_top.exprs = exprs;
    return r;
}   
static inline
voba_value_t make_ast_CONSTANT(voba_value_t value)
{
    voba_value_t r = voba_make_user_data(voba_cls_ast,sizeof(ast_t));
    AST(r)->type = CONSTANT;
    AST(r)->u.constant.value = value;
    return r;
}   
inline static void report_error(const char * msg,voba_value_t syn)
{
    // TODO filename 
    printf("error: %d:%d - %d: %d:%s\n",SYNTAX(syn)->first_line,SYNTAX(syn)->first_column,
           SYNTAX(syn)->last_line, SYNTAX(syn)->last_column,msg);
}
inline static void report_warn(const char * msg,voba_value_t syn)
{
    // TODO filename 
    printf("warnning: %d:%d - %d: %d:%s\n",SYNTAX(syn)->first_line,SYNTAX(syn)->first_column,
           SYNTAX(syn)->last_line, SYNTAX(syn)->last_column,msg);
}
static inline int is_keyword(voba_value_t * keywords, voba_value_t x)
{
    for(int i = 0; i < K_N_OF_KEYWORDS; ++i){
        if(voba_eq(x,keywords[i])){
            return 1;
        }
    }
    return 0;
}
static inline voba_value_t compile_expr(voba_value_t syn_expr)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t expr = SYNTAX(syn_expr)->v;
    voba_value_t cls = voba_get_class(expr);
    if(cls == voba_cls_nil){
        ret = make_ast_CONSTANT(VOBA_NIL);
    }
#define COMPILE_EXPR_SMALL_TYPES(tag,name,type)                         \
    else if(cls == voba_cls_##name){                                    \
        ret = make_ast_CONSTANT(expr);                                  \
    }
    VOBA_SMALL_TYPES(COMPILE_EXPR_SMALL_TYPES)
    else if(cls == voba_cls_str){
        ret = make_ast_CONSTANT(expr);
    }
    else {
        report_error("invalid expression",syn_expr);
    }
    return ret;
}
static inline voba_value_t compile_exprs(voba_value_t syn_exprs, uint32_t from)
{
    // exprs: (X X X X ....)
    //        `------'
    //         # = from
    voba_value_t exprs = SYNTAX(syn_exprs)->v;
    uint32_t len = (uint32_t) voba_array_len(exprs);
    voba_value_t ret = voba_make_array_0();
    if(from < len){
        for(uint32_t i = from ; i < len ; ++i){
            voba_value_t ast_expr = compile_expr(voba_array_at(exprs,i));
            if(!voba_is_nil(ast_expr)){
                ret = voba_array_push(ret,ast_expr);
            }
        }
    }else {
        ret = voba_array_push(ret,make_ast_CONSTANT(VOBA_NIL));
    }
    return ret;
}
__attribute__((used))
static voba_value_t compile_top_expr_next(voba_value_t self, voba_value_t args) 
{
    VOBA_DEF_CVAR(fname,self,0);
    VOBA_DEF_CVAR(fargs,self,1);
    VOBA_DEF_CVAR(fbody,self,2);
    return fname = fargs = fbody;
}
VOBA_FUNC
static voba_value_t compile_top_expr_def_name_next(voba_value_t self, voba_value_t args)
{
    VOBA_DEF_CVAR(syn_name,self,0);
    VOBA_DEF_CVAR(syn_exprs,self,1);
    VOBA_DEF_CVAR(from,self,2);
    if(0)fprintf(stderr,__FILE__ ":%d:[%s] \n", __LINE__, __FUNCTION__);
    voba_value_t ret = make_ast_set_top(SYNTAX(syn_name)->v,
                                        compile_exprs(syn_exprs,voba_value_to_u32(from)));
    if(0) fprintf(stderr,__FILE__ ":%d:[%s] v = 0x%lx type = 0x%lx\n", __LINE__, __FUNCTION__,
            ret, voba_get_class(ret));
    return ret;

}
static inline voba_value_t compile_top_expr_def_name(voba_value_t syn_top_expr,
                                                     voba_value_t module,
                                                     voba_value_t * keywords,
                                                     voba_value_t next)
{
    // syn_top_expr: (def name ...)
    voba_value_t top_expr = SYNTAX(syn_top_expr)->v;
    voba_value_t syn_name = voba_array_at(top_expr,1);
    voba_value_t name = SYNTAX(syn_name)->v;
    voba_value_t from = voba_make_u32(2);
    voba_value_t closure = voba_make_closure_f_a(compile_top_expr_def_name_next,
                                                 voba_make_array_3(syn_name,syn_top_expr,from));
    voba_array_push(next, closure);
    return name;
}

static inline voba_value_t compile_top_expr_def(voba_value_t syn_top_expr,
                                                voba_value_t module,
                                                voba_value_t * keywords,
                                                voba_value_t next)
{
    voba_value_t top_name = VOBA_NIL;
    voba_value_t top_expr = SYNTAX(syn_top_expr)->v;
    int64_t len =  voba_array_len(top_expr);    
    if(len > 1){
        voba_value_t syn_var_form = voba_array_at(top_expr,1);
        voba_value_t var_form = SYNTAX(syn_var_form)->v;
        if(voba_is_symbol(var_form)){
            if(!is_keyword(keywords,var_form)){
                top_name = compile_top_expr_def_name(syn_top_expr,module,keywords,next);
            }else{
                report_error("redefine keyword",var_form);
            }
        }else if(voba_is_array(var_form)){
            report_error("not implemented yet", syn_top_expr);
        }else{
            report_error("(def x), x must be a symbol or list",syn_top_expr);
        }
    }else{
        report_error("bare def",syn_top_expr);
    }
    return top_name;
}
static inline voba_value_t compile_top_expr(voba_value_t syn_top_expr,
                                            voba_value_t module,
                                            voba_value_t * keywords,
                                            voba_value_t next)
{
    voba_value_t top_name = VOBA_NIL;
    voba_value_t top_expr = SYNTAX(syn_top_expr)->v;
    if(voba_is_array(top_expr)){
        int64_t len =  voba_array_len(top_expr);
        if(len > 0){
            voba_value_t syn_key_word = voba_array_at(top_expr,0);
            voba_value_t key_word = SYNTAX(syn_key_word)->v;
            if(voba_eq(key_word, K(def))){
                top_name = compile_top_expr_def(syn_top_expr,module,keywords,next);
            }else{
                report_error("unrecognised keyword",syn_key_word);
            }
        }else{
            report_warn("top expr is an empty list", syn_top_expr);
        }
    }else{
        report_error("unrecognised form",syn_top_expr);
    }
    return top_name;
}

static inline voba_value_t compile_top_level_list(voba_value_t syn,
                                                  voba_value_t module,
                                                  voba_value_t * keywords,
                                                  voba_value_t * pnext)
{
    if(!voba_is_array(SYNTAX(syn)->v)){
        report_error("module must be a list.",syn);
        return VOBA_NIL;
    }
    voba_value_t next = voba_make_array_0();
    *pnext = next;
    voba_value_t top_exprs = SYNTAX(syn)->v;
    voba_value_t top_names = voba_make_array_0();
    size_t len = voba_array_len(top_exprs);
    for(size_t i = 0; i < len ; ++i){
        voba_value_t syn_top_expr = voba_array_at(top_exprs,i);
        voba_value_t top_name     = compile_top_expr(syn_top_expr,module,keywords,next);
        if(!voba_is_nil(top_name)) {
            voba_array_push(top_names,top_name);
        }
    }
    return top_names;
}
voba_value_t compile_ast(voba_value_t syn,voba_value_t module)
{
    voba_value_t keywords[] = {
        VOBA_KEYWORDS(VOBA_DEFINE_KEYWORD)
        VOBA_NIL
    };
    voba_value_t next = VOBA_NIL;
    voba_value_t top_list = compile_top_level_list(syn,module,keywords,&next);
    int64_t len =  voba_array_len(next);
    voba_value_t args [] = {1, top_list};
    voba_value_t asts = voba_make_array_0();
    for(int64_t i = 0 ; i < len ; ++i) {
        voba_value_t ast = voba_apply(voba_array_at(next,i),voba_make_array(args));
        voba_array_push(asts,ast);
    }
    return asts;
}
static inline int voba_is_ast(voba_value_t x)
{
    return voba_get_class(x) == voba_cls_ast;
}
static inline voba_str_t* indent(int level)
{
    return voba_str_from_char(' ',level * 4);
}
static voba_str_t* print_ast_SET_TOP(ast_t * ast, int level)
{
    voba_str_t* ret = voba_str_empty();
    ret = voba_vstrcat
        (ret,
         indent(level),
         VOBA_CONST_CHAR("ast "),
         voba_str_from_cstr(AST_TYPE_NAMES[ast->type]),
         VOBA_CONST_CHAR(" : name = "),
         voba_value_to_str(voba_symbol_name(ast->u.set_top.name)),
         VOBA_CONST_CHAR("\n"),
         NULL);
    voba_value_t exprs = ast->u.set_top.exprs;
    int64_t len = voba_array_len(exprs);
    for(int64_t i = 0 ; i < len ;++i){
        ret = voba_vstrcat
            (ret,
             indent(level+1),
             print_ast(AST(voba_array_at(exprs,i)),level + 1),
             VOBA_CONST_CHAR("\n"),
             NULL);
    }
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
static voba_str_t* print_ast(ast_t * ast, int level)
{
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
    VOBA_DEF_ARG(ast, args, 0, voba_is_ast);
    voba_str_t * x = voba_str_empty();
    x = voba_vstrcat
        (x,
         VOBA_CONST_CHAR("\n"),
         print_ast(AST(ast),0),
         NULL);
    return voba_make_string(x);
}

EXEC_ONCE_START;
