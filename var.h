#pragma once
enum var_flag {
    VAR_LOCAL,
    VAR_ARGUMENT,
    VAR_CLOSURE,
    VAR_PUBLIC_TOP, // local variable
    VAR_PRIVATE_TOP, // public, module variable, defined in this module
    VAR_FOREIGN_TOP, // imported variable from other module, not defined in this module
};
typedef struct var_s {
    voba_value_t syn_s_name; // unique id
    enum var_flag flag;
    union {
        struct {
            voba_value_t module_id; // module id for top var
            voba_value_t module_name;
        }m;
        int32_t index; // for closure and argument 
    }u;
}var_t;
#define VAR(s)  VOBA_USER_DATA_AS(var_t *,s)
extern voba_value_t voba_cls_var;

voba_value_t make_var(voba_value_t syn_name, enum var_flag flag);
int var_is_top(var_t* v);

