#include <stdint.h>
#include <assert.h>
#include <voba/include/value.h>
#include <exec_once.h>
#include "syn.h"
#include "ast.h"
#include "c_backend.h"
#include "module_info.h"
#include "ast2c.h"
static void import_modules(toplevel_env_t* toplevel, c_backend_t* out);
static void import_module(voba_value_t a_modules, c_backend_t * bk);
static voba_str_t * quote_string(voba_str_t * s);
static voba_str_t* new_uniq_id();
static voba_str_t* ast2c_ast(ast_t* ast, c_backend_t * bk, voba_str_t ** s);
static voba_str_t* ast2c_ast_set_top(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static voba_str_t* ast2c_ast_constant(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static voba_str_t* ast2c_ast_fun(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static voba_str_t* ast2c_ast_arg(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static voba_str_t* ast2c_ast_closure(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static voba_str_t* ast2c_ast_top(ast_t* ast, c_backend_t* bk, voba_str_t** s);
static voba_str_t* ast2c_ast_apply(ast_t* ast, c_backend_t* bk, voba_str_t** s);

#define DECL(s)  bk->decl = voba_strcat(bk->decl,(s));
#define START(s)  bk->start = voba_strcat(bk->start,(s));
#define IMPL(s)  bk->impl = voba_strcat(bk->impl,(s));
#define OUT(stream,s)  (stream) = voba_strcat(stream,s);

static voba_str_t * symbol_id(voba_value_t s)
{
    return voba_strcat(voba_str_from_char('x',1), voba_str_fmt_int64_t(s,16));
}
static voba_str_t * ast_top_var_symbol_id(ast_t* ast)
{
    return symbol_id(SYNTAX(ast->u.top_var.syn_s_name)->v);
}
static voba_str_t * ast_set_top_symbol_id(ast_t* ast)
{
    return symbol_id(SYNTAX(ast->u.set_top.syn_s_name)->v);
}
static voba_str_t * ast_top_var_symbol_name(ast_t* ast)
{
    return voba_value_to_str(voba_symbol_name(SYNTAX(ast->u.top_var.syn_s_name)->v));
}
static voba_str_t * ast_set_top_symbol_name(ast_t* ast)
{
    return voba_value_to_str(voba_symbol_name(SYNTAX(ast->u.set_top.syn_s_name)->v));
}
static void ast2c_decl_top_var(voba_value_t a_top_vars, c_backend_t * bk)
{
    int64_t len = voba_array_len(a_top_vars);
    for(int64_t i = 0 ; i < len ; ++i){
        DECL(VOBA_CONST_CHAR("static voba_value_t "));
        ast_t * ast = AST(voba_array_at(a_top_vars,i));
        DECL(ast_top_var_symbol_id(ast));
        DECL(VOBA_CONST_CHAR("= VOBA_NIL;/*"));
        DECL(ast_top_var_symbol_name(ast));
        DECL(VOBA_CONST_CHAR(";*/\n"));
    }
    START(VOBA_CONST_CHAR("EXEC_ONCE_PROGN {\n"));
    START(VOBA_CONST_CHAR("    voba_value_t m = VOBA_NIL;\n"));
    START(VOBA_CONST_CHAR("    voba_value_t s = VOBA_NIL;\n"));
    START(VOBA_CONST_CHAR("    voba_value_t id = VOBA_NIL;\n"));
    START(VOBA_CONST_CHAR("    voba_value_t name = VOBA_NIL;\n"));
    for(int64_t i = 0 ; i < len ; ++i){
        ast_t * ast = AST(voba_array_at(a_top_vars,i));
        assert(ast->type == TOP_VAR);
        if(ast->u.top_var.flag == LOCAL_VAR){
            // local variables are defined by SET_TOP
            START(VOBA_CONST_CHAR("    "));
            START(ast_top_var_symbol_id(ast));
            START(VOBA_CONST_CHAR(" = s;\n"));
        }else{
            assert(!voba_is_nil(ast->u.top_var.module_id));
            START(VOBA_CONST_CHAR("    id = voba_make_string(voba_str_from_cstr("));
            START(quote_string(voba_value_to_str(ast->u.top_var.module_id)));
            START(VOBA_CONST_CHAR("    ));\n"));
            START(VOBA_CONST_CHAR("    name = voba_make_string(voba_str_from_cstr("));
            START(quote_string(ast_top_var_symbol_name(ast)));
            START(VOBA_CONST_CHAR("    ));\n"));
            //START(VOBA_CONST_CHAR("    fprintf(stderr,__FILE__ \":%d:[%s] voba_modules =  0x%lx\\n\", __LINE__, __FUNCTION__,voba_modules);\n"));
            START(VOBA_CONST_CHAR("    m = voba_hash_find(voba_modules,id);\n"));
            START(VOBA_CONST_CHAR("    if(voba_is_nil(m)){\n"));
            START(VOBA_CONST_CHAR("        VOBA_THROW("));
            START(VOBA_CONST_CHAR("VOBA_CONST_CHAR(\""));
            START(VOBA_CONST_CHAR("module \" "));
            START(quote_string(voba_value_to_str(ast->u.top_var.module_id)));
            START(VOBA_CONST_CHAR(" \" is not imported."));
            START(VOBA_CONST_CHAR("\"));\n"));
            START(VOBA_CONST_CHAR("    }\n"));
            START(VOBA_CONST_CHAR("    s = voba_lookup_symbol(voba_make_string(voba_str_from_cstr("));
            START(quote_string(ast_top_var_symbol_name(ast)));
            START(VOBA_CONST_CHAR("))"));
            START(VOBA_CONST_CHAR(",voba_tail(m));\n"));
            START(VOBA_CONST_CHAR("    if(voba_is_nil(s)){\n"));
            START(VOBA_CONST_CHAR("        VOBA_THROW("));
            START(VOBA_CONST_CHAR("VOBA_CONST_CHAR(\""));
            START(VOBA_CONST_CHAR("module \" "));
            START(quote_string(voba_value_to_str(ast->u.top_var.module_id)));
            START(VOBA_CONST_CHAR(" \" should contains \" "));
            START(quote_string(ast_top_var_symbol_name(ast)));
            START(VOBA_CONST_CHAR("));\n"));
            START(VOBA_CONST_CHAR("    }\n"));
            START(VOBA_CONST_CHAR("    "));
            START(ast_top_var_symbol_id(ast));
            START(VOBA_CONST_CHAR(" = s;\n"));
        }
    }
    START(VOBA_CONST_CHAR("}\n"));
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
    START(VOBA_CONST_CHAR("EXEC_ONCE_PROGN {\n"));
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
    START(VOBA_CONST_CHAR("    voba_value_t vid = voba_make_string(voba_str_from_cstr(id));\n"));
    START(VOBA_CONST_CHAR("    //voba_value_t vname = voba_make_string(voba_str_from_cstr(name));\n"));
    START(VOBA_CONST_CHAR("    voba_value_t m = voba_hash_find(voba_modules,vid);\n"));
    START(VOBA_CONST_CHAR("    voba_value_t s = VOBA_NIL;\n"));
    START(VOBA_CONST_CHAR("    assert(!voba_is_nil(m) && \"module "));START(mi->name);
    START(VOBA_CONST_CHAR(" should already be there.\");\n"));
    for(int64_t i = 0; i < len; ++i){
        START(VOBA_CONST_CHAR("    s = voba_lookup_symbol(voba_make_string(voba_str_from_cstr("));
        START(quote_string(voba_value_to_str(voba_array_at(mi->symbols,i))));
        START(VOBA_CONST_CHAR(")), voba_tail(m));\n"));
        START(VOBA_CONST_CHAR("    if(voba_is_nil(s)){\n"));
        START(VOBA_CONST_CHAR("       VOBA_THROW(VOBA_CONST_CHAR(\"module \" "));
        START(quote_string(mi->name));
        START(VOBA_CONST_CHAR(" \" should contain symbol \" "));
        START(quote_string(voba_value_to_str(voba_array_at(mi->symbols,i))));
        START(VOBA_CONST_CHAR(" \".\"));\n"));
        START(VOBA_CONST_CHAR("    }\n"));
    }
    START(VOBA_CONST_CHAR("}\n"));
}
static void ast2c_decl_prelude(c_backend_t* bk)
{
    DECL(VOBA_CONST_CHAR("#include <stdint.h>\n"));
    DECL(VOBA_CONST_CHAR("#include <assert.h>\n"));
    DECL(VOBA_CONST_CHAR("#include <voba/include/value.h>\n"));
    DECL(VOBA_CONST_CHAR("#include <exec_once.h>\n"));
    DECL(VOBA_CONST_CHAR("extern voba_value_t voba_import_module(const char * module_name, const char * module_id, const char * symbols[]);\n"));
    DECL(VOBA_CONST_CHAR("extern voba_value_t voba_modules;\n"));
}
EXEC_ONCE_START;
static void ast2c_all_asts(voba_value_t a_asts, c_backend_t* bk)
{
    int64_t len = voba_array_len(a_asts);
    for(int64_t i = 0; i < len; ++i){
        ast2c_ast(AST(voba_array_at(a_asts,i)),bk,&bk->start);
    }
}
static voba_str_t* ast2c_ast(ast_t* ast, c_backend_t * bk, voba_str_t ** s)
{
    voba_str_t* ret = voba_str_empty();
    switch(ast->type){
    case SET_TOP:
        ret = ast2c_ast_set_top(ast,bk,s);
        break;
    case CONSTANT:
        ret = ast2c_ast_constant(ast,bk,s);
        break;
    case FUN:
        ret = ast2c_ast_fun(ast,bk,s);
        break;
    case ARG:
        ret = ast2c_ast_arg(ast,bk,s);
        break;
    case CLOSURE_VAR:
        ret = ast2c_ast_closure(ast,bk,s);
        break;
    case TOP_VAR:
        ret = ast2c_ast_top(ast,bk,s);
        break;
    case APPLY:
        ret = ast2c_ast_apply(ast,bk,s);
        break;
    default:
        assert(0 && "never goes here");
    }
    return ret;
}
static voba_str_t* ast2c_ast_set_top(ast_t* ast, c_backend_t * bk, voba_str_t ** s)
{
    OUT(*s, VOBA_CONST_CHAR("EXEC_ONCE_PROGN {\n"));
    voba_str_t* expr = voba_str_empty();
    voba_value_t exprs = ast->u.set_top.a_ast_exprs;
    int64_t len = voba_array_len(exprs);
    for(int64_t i = 0; i < len; ++i){
        expr = ast2c_ast(AST(voba_array_at(exprs,i)),bk,s);
    }
    voba_str_t * ret = ast_set_top_symbol_id(ast);
    OUT(*s, ret);
    OUT(*s, VOBA_CONST_CHAR(" = "));
    OUT(*s, expr);
    OUT(*s, VOBA_CONST_CHAR("; /* set top  "));
    OUT(*s, ast_set_top_symbol_name(ast));
    OUT(*s, VOBA_CONST_CHAR(";*/\n"));
    OUT(*s, VOBA_CONST_CHAR("}"));
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
    }else{
        fprintf(stderr,__FILE__ ":%d:[%s] type %s is not supported yet\n", __LINE__, __FUNCTION__,
            voba_get_class_name(value));
        assert(0&&"type is supported yet");
    }
    return ret;
}
static voba_str_t* ast2c_ast_fun(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = VOBA_CONST_CHAR("fun is not implemented");
    return ret;
}
static voba_str_t* ast2c_ast_arg(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = VOBA_CONST_CHAR("arg is not implemented");
    return ret;
}
static voba_str_t* ast2c_ast_closure(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = VOBA_CONST_CHAR("closure is not implemented");
    return ret;
}
static voba_str_t* ast2c_ast_top(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t* ret = voba_str_empty();
    ast_top_var_t* tv = &ast->u.top_var;
    switch(tv->flag){
    case FOREIGN_VAR:
    case MODULE_VAR:
        ret = voba_strcat(ret,VOBA_CONST_CHAR("voba_symbol_value("));
        ret = voba_strcat(ret,ast_top_var_symbol_id(ast));
        ret = voba_strcat(ret,VOBA_CONST_CHAR("/*"));
        ret = voba_strcat(ret,ast_top_var_symbol_name(ast));
        ret = voba_strcat(ret,VOBA_CONST_CHAR("*/"));
        ret = voba_strcat(ret,VOBA_CONST_CHAR(")"));        
        break;
    case LOCAL_VAR:
        ret = voba_strcat(ret,ast_top_var_symbol_id(ast));
        ret = voba_strcat(ret,VOBA_CONST_CHAR("/*"));
        ret = voba_strcat(ret,ast_top_var_symbol_name(ast));
        ret = voba_strcat(ret,VOBA_CONST_CHAR("*/"));
        break;
    default:
        assert(0&&"never goes here");
    }
    return ret;
}
static voba_str_t* ast2c_ast_apply(ast_t* ast, c_backend_t* bk, voba_str_t** s)
{
    voba_str_t * ret = new_uniq_id();
    OUT(*s, VOBA_CONST_CHAR("voba_value_t "));
    OUT(*s, ret);
    OUT(*s, VOBA_CONST_CHAR(" = VOBA_NIL;{\n"));
    voba_str_t * s2 = voba_str_empty(); // stream for sub-expression
    voba_value_t exprs = ast->u.apply.a_ast_exprs;
    voba_str_t * s_my = voba_str_empty(); 
    OUT(s_my, VOBA_CONST_CHAR("   voba_value_t args [] = {"));
    int64_t len = voba_array_len(exprs);
    assert(len >=1);
    OUT(s_my, voba_str_fmt_int64_t(len-1,10));
    OUT(s_my, VOBA_CONST_CHAR("\n"));
    for(int64_t i = 1; i < len; ++i){
        voba_value_t expr = voba_array_at(exprs,i);
        voba_str_t * sexpr = ast2c_ast(AST(expr), bk, &s2);
        OUT(s_my,VOBA_CONST_CHAR("        ,"));
        OUT(s_my, sexpr);
        OUT(s_my, VOBA_CONST_CHAR("\n"));
        ;
    }
    OUT(s_my, VOBA_CONST_CHAR("    };\n")); // end args
    voba_str_t * fun = ast2c_ast(AST(voba_array_at(exprs,0)), bk, &s_my);
    OUT(s_my, VOBA_CONST_CHAR("    "));
    OUT(s_my, ret);
    OUT(s_my, VOBA_CONST_CHAR("   = voba_apply(\n")); // end args
    OUT(s_my, fun);
    OUT(s_my, VOBA_CONST_CHAR(", voba_make_array(args));\n"));
    OUT(*s, s2);
    OUT(*s, s_my);
    OUT(*s, VOBA_CONST_CHAR("}\n"));
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
    ast2c_decl_top_var(toplevel->a_ast_toplevel_vars,bk);
    // output all asts
    ast2c_all_asts(toplevel->a_asts,bk);
    return ret;
}
