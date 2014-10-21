#pragma once
static inline voba_value_t compile_exprs(voba_value_t la_syn_exprs, voba_value_t env, voba_value_t toplevel_env);
static inline voba_value_t compile_expr(voba_value_t syn_expr,voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_fun(voba_value_t syn_form, voba_value_t env, voba_value_t toplevel_env);
static inline voba_value_t compile_arg_list(voba_value_t la_arg, voba_value_t toplevel_env);
static inline voba_value_t compile_arg(voba_value_t a, int32_t index, voba_value_t toplevel_env);
static inline voba_value_t compile_array(voba_value_t syn_form, voba_value_t env, voba_value_t toplevel_env);
static inline voba_value_t compile_symbol(voba_value_t syn_symbol, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_let(voba_value_t syn_symbol, voba_value_t env,voba_value_t toplevel_env);
static inline voba_value_t compile_match(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env);
