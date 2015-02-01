#pragma once
#include "syn.h"
typedef struct module_info_s{
    voba_value_t syn_name;
    voba_value_t syn_id;
    voba_value_t a_syn_symbols; // an array of names
}module_info_t;

#define MODULE_INFO(v) VOBA_USER_DATA_AS(module_info_t*,v)
extern voba_value_t voba_cls_module_info;
voba_value_t module_info_id(voba_value_t v);
voba_value_t module_info_name(voba_value_t v);
voba_value_t module_info_symbols(voba_value_t v);
voba_value_t make_module_info();
