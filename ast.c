#include "voba_value.h"
#include "ast.h"
#include "parser.syn.h"
inline static void report_error(const char * msg,voba_value_t syn)
{
    // TODO filename 
    printf("%d:%d - %d: %d:%s\n",SYNTAX(syn)->first_line,SYNTAX(syn)->first_column,
           SYNTAX(syn)->last_line, SYNTAX(syn)->last_column,msg);
}
voba_value_t compile_build_top_level_list(voba_value_t syn)
{
    if(!voba_is_array(SYNTAX(syn)->v)){
        report_error("module must be a list.",syn);
        return VOBA_NIL;
    }
    voba_value_t syns = SYNTAX(syn)->v;
    voba_value_t ret = voba_make_array_0();
    size_t len = voba_array_len(syns);
    for(size_t i = 0; i < len ; ++i){
        voba_value_t syn = voba_array_at(syns,i);
        voba_value_t sv = SYNTAX(syn)->v;
        if(!voba_is_array(sv)){
            report_error("top level definition is not a list.",syn);
            continue;
        }
        if(voba_array_len(sv) < 2){
            report_error("top level definition is not in form of (SYMBOL (ARGS...) BODY...). array_len < 3",syn);
            continue;
        }
        voba_value_t syn_name = voba_array_at(sv,0);
        voba_value_t name = SYNTAX(syn_name)->v;
        if(!voba_is_symbol(name)){
            report_error("top level definition is not in form of (SYMBOL (ARGS...) BODY...)."
                         "`NAME` is not a symbol.",syn);
            continue;
        }
        voba_array_push(ret, name);
    }
    return ret;
}
static voba_value_t compile_module_ast(voba_value_t top_list, voba_value_t syns)
{
    voba_value_t ret = voba_make_array_0();
    size_t len = voba_array_len(top_list);
    for(size_t i = 0; i < len ; ++i){
        voba_value_t t = compile_toplevel_def(top_list,voba_array_at(syns,i));
        if(!voba_is_nil(t)){
            voba_array_push(ret, t);
        }
    }
    return top_list;
}
voba_value_t compile_module(voba_value_t syn)
{
    voba_value_t top_list = compile_build_top_level_list(syn);
    return compile_module_ast(top_list,SYNTAX(syn)->v);
}
