#include <stdint.h>
#include <time.h>
#include <assert.h>
#include <voba/value.h>
#include <exec_once.h>
#include <voba/module.h>
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
static inline void ast2c_decl_prelude(c_backend_t* bk,voba_value_t tu_id);
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
    ast2c_decl_prelude(bk, toplevel->full_file_name);
    // EXEC_ONCE_PROGN { for each module; import module; }
    import_modules(toplevel,bk);
    // declare static voba_value_t top_var = VOBA_NIL
    // top_var = m["var_top"]
    ast2c_decl_top_var(ENV(toplevel->env),bk);
    // output all asts
    ast2c_all_asts(toplevel->a_asts,bk);
    return ret;
}
static inline voba_value_t random_module_id(voba_value_t module_name)
{
    time_t xt;
    time(&xt);
    struct tm xtm;
    gmtime_r(&xt,&xtm);
    voba_str_t* ret = voba_str_empty();
    ret = VOBA_STRCAT(ret,
                      voba_value_to_str(module_name),
                      VOBA_CONST_CHAR(" built at "),
                      voba_str_fmt_int32_t(xtm.tm_year + 1900,10),
                      VOBA_CONST_CHAR("-"),
                      voba_str_fmt_int32_t(xtm.tm_mon + 1,10),
                      VOBA_CONST_CHAR("-"),
                      voba_str_fmt_int32_t(xtm.tm_mday,10),
                      VOBA_CONST_CHAR(" "),
                      voba_str_fmt_int32_t(xtm.tm_hour,10),
                      VOBA_CONST_CHAR(":"),
                      voba_str_fmt_int32_t(xtm.tm_min,10),
                      VOBA_CONST_CHAR(":"),
                      voba_str_fmt_int32_t(xtm.tm_sec,10),
                      VOBA_CONST_CHAR("."));
    return voba_make_string(ret);
}
static inline void ast2c_decl_prelude(c_backend_t* bk, voba_value_t tu_id)
{
    TEMPLATE(&bk->decl,
             VOBA_CONST_CHAR("##include <stdint.h>\n"
                             "##include <assert.h>\n"
                             "##define EXEC_ONCE_TU_NAME \"#0\"\n"
                             "##define EXEC_ONCE_DEPENDS {\"voba.module\"};\n"
                             "##include <voba/value.h>\n"
                             "##include <voba/core/builtin.h> // import builtin by default\n"                             
                             "##include <exec_once.h>\n"
                             "##include <voba/module.h>\n"
                             "##define voba_match_eq voba_eql\n"
                 )
             ,voba_value_to_str(random_module_id(tu_id)));
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
            assert(!voba_is_nil(var->u.m.syn_module_id));
            ast2c_import_var(bk,
                             voba_value_to_str(SYNTAX(var->u.m.syn_module_name)->v),
                             voba_value_to_str(SYNTAX(var->u.m.syn_module_id)->v),
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
                 "    #2 = voba_module_var(#3,#0,voba_make_string(voba_str_from_cstr(#1)));\n"
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
    TEMPLATE(&bk->start,
             VOBA_CONST_CHAR(
                 "{\n"
                 "    const char * name = #0;\n"
                 "    const char * id = #1;\n"
                 "    const char * symbols[] = {\n"
                 )
             ,quote_string(voba_value_to_str(SYNTAX(module_info_name(module_info))->v))
             ,quote_string(voba_value_to_str(SYNTAX(module_info_id(module_info))->v)));
    voba_value_t a_syn_symbols = module_info_symbols(module_info);
    int64_t len = voba_array_len(a_syn_symbols);
    for(int64_t i = 0; i < len; ++i){
        TEMPLATE(&bk->start,
                 VOBA_CONST_CHAR(
                     "         #0,\n"
                     )
                 ,quote_string(voba_value_to_str(voba_symbol_name(SYNTAX(voba_array_at(a_syn_symbols,i))->v))));
    }
    TEMPLATE(&bk->start,
             VOBA_CONST_CHAR(
                 "         NULL\n"
                 "    };\n"
                 "    //fprintf(stderr,\"loading %s(%s)\\n\",name,id);\n"
                 "    static voba_value_t                                    \n"
                 "        symbols2[sizeof(symbols)/sizeof(const char*) + 1] = {       \n"
                 "        sizeof(symbols)/sizeof(const char*) - 1, VOBA_NIL,          \n"
                 "    };                                                              \n"
		 "    size_t i;"
                 "    for(i = 0 ; symbols[i]!=NULL; ++i){                      \n"
                 "        symbols2[i+1] =                                             \n"
                 "            voba_make_string(                                       \n"
                 "                    voba_str_from_cstr(symbols[i]));                \n"
                 "    }                                                               \n"
		 "    symbols2[i+1] = VOBA_BOX_END;"
                 "    voba_import_module(name,id,voba_make_tuple(symbols2));          \n"
                 "}\n"));
}

static inline voba_str_t* ast2c_ast(ast_t* ast, c_backend_t * bk, voba_str_t ** s,int last_compound_expr,int last_expr);
static inline void ast2c_all_asts(voba_value_t a_asts, c_backend_t* bk)
{
    int64_t len = voba_array_len(a_asts);
    for(int64_t i = 0; i < len; ++i){
        voba_str_t * s = voba_str_empty();
        voba_value_t ast  = voba_array_at(a_asts,i);
        assert(voba_is_a(ast,voba_cls_ast));
        ast_t * p_ast = AST(ast);
        bk->it = ast2c_ast(p_ast,bk,&s,0/*not last compound expr*/,0/*not last expr*/);
        bk->start = voba_strcat(bk->start,s);
    }
    TEMPLATE(&bk->impl,
        VOBA_CONST_CHAR(
            "#ifndef VOBA_MODULE_DIRTY_HACK\n"
            "voba_value_t voba_init(voba_value_t this_module)\n"
            "#else\n"
            "int main(int argc, char *argv[])\n"
            "#endif\n"
            "{\n"
            "    exec_once_init();\n"
            "    return VOBA_NIL;\n"
            "}\n"
            ));
}
static inline voba_str_t* ast2c_ast_set_var(ast_t* ast, c_backend_t* bk, voba_str_t** s,int last_compound_expr,int last_expr);
static inline voba_str_t* ast2c_ast_constant(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_fun(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_var(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_arg(int32_t index, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_apply(ast_t* ast, c_backend_t* bk, voba_str_t** s,int last_compound_expr,int last_expr);
static inline voba_str_t* ast2c_ast_let(ast_t* ast, c_backend_t* bk, voba_str_t** s,int last_compound_expr);
static inline voba_str_t* ast2c_ast_match(ast_t* ast, c_backend_t* bk, voba_str_t** s,int last_compound_expr);
static inline voba_str_t* ast2c_ast_for(ast_t* ast, c_backend_t* bk, voba_str_t** s,int last_compound_expr);
static inline voba_str_t* ast2c_ast_it(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_break(ast_t* ast, c_backend_t* bk, voba_str_t** s,int last_compound_expr);
static inline voba_str_t* ast2c_ast_and(ast_t* ast, c_backend_t* bk, voba_str_t** s, int last_compound_expr);
static inline voba_str_t* ast2c_ast_or(ast_t* ast, c_backend_t* bk, voba_str_t** s, int last_compound_expr);
static inline voba_str_t* ast2c_ast_yield(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_args(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast(ast_t* ast, c_backend_t * bk, voba_str_t ** s, int last_compound_expr, int last_expr)
{
    voba_str_t* ret = voba_str_empty();
    switch(ast->type){
    case SET_VAR:
	/* bug?  for tail call, the variable is set VOBA_TAIL_CALL,
	   but because the function returns immediately, the
	   variable's value does not make any sense, fix me if I am wrong.
	 */
        ret = ast2c_ast_set_var(ast,bk,s,last_compound_expr,last_expr);
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
        ret = ast2c_ast_apply(ast,bk,s,last_compound_expr,last_expr);
        break;
    case LET:
        ret = ast2c_ast_let(ast,bk,s,last_compound_expr);
        break;
    case MATCH:
        ret = ast2c_ast_match(ast,bk,s,last_compound_expr);
        break;
    case FOR:
        ret = ast2c_ast_for(ast,bk,s,last_compound_expr);
        break;
    case IT:
        ret = ast2c_ast_it(ast,bk,s);
        break;
    case BREAK:
        ret = ast2c_ast_break(ast,bk,s,last_compound_expr);
        break;
    case AND:
        ret = ast2c_ast_and(ast,bk,s,last_compound_expr);
        break;
    case OR:
        ret = ast2c_ast_or(ast,bk,s,last_compound_expr);
        break;
    case YIELD:
        ret = ast2c_ast_yield(ast,bk,s);
        break;
    case ARGS:
        ret = ast2c_ast_args(ast,bk,s);
        break;
    default:
        assert(0 && "never goes here");
    }
    return ret;
}
static inline voba_str_t* ast2c_ast_exprs(voba_value_t exprs, c_backend_t * bk, voba_str_t ** s, int last_compound_expr)
{
    voba_str_t* ret = VOBA_CONST_CHAR("VOBA_NIL");
    int64_t len = voba_array_len(exprs);
    voba_str_t * old_it = bk->it;
    for(int64_t i = 0; i < len; ++i){
        ret = ast2c_ast(AST(voba_array_at(exprs,i)),bk,s, last_compound_expr, i == len -1);
        bk->it = ret;
    }
    bk->it = old_it;
    return ret;
}
static inline voba_str_t* ast2c_ast_exprs_for_fun_body(voba_value_t exprs, c_backend_t * bk, voba_str_t ** s,int is_generator)
{
    voba_str_t* ret = VOBA_CONST_CHAR("VOBA_NIL");
    int64_t len = voba_array_len(exprs);
    voba_str_t * old_it = bk->it;
    const int is_last_expr = 1;
    for(int64_t i = 0; i < len; ++i){
	int last_compound_expr = i==len-1;
	if(is_generator) last_compound_expr = 0; /* generator does not support TCO*/
        ret = ast2c_ast(AST(voba_array_at(exprs,i)),bk,s,last_compound_expr,is_last_expr);
        bk->it = ret;
    }
    bk->it = old_it;
    return ret;
}
static inline voba_str_t* ast2c_ast_set_var(ast_t* ast, c_backend_t * bk, voba_str_t ** s, int last_compound_expr, int last_expr)
{
    voba_value_t exprs = ast->u.set_var.a_ast_exprs;
    var_t * var = ast->u.set_var.var;
    voba_str_t* expr = ast2c_ast_exprs(exprs,bk,s,last_compound_expr);
    voba_str_t * ret = new_uniq_id(voba_value_to_str(voba_symbol_name(SYNTAX(var->syn_s_name)->v)));
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
                 VOBA_CONST_CHAR("    voba_tuple_set_at(fun_args,#0,#1); /* set #2 */\n"),
                 voba_str_fmt_uint32_t(var->u.index,10),ret,var_c_symbol_name(var));
        break;
    case VAR_CLOSURE:
        TEMPLATE(s,
                 VOBA_CONST_CHAR("    voba_tuple_set_at(fun,#0,#1); /* set #2 */\n"),
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
    voba_str_t* s_const = new_uniq_id(voba_str_from_cstr("const_array"));
    voba_str_t* s_c_const = new_uniq_id(voba_str_from_cstr("const_tuple_in_c"));
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
            ",VOBA_BOX_END};\n"
            "    #0 = voba_array_from_tuple(voba_make_tuple(#1)); /* constant */\n"
            ),
        s_const,
        s_c_const);
    bk->start = voba_strcat(bk->start,s0);
    return s_const;
}
static inline voba_str_t* ast2c_ast_fun_body(ast_t* ast, c_backend_t* bk, voba_str_t** s, int is_generator);
static inline voba_str_t* ast2c_ast_generator(c_backend_t* bk, voba_str_t* gname);
static inline voba_str_t* ast2c_ast_fun_closure(c_backend_t *bk
                                                ,voba_str_t**s
                                                ,voba_value_t a_var_C
                                                ,voba_str_t* fun_name);
static inline voba_str_t* ast2c_ast_fun(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t* ret = voba_str_empty();
    voba_str_t* str_fun_name = NULL;
    ast_fun_t* ast_fn = &ast->u.fun;
    int64_t len = voba_array_len(ast_fn->f->a_var_C);
    int is_a_closure = (len != 0);
    str_fun_name = ast2c_ast_fun_body(ast,bk,s,ast_fn->f->is_generator);
    if(!ast_fn->f->is_generator){
        if(!is_a_closure){
            TEMPLATE(&ret, VOBA_CONST_CHAR("voba_make_func(#0)"),str_fun_name);
        }else{
            ret = ast2c_ast_fun_closure(bk,s,ast_fn->f->a_var_C,str_fun_name);
        }
    }else{
        // a generator is also a closure?
        if(len == 0){
            ret = ast2c_ast_generator(bk,str_fun_name);
        }else{
            assert(0 && " not implemented yet ");
        }
    }
    return ret;
}
static inline voba_str_t* ast2c_ast_var_var(var_t* var, c_backend_t* bk, voba_str_t** s);
static inline voba_str_t* ast2c_ast_fun_closure(c_backend_t *bk
                                                ,voba_str_t** s
                                                ,voba_value_t a_var_C
                                                ,voba_str_t* fun_name)
{
    voba_str_t * ret = voba_str_empty();
    int64_t len = voba_array_len(a_var_C);
    voba_str_t * p_for_closure = new_uniq_id(voba_str_from_cstr("p_for_closure"));
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "    /* memory block for the closure */\n"
                 "    voba_value_t* #0  = voba_alloc(#1);\n"
                 "    #0[0] = voba_make_func(#2);\n"
                 "    #0[1] = #1 - 2;\n"
                 )
             ,p_for_closure
             ,voba_str_fmt_int32_t((int32_t)len + 2,10)
             ,fun_name
        );
    for(int64_t i = 0; i < len ; ++i){
        voba_value_t var_obj = voba_array_at(a_var_C,i);
        voba_str_t * var = ast2c_ast_var_var(VAR(var_obj),bk,&ret);
        TEMPLATE(s,
                 VOBA_CONST_CHAR(
                     "    #0[#1+2] = #2;\n")
                 ,p_for_closure
                 ,voba_str_fmt_int32_t((int32_t)i,10)
                 ,var);
    }
    TEMPLATE(&ret,
             VOBA_CONST_CHAR(
                 "   voba_from_pointer((void*)#0,VOBA_TYPE_CLOSURE);\n"
                 )
             , p_for_closure);
    return ret;
}

static inline voba_str_t* ast2c_ast_fun_body(ast_t* ast, c_backend_t* bk, voba_str_t** s, int is_generator)
{
    ast_fun_t* ast_fn = &(ast->u.fun);
    voba_value_t syn_s_name = ast_fn->syn_s_name;
    voba_str_t * fun_name = new_uniq_id(voba_is_nil(syn_s_name)?
                                    voba_str_from_cstr("anonymous"):
                                    voba_value_to_str(voba_symbol_name(SYNTAX(syn_s_name)->v))
        );
    voba_str_t * s1 = voba_str_empty();
    voba_str_t * stream_fun_body = voba_str_empty();
    voba_value_t exprs = ast_fn->a_ast_exprs;
    voba_str_t* expr = ast2c_ast_exprs_for_fun_body(exprs,bk,&stream_fun_body,is_generator);
    if(!is_generator){
	TEMPLATE(&bk->decl,
		 VOBA_CONST_CHAR("VOBA_FUNC voba_value_t #0 (voba_value_t fun, voba_value_t fun_args, voba_value_t* next_fun, voba_value_t next_args[]);\n")
		 ,fun_name);
	TEMPLATE(&s1,
		 VOBA_CONST_CHAR("VOBA_FUNC voba_value_t #0 (voba_value_t fun, voba_value_t fun_args, voba_value_t* next_fun, voba_value_t next_args[])\n"
				 "{\n"
				 "#1"
				 "    return #2; /* return #0 */\n"
				 "}\n")
		 ,fun_name,indent(stream_fun_body),expr);
	bk->impl = voba_strcat(bk->impl, s1);
    }else{
	/* generator does not support TCO, so that there is no next_fun and next_args */
	TEMPLATE(&bk->decl,
		 VOBA_CONST_CHAR("VOBA_FUNC voba_value_t #0 (voba_value_t fun, voba_value_t fun_args);\n")
		 ,fun_name);
	TEMPLATE(&s1,
		 VOBA_CONST_CHAR("VOBA_FUNC voba_value_t #0 (voba_value_t fun, voba_value_t fun_args)\n"
				 "{\n"
				 "#1"
				 "    return #2; /* return #0 */\n"
				 "}\n")
		 ,fun_name,indent(stream_fun_body),expr);
	bk->impl = voba_strcat(bk->impl, s1);
    }
    return fun_name;
}
static inline voba_str_t* ast2c_ast_generator(c_backend_t* bk, voba_str_t* gname)
{
    voba_str_t * fname = new_uniq_id(gname);
    TEMPLATE(&bk->decl,
             VOBA_CONST_CHAR("VOBA_FUNC voba_value_t #0 (voba_value_t fun, voba_value_t fun_args, voba_value_t* next_fun, voba_value_t next_args[]);\n")
             ,fname);
    TEMPLATE(&bk->impl,
             VOBA_CONST_CHAR("VOBA_FUNC voba_value_t #0 (voba_value_t fun, voba_value_t fun_args, voba_value_t* next_fun, voba_value_t next_args[])\n"
                             "{\n"
                             " /* a bridge to create a generator */\n"
                             "    return voba_make_generator(#1, fun, voba_tuple_copy(fun_args));\n"
                             "}\n")
             ,fname,gname);
    voba_str_t * s = voba_str_empty();
    TEMPLATE(&s,
             VOBA_CONST_CHAR("voba_make_func(#0)")
             ,fname);
    return s;
}
static inline voba_str_t* ast2c_ast_arg(int32_t index, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = voba_str_empty();
    TEMPLATE(&ret,
             VOBA_CONST_CHAR("voba_tuple_at(fun_args,#0)")
             ,voba_str_fmt_int32_t(index,10));
    return ret;
}
static inline voba_str_t* ast2c_ast_closure(int32_t index, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = voba_str_empty();
    TEMPLATE(&ret,
             VOBA_CONST_CHAR("voba_tuple_at(fun,#0)")
             ,voba_str_fmt_int32_t(index,10));
    return ret;
}

static inline voba_str_t* ast2c_ast_var(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    ast_var_t* tv = &ast->u.var;
    var_t * var = tv->var;
    return ast2c_ast_var_var(var,bk,s);
}
static inline voba_str_t* ast2c_ast_var_var(var_t* var, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t* ret = voba_str_empty();
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
static inline voba_str_t* ast2c_ast_apply(ast_t* ast, c_backend_t* bk, voba_str_t** s, int last_compound_expr, int last_expr)
{
    voba_str_t * ret = new_uniq_id(voba_str_from_cstr("apply_ret"));
    voba_value_t exprs = ast->u.apply.a_ast_exprs;
    voba_value_t args = voba_make_array_0();
    int64_t len = voba_array_len(exprs);
    voba_str_t * args_name = new_uniq_id(voba_str_from_cstr("apply_args"));
    assert(len >=1);
    voba_str_t * old_it = bk->it;
    for(int64_t i = 0; i < len; ++i){
        voba_value_t expr = voba_array_at(exprs,i);
        voba_str_t * sexpr = ast2c_ast(AST(expr), bk, s, last_compound_expr, 0); // not the last expr
        if(i > 0){
            bk->it = sexpr;
        }
        voba_array_push(args,voba_make_string(sexpr));
    }
    bk->it = old_it;
    voba_str_t * fun = voba_value_to_str(voba_array_at(args,0));
    if(last_compound_expr == 1 && last_expr == 1 && (len <= VOBA_MAX_NUM_OF_TAIL_CALL_ARGS)){
    // When VOBA_MAX_NUM_OF_TAIL_CALL_ARGS == 20, `voba_apply`
	// reserves a memory space for tuple with 20 elements at most,
	// we can only apply tail call when the number of argument is
	// less than 20, otherwise, `voba_apply` don't know arguments,
	// because `voba_apply` assumes `next_args` is filled in
	// properly here.
	TEMPLATE(s,
		 VOBA_CONST_CHAR(
		     "    voba_value_t #0 = VOBA_TAIL_CALL;/* tail call for `apply` */\n"
		     "    next_args[0] = #1;/* set the number of argument for tail call.*/\n"
		     "    *next_fun = #2;/*set the next function to call*/\n"
		     )
		 ,ret
		 ,voba_str_fmt_int64_t(len-1,10)
		 ,fun
	    );
	TEMPLATE(s,
		 VOBA_CONST_CHAR(
		     "    /* start to fill in argument for tail call */\n"
		     ));
	for(int64_t i = 1; i < len; ++i){
	    TEMPLATE(s,
		     VOBA_CONST_CHAR(
			 "    next_args[#0] = #1; /* argument #0 */\n")
		     , voba_str_fmt_int64_t(i,10)
		     , voba_value_to_str(voba_array_at(args,i))
		);
	}
	TEMPLATE(s,
		 VOBA_CONST_CHAR(
		     "    next_args[#0] = VOBA_BOX_END; /* argument #0 */\n")
		 , voba_str_fmt_int64_t(len,10));
	
    }else{
	// non tail call
	TEMPLATE(s,
		 VOBA_CONST_CHAR("    voba_value_t #0 __attribute__((unused)) = VOBA_UNDEF;/* return value for apply */\n"
				 "    voba_value_t #1 [] = { #2 /* prepare arguments for apply */\n")
		 ,ret
		 ,args_name
		 ,voba_str_fmt_int64_t(len-1,10));
	for(int64_t i = 1; i < len; ++i){
	    TEMPLATE(s,
		     VOBA_CONST_CHAR("         ,#0 /* argument #1 */\n")
		     ,voba_value_to_str(voba_array_at(args,i))
		     ,voba_str_fmt_int64_t(i,10));
	}
	TEMPLATE(s,
		 VOBA_CONST_CHAR("    , VOBA_BOX_END };\n"
				 "    #0 = voba_apply(#1,voba_make_tuple(#2));/* return value for apply */\n"
		     )
		 ,ret, fun, args_name);
    }
    return ret;
}
static inline voba_str_t* ast2c_ast_let(ast_t* ast, c_backend_t* bk, voba_str_t** s, int last_compound_expr)
{
    ast_let_t * ast_let  = &ast->u.let;
    voba_str_t * id = new_uniq_id(voba_str_from_cstr("let_ret"));
    voba_str_t * s1 = voba_str_empty();
    voba_str_t * s2 = voba_str_empty();
    ast2c_decl_env(ast_let->env,bk,&s1);
    voba_value_t a_ast_exprs = ast_let->a_ast_exprs;
    voba_str_t * body = ast2c_ast_exprs(a_ast_exprs,bk,&s2,last_compound_expr);
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
/** @brief generate c code for `match` form
    @param ast the AST object
    @param bk  the backend object
    @param s   the current output stream

a match form is as below

@verbatim
(match <match-value> 
  <match-rule> ...)

<match-rule> :: (<pattern> <exprs> ...)
<exprs>... is also called <action>
<pattern> :: <constant> | <var> | <apply> | <value>

<constant>:: <integer> | <float> | <string> | <quote-form>
<value> :: (value expr...)
<var>   :: <a symbol>
<apply> :: (<first> <sub-pattern> ....)

<first> is evaluated and return value is expected to be callable. 
1. it is called to test whether the number of <sub-pattern>s are matched,
(first <match-value> -1 <number of sub-patterns>)
2. it is called subsequencely to extract sub-match value as below
(first <match-value> n <number of sub-pattern>) where n is 0 to <number of subpattern> -1
3. match each sub-match value agaist sub-pattern.
@endverbatim


*/
static inline voba_str_t* ast2c_ast_match(ast_t* ast, c_backend_t* bk, voba_str_t** s, int last_compound_expr);
/** @brief generate c code `match` form

    @param match_ret the name of return value of `match` form
    @param match_value the value of `match` form, which is matched agaist.
    @param label_success if match is OK, goto this label, for the whole `match` form
    @param label_fail   if match is fail, goto this label, for the whole `match` form
    @param match the `match` object
    @param bk same as usual
    @param s  same as usual

    house keep around the whole `match` form.

 */
static inline void ast2c_match(voba_str_t * match_ret,
                               voba_str_t * match_value,
                               voba_str_t * label_success,
                               voba_str_t * label_fail,
                               voba_value_t match,
                               c_backend_t* bk,
                               voba_str_t** s,
			       int last_compound_expr);
/** @brief generate c code for one of rule in a `match` form 

    @param match_value the vale of `match` form, which is matched agaist.
    @param match_ret the name of return value of `match` form.
    @param rule the `rule` object.
 */
static inline voba_str_t* ast2c_match_rule(voba_str_t* match_value,
                                           voba_str_t * match_ret,
                                           voba_value_t rule,
                                           voba_str_t* label_success,
                                           voba_str_t* label_fail,
                                           c_backend_t* bk,
                                           voba_str_t** s,
					   int last_compound_expr);

static inline void ast2c_match_pattern(voba_str_t* v,
				       voba_str_t* label_fail,
				       voba_value_t pattern,
				       c_backend_t* bk,
				       voba_str_t** s);
static inline void ast2c_match_action(voba_str_t * match_ret,
				      voba_value_t a_ast_action,
				      c_backend_t* bk,
				      voba_str_t** s,
				      int last_compound_expr);
static inline void ast2c_match_pattern_value(voba_str_t* v,
                                             voba_str_t* label_fail,
                                             pattern_t* p_pattern,
                                             c_backend_t* bk,
                                             voba_str_t** s);
static inline void ast2c_match_pattern_else(voba_str_t* v,
                                            voba_str_t* label_fail,
                                            pattern_t* p_pattern,
                                            c_backend_t* bk,
                                            voba_str_t** s);
static inline void ast2c_match_pattern_var(voba_str_t* v,
                                           voba_str_t* label_fail,
                                           pattern_t* p_pattern,
                                           c_backend_t* bk,
                                           voba_str_t** s);
static inline void ast2c_match_pattern_apply(voba_str_t* v,
                                             voba_str_t* label_fail,
                                             pattern_t* p_pattern,
                                             c_backend_t* bk,
                                             voba_str_t** s);
static inline void ast2c_match_pattern_if(voba_value_t ast_if,
                                          voba_str_t* label_fail,
                                          c_backend_t* bk,
                                          voba_str_t** s);
/// -------------------------------------------------------------------
static inline voba_str_t* ast2c_ast_match(ast_t* ast,
                                          c_backend_t* bk,
                                          voba_str_t** s,
					  int last_compound_expr)
{
    voba_str_t * match_value = ast2c_ast(AST(ast->u.match.ast_value),bk,s,last_compound_expr,0 /* not the last expr*/);
    voba_str_t * match_ret = new_uniq_id(voba_str_from_cstr("match_ret"));
    voba_str_t * label_success = new_uniq_id(voba_str_from_cstr("match_label_success"));
    voba_str_t * label_failure = new_uniq_id(voba_str_from_cstr("match_label_failure"));
    voba_value_t match = ast->u.match.match;
    TEMPLATE(s, VOBA_CONST_CHAR(
                 "    /* start of a match statement */\n"
                 "    voba_value_t #0 __attribute__((unused)) = VOBA_UNDEF; /* return value for match statement */\n")
             ,match_ret);
    ast2c_match(match_ret,
		match_value,
                label_success,
		label_failure,
                match, bk,s,last_compound_expr);
    /// @todo better debug information when no match happen.
    TEMPLATE(s,
             VOBA_CONST_CHAR( "    #0:if(0) goto #0;/*suppress warning*/; /* the whole match statement failed. */\n"
                              "    voba_throw_exception(voba_make_string(voba_str_from_cstr(\"no match\")));\n"
                              "    #1:; /* the whole match statement success */\n"
                              "    /* end of a match statement */\n")
             ,label_failure
             ,label_success
        );
    return match_ret;
}
static inline void ast2c_match(voba_str_t * match_ret,
                               voba_str_t * match_value,
                               voba_str_t * label_success,
                               voba_str_t * label_fail,
                               voba_value_t match,
                               c_backend_t* bk,
                               voba_str_t** s,
			       int last_compound_expr)
{
    match_t * p_match = MATCH(match);
    voba_value_t a_rules = p_match->a_rules;
    int64_t len = voba_array_len(a_rules);
    voba_str_t * rule_label =
	new_uniq_id(voba_strcat(voba_str_from_cstr("rule_label_")
				,voba_str_fmt_int32_t(0,10)));
    voba_str_t * old_it = bk->it;
    bk->it = match_value;
    for(int64_t i = 0; i < len; ++i){
        voba_value_t rule = voba_array_at(a_rules,i);
	// if it is the last rule, the fail label is the label for the
	// whole `match` form, otherwise, it is the start label of the
	// next rule.
        voba_str_t * label_next = ((i == (len - 1))?
				   label_fail:
				   new_uniq_id(voba_strcat(voba_str_from_cstr("rule_label_")
							   ,voba_str_fmt_int32_t(i,10))));
	voba_str_t * s_rule = voba_str_empty();
	ast2c_match_rule(match_value, match_ret, rule, label_success, label_next,bk,&s_rule,last_compound_expr);
        TEMPLATE(s, VOBA_CONST_CHAR("    /* match rule #2 start*/\n"
                                    "    #0 {\n"
                                    "    #1\n"
                                    "    }\n"
                                    "    /* match rule #2 end*/ \n")
                 , (i == 0?VOBA_CONST_CHAR("/*empty label*/") : voba_strcat_char(rule_label,':'))
                 ,indent(s_rule)
                 ,voba_str_fmt_int64_t(i + 1,10)
            );
        rule_label = label_next;
    }
    bk->it = old_it;
    return;
}
static inline voba_str_t* ast2c_match_rule(voba_str_t* v,
                                           voba_str_t * match_ret,
                                           voba_value_t rule,
                                           voba_str_t* label_success,
                                           voba_str_t* label_fail,
                                           c_backend_t* bk,
                                           voba_str_t** s,
					   int last_compound_expr)
{
    voba_str_t * ret = voba_str_empty();
    rule_t* p_rule = RULE(rule);
    ast2c_decl_env(ENV(p_rule->env),bk,s);
    ast2c_match_pattern(v,label_fail,p_rule->pattern,bk,s);
    ast2c_match_action(match_ret,p_rule->a_ast_action,bk,s,last_compound_expr);
    TEMPLATE(s,
             VOBA_CONST_CHAR("    goto #0; // match goto end\n")
             ,label_success);
    return ret;
}
static inline void ast2c_match_pattern(voba_str_t* v,
                                       voba_str_t* label_fail,
                                       voba_value_t pattern,
                                       c_backend_t* bk,
                                       voba_str_t** s)
{
    /* no tail call check, because it will never be the last compound expression */
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
static inline void ast2c_match_pattern_if(voba_value_t ast_if,
                                          voba_str_t* label_fail,
                                          c_backend_t* bk,
                                          voba_str_t** s)
{
    ast_t* p_ast = AST(ast_if);
    voba_str_t * s_if = ast2c_ast(p_ast,bk,s,0/*not last compound*/,0/*not last expr*/);
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
static inline void ast2c_match_pattern_value(voba_str_t* v,
                                             voba_str_t* label_fail,
                                             pattern_t* p_pattern,
                                             c_backend_t* bk,
                                             voba_str_t** s)
{
    voba_value_t a_ast_exprs = p_pattern->u.value.a_ast_value;
    voba_str_t * expr = ast2c_ast_exprs(a_ast_exprs,bk,s,0/*not last compound*/);
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "     if(!voba_match_eq(#0,#1)){\n"
                 "         goto #2; /* match failed */\n"
                 "     }\n")
             ,v,expr,label_fail);
    return;
}
static inline void ast2c_match_pattern_else(voba_str_t* v,
                                            voba_str_t* label_fail,
                                            pattern_t* p_pattern,
                                            c_backend_t* bk,
                                            voba_str_t** s)
{
    TEMPLATE(s, VOBA_CONST_CHAR("     /* match else */\n"));
    return;
}
static inline void ast2c_match_pattern_var(voba_str_t* v,
                                           voba_str_t* label_fail,
                                           pattern_t* p_pattern,
                                           c_backend_t* bk,
                                           voba_str_t** s)
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
static inline void ast2c_match_pattern_apply(voba_str_t* v,
                                             voba_str_t* label_fail,
                                             pattern_t* p_pattern,
                                             c_backend_t* bk,
                                             voba_str_t** s)
{
    pattern_apply_t* p_pat = &p_pattern->u.apply;
    voba_value_t ast_cls = p_pat->ast_cls;
    voba_value_t a_patterns = p_pat->a_patterns;
    voba_str_t * s_first_elt_body = voba_str_empty();
    voba_str_t * s_first_elt = ast2c_ast(AST(ast_cls),bk,&s_first_elt_body,0/* not last compound expr*/,0/* last expr*/);
    int64_t len = voba_array_len(a_patterns);
    voba_str_t * s_len = voba_str_fmt_int64_t(len,10);
    voba_str_t * pat_ret = new_uniq_id(voba_str_from_cstr("pattern_ret"));
    voba_str_t * pat_args = new_uniq_id(voba_str_from_cstr("pattern_args"));
    TEMPLATE(s,
             VOBA_CONST_CHAR("    /* evaluate first element of pattern */\n"
                             "    #0\n"
                             "    voba_value_t #1 [] = {3, #2, voba_make_i32(-1), voba_make_i32(#3), VOBA_BOX_END};\n"
                             "    voba_value_t #4 = voba_apply(#5, voba_make_tuple(#1));\n"
                             "    if(!voba_eq(VOBA_TRUE,#4)){\n"
                             "        goto #6; /* number of var does not match*/\n"
                             "    }\n"
                 )
             ,indent(s_first_elt_body) // 0
             ,pat_args // 1
             ,v // 2
             ,s_len // 3
             ,pat_ret //4
             ,s_first_elt //5
             ,label_fail //6
        );
    for(int64_t i = 0; i < len; ++i){
        voba_value_t sp = voba_array_at(a_patterns,i);
        voba_str_t * pat_args = new_uniq_id(voba_str_from_cstr("pattern_args"));
        voba_str_t * pat_ret =  new_uniq_id(voba_str_from_cstr("pattern_ret"));
        TEMPLATE(s,
                 VOBA_CONST_CHAR(
                     "    /* extract #2 of #3 sub-value from the main value*/\n"
                     "    voba_value_t #0 [] = {3, #1,voba_make_i32(#2), voba_make_i32(#3), VOBA_BOX_END};\n"
                     "    voba_value_t #4 = voba_apply(#5,voba_make_tuple(#0));/* the sub-value #2 of #3*/\n"
                     )
                 , pat_args // 0
                 , v // 1
                 , voba_str_fmt_int64_t(i,10) // 2
                 , s_len // 3
                 , pat_ret // 4
                 , s_first_elt // 5
            );
        ast2c_match_pattern(pat_ret,label_fail,sp,bk,s);
    }
    return;
}
static inline void ast2c_match_action(voba_str_t * match_ret,
                                      voba_value_t a_ast_action,
                                      c_backend_t* bk,
                                      voba_str_t** s,
				      int last_compound_expr)
{
    voba_str_t * s1 = voba_str_empty();
    voba_str_t * expr = ast2c_ast_exprs(a_ast_action,bk,&s1,last_compound_expr);
    TEMPLATE(s,
             VOBA_CONST_CHAR("    #0\n"
                             "    #1 = #2; /* match statement return value*/\n")
             ,indent(s1),match_ret, expr);
    return;
}
static inline voba_str_t* ast2c_ast_for(ast_t* ast, c_backend_t* bk, voba_str_t** s,int last_compound_expr)
{
    // @TODO tail call checking is ignored for `for` form
    ast_for_t *  p_ast_for            = &ast->u._for;
    voba_value_t each             = p_ast_for->each;
    ast_t *      p_each           = AST(each);
    voba_value_t match                = p_ast_for->match;
    voba_str_t * for_each_expr        = ast2c_ast(p_each,bk,s,0/*not the last compound expr*/,0 /*not the last*/ );
    voba_str_t * for_if               = voba_is_nil(p_ast_for->_if)?NULL:ast2c_ast(AST(p_ast_for->_if),bk,s,0/*not the last compound expr*/,0 /*not the last*/);
    voba_str_t * for_init             = voba_is_nil(p_ast_for->init)?NULL:ast2c_ast(AST(p_ast_for->init),bk,s,0/*not the last compound expr*/,0 /*not the last*/);
    voba_str_t * for_accumulate       = voba_is_nil(p_ast_for->accumulate)?NULL:ast2c_ast(AST(p_ast_for->accumulate),bk,s,0/*not the last compound expr*/,0 /*not the last*/);
    voba_str_t * for_each_begin       = new_uniq_id(voba_str_from_cstr("for_each_begin"));
    voba_str_t * for_each_end_success = new_uniq_id(voba_str_from_cstr("for_each_end_match_success"));
    voba_str_t * for_each_end_failure = new_uniq_id(voba_str_from_cstr("for_each_end_match_failure"));
    voba_str_t * for_end              = new_uniq_id(voba_str_from_cstr("for_end"));
    voba_str_t * for_ret_value            = new_uniq_id(voba_str_from_cstr("for_ret_value"));
    //voba_str_t * for_each_args        = new_uniq_id(voba_str_from_cstr("for_each_args"));
    voba_str_t * for_each_input       = new_uniq_id(voba_str_from_cstr("for_each_input"));
    voba_str_t * for_each_iter        = new_uniq_id(voba_str_from_cstr("for_each_iter"));
    voba_str_t * for_each_iter_f      = new_uniq_id(voba_str_from_cstr("for_each_iter_f"));
    voba_str_t * for_each_iter_self      = new_uniq_id(voba_str_from_cstr("for_each_iter_self"));
    voba_str_t * for_each_output      = new_uniq_id(voba_str_from_cstr("for_each_output"));
    voba_str_t * for_each_last_output = new_uniq_id(voba_str_from_cstr("for_each_last_output"));
    voba_str_t * for_each_iter_args   = new_uniq_id(voba_str_from_cstr("for_each_iter_args"));

    
    voba_str_t * s_body = voba_str_empty();
    voba_str_t * old_it = bk->it;
    voba_str_t * old_for_end = bk->latest_for_end_label;
    voba_str_t * old_for_ret_value = bk->latest_for_final;
    
    bk->it = for_each_input;
    bk->latest_for_end_label = for_end;
    bk->latest_for_final = for_ret_value;
    ast2c_match(for_each_output, for_each_input,for_each_end_success,for_each_end_failure,match, bk,&s_body
		, 0 /* not the last compound expr*/);
    bk->latest_for_final = old_for_ret_value;
    bk->latest_for_end_label = old_for_end;
    bk->it = old_it;

    /* declare variables */
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "    /* return value for `for' statement */\n"
                 "    voba_value_t #0 __attribute__((unused)) = VOBA_UNDEF;\n")
             ,for_ret_value);
    
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "    /* output value of each iteration */\n"
                 "    voba_value_t #0 __attribute__((unused)) = VOBA_UNDEF;\n")
             ,for_each_output);
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "    /* value specified by :init keyword */\n"
                 "    voba_value_t #0 __attribute__((unused)) = VOBA_UNDEF;\n")
             , for_each_last_output);
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "    /* for-each-iterator, specified by :each keyword*/\n"
                 "    voba_value_t #0 __attribute__((unused)) = VOBA_UNDEF;/* */\n")
             ,for_each_iter);
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "    /* arguments for for-each-iterator to get for-each-input*/\n"
                 "    voba_value_t #0 [] __attribute__((unused)) = {1,VOBA_NIL,VOBA_BOX_END};\n")
             ,for_each_iter_args);
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "    /* function pointer of for-each-iterator */\n"
                 "    voba_func_t #0 __attribute__((unused)) = NULL;\n")
             ,for_each_iter_f);
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "    /* function self of for-each-iterator */\n"
                 "    voba_value_t #0 __attribute__((unused)) = VOBA_NIL;\n")
             ,for_each_iter_self);
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "    /* for-each-input, the return value of for-each-iterator.*/\n"
                 "    voba_value_t #0 __attribute__((unused)) = VOBA_NIL;\n")
             ,for_each_input);
    TEMPLATE(s
             ,VOBA_CONST_CHAR(
                 "    #1 = #0;/* get for-each-iterator from for-each-value */\n"
                 )
             , for_each_expr
             , for_each_iter);
    if(for_init){
        TEMPLATE(s,
                 VOBA_CONST_CHAR(
                     "    #0 = #1 ; /* initialize the default initial value*/\n"
                     )
                 , for_each_last_output //  #0
                 , for_init     // #1
            );
    }
    /* evaluate `for' input for each iteratation */
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "    #0:{ /*prelude of `for' statement */\n"
                 "    #3 = voba_apply(#1, voba_make_tuple(#2)); /*invoke the iterator to get the input value*/\n"
                 "    if(voba_eq(#3,VOBA_DONE)){\n"
                 "        goto #4;/* exit for loop */\n"
                 "    }\n"
                 )
             ,for_each_begin // #0
             ,for_each_iter  // #1
             ,for_each_iter_args // #2
             ,for_each_input // #3
             ,for_end // #4
        ); 
    if(for_if){
        /// @todo speed it up.
        voba_str_t* if_args  = new_uniq_id(voba_str_from_cstr("if_args"));
        voba_str_t* if_value = new_uniq_id(voba_str_from_cstr("if_value"));
        TEMPLATE(s,
                 VOBA_CONST_CHAR(
                     "    /* try to apply for-if */\n"
                     "    voba_value_t #0 [] = {1, #1, VOBA_BOX_END}; /* args for `if', the filter*/\n"
                     "    voba_value_t #2 = voba_apply(#3, voba_make_tuple(#0)); /* for-if */\n"
                     "    if(voba_is_false(#2)){ /* skip this iteration, continue  */\n"
                     "        goto #4;/* for-if failed */\n"
                     "    }\n"
                     )
                 ,if_args
                 ,for_each_input
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
		 "    #1: /* end label for each iteration if match failure*/\n"
		 "    voba_throw_exception(voba_make_string(voba_str_from_cstr(\"no match\")));\n"		 
                 "    #2: /* end label for each iteration if match success*/\n"
                 )
             , indent(s_body)
             , for_each_end_failure
	     , for_each_end_success
        );
    if(for_accumulate){
        /// @todo speed it up
        voba_str_t* acc_args  = new_uniq_id(voba_str_from_cstr("acc_args"));
        TEMPLATE(s,
                 VOBA_CONST_CHAR(
                     "    /* collect return value for `for' statement  */\n"
                     "    {voba_value_t #0 [] = {2, #1, #2, VOBA_BOX_END}; /*arguments for accumulator*/\n"
                     "    #1 = voba_apply(#3, voba_make_tuple(#0));}/* apply the accumulator */\n"
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
             ,for_ret_value
             ,for_each_last_output
        );
    return for_ret_value;
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
static inline voba_str_t* ast2c_ast_break(ast_t* ast, c_backend_t* bk, voba_str_t** s, int last_compound_expr)
{
    voba_str_t *  ret  = voba_str_empty();
    voba_str_t * v = ast2c_ast(AST(ast->u._break.ast_value),bk,s,last_compound_expr, 1 /* last expr */);
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
static inline voba_str_t* ast2c_ast_and(ast_t* ast, c_backend_t* bk, voba_str_t** s,int last_compound_expr)
{
    voba_value_t a_ast_exprs = ast->u.and.a_ast_exprs;
    int64_t len = voba_array_len(a_ast_exprs);

    voba_str_t * and_return_value = new_uniq_id(voba_str_from_cstr("and_ret_val"));
    voba_str_t * and_end          = new_uniq_id(voba_str_from_cstr("and_end"));    
    TEMPLATE
        (s,
         VOBA_CONST_CHAR(
             "      voba_value_t #0 = VOBA_UNDEF;/* return value for `and' statement */\n")
         , and_return_value);
    voba_str_t* old_it = bk->it;
    for(int64_t i = 0; i < len; ++i){
        voba_value_t ast_expr = voba_array_at(a_ast_exprs,i);
        assert(voba_is_a(ast_expr,voba_cls_ast));
        voba_str_t * s_ast_expr = ast2c_ast(AST(ast_expr),bk,s,last_compound_expr, i == len - 1);
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
static inline voba_str_t* ast2c_ast_or(ast_t* ast, c_backend_t* bk, voba_str_t** s, int last_compound_expr)
{
    voba_value_t a_ast_exprs = ast->u.or.a_ast_exprs;
    int64_t len = voba_array_len(a_ast_exprs);

    voba_str_t * or_return_value = new_uniq_id(voba_str_from_cstr("or_ret_val"));
    voba_str_t * or_end          = new_uniq_id(voba_str_from_cstr("or_end"));    
    TEMPLATE
        (s,
         VOBA_CONST_CHAR(
             "       voba_value_t #0 = VOBA_UNDEF;/* return value for `or' statement */\n")
         , or_return_value);
    for(int64_t i = 0; i < len; ++i){
        voba_value_t ast_expr = voba_array_at(a_ast_exprs,i);
        assert(voba_is_a(ast_expr,voba_cls_ast));
        voba_str_t * s_ast_expr = ast2c_ast(AST(ast_expr),bk,s,last_compound_expr, i == len - 1);
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
static inline voba_str_t* ast2c_ast_yield(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_value_t ast_expr = ast->u.yield.ast_expr;
    voba_str_t * s_ast_expr = ast2c_ast(AST(ast_expr),bk,s,0/*not last compound expr*/,0/*last expr*/);
    voba_str_t * yield_return_value = new_uniq_id(voba_str_from_cstr("yield_ret_val"));
    TEMPLATE(s,
             VOBA_CONST_CHAR(
                 "    voba_value_t #0 __attribute__((unused)) = 0;\n"
                 "    #0 = cg_yield(__builtin_frame_address(0), #1);\n"
                 )
             , yield_return_value
             , s_ast_expr
        );
    return yield_return_value;
}
static inline voba_str_t* ast2c_ast_args(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    return voba_str_from_cstr("fun_args");
}
