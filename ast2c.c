#include <stdint.h>
#include <assert.h>
#include <voba/include/value.h>
#include <exec_once.h>
#include <voba/include/module.h>
#include "syn.h"
#include "ast.h"
#include "var.h"
#include "env.h"
#include "match.h"
#include "syn2ast_report.h"
#include "c_backend.h"
#include "module_info.h"
#include "ast2c.h"
#include "ast2c_utils.h"
#include "ast2c_utils.c"
static inline void ast2c_decl_prelude(c_backend_t* bk);
static inline void import_modules(toplevel_env_t* toplevel, c_backend_t* out);
static inline void ast2c_decl_top_var(env_t* env, c_backend_t * bk);
static inline void ast2c_all_asts(voba_value_t a_asts, c_backend_t* bk);
voba_value_t ast2c(voba_value_t tvl)
{
    toplevel_env_t* toplevel = TOPLEVEL_ENV(tvl);
    voba_value_t ret = make_c_backend();
    c_backend_t * bk = C_BACKEND(ret);
    bk->toplevel_env = tvl;
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
static inline void ast2c_decl_prelude(c_backend_t* bk)
{
    TEMPLATE(&bk->decl,
             VOBA_CONST_CHAR("##include <stdint.h>\n"
                             "##include <assert.h>\n"
                             "##include <voba/include/value.h>\n"
                             "##define EXEC_ONCE_TU_NAME \"TODO ADD \"\n"
                             "##include <exec_once.h>\n"
                             "##include <voba/include/module.h>\n"
                             "##define voba_match_eq voba_eql\n"
                             "static voba_value_t gf_match __attribute__((unused)) = VOBA_UNDEF;\n"));
}
static void import_module(voba_value_t a_modules, c_backend_t * bk);
static void import_modules(toplevel_env_t* toplevel, c_backend_t* out)
{
    voba_value_t a_modules  = toplevel->a_modules;
    int64_t len = voba_array_len(a_modules);
    for(int64_t i = 0 ; i < len ; ++i){
        import_module(voba_array_at(a_modules,i),out);
    }
}
static inline void ast2c_decl_env(env_t * p_env, c_backend_t * bk, voba_str_t ** s);
static inline void ast2c_import_var(c_backend_t * bk, voba_str_t * module_name, voba_str_t* module_id, voba_str_t * syn_name, voba_str_t * var_name);
static inline void ast2c_decl_top_var(env_t* env, c_backend_t * bk)
{
    ast2c_decl_env(env,bk,&bk->decl);
    ast2c_import_var(bk,
                     voba_str_from_cstr(VOBA_MODULE_LANG_ID),
                     voba_str_from_cstr(VOBA_MODULE_LANG_ID),
                     voba_str_from_cstr(VOBA_MODULE_LANG_MATCH),
                     VOBA_CONST_CHAR("gf_match"));
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
            assert(!voba_is_nil(var->u.m.module_id));
            ast2c_import_var(bk,
                             voba_value_to_str(var->u.m.module_name),
                             voba_value_to_str(var->u.m.module_id),
                             var_c_symbol_name(var),
                             var_c_id(var));
            break;
        default:
            assert(0 && "never goes heree");
        }
    }
}
static inline void ast2c_import_var(c_backend_t * bk, voba_str_t * module_name, voba_str_t* module_id, voba_str_t * syn_name, voba_str_t * var_name)
{
    TEMPLATE(&bk->start,
             VOBA_CONST_CHAR(
                 "{\n"
                 "    #2 = voba_module_var(#3,#0,#1);\n"
                 "}\n")
             ,quote_string(module_id)
             ,quote_string(syn_name)
             ,var_name
             ,quote_string(module_name)
        );
    return;
}

static void import_module(voba_value_t module_info, c_backend_t * bk)
{
    module_info_t * mi  = MODULE_INFO(module_info);
    TEMPLATE(&bk->start,
             VOBA_CONST_CHAR(
                 "{\n"
                 "    const char * name = #0;\n"
                 "    const char * id = #1;\n"
                 "    const char * symbols[] = {\n"
                 )
             ,quote_string(mi->name)
             ,quote_string(mi->id));
    int64_t len = voba_array_len(mi->symbols);
    for(int64_t i = 0; i < len; ++i){
        TEMPLATE(&bk->start,
                 VOBA_CONST_CHAR(
                     "         #0,\n"
                     )
                 ,quote_string(voba_value_to_str(voba_array_at(mi->symbols,i))));
    }
    TEMPLATE(&bk->start,
             VOBA_CONST_CHAR(
                 "         NULL\n"
                 "    };\n"
                 "    fprintf(stderr,\"loading %s(%s)\\n\",name,id);\n"
                 "    voba_import_module(name,id,symbols);\n"
                 "}\n"));
}

