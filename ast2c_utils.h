#pragma once
static inline voba_str_t * quote_string(voba_str_t * s);
static inline voba_str_t* new_uniq_id();
static inline voba_str_t* indent(voba_str_t * s);
static inline voba_str_t * var_c_id(var_t* var);
static inline voba_str_t * var_c_symbol_name(var_t* var);
static inline void ast2c_template(const char * file, int line,const char * fn, voba_str_t **s, voba_str_t* template, size_t len, voba_str_t* array[]);
#define TEMPLATE(s,template,...)                                        \
    do{                                                                 \
        voba_str_t* args_____never [] = {(voba_str_t*)0, ##__VA_ARGS__ }; \
        size_t args_len____never = sizeof(args_____never)/sizeof(voba_value_t) - 1; \
        ast2c_template(__FILE__,__LINE__,__func__,s, template, args_len____never, &args_____never[1]); \
    }while(0)

