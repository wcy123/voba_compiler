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

#define DECL(s)  bk->decl = voba_strcat(bk->decl,(s));
#define START(s)  bk->start = voba_strcat(bk->start,(s));
#define IMPL(s)  bk->impl = voba_strcat(bk->impl,(s));

static voba_str_t * symbol_id(voba_value_t s)
{
    return voba_strcat(voba_str_from_char('x',1), voba_str_fmt_int64_t(s,16));
}
static voba_str_t * ast_symbol_id(ast_t* ast)
{
    return symbol_id(SYNTAX(ast->u.top_var.syn_s_name)->v);
}
static voba_str_t * ast_symbol_name(ast_t* ast)
{
    return voba_value_to_str(voba_symbol_name(SYNTAX(ast->u.top_var.syn_s_name)->v));
}
static void ast2c_decl_top_var(voba_value_t a_top_vars, c_backend_t * bk)
{
    int64_t len = voba_array_len(a_top_vars);
    for(int64_t i = 0 ; i < len ; ++i){
        DECL(VOBA_CONST_CHAR("static voba_value_t "));
        ast_t * ast = AST(voba_array_at(a_top_vars,i));
        DECL(ast_symbol_id(ast));
        DECL(VOBA_CONST_CHAR("= VOBA_NIL;/*"));
        DECL(ast_symbol_name(ast));
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
            START(ast_symbol_id(ast));
            START(VOBA_CONST_CHAR(" = s;\n"));
        }else{
            assert(!voba_is_nil(ast->u.top_var.module_id));
            START(VOBA_CONST_CHAR("    id = voba_make_string(voba_str_from_cstr("));
            START(quote_string(voba_value_to_str(ast->u.top_var.module_id)));
            START(VOBA_CONST_CHAR("    ));\n"));
            START(VOBA_CONST_CHAR("    name = voba_make_string(voba_str_from_cstr("));
            START(quote_string(ast_symbol_name(ast)));
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
            START(quote_string(ast_symbol_name(ast)));
            START(VOBA_CONST_CHAR("))"));
            START(VOBA_CONST_CHAR(",voba_tail(m));\n"));
            START(VOBA_CONST_CHAR("    if(voba_is_nil(s)){\n"));
            START(VOBA_CONST_CHAR("        VOBA_THROW("));
            START(VOBA_CONST_CHAR("VOBA_CONST_CHAR(\""));
            START(VOBA_CONST_CHAR("module \" "));
            START(quote_string(voba_value_to_str(ast->u.top_var.module_id)));
            START(VOBA_CONST_CHAR(" \" should contains \" "));
            START(quote_string(ast_symbol_name(ast)));
            START(VOBA_CONST_CHAR("));\n"));
            START(VOBA_CONST_CHAR("    }\n"));
            START(VOBA_CONST_CHAR("    "));
            START(ast_symbol_id(ast));
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
voba_value_t ast2c(toplevel_env_t* toplevel)
{
    voba_value_t ret = make_c_backend();
    c_backend_t * bk = C_BACKEND(ret);
    ast2c_decl_prelude(bk);
    import_modules(toplevel,bk);
    ast2c_decl_top_var(toplevel->a_ast_toplevel_vars,bk);
    return ret;
}
