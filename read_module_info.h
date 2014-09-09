#pragma once

typedef struct module_info_s{
    voba_str_t * name;
    voba_str_t * id;
    voba_value_t symbols; // an array of names
}module_info_t;

#define MODULE_INFO(v) VOBA_USER_DATA_AS(module_info_t*,v)

voba_value_t read_module_info(voba_value_t self, voba_value_t args);
voba_value_t module_info_id(voba_value_t self, voba_value_t args);
voba_value_t module_info_name(voba_value_t self, voba_value_t args);
voba_value_t module_info_symbols(voba_value_t self, voba_value_t args);
