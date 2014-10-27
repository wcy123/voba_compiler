#include <errno.h>

#define EXEC_ONCE_TU_NAME "syn2ast_decl_top_var"
#include <exec_once.h>
#include <voba/include/value.h>
#include "src.h"
#include "ast.h"
#include "syn.h"
#include "module_info.h"
#include "read_module_info.h"
#include "keywords.h"
#include "syn2ast_report.h"
#include "syn2ast_basic.h"
#include "syn2ast_decl_top_var.h"

// for import, try to find the var, 
//     no var existed, it is a foreign var.
//     if a local var, var becomes a module var
//     if a module var, throw an error, name conflicts
//     if a foreign var, throw an error, name conficts again.
static inline void create_topleve_var_for_import(voba_value_t syn_symbol, voba_value_t module_id, voba_value_t module_name, voba_value_t toplevel_env)
{
    voba_value_t env = TOPLEVEL_ENV(toplevel_env)->env;
    voba_value_t symbol = SYNTAX(syn_symbol)->v;
    voba_value_t var = search_symbol(syn_symbol,env);
    if(voba_is_nil(var)){
        voba_value_t top_var = make_var(syn_symbol,VAR_FOREIGN_TOP);
        VAR(top_var)->u.m.module_id = module_id;
        VAR(top_var)->u.m.module_name = module_name;
        env_push_var(env,top_var);
        if(0){
            fprintf(stderr,__FILE__ ":%d:[%s] pushing var for import %lx\n", __LINE__, __FUNCTION__
                    ,top_var);
        }
    }else{
        voba_value_t top_var = var;
        assert(voba_is_a(top_var,voba_cls_var) && var_is_top(VAR(top_var)));
        switch(VAR(top_var)->flag){
        case VAR_PRIVATE_TOP:
            VAR(top_var)->flag = VAR_PUBLIC_TOP;
            break;
        case VAR_PUBLIC_TOP:
        case VAR_FOREIGN_TOP:
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
static inline voba_value_t create_topleve_var_for_def(voba_value_t syn_symbol, voba_value_t toplevel_env)
{
    voba_value_t ret = VOBA_NIL;
    voba_value_t env = TOPLEVEL_ENV(toplevel_env)->env;
    voba_value_t symbol = SYNTAX(syn_symbol)->v;
    voba_value_t var = search_symbol(syn_symbol,TOPLEVEL_ENV(toplevel_env)->env);
    if(voba_is_nil(var)){
        /* fprintf(stderr,__FILE__ ":%d:[%s] debug cannot found symbol %s\n ", __LINE__, __FUNCTION__, */
        /*         voba_str_to_cstr(voba_value_to_str(voba_symbol_name(symbol)))); */
        voba_value_t top_var = make_var(syn_symbol,VAR_PRIVATE_TOP);
        env_push_var(env,top_var);
        ret = top_var;
    }else{
        voba_value_t top_var = var;
        ret = top_var;
        assert(voba_is_a(top_var,voba_cls_var) && var_is_top(VAR(top_var)));
        switch(VAR(top_var)->flag){
        case VAR_PRIVATE_TOP:
        case VAR_PUBLIC_TOP:
            report_error(
                VOBA_STRCAT(VOBA_CONST_CHAR("redefine a symbol. symbol = "),
                            voba_value_to_str(voba_symbol_name(symbol))),
                // TODO: report previous definitions position.
                syn_symbol,
                toplevel_env
                );
            break;
        case VAR_FOREIGN_TOP:
            VAR(top_var)->flag = VAR_PUBLIC_TOP;
            /* fprintf(stderr,__FILE__ ":%d:[%s] %s %s\n", __LINE__, __FUNCTION__, */
            /*         voba_str_to_cstr(voba_value_to_str(voba_symbol_name(symbol))), */
            /*         voba_is_nil(AST(top_var)->u.top_var.module_id)?"NIL":voba_str_to_cstr(voba_value_to_str(AST(top_var)->u.top_var.module_id))); */
            break;
        default:
            assert(0);
        }
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
    VOBA_DEF_ARG3(toplevel_env, args, 0);
    if(0)fprintf(stderr,__FILE__ ":%d:[%s] \n", __LINE__, __FUNCTION__);
    voba_value_t exprs = compile_exprs(la_syn_exprs,TOPLEVEL_ENV(toplevel_env)->env,toplevel_env);
    voba_value_t ret = VOBA_NIL;
    if(!voba_is_nil(exprs)){
        ret = make_ast_set_var(syn_name, exprs);
    }
    if(0) fprintf(stderr,__FILE__ ":%d:[%s] v = 0x%lx type = 0x%lx\n", __LINE__, __FUNCTION__,
            ret, voba_get_class(ret));
    return ret;

}
static inline void compile_top_expr_def_name(voba_value_t syn_name, voba_value_t la_syn_exprs,voba_value_t toplevel_env)
{
    voba_value_t closure = voba_make_closure_2
        (compile_top_expr_def_name_next,syn_name,la_syn_exprs);
    voba_array_push(TOPLEVEL_ENV(toplevel_env)->next, closure);
    create_topleve_var_for_def(syn_name,toplevel_env);
    return;
}
VOBA_FUNC
static voba_value_t compile_top_expr_def_fun_next(voba_value_t self, voba_value_t args) 
{
    voba_value_t top_var = voba_array_at(self,0);
    voba_value_t syn_form = voba_array_at(self,1);
    voba_value_t env = voba_array_at(self,2);
    VOBA_DEF_ARG3(toplevel_env, args, 0);
    return compile_def(top_var,syn_form,env,toplevel_env);
}
static inline void compile_top_expr_def_fun(voba_value_t syn_top_expr, voba_value_t toplevel_env)
{
    voba_value_t top_expr = SYNTAX(syn_top_expr)->v;
#ifndef NDEBUG    
    int64_t len = voba_array_len(top_expr);
#endif
    assert(len > 0);
    //voba_value_t syn_def = voba_array_at(top_expr,0);
    voba_value_t syn_x_form = voba_array_at(top_expr,1);
    voba_value_t x_form = SYNTAX(syn_x_form)->v;
    int64_t xlen = voba_array_len(x_form);
    if(xlen > 1){
        voba_value_t syn_s_name = voba_array_at(x_form,0);
        if(voba_is_a(syn_s_name,voba_cls_syn)&&
           voba_is_a(SYNTAX(syn_s_name)->v,voba_cls_symbol)){
            voba_value_t top_var = create_topleve_var_for_def(syn_s_name,toplevel_env);
            voba_value_t env = TOPLEVEL_ENV(toplevel_env)->env;
            voba_value_t next = voba_make_closure_3(
                compile_top_expr_def_fun_next, top_var, syn_top_expr, env);
            voba_array_push(TOPLEVEL_ENV(toplevel_env)->next,next);
        }else{
            report_error(VOBA_CONST_CHAR("illegal form. function name must be a symbol."),
                         syn_s_name,toplevel_env);
        }
    }else{
        report_error(VOBA_CONST_CHAR("illegal form. no fun name"),
                     syn_x_form,toplevel_env);
    }
    return;
}

static inline void compile_top_expr_def(voba_value_t syn_top_expr,voba_value_t toplevel_env)
{
    voba_value_t top_expr = SYNTAX(syn_top_expr)->v;
    voba_value_t cur = voba_la_from_array1(top_expr,0);
    voba_value_t syn_def = voba_la_car(cur);
    voba_value_t la_syn_top_expr = voba_la_cdr(cur);
    if(!voba_la_is_nil(la_syn_top_expr)){
        voba_value_t syn_var_form = voba_la_car(la_syn_top_expr);
        voba_value_t var_form = SYNTAX(syn_var_form)->v;
        if(voba_is_a(var_form,voba_cls_symbol)){
            if(!is_keyword(toplevel_env,var_form)){
                compile_top_expr_def_name(syn_var_form, voba_la_cdr(la_syn_top_expr),toplevel_env);
            }else{
                report_error(VOBA_CONST_CHAR("redefine keyword")
                             ,var_form
                             ,toplevel_env);
            }
        }else if(voba_is_a(var_form,voba_cls_array)){
            compile_top_expr_def_fun(syn_top_expr,toplevel_env);
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
    voba_value_t module_name = module_info_name(info);
    int64_t len = voba_array_len(symbol_names);
    voba_array_push(TOPLEVEL_ENV(toplevel_env)->a_modules,info);
    for(int64_t i = 0; i < len ; i ++){
        voba_value_t symbol_name = voba_array_at(symbol_names,i);
        voba_value_t symbol = voba_make_symbol(voba_value_to_str(symbol_name),
                                               TOPLEVEL_ENV(toplevel_env)->module);
        voba_value_t syn_module_var_name = make_syntax(symbol,yyloc.start_pos,yyloc.end_pos);
        attach_src(syn_module_var_name,
                   make_src(voba_value_to_str(filename),voba_value_to_str(content)));
        create_topleve_var_for_import(syn_module_var_name,module_id,module_name,toplevel_env);
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
            if(voba_is_a(module_info,voba_cls_module_info)){
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
static inline void compile_top_expr_import(voba_value_t syn_top_expr,voba_value_t toplevel_env)
{
    voba_value_t top_expr = SYNTAX(syn_top_expr)->v;
    voba_value_t cur = voba_la_from_array1(top_expr,0);
    voba_value_t syn_import = voba_la_car(cur);
    voba_value_t la_syn_top_expr = voba_la_cdr(cur);
    if(!voba_la_is_nil(la_syn_top_expr)){
        voba_value_t syn_module_name = voba_la_car(la_syn_top_expr);
        voba_value_t module_name = SYNTAX(syn_module_name)->v;
        if(voba_is_a(module_name,voba_cls_symbol)){
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
    VOBA_DEF_ARG3(toplevel_env, args, 0);
    return compile_expr(syn_top_expr, TOPLEVEL_ENV(toplevel_env)->env, toplevel_env);
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
    if(voba_is_a(top_expr,voba_cls_array)){
        int64_t len =  voba_array_len(top_expr);
        if(len > 0){
            voba_value_t cur = voba_la_from_array1(top_expr,0);
            voba_value_t syn_key_word = voba_la_car(cur);
            voba_value_t key_word = SYNTAX(syn_key_word)->v;
            if(voba_eq(key_word, K(toplevel_env,def))){
                compile_top_expr_def(syn_top_expr,toplevel_env);
            }else if(voba_eq(key_word, K(toplevel_env,import))){
                compile_top_expr_import(syn_top_expr,toplevel_env);
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
void compile_toplevel_exprs(voba_value_t la_syn_top_exprs, voba_value_t toplevel_env)
{
    voba_value_t cur = la_syn_top_exprs;
    while(!voba_la_is_nil(cur)){
        voba_value_t syn_top_expr = voba_la_car(cur);
        compile_top_expr(syn_top_expr,toplevel_env);
        cur = voba_la_cdr(cur);
    }
    return;
}
/* Local Variables: */
/* mode:c */
/* coding: utf-8-unix */
/* End: */
