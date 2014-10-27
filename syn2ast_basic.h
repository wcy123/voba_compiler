#pragma once
voba_value_t compile_exprs(voba_value_t la_syn_exprs, voba_value_t env, voba_value_t toplevel_env);
voba_value_t compile_expr(voba_value_t syn_expr,voba_value_t env,voba_value_t toplevel_env);
voba_value_t compile_def(voba_value_t top_var, voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env);

