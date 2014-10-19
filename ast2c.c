#include <stdint.h>
#include <assert.h>
#include <voba/include/value.h>
#include <exec_once.h>
#include "syn.h"
#include "ast.h"
#include "var.h"
#include "env.h"
#include "match.h"
#include "c_backend.h"
#include "module_info.h"
#include "ast2c.h"
static void import_modules(toplevel_env_t* toplevel, c_backend_t* out);
static void import_module(voba_value_t a_modules, c_backend_t * bk);
static voba_str_t * quote_string(voba_str_t * s);
static voba_str_t* new_uniq_id();
static voba_str_t* ast2c_ast(ast_t* ast, c_backend_t * bk, voba_str_t ** s);
static voba_str_t* ast2c_ast_set_var(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static voba_str_t* ast2c_ast_constant(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static voba_str_t* ast2c_ast_fun(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static voba_str_t* ast2c_ast_arg(int32_t index, c_backend_t* bk, voba_str_t** s);
static voba_str_t* ast2c_ast_closure(int32_t index, c_backend_t* bk, voba_str_t** s);
static voba_str_t* ast2c_ast_var(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static voba_str_t* ast2c_ast_apply(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static voba_str_t* ast2c_ast_let(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static voba_str_t* ast2c_ast_match(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline void ast2c_decl_env(env_t * p_env, c_backend_t * bk, voba_str_t ** s);
static inline voba_str_t * ast2c_new_var(voba_str_t* comment, c_backend_t * bk, voba_str_t** s);
//static inline void ast2c_begin_scope(c_backend_t * bk, voba_str_t** s);
//static inline void ast2c_end_scope(c_backend_t * bk, voba_str_t** s);
static inline void ast2c_assign(voba_str_t * var, voba_str_t * value, c_backend_t* bk, voba_str_t**s);
static inline void ast2c_comment(voba_str_t * c, c_backend_t* bk, voba_str_t**s);
#define DECL(s)  OUT(bk->decl,s);
#define START(s) OUT(bk->start,s);
#define IMPL(s)  OUT(bk->impl,s);

static inline voba_str_t * output(voba_str_t * stream, voba_str_t * s, const char * file, int line, const char * fn)
{
    if(file){
        uint32_t i = 0 ; 
        for(i = 0; i < s->len; ++i){
            if(s->data[i] == '\n'){
                stream = voba_strcat(stream,voba_str_from_cstr("/*"));
                stream = voba_strcat(stream,voba_str_from_cstr(file));
                stream = voba_strcat(stream,voba_str_from_cstr(":"));
                stream = voba_strcat(stream,voba_str_fmt_int32_t(line,10));
                stream = voba_strcat(stream,voba_str_from_cstr(":["));
                stream = voba_strcat(stream,voba_str_from_cstr(fn));
                stream = voba_strcat(stream,voba_str_from_cstr("]"));
                stream = voba_strcat(stream,voba_str_from_cstr("*/"));
                stream = voba_strcat(stream,voba_str_from_cstr("\n"));
            }else{
                stream = voba_strcat_char(stream,s->data[i]);
            }
        }
    }else{
        stream = voba_strcat(stream,s);
    }
    return stream;
}
#define DEBUG_OUTPUT
#ifdef DEBUG_OUTPUT
#define OUT(stream,s)  (stream) = output(stream,s,__FILE__,__LINE__, __func__);
#else
#define OUT(stream,s)  (stream) = output(stream,s,NULL,0,NULL);
#endif
static voba_str_t * var_c_id(var_t* var)
{
    voba_value_t syn_s_name = var->syn_s_name;
    return voba_strcat(voba_str_from_char('x',1), voba_str_fmt_int64_t(syn_s_name,16));
}
static voba_str_t * var_c_symbol_name(var_t* var)
{
    voba_value_t syn_s_name = var->syn_s_name;
    return voba_value_to_str(voba_symbol_name(SYNTAX(syn_s_name)->v));
}
static void ast2c_decl_top_var(env_t* env, c_backend_t * bk)
{
    ast2c_decl_env(env,bk,&bk->decl);
    voba_value_t a_top_vars = env->a_var;
    int64_t len = voba_array_len(a_top_vars);
    for(int64_t i = 0 ; i < len ; ++i){
        var_t * var = VAR(voba_array_at(a_top_vars,i));
        assert(var_is_top(var));
        switch(var->flag){
        case VAR_PRIVATE_TOP:
            break;
        case VAR_PUBLIC_TOP:
        case VAR_FOREIGN_TOP:
            assert(!voba_is_nil(var->u.module_id));
            START(VOBA_CONST_CHAR("{\n"));
            START(VOBA_CONST_CHAR("    voba_value_t m = VOBA_NIL;\n"));
            START(VOBA_CONST_CHAR("    voba_value_t s = VOBA_NIL;\n"));
            START(VOBA_CONST_CHAR("    voba_value_t id = VOBA_NIL;\n"));
            START(VOBA_CONST_CHAR("    id = voba_make_string(voba_str_from_cstr("));
            START(quote_string(voba_value_to_str(var->u.module_id)));
            START(VOBA_CONST_CHAR("    ));\n"));
            START(VOBA_CONST_CHAR("    m = voba_hash_find(voba_modules,id);\n"));
            START(VOBA_CONST_CHAR("    if(voba_is_nil(m)){\n"));
            START(VOBA_CONST_CHAR("        VOBA_THROW("));
            START(VOBA_CONST_CHAR("VOBA_CONST_CHAR(\""));
            START(VOBA_CONST_CHAR("module \" "));
            START(quote_string(voba_value_to_str(var->u.module_id)));
            START(VOBA_CONST_CHAR(" \" is not imported."));
            START(VOBA_CONST_CHAR("\"));\n"));
            START(VOBA_CONST_CHAR("    }\n"));
            START(VOBA_CONST_CHAR("    s = voba_lookup_symbol(voba_make_string(voba_c_id_decode(voba_str_from_cstr("));
            START(quote_string(var_c_symbol_name(var)));
            START(VOBA_CONST_CHAR(")))"));
            START(VOBA_CONST_CHAR(",voba_tail(m));\n"));
            START(VOBA_CONST_CHAR("    if(voba_is_nil(s)){\n"));
            START(VOBA_CONST_CHAR("        VOBA_THROW("));
            START(VOBA_CONST_CHAR("VOBA_CONST_CHAR(\""));
            START(VOBA_CONST_CHAR("module \" "));
            START(quote_string(voba_value_to_str(var->u.module_id)));
            START(VOBA_CONST_CHAR(" \" should contains \" "));
            START(quote_string(var_c_symbol_name(var)));
            START(VOBA_CONST_CHAR("));\n"));
            START(VOBA_CONST_CHAR("    }\n"));
            START(VOBA_CONST_CHAR("    "));
            START(var_c_id(var));
            START(VOBA_CONST_CHAR(" = s;\n"));
            START(VOBA_CONST_CHAR("}\n"));
            break;
        default:
            assert(0 && "never goes heree");
        }
    }
}
static void import_modules(toplevel_env_t* toplevel, c_backend_t* out)
{
    voba_value_t a_modules  = toplevel->a_modules;
    int64_t len = voba_array_len(a_modules);
    for(int64_t i = 0 ; i < len ; ++i){
        import_module(voba_array_at(a_modules,i),out);
    }
}
static voba_str_t * quote_string(voba_str_t * s)
{
    voba_str_t * ret = voba_str_empty();
    ret = voba_strcat_char(ret,'"');
    ret = voba_strcat(ret,s);
    ret = voba_strcat_char(ret,'"');
    return ret;
}
static void import_module(voba_value_t module_info, c_backend_t * bk)
{
    module_info_t * mi  = MODULE_INFO(module_info);
    START(VOBA_CONST_CHAR("{\n"));
    START(VOBA_CONST_CHAR("    const char * name = "));
    START(quote_string(mi->name));
    START(VOBA_CONST_CHAR(";\n"));
    START(VOBA_CONST_CHAR("    const char * id = "));
    START(quote_string(mi->id));
    START(VOBA_CONST_CHAR(";\n"));
    int64_t len = voba_array_len(mi->symbols);
    START(VOBA_CONST_CHAR("    const char * symbols[] = {\n"));
    for(int64_t i = 0; i < len; ++i){
        START(VOBA_CONST_CHAR("         "));
        START(quote_string(voba_value_to_str(voba_array_at(mi->symbols,i))));
        START(VOBA_CONST_CHAR(",\n"));
    }
    START(VOBA_CONST_CHAR("         NULL\n"));
    START(VOBA_CONST_CHAR("    };\n"));
    START(VOBA_CONST_CHAR("    fprintf(stderr,\"loading %s(%s)\\n\",name,id);\n"));
    START(VOBA_CONST_CHAR("    voba_import_module(name,id,symbols);\n"));
    START(VOBA_CONST_CHAR("}\n"));
}
static void ast2c_decl_prelude(c_backend_t* bk)
{
    DECL(VOBA_CONST_CHAR("#include <stdint.h>\n"));
    DECL(VOBA_CONST_CHAR("#include <assert.h>\n"));
    DECL(VOBA_CONST_CHAR("#include <voba/include/value.h>\n"));
    DECL(VOBA_CONST_CHAR("#include <exec_once.h>\n"));
    DECL(VOBA_CONST_CHAR("#include <voba/include/module.h>\n"));
}
EXEC_ONCE_START;
static void ast2c_all_asts(voba_value_t a_asts, c_backend_t* bk)
{
    int64_t len = voba_array_len(a_asts);
    for(int64_t i = 0; i < len; ++i){
        voba_str_t * s = voba_str_empty();
        ast2c_ast(AST(voba_array_at(a_asts,i)),bk,&s);
        START(s);
    }
}
static voba_str_t* ast2c_ast(ast_t* ast, c_backend_t * bk, voba_str_t ** s)
{
    voba_str_t* ret = voba_str_empty();
    switch(ast->type){
    case SET_VAR:
        ret = ast2c_ast_set_var(ast,bk,s);
        break;
    case CONSTANT:
        ret = ast2c_ast_constant(ast,bk,s);
        break;
    case FUN:
        ret = ast2c_ast_fun(ast,bk,s);
        break;
    case VAR:
        ret = ast2c_ast_var(ast,bk,s);
        break;
    case APPLY:
        ret = ast2c_ast_apply(ast,bk,s);
        break;
    case LET:
        ret = ast2c_ast_let(ast,bk,s);
        break;
    case MATCH:
        ret = ast2c_ast_match(ast,bk,s);
        break;
    default:
        assert(0 && "never goes here");
    }
    return ret;
}
static voba_str_t* ast2c_ast_exprs(voba_value_t exprs, c_backend_t * bk, voba_str_t ** s)
{
    voba_str_t* ret = VOBA_CONST_CHAR("VOBA_NIL");
    int64_t len = voba_array_len(exprs);
    for(int64_t i = 0; i < len; ++i){
        ret = ast2c_ast(AST(voba_array_at(exprs,i)),bk,s);
    }
    return ret;
}
static voba_str_t* ast2c_ast_set_var(ast_t* ast, c_backend_t * bk, voba_str_t ** s)
{
    voba_value_t exprs = ast->u.set_var.a_ast_exprs;
    var_t * var = ast->u.set_var.var;
    voba_str_t* expr = ast2c_ast_exprs(exprs,bk,s);
    voba_str_t * ret = ast2c_new_var(VOBA_CONST_CHAR("set var"),bk,s);
    ast2c_assign(ret, expr, bk,s);
    switch(var->flag){
    case VAR_FOREIGN_TOP:
    case VAR_PUBLIC_TOP:
        OUT(*s,VOBA_CONST_CHAR("voba_symbol_set_value("));
        OUT(*s,var_c_id(var));
        ast2c_comment(var_c_symbol_name(var),bk,&ret);
        OUT(*s,ret);
        OUT(*s,VOBA_CONST_CHAR(");\n"));
        break;
    case VAR_LOCAL:
    case VAR_PRIVATE_TOP:
        ast2c_comment(VOBA_STRCAT(
                          VOBA_CONST_CHAR(" -- set -- "),
                          var_c_symbol_name(var))
                      ,bk,s);
        OUT(*s,VOBA_CONST_CHAR("\n"));
        ast2c_assign(var_c_id(var),ret,bk,s);
        break;
    case VAR_ARGUMENT:
        OUT(*s,VOBA_CONST_CHAR("voba_array_set_at(fun_args,"));
        OUT(*s,voba_str_fmt_uint32_t(var->u.index,10));
        OUT(*s,VOBA_CONST_CHAR(","));
        OUT(*s,ret);
        OUT(*s,VOBA_CONST_CHAR(");\n"));
        break;
    case VAR_CLOSURE:
        OUT(*s,VOBA_CONST_CHAR("voba_array_set_at(self,"));
        OUT(*s,voba_str_fmt_uint32_t(var->u.index,10));
        OUT(*s,VOBA_CONST_CHAR(","));
        OUT(*s,ret);
        OUT(*s,VOBA_CONST_CHAR(");\n"));
        break;
    default:
        assert(0&&"never goes here");
    }
    return ret;
}
static voba_str_t* ast2c_ast_constant(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_value_t syn_value = ast->u.constant.value;
    voba_value_t value = SYNTAX(syn_value)->v;    
    voba_str_t * ret = voba_str_empty();
    voba_value_t cls = voba_get_class(value);
    if(cls == voba_cls_str){
        ret = voba_strcat(ret,VOBA_CONST_CHAR("voba_make_string(voba_str_from_cstr("));
        ret = voba_strcat(ret,quote_string(voba_value_to_str(value)));
        ret = voba_strcat(ret,VOBA_CONST_CHAR("))"));
    }else if(cls == voba_cls_nil){
        ret = voba_strcat(ret,VOBA_CONST_CHAR("VOBA_NIL"));
    }else{
        fprintf(stderr,__FILE__ ":%d:[%s] type %s is not supported yet\n", __LINE__, __FUNCTION__,
            voba_get_class_name(value));
        assert(0&&"type is supported yet");
    }
    return ret;
}
static voba_str_t* ast2c_ast_fun_with_closure(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = VOBA_CONST_CHAR("ast2c_ast_fun_with_closure is not implemented");
    return ret;
}
static voba_str_t* ast2c_ast_fun_without_closure(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    ast_fun_t* ast_fn = &(ast->u.fun);
    voba_str_t * uuid = new_uniq_id();
    DECL(VOBA_CONST_CHAR("VOBA_FUNC voba_value_t "));
    DECL(uuid);
    DECL(VOBA_CONST_CHAR("(voba_value_t self, voba_value_t fun_args);\n"));
    voba_str_t * s1 = voba_str_empty();
    OUT(s1,VOBA_CONST_CHAR("VOBA_FUNC voba_value_t "));
    OUT(s1,uuid);
    OUT(s1,VOBA_CONST_CHAR("(voba_value_t self, voba_value_t fun_args){\n"));
    voba_value_t exprs = ast_fn->a_ast_exprs;
    voba_str_t* expr = ast2c_ast_exprs(exprs,bk,&s1);
    OUT(s1, VOBA_CONST_CHAR("return "));
    OUT(s1, expr);
    OUT(s1, VOBA_CONST_CHAR(";\n"));
    OUT(s1,VOBA_CONST_CHAR("}\n"));
    IMPL(s1);
    voba_str_t * ret = voba_str_empty();
    OUT(ret, VOBA_CONST_CHAR("voba_make_func("));
    OUT(ret, uuid);
    OUT(ret, VOBA_CONST_CHAR(")"));
    return ret;
}
static voba_str_t* ast2c_ast_fun(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t* ret = voba_str_empty();
    ast_fun_t* ast_fn = &ast->u.fun;
    int64_t len = voba_array_len(ast_fn->f->a_var_C);
    if(len == 0){
        ret = ast2c_ast_fun_without_closure(ast,bk,s);
    }else{
        ret = ast2c_ast_fun_with_closure(ast,bk,s);
    }
    return ret;
}
static voba_str_t* ast2c_ast_arg(int32_t index, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = voba_str_empty();
    OUT(ret, VOBA_CONST_CHAR("voba_array_at(fun_args,"));
    OUT(ret, voba_str_fmt_int32_t(index,10));
    OUT(ret, VOBA_CONST_CHAR(")"));
    return ret;
}
static voba_str_t* ast2c_ast_closure(int32_t index, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = voba_str_empty();
    OUT(ret, VOBA_CONST_CHAR("voba_array_at(self,"));
    OUT(ret, voba_str_fmt_int32_t(index,10));
    OUT(ret, VOBA_CONST_CHAR(")"));
    return ret;
}
static voba_str_t* ast2c_ast_var(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t* ret = voba_str_empty();
    ast_var_t* tv = &ast->u.var;
    var_t * var = tv->var;
    switch(var->flag){
    case VAR_FOREIGN_TOP:
    case VAR_PUBLIC_TOP:
        ret = voba_strcat(ret,VOBA_CONST_CHAR("voba_symbol_value("));
        ret = voba_strcat(ret,var_c_id(var));
        ret = voba_strcat(ret,VOBA_CONST_CHAR(")"));        
        ast2c_comment(var_c_symbol_name(var),bk,&ret);
        break;
    case VAR_LOCAL:
    case VAR_PRIVATE_TOP:
        ret = voba_strcat(ret,var_c_id(var));
        ast2c_comment(var_c_symbol_name(var),bk,&ret);
        break;
    case VAR_ARGUMENT:
        ret = ast2c_ast_arg(var->u.index,bk,s);
        break;
    case VAR_CLOSURE:
        ret = ast2c_ast_closure(var->u.index,bk,s);
        break;
    default:
        assert(0&&"never goes here");
    }
    return ret;
}
static voba_str_t* ast2c_ast_apply(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = ast2c_new_var(VOBA_CONST_CHAR("apply"),bk,s);
    voba_value_t exprs = ast->u.apply.a_ast_exprs;
    voba_value_t args = voba_make_array_0();
    int64_t len = voba_array_len(exprs);
    assert(len >=1);
    for(int64_t i = 0; i < len; ++i){
        voba_value_t expr = voba_array_at(exprs,i);
        voba_str_t * sexpr = ast2c_ast(AST(expr), bk, s);
        voba_array_push(args,voba_make_string(sexpr));
    }
    voba_str_t * args_name = new_uniq_id();
    OUT(*s, VOBA_CONST_CHAR("voba_value_t "));
    OUT(*s, args_name);
    OUT(*s, VOBA_CONST_CHAR("[] = {"));
    OUT(*s, voba_str_fmt_int64_t(len-1,10));
    for(int64_t i = 1; i < len; ++i){
        OUT(*s,VOBA_CONST_CHAR(",\n"));
        OUT(*s,voba_value_to_str(voba_array_at(args,i)));
    }
    OUT(*s, VOBA_CONST_CHAR("\n};\n")); // end args
    voba_str_t * fun = voba_value_to_str(voba_array_at(args,0));
    OUT(*s, ret);
    OUT(*s, VOBA_CONST_CHAR(" = voba_apply(")); // end args
    OUT(*s, fun);
    OUT(*s, VOBA_CONST_CHAR(", voba_make_array("));
    OUT(*s, args_name);
    OUT(*s, VOBA_CONST_CHAR("));\n"));
    return ret;
}
static voba_str_t* ast2c_ast_let(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    ast_let_t * ast_let  = &ast->u.let;
    voba_str_t * id = ast2c_new_var(VOBA_CONST_CHAR("let"),bk,s);
    ast2c_decl_env(ast_let->env,bk,s);
    voba_value_t a_ast_exprs = ast_let->a_ast_exprs;
    voba_str_t * body = ast2c_ast_exprs(a_ast_exprs,bk,s);
    ast2c_assign(id,body,bk,s);
    return id;
}
static inline void ast2c_decl_env(env_t * p_env, c_backend_t * bk, voba_str_t ** s)
{
    voba_value_t a_var = p_env->a_var;
    int64_t len = voba_array_len(a_var);
    for(int64_t i = 0 ; i < len ; ++i){
        var_t * var  = VAR(voba_array_at(a_var,i));
        OUT(*s,VOBA_CONST_CHAR("voba_value_t "));
        OUT(*s,var_c_id(var));
        ast2c_comment(var_c_symbol_name(var),bk,s);
        OUT(*s,VOBA_CONST_CHAR(" __attribute__ ((unused)) = VOBA_UNDEF;\n"));
    }
    return;
}
static inline voba_str_t * ast2c_new_var(voba_str_t * comment, c_backend_t * bk, voba_str_t** s)
{
    voba_str_t * id = new_uniq_id();
    OUT(*s, VOBA_CONST_CHAR("voba_value_t "));
    OUT(*s, id);
    ast2c_comment(comment,bk,s);
    OUT(*s,VOBA_CONST_CHAR(" __attribute__ ((unused)) = VOBA_UNDEF;\n"));
    return id;
}
static inline void ast2c_assign(voba_str_t * var, voba_str_t * value, c_backend_t* bk, voba_str_t**s)
{
    OUT(*s, var);    
    OUT(*s, VOBA_CONST_CHAR("="));
    OUT(*s, value);
    OUT(*s, VOBA_CONST_CHAR(";\n"));
    return;
}
static inline void ast2c_comment(voba_str_t * c, c_backend_t* bk, voba_str_t**s)
{
    OUT(*s, VOBA_CONST_CHAR("/*"));    
    OUT(*s, c);
    OUT(*s, VOBA_CONST_CHAR("*/"));
}
inline static voba_str_t* ast2c_ast_match_rule(voba_str_t* v, voba_str_t* ret_id, voba_value_t rule, c_backend_t* bk, voba_str_t** s);
static voba_str_t* ast2c_ast_match(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    ast_match_t* ast_match  = &ast->u.match;
    voba_value_t ast_value = ast_match->ast_value;
    ast_t * p_ast_value = AST(ast_value);
    voba_str_t* s_ast_value = ast2c_ast(p_ast_value,bk,s);
    voba_str_t * id = ast2c_new_var(VOBA_CONST_CHAR("match"),bk,s);
    match_t * p_match = MATCH(ast_match->match);
    voba_value_t a_rules = p_match->a_rules;
    int64_t len = voba_array_len(a_rules);
    for(int64_t i = 0; i < len; ++i){
        voba_value_t rule = voba_array_at(a_rules,i);
        ast2c_ast_match_rule(s_ast_value, id, rule, bk,s);
    }
    return id;
}
inline static voba_str_t* ast2c_ast_match_rule(voba_str_t* v, voba_str_t * ret_id, voba_value_t rule, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = voba_str_empty();
    rule_t* p_rule = RULE(rule);
    ast2c_decl_env(ENV(p_rule->env),bk,s);
    return ret;
}
static voba_str_t* new_uniq_id()
{
    static int c = 0;
    return voba_strcat(VOBA_CONST_CHAR("s"),voba_str_fmt_int32_t(c++,10));
}
voba_value_t ast2c(toplevel_env_t* toplevel)
{
    voba_value_t ret = make_c_backend();
    c_backend_t * bk = C_BACKEND(ret);
    // #include blahblah
    ast2c_decl_prelude(bk);
    // EXEC_ONCE_PROGN { for each module; import module; }
    import_modules(toplevel,bk);
    // declare static voba_value_t top_var = VOBA_NIL
    // top_var = m["var_top"]
    ast2c_decl_top_var(ENV(toplevel->env),bk);
    // output all asts
    ast2c_all_asts(toplevel->a_asts,bk);
    return ret;
}
