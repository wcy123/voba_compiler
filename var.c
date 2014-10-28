#define EXEC_ONCE_TU_NAME "var"
#include <exec_once.h>
#include <voba/include/value.h>
#include <voba/core/builtin.h>
#include "var.h"
#include "syn.h"
VOBA_DEF_CLS(sizeof(var_t),var);
voba_value_t make_var(voba_value_t syn_name, enum var_flag flag)
{
    voba_value_t ret = voba_make_user_data(voba_cls_var);
    VAR(ret)->syn_s_name = syn_name;
    VAR(ret)->flag = flag;
    VAR(ret)->u.m.module_id = VOBA_NIL;
    VAR(ret)->u.m.module_name = VOBA_NIL;
    return ret;
}
int var_is_top(var_t* v)
{
    return ((v)->flag == VAR_PUBLIC_TOP ||
            (v)->flag == VAR_PRIVATE_TOP ||
            (v)->flag == VAR_FOREIGN_TOP);
}
static inline voba_str_t* to_string_flag(var_t * var)
{
    voba_str_t * ret = VOBA_CONST_CHAR("ERRRO");
    switch(var->flag){
    case VAR_LOCAL:
        ret = VOBA_CONST_CHAR("VAR_LOCAL");
        break;
    case VAR_ARGUMENT:
        ret = VOBA_CONST_CHAR("VAR_ARGUMENT");
        break;
    case VAR_CLOSURE:
        ret = VOBA_CONST_CHAR("VAR_CLOSURE");
        break;
    case VAR_PUBLIC_TOP:
        ret = VOBA_CONST_CHAR("VAR_PUBLIC_TOP");
        break;
    case VAR_PRIVATE_TOP:
        ret = VOBA_CONST_CHAR("VAR_PRIVATE_TOP");
        break;
    case VAR_FOREIGN_TOP:
        ret = VOBA_CONST_CHAR("VAR_FOREIGN_TOP");
        break;
    default:
        assert(0&&"never goes here");
    }
    return ret;
}
VOBA_FUNC static voba_value_t to_string_var(voba_value_t self,voba_value_t vs);
EXEC_ONCE_PROGN{voba_gf_add_class(voba_symbol_value(s_to_string),voba_cls_var,voba_make_func(to_string_var));}
VOBA_FUNC static voba_value_t to_string_var(voba_value_t self,voba_value_t vs)
{
    voba_value_t v = voba_array_at(vs,0);
    voba_str_t* ret = voba_str_empty();
    var_t * var = VAR(v);
    ret = VOBA_STRCAT(ret,
                      VOBA_CONST_CHAR("<VAR ID="), voba_str_fmt_int64_t(var->syn_s_name,16),
                      VOBA_CONST_CHAR(" NAME="), voba_value_to_str(voba_symbol_name(SYNTAX(var->syn_s_name)->v)),
                      VOBA_CONST_CHAR(" FLAG="), to_string_flag(var),
                      VOBA_CONST_CHAR(">"));
    return voba_make_string(ret);
    
}