static inline voba_str_t* ast2c_ast(ast_t* ast, c_backend_t * bk, voba_str_t ** s);
static inline void ast2c_all_asts(voba_value_t a_asts, c_backend_t* bk)
{
    int64_t len = voba_array_len(a_asts);
    for(int64_t i = 0; i < len; ++i){
        voba_str_t * s = voba_str_empty();
        voba_value_t ast  = voba_array_at(a_asts,i);
        assert(voba_is_a(ast,voba_cls_ast));
        ast_t * p_ast = AST(ast);
        bk->it = ast2c_ast(p_ast,bk,&s);
        bk->start = voba_strcat(bk->start,s);
    }
    TEMPLATE(&bk->impl,
        VOBA_CONST_CHAR("voba_value_t voba_init(voba_value_t this_module)\n"
                        "{\n"
                        "    exec_once_init();\n"
                        "    return VOBA_NIL;\n"
                        "}\n"
            ));
}
static inline voba_str_t* ast2c_ast_set_var(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_constant(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_fun(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_var(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_arg(int32_t index, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_apply(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_let(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_match(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_for(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_it(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_break(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_and(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_or(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast(ast_t* ast, c_backend_t * bk, voba_str_t ** s)
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
    case FOR:
        ret = ast2c_ast_for(ast,bk,s);
        break;
    case IT:
        ret = ast2c_ast_it(ast,bk,s);
        break;
    case BREAK:
        ret = ast2c_ast_break(ast,bk,s);
        break;
    case AND:
        ret = ast2c_ast_and(ast,bk,s);
        break;
    case OR:
        ret = ast2c_ast_or(ast,bk,s);
        break;
    default:
        assert(0 && "never goes here");
    }
    return ret;
}
static inline voba_str_t* ast2c_ast_exprs(voba_value_t exprs, c_backend_t * bk, voba_str_t ** s)
{
    voba_str_t* ret = VOBA_CONST_CHAR("VOBA_NIL");
    int64_t len = voba_array_len(exprs);
    voba_str_t * old_it = bk->it;
    for(int64_t i = 0; i < len; ++i){
        ret = ast2c_ast(AST(voba_array_at(exprs,i)),bk,s);
        bk->it = ret;
    }
    bk->it = old_it;
    return ret;
}
static inline voba_str_t* ast2c_ast_set_var(ast_t* ast, c_backend_t * bk, voba_str_t ** s)
{
    voba_value_t exprs = ast->u.set_var.a_ast_exprs;
    var_t * var = ast->u.set_var.var;
    voba_str_t* expr = ast2c_ast_exprs(exprs,bk,s);
    voba_str_t * ret = new_uniq_id();
    TEMPLATE(s,
             VOBA_CONST_CHAR("    voba_value_t #0 __attribute__((unused)) = VOBA_UNDEF;\n"
                             "    #0 = #1;/* value for set var*/\n")
             ,ret
             ,expr);
    switch(var->flag){
    case VAR_FOREIGN_TOP:
    case VAR_PUBLIC_TOP:
        TEMPLATE(s,
                 VOBA_CONST_CHAR("voba_symbol_set_value(#0,#1);/*set var #2*/\n")
                 ,var_c_id(var)
                 ,ret
                 ,var_c_symbol_name(var));
        break;
    case VAR_LOCAL:
    case VAR_PRIVATE_TOP:
        TEMPLATE(s,
                 VOBA_CONST_CHAR("    #0 = #1; /* set #2 */\n"),
                 var_c_id(var),ret,var_c_symbol_name(var));
        break;
    case VAR_ARGUMENT:
        TEMPLATE(s,
                 VOBA_CONST_CHAR("    voba_array_set_at(fun_args,#0,#1); /* set #2 */\n"),
                 voba_str_fmt_uint32_t(var->u.index,10),ret,var_c_symbol_name(var));
        break;
    case VAR_CLOSURE:
        TEMPLATE(s,
                 VOBA_CONST_CHAR("    voba_array_set_at(self,#0,#1); /* set #2 */\n"),
                 voba_str_fmt_uint32_t(var->u.index,10),ret,var_c_symbol_name(var));
        break;
    default:
        assert(0&&"never goes here");
    }
    return ret;
}
static inline voba_str_t* ast2c_constant_array(voba_value_t syn_a, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_constant(voba_value_t syn_a, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_constant(ast_t * ast, c_backend_t* bk, voba_str_t** s)
{
    voba_value_t syn_value = ast->u.constant.syn_value;
    return ast2c_constant(syn_value, bk, s);
}
static inline voba_str_t* ast2c_constant(voba_value_t syn_value, c_backend_t* bk, voba_str_t** s)
{
    voba_value_t value = SYNTAX(syn_value)->v;
    voba_str_t * ret = voba_str_empty();
    voba_value_t cls = voba_get_class(value);
    if(cls == voba_cls_str){
        ret = voba_strcat(ret,VOBA_CONST_CHAR("voba_make_string(voba_str_from_cstr("));
        ret = voba_strcat(ret,quote_string(voba_value_to_str(value)));
        ret = voba_strcat(ret,VOBA_CONST_CHAR("))"));
    }else if(cls == voba_cls_nil){
        ret = VOBA_CONST_CHAR("VOBA_NIL");
    }else if(cls == voba_cls_i32){
        ret = VOBA_STRCAT(
            VOBA_CONST_CHAR("voba_make_i32("),
            voba_str_fmt_int32_t(voba_value_to_i32(value),10),
            VOBA_CONST_CHAR(")"));
    }else if(cls == voba_cls_bool){
        if(voba_is_false(value)){
            ret = VOBA_CONST_CHAR("VOBA_FALSE");
        }else{
            assert(voba_is_true(value));
            ret = VOBA_CONST_CHAR("VOBA_TRUE");
        }
    }else if(cls == voba_cls_array){
        ret = ast2c_constant_array(syn_value,bk,s);
    }else{
        fprintf(stderr,__FILE__ ":%d:[%s] type %s is not supported yet\n", __LINE__, __FUNCTION__,
            voba_get_class_name(value));
        assert(0&&"type is supported yet");
    }
    return ret;
}
static inline voba_str_t* ast2c_constant_array(voba_value_t syn_a, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t* s_const = new_uniq_id();
    voba_str_t* s_c_const = new_uniq_id();
    TEMPLATE(
        &bk->decl,
        VOBA_CONST_CHAR("    voba_value_t #0 __attribute__((unused)) = VOBA_UNDEF; /* var for constant*/\n"),
        s_const
        );
    voba_str_t* s0 = voba_str_empty();
    voba_value_t a = SYNTAX(syn_a)->v;
    int64_t len = voba_array_len(a);
    TEMPLATE(
        &s0,
        VOBA_CONST_CHAR("    voba_value_t #0 [] = { #1"),
        s_c_const,
        voba_str_fmt_int64_t(len,10)
        );
    for(int64_t i = 0; i < len; ++i){
        TEMPLATE(
            &s0,
            VOBA_CONST_CHAR(",#0"),
            ast2c_constant(voba_array_at(a,i),bk,s));
    }
    TEMPLATE(
        &s0,
        VOBA_CONST_CHAR(
            "};\n"
            "    #0 = voba_make_array(#1); /* constant */\n"
            ),
        s_const,
        s_c_const);
    bk->start = voba_strcat(bk->start,s0);
    return s_const;
}
static inline voba_str_t* ast2c_ast_fun_with_closure(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = VOBA_CONST_CHAR("ast2c_ast_fun_with_closure is not implemented");
    return ret;
}
static inline voba_str_t* ast2c_ast_fun_without_closure(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    ast_fun_t* ast_fn = &(ast->u.fun);
    voba_str_t * uuid = new_uniq_id();
    voba_str_t * s1 = voba_str_empty();
    voba_str_t * s2 = voba_str_empty();
    voba_value_t exprs = ast_fn->a_ast_exprs;
    voba_str_t* expr = ast2c_ast_exprs(exprs,bk,&s2);
    TEMPLATE(&bk->decl,
             VOBA_CONST_CHAR("VOBA_FUNC voba_value_t #0 (voba_value_t self, voba_value_t fun_args);\n")
             ,uuid);
    TEMPLATE(&s1,
             VOBA_CONST_CHAR("VOBA_FUNC voba_value_t #0 (voba_value_t self, voba_value_t fun_args)\n"
                             "{\n"
                             "#1"
                             "    return #2; /* return #0 */\n"
                             "}\n")
             ,uuid,indent(s2),expr);
    bk->impl = voba_strcat(bk->impl, s1);
    voba_str_t * ret = voba_str_empty();
    TEMPLATE(&ret, VOBA_CONST_CHAR("voba_make_func(#0)"),uuid);
    return ret;
}
static inline voba_str_t* ast2c_ast_fun(ast_t* ast, c_backend_t* bk, voba_str_t** s)
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
static inline voba_str_t* ast2c_ast_arg(int32_t index, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = voba_str_empty();
    TEMPLATE(&ret,
             VOBA_CONST_CHAR("voba_array_at(fun_args,#0)")
             ,voba_str_fmt_int32_t(index,10));
    return ret;
}
static inline voba_str_t* ast2c_ast_closure(int32_t index, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = voba_str_empty();
    TEMPLATE(&ret,
             VOBA_CONST_CHAR("voba_array_at(self,#0)")
             ,voba_str_fmt_int32_t(index,10));
    return ret;
}
static inline voba_str_t* ast2c_ast_var(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t* ret = voba_str_empty();
    ast_var_t* tv = &ast->u.var;
    var_t * var = tv->var;
    switch(var->flag){
    case VAR_FOREIGN_TOP:
    case VAR_PUBLIC_TOP:
        TEMPLATE(&ret,
                 VOBA_CONST_CHAR("voba_symbol_value(#0 /* #1 */)")
                 ,var_c_id(var),var_c_symbol_name(var));
        break;
    case VAR_LOCAL:
    case VAR_PRIVATE_TOP:
        TEMPLATE(&ret,
                 VOBA_CONST_CHAR("#0 /* #1 */")
                 ,var_c_id(var),var_c_symbol_name(var));
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
static inline voba_str_t* ast2c_ast_apply(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = new_uniq_id();
    voba_value_t exprs = ast->u.apply.a_ast_exprs;
    voba_value_t args = voba_make_array_0();
    int64_t len = voba_array_len(exprs);
    voba_str_t * args_name = new_uniq_id();
    assert(len >=1);
    voba_str_t * old_it = bk->it;
    for(int64_t i = 0; i < len; ++i){
        voba_value_t expr = voba_array_at(exprs,i);
        voba_str_t * sexpr = ast2c_ast(AST(expr), bk, s);
        if(i > 0){
            bk->it = sexpr;
        }
        voba_array_push(args,voba_make_string(sexpr));
    }
    bk->it = old_it;
    voba_str_t * fun = voba_value_to_str(voba_array_at(args,0));
    TEMPLATE(s,
             VOBA_CONST_CHAR("    voba_value_t #0 __attribute__((unused)) = VOBA_UNDEF;\n"
                             "    voba_value_t #1 [] = { #2\n")
             ,ret
             ,args_name
             ,voba_str_fmt_int64_t(len-1,10));
    for(int64_t i = 1; i < len; ++i){
        TEMPLATE(s,
                 VOBA_CONST_CHAR("         ,#0\n")
                 ,voba_value_to_str(voba_array_at(args,i)));
    }
    TEMPLATE(s,
             VOBA_CONST_CHAR("    };\n"
                             "    #0 = voba_apply(#1,voba_make_array(#2));\n"
                 )
             ,ret, fun, args_name);
    return ret;
}
static inline voba_str_t* ast2c_ast_let(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    ast_let_t * ast_let  = &ast->u.let;
    voba_str_t * id = new_uniq_id();
    voba_str_t * s1 = voba_str_empty();
    voba_str_t * s2 = voba_str_empty();
    ast2c_decl_env(ast_let->env,bk,&s1);
    voba_value_t a_ast_exprs = ast_let->a_ast_exprs;
    voba_str_t * body = ast2c_ast_exprs(a_ast_exprs,bk,&s2);
    TEMPLATE(s,
             VOBA_CONST_CHAR("    /*let env*/\n"
                             "#2\n"
                             "    /*decl let return value*/\n"
                             "    voba_value_t #0 __attribute__((unused)) = VOBA_UNDEF;\n"
                             "    /*let body*/\n"
                             "#3\n" 
                             "    #0 = #1; /* set letreturn value*/\n")
             ,id
             ,body
             ,indent(s1)
             ,indent(s2));
    return id;
}
static inline void ast2c_decl_env(env_t * p_env, c_backend_t * bk, voba_str_t ** s)
{
    voba_value_t a_var = p_env->a_var;
    int64_t len = voba_array_len(a_var);
    for(int64_t i = 0 ; i < len ; ++i){
        var_t * var  = VAR(voba_array_at(a_var,i));
        TEMPLATE(s,
                 VOBA_CONST_CHAR("    voba_value_t #0 /* #1 */ __attribute__((unused)) = VOBA_UNDEF;\n")
                 ,var_c_id(var)
                 ,var_c_symbol_name(var));
    }
    return;
}
static inline void ast2c_match(voba_str_t * s_match_ret, voba_str_t * s_ast_value, voba_str_t * label_success, voba_value_t match, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_match(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    ast_match_t* ast_match  = &ast->u.match;
    voba_value_t ast_value = ast_match->ast_value;
    ast_t * p_ast_value = AST(ast_value);
    voba_str_t* s_ast_value = ast2c_ast(p_ast_value,bk,s);
    voba_str_t * s_match_ret = new_uniq_id();
    voba_str_t * label_success = new_uniq_id();
    voba_value_t match = ast_match->match;
    TEMPLATE(s, VOBA_CONST_CHAR(
                 "    /* start of a match statement */\n"
                 "    voba_value_t #0 __attribute__((unused)) = VOBA_UNDEF; /* return value for match statement */\n")
             ,s_match_ret);
    ast2c_match(s_match_ret, s_ast_value, label_success,match,bk,s);
    TEMPLATE(s,
             VOBA_CONST_CHAR("    #0:;\n"
                             "    /* end of a match statement */\n")
             ,label_success);
    return s_match_ret;
}
static inline voba_str_t* ast2c_match_rule(voba_str_t* v, voba_str_t * ret_id, voba_value_t rule, voba_str_t* label_success, voba_str_t* label_fail, c_backend_t* bk, voba_str_t** s);
static inline void ast2c_match(voba_str_t * s_match_ret, voba_str_t * s_ast_value, voba_str_t * label_success, voba_value_t match, c_backend_t* bk, voba_str_t** s)
{
    match_t * p_match = MATCH(match);
    voba_value_t a_rules = p_match->a_rules;
    int64_t len = voba_array_len(a_rules);
    voba_str_t * label_this = new_uniq_id();
    voba_str_t * old_it = bk->it;
    bk->it = s_ast_value;
    for(int64_t i = 0; i < len; ++i){
        voba_value_t rule = voba_array_at(a_rules,i);
        voba_str_t * label_next = ((i == (len - 1))?label_success:new_uniq_id());
        voba_str_t * s_rule = voba_str_empty();
        ast2c_match_rule(s_ast_value, s_match_ret, rule,label_success, label_next,bk,&s_rule);
        TEMPLATE(s, VOBA_CONST_CHAR("    /* match pattern #2 start*/\n"
                                    "    #0 {\n"
                                    "    #1\n"
                                    "    }\n"
                                    "    /* match pattern #2 end*/ \n")
                 , (i == 0?VOBA_CONST_CHAR("/*empty label*/") : voba_strcat_char(label_this,':'))
                 ,indent(s_rule)
                 ,voba_str_fmt_int64_t(i + 1,10)
            );
        label_this = label_next;
    }
    bk->it = old_it;
    return;
}
static inline void ast2c_match_pattern(voba_str_t* v, voba_str_t* label_fail, voba_value_t pattern,c_backend_t* bk, voba_str_t** s);
static inline void ast2c_match_action(voba_str_t * ret_id, voba_value_t a_ast_action, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_match_rule(voba_str_t* v, voba_str_t * ret_id, voba_value_t rule, voba_str_t* label_success, voba_str_t* label_fail, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = voba_str_empty();
    rule_t* p_rule = RULE(rule);
    ast2c_decl_env(ENV(p_rule->env),bk,s);
    ast2c_match_pattern(v,label_fail,p_rule->pattern,bk,s);
    ast2c_match_action(ret_id,p_rule->a_ast_action,bk,s);
    TEMPLATE(s,
             VOBA_CONST_CHAR("    goto #0; // match goto end\n")
             ,label_success);
    return ret;
}
static inline void ast2c_match_pattern_value(voba_str_t* v, voba_str_t* label_fail, pattern_t* p_pattern,c_backend_t* bk, voba_str_t** s);
static inline void ast2c_match_pattern_else(voba_str_t* v, voba_str_t* label_fail, pattern_t* p_pattern,c_backend_t* bk, voba_str_t** s);
static inline void ast2c_match_pattern_var(voba_str_t* v, voba_str_t* label_fail, pattern_t* p_pattern,c_backend_t* bk, voba_str_t** s);
static inline void ast2c_match_pattern_apply(voba_str_t* v, voba_str_t* label_fail, pattern_t* p_pattern,c_backend_t* bk, voba_str_t** s);
static inline void ast2c_match_pattern_if(voba_value_t ast_if, voba_str_t* label_fail,c_backend_t* bk, voba_str_t** s);
static inline void ast2c_match_pattern(voba_str_t* v, voba_str_t* label_fail, voba_value_t pattern,c_backend_t* bk, voba_str_t** s)
{
    pattern_t * p_pattern = PATTERN(pattern);
    switch(p_pattern->type){
    case PATTERN_VALUE:
        ast2c_match_pattern_value(v,label_fail,p_pattern,bk,s);
        break;
    case PATTERN_VAR:
        ast2c_match_pattern_var(v,label_fail,p_pattern,bk,s);
        break;
    case PATTERN_APPLY:
        ast2c_match_pattern_apply(v,label_fail,p_pattern,bk,s);
        break;
    case PATTERN_ELSE:
        ast2c_match_pattern_else(v,label_fail,p_pattern,bk,s);
        break;
    default:
        assert(0&&"never goes here");
    }
    /* all variables in the enviroment is already defined
     * see ast2c_match_rule ast2c_decl_env
     * they are already set by the corresponding patterns
     */
    if(!voba_is_nil(p_pattern->ast_if)){
        ast2c_match_pattern_if(p_pattern->ast_if,label_fail,bk,s);
    }
    return;
}
static inline void ast2c_match_pattern_if(voba_value_t ast_if, voba_str_t* label_fail,c_backend_t* bk, voba_str_t** s)
{
    ast_t* p_ast = AST(ast_if);
    voba_str_t * s_if = ast2c_ast(p_ast,bk,s);
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "    if(voba_eq(#0,VOBA_FALSE)){/* if pattern guard failed */\n"
                 "         goto #1; /* goto the pattern failed label */\n"
                 "    }\n"
                 )
             ,s_if
             ,label_fail);
    return;
}
static inline void ast2c_match_pattern_value(voba_str_t* v, voba_str_t* label_fail, pattern_t* p_pattern,c_backend_t* bk, voba_str_t** s)
{
    voba_value_t a_ast_exprs = p_pattern->u.value.a_ast_value;
    voba_str_t * expr = ast2c_ast_exprs(a_ast_exprs,bk,s);
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "     if(!voba_match_eq(#0,#1)){\n"
                 "         goto #2; /* match failed */\n"
                 "     }\n")
             ,v,expr,label_fail);
    return;
}
static inline void ast2c_match_pattern_else(voba_str_t* v, voba_str_t* label_fail, pattern_t* p_pattern,c_backend_t* bk, voba_str_t** s)
{
    TEMPLATE(s, VOBA_CONST_CHAR("     /* match else */\n"));
    return;
}
static inline void ast2c_match_pattern_var(voba_str_t* v, voba_str_t* label_fail, pattern_t* p_pattern,c_backend_t* bk, voba_str_t** s)
{
    var_t* var = VAR(p_pattern->u.var.var);
    voba_str_t * c_id = var_c_id(var);
    TEMPLATE(s,
             VOBA_CONST_CHAR("    /* start to match the single variable #0 */\n"
                             "    if(voba_eq(#0,VOBA_UNDEF)){\n"
                             "        #0 = #1; /* when #0 is unbound, bound it*/\n"
                             "    }else{\n"
                             "         /* when #0 is already bound, try to test whether equal*/\n"
                             "         if(!voba_match_eq(#0,#1)){\n"
                             "              goto #2;/*if not equal goto the fail lable */\n"
                             "         }\n"
                             "    }\n"
                             "    /* end to match the single variable #0*/\n"
                 )
             ,c_id, v, label_fail);
    return;
}
static inline void ast2c_match_pattern_apply(voba_str_t* v, voba_str_t* label_fail, pattern_t* p_pattern,c_backend_t* bk, voba_str_t** s)
{
    pattern_apply_t* p_pat = &p_pattern->u.apply;
    voba_value_t ast_cls = p_pat->ast_cls;
    voba_value_t a_patterns = p_pat->a_patterns;
    voba_str_t * s_cls_id = new_uniq_id();
    voba_str_t * tmpf = new_uniq_id();
    voba_str_t * tmpv1 = new_uniq_id();
    voba_str_t * tmpv2 = new_uniq_id();
    voba_str_t * s1 = voba_str_empty();
    voba_str_t * s_cls = ast2c_ast(AST(ast_cls),bk,&s1);
    int64_t len = voba_array_len(a_patterns);
    voba_str_t * s_len = voba_str_fmt_int64_t(len,10);
    TEMPLATE(s,
             VOBA_CONST_CHAR("    /* evaluate cls */\n"
                             "    #0\n"
                             "    voba_value_t #1 = #2; /* assign cls to a variable */\n"
                             "    if(!voba_is_a(#1,voba_cls_cls)){\n"
                             "          goto #3; /* does not match, not a cls object*/\n"
                             "    }\n"
                             "    voba_value_t #5 = voba_gf_lookup(voba_symbol_value(gf_match),#1);\n"
                             "    if(voba_is_nil(#5)){\n"
                             "        goto #3; /* no function registered*/\n"                             
                             "    }\n"
                             "    voba_value_t #7 [] = {4, #1, #4, voba_make_i32(-1), voba_make_i32(#8)};\n"
                             "    voba_value_t #6 = voba_apply(#5, voba_make_array(#7));\n"
                             "    if(!voba_eq(VOBA_TRUE,#6)){\n"
                             "        goto #3; /* number of var does not match*/\n"
                             "    }\n"
                 )
             ,indent(s1) // 0
             ,s_cls_id   //1
             ,s_cls //2
             ,label_fail //3
             ,v //4
             ,tmpf //5
             ,tmpv1 // 6
             ,tmpv2 // 7
             ,s_len // 8
        );
    for(int64_t i = 0; i < len; ++i){
        voba_value_t sp = voba_array_at(a_patterns,i);
        voba_str_t * tmp_args = new_uniq_id();
        voba_str_t * tmp_apply_ret =  new_uniq_id();
        TEMPLATE(s,
                 VOBA_CONST_CHAR(
                     "    /* extract #3 sub-value from the main value*/\n"
                     "    voba_value_t #0 [] = {4, #1, #2, voba_make_i32(#3), voba_make_i32(#4)};\n"
                     "    voba_value_t #5 = voba_apply(#6, voba_make_array(#0));/* the sub-value #3*/\n"
                     )
                 ,tmp_args //0
                 ,s_cls_id //1
                 ,v        //2
                 ,voba_str_fmt_int64_t(i,10) // 3
                 ,s_len // 4
                 ,tmp_apply_ret // 5
                 ,tmpf //6
            );
        ast2c_match_pattern(tmp_apply_ret,label_fail,sp,bk,s);
    }
    return;
}
static inline void ast2c_match_action(voba_str_t * ret_id, voba_value_t a_ast_action, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * s1 = voba_str_empty();
    voba_str_t * expr = ast2c_ast_exprs(a_ast_action,bk,&s1);
    TEMPLATE(s,
             VOBA_CONST_CHAR("    #0\n"
                             "    #1 = #2; /* match statement return value*/\n")
             ,indent(s1),ret_id, expr);
    return;
}
static inline voba_str_t* ast2c_ast_for(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    ast_for_t *  p_ast_for            = &ast->u._for;
    voba_value_t each             = p_ast_for->each;
    ast_t *      p_each           = AST(each);
    voba_value_t match                = p_ast_for->match;
    voba_str_t * for_each_generator   = ast2c_ast(p_each,bk,s);
    voba_str_t * for_if               = voba_is_nil(p_ast_for->_if)?NULL:ast2c_ast(AST(p_ast_for->_if),bk,s);
    voba_str_t * for_init             = voba_is_nil(p_ast_for->init)?NULL:ast2c_ast(AST(p_ast_for->init),bk,s);
    voba_str_t * for_accumulate       = voba_is_nil(p_ast_for->accumulate)?NULL:ast2c_ast(AST(p_ast_for->accumulate),bk,s);
    voba_str_t * for_each_begin       = new_uniq_id();
    voba_str_t * for_each_end         = new_uniq_id();
    voba_str_t * for_end              = new_uniq_id();
    voba_str_t * for_final            = new_uniq_id();
    voba_str_t * for_each_args        = new_uniq_id();
    voba_str_t * for_each_value       = new_uniq_id();
    voba_str_t * for_each_output      = new_uniq_id();
    voba_str_t * for_each_last_output = new_uniq_id();

    
    voba_str_t * s_body = voba_str_empty();
    voba_str_t * old_it = bk->it;
    voba_str_t * old_for_end = bk->latest_for_end_label;
    voba_str_t * old_for_final = bk->latest_for_final;
    
    bk->it = for_each_value;
    bk->latest_for_end_label = for_end;
    bk->latest_for_final = for_final;
    ast2c_match(for_each_output, for_each_value, for_each_end, match, bk,&s_body);
    bk->latest_for_final = old_for_final;
    bk->latest_for_end_label = old_for_end;
    bk->it = old_it;

    /* declare variables */
    TEMPLATE(s,
             VOBA_CONST_CHAR("    voba_value_t #0 __attribute__((unused)) = VOBA_UNDEF;/* return value for `for' statement */\n"
                             "    voba_value_t #1 = VOBA_UNDEF;/* input value of each iteration  */\n"
                             "    voba_value_t #2 __attribute__((unused)) = VOBA_UNDEF;/* output value of each iteration  */\n"
                             "    voba_value_t #3 __attribute__((unused)) = VOBA_UNDEF;/* init value for `for' statement*/\n"
                 ),
             for_final,
             for_each_value,
             for_each_output,
             for_each_last_output);
    if(for_init){
        TEMPLATE(s,
                 VOBA_CONST_CHAR("    #0 = #1 ; /* initialize the default initial value*/\n")
                 , for_each_last_output
                 , for_init);
    }
    /* evaluate `for' input for each iteratation */
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "    #0:{ /*prelude of `for' statement */\n"
                 "    voba_value_t #1 [] = {0}; /* args for iterator*/\n"
                 "    #3 = voba_apply(#2,voba_make_array(#1)); /*invoke the iterator*/\n"
                 "    if(voba_eq(#3,VOBA_DONE)){\n"
                 "        goto #4;/* exit for loop */\n"
                 "    }\n"
                 )
             ,for_each_begin
             ,for_each_args
             ,for_each_generator
             ,for_each_value
             ,for_end
        );
    if(for_if){
        voba_str_t* if_args  = new_uniq_id();
        voba_str_t* if_value = new_uniq_id();
        TEMPLATE(s,
                 VOBA_CONST_CHAR(
                     "    /* try to apply for-if */\n"
                     "    voba_value_t #0 [] = {1, #1}; /* args for `if', the filter*/\n"
                     "    voba_value_t #2 = voba_apply(#3, voba_make_array(#0)); /* for-if */\n"
                     "    if(voba_is_false(#2)){ /* skip this iteration, continue  */\n"
                     "        goto #4;/* for-if failed */\n"
                     "    }\n"
                     )
                 ,if_args
                 ,for_each_value
                 ,if_value
                 ,for_if
                 ,for_each_begin
            );
    }
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "    /*for body begin*/\n"
                 "#0\n"
                 "    /*for body end*/\n"
                 "    #1: /* end label for each iteration */\n"
                 )
             , indent(s_body)
             , for_each_end
        );
    if(for_accumulate){
        voba_str_t* acc_args  = new_uniq_id();
        TEMPLATE(s,
                 VOBA_CONST_CHAR(
                     "    /* collect return value for `for' statement  */\n"
                     "    {voba_value_t #0 [] = {2, #1, #2}; /*arguments for accumulator*/\n"
                     "    #1 = voba_apply(#3, voba_make_array(#0));}/* apply the accumulator */\n"
                     )
                 ,acc_args
                 ,for_each_last_output
                 ,for_each_output
                 ,for_accumulate
            );
    }
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "    #2 = #3;  /* assign the final value for `for' statement */\n"
                 "    goto #0; /* for goto begin */\n"
                 "    #1:  /* end label `for' */\n"
                 "    if(0) goto #1;  /* suppress warning `label at end of compound statement' */\n"
                 "    }\n"
                 )
             ,for_each_begin
             ,for_end
             ,for_final
             ,for_each_last_output
        );
    return for_final;
}

static inline voba_str_t* ast2c_ast_it(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t *  ret  = voba_str_empty();
    if(bk->it != NULL){
        ret = voba_strcat(ret,bk->it);
        ret = voba_strcat(ret,VOBA_CONST_CHAR("/*it*/"));
    }else{
        report_error(VOBA_CONST_CHAR("no appropriate `it' in this context"),ast->u.it.syn_it,bk->toplevel_env);
    }
    return ret;
}
static inline voba_str_t* ast2c_ast_break(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t *  ret  = voba_str_empty();
    voba_str_t * v = ast2c_ast(AST(ast->u._break.ast_value),bk,s);
    voba_str_t * label = bk->latest_for_end_label;
    voba_str_t * bv = bk->latest_for_final;
    if(label){
        TEMPLATE(s,
            VOBA_CONST_CHAR(
                "    #0 = #1; /* break the `for' loop */\n"
                "    goto #2;\n"
                )
                 ,bv
                 ,v
                 ,label
            );
        assert(bv);
        ret = v;
    }else{
        report_error(VOBA_CONST_CHAR("`break' is not in a `for' statement."),ast->u._break.syn_break,bk->toplevel_env);
    }
    return ret;
}
static inline voba_str_t* ast2c_ast_and(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_value_t a_ast_exprs = ast->u.and.a_ast_exprs;
    int64_t len = voba_array_len(a_ast_exprs);

    voba_str_t * and_return_value = new_uniq_id();
    voba_str_t * and_end          = new_uniq_id();    
    TEMPLATE
        (s,
         VOBA_CONST_CHAR(
             "    static voba_value_t #0 = VOBA_UNDEF;/* return value for `and' statement */\n")
         , and_return_value);
    voba_str_t* old_it = bk->it;
    for(int64_t i = 0; i < len; ++i){
        voba_value_t ast_expr = voba_array_at(a_ast_exprs,i);
        assert(voba_is_a(ast_expr,voba_cls_ast));
        voba_str_t * s_ast_expr = ast2c_ast(AST(ast_expr),bk,s);
        bk->it = s_ast_expr;
        TEMPLATE
            (s,
             VOBA_CONST_CHAR(
                 "    #0 = #1; /*set return value for `and'*/\n"
                 "    if(voba_eq(#0,VOBA_FALSE)){/* if any Ok, jump to end*/\n"
                 "        goto #2; /* skip following exprs in `and' */\n"
                 "    }\n"
                 )
             , and_return_value
             , s_ast_expr
             , and_end
                );
    }
    bk->it = old_it;
    TEMPLATE
        (s,
         VOBA_CONST_CHAR(
             "    #0: if(0) goto #0; /* `and` statement end here, suppress warning message */\n")
         , and_end);
    return and_return_value;
}
static inline voba_str_t* ast2c_ast_or(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_value_t a_ast_exprs = ast->u.or.a_ast_exprs;
    int64_t len = voba_array_len(a_ast_exprs);

    voba_str_t * or_return_value = new_uniq_id();
    voba_str_t * or_end          = new_uniq_id();    
    TEMPLATE
        (s,
         VOBA_CONST_CHAR(
             "    static voba_value_t #0 = VOBA_UNDEF;/* return value for `or' statement */\n")
         , or_return_value);
    for(int64_t i = 0; i < len; ++i){
        voba_value_t ast_expr = voba_array_at(a_ast_exprs,i);
        assert(voba_is_a(ast_expr,voba_cls_ast));
        voba_str_t * s_ast_expr = ast2c_ast(AST(ast_expr),bk,s);
        TEMPLATE
            (s,
             VOBA_CONST_CHAR(
                 "    #0 = #1; /*set return value for `or'*/\n"
                 "    if(!voba_eq(#0,VOBA_FALSE)){/* if any Ok, jump to end*/\n"
                 "        goto #2; /* skip following exprs in `or' */\n"
                 "    }\n"
                 )
             , or_return_value
             , s_ast_expr
             , or_end
                );
    }
    TEMPLATE
        (s,
         VOBA_CONST_CHAR(
             "    #0: if(0) goto #0; /* `or` statement end here, suppress warning message */\n")
         , or_end);
    return or_return_value;
}

