#pragma once
enum var_flag {
    VAR_LOCAL,       // created by `let` form or `match` form
    VAR_ARGUMENT,    // function arguments
    VAR_CLOSURE,     // closure variable
    VAR_PUBLIC_TOP,  // public top level variable, module variable, defined in this module
    VAR_PRIVATE_TOP, // private top level variable, 
    VAR_FOREIGN_TOP, // imported variable from other module, not defined in this module
};
typedef struct var_s {
    voba_value_t syn_s_name; // unique id
    enum var_flag flag;
    union {
        struct {
	    // module id for top var,
	    // for VAR_PUBLIC_TOP, VAR_PRIVATE_TOP and VAR_FOREIGN_TOP
            voba_value_t syn_module_id; 
            voba_value_t syn_module_name;
        }m;
        int32_t index; // for closure and argument , VAR_LOCAL and VAR_ARGUMENT
    }u;
}var_t;
#define VAR(s)  VOBA_USER_DATA_AS(var_t *,s)
extern voba_value_t voba_cls_var;

voba_value_t make_var(voba_value_t syn_name, enum var_flag flag);
int var_is_top(var_t* v);

