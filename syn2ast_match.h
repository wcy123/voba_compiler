#pragma once
/* return an ast object, ast_match */
voba_value_t compile_match(voba_value_t syn_form, voba_value_t env,voba_value_t toplevel_env);
/* export it for statement `for' return a match object */
voba_value_t compile_match_match(voba_value_t la_syn_form, voba_value_t env,voba_value_t toplevel_env);

