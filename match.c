#define EXEC_ONCE_TU_NAME "match"
#include <exec_once.h>
#include <voba/include/value.h>
#include <voba/core/builtin.h>
#include "match.h"
#include "env.h"
DEFINE_CLS(sizeof(match_t),match);
DEFINE_CLS(sizeof(rule_t),rule);
DEFINE_CLS(sizeof(pattern_t),pattern);
voba_value_t make_match(voba_value_t a_rules)
{
    voba_value_t ret = voba_make_user_data(voba_cls_match);
    match_t * p_match = MATCH(ret);
    assert(voba_is_a(a_rules,voba_cls_array));
    p_match->a_rules = a_rules;
    return ret;
}
voba_value_t make_rule(voba_value_t pat, voba_value_t action, voba_value_t env)
{
    voba_value_t ret = voba_make_user_data(voba_cls_rule);
    assert(voba_is_a(pat,voba_cls_pattern));
    assert(voba_is_a(action,voba_cls_array));
    assert(voba_is_a(env,voba_cls_env));
    rule_t * p_rule = RULE(ret);
    p_rule->pattern = pat;
    p_rule->a_ast_action = action;
    p_rule->env = env;
    return ret;
}
voba_value_t make_pattern_value(voba_value_t a_ast_value)
{
    voba_value_t ret = voba_make_user_data(voba_cls_pattern);
    assert(voba_is_a(a_ast_value,voba_cls_array));
    pattern_t * p_pattern = PATTERN(ret);
    p_pattern->type = PATTERN_VALUE;
    p_pattern->u.value.a_ast_value = a_ast_value;
    return ret;
}
voba_value_t make_pattern_var(voba_value_t var)
{
    voba_value_t ret = voba_make_user_data(voba_cls_pattern);
    pattern_t * p_pattern = PATTERN(ret);
    p_pattern->type = PATTERN_VAR;
    p_pattern->u.var.var = var;
    return ret;

}
voba_value_t make_pattern_apply(voba_value_t ast_cls, voba_value_t a_patterns)
{
    voba_value_t ret = voba_make_user_data(voba_cls_pattern);
    pattern_t * p_pattern = PATTERN(ret);
    p_pattern->type = PATTERN_APPLY;
    p_pattern->u.apply.ast_cls = ast_cls;
    p_pattern->u.apply.a_patterns = a_patterns;
    return ret;

}
voba_value_t make_pattern_else()
{
    voba_value_t ret = voba_make_user_data(voba_cls_pattern);
    pattern_t * p_pattern = PATTERN(ret);
    p_pattern->type = PATTERN_ELSE;
    return ret;    
}
void walk_pattern_env(voba_value_t pattern, voba_value_t env);
voba_value_t calculate_pattern_env(voba_value_t pattern, voba_value_t env)
{
    voba_value_t ret = VOBA_NIL;
    ret = make_env();
    env_t * p_env = ENV(ret);
    p_env->parent = env;
    walk_pattern_env(pattern,ret);
    return ret;
}
void walk_pattern_env(voba_value_t pattern, voba_value_t env)
{
    pattern_t * p = PATTERN(pattern);
    switch(p->type){
    case PATTERN_VALUE:
    case PATTERN_ELSE:
        break;
    case PATTERN_VAR:
        env_push_var(env,p->u.var.var);
        break;
    case PATTERN_APPLY:{
        voba_value_t a_patterns = p->u.apply.a_patterns;
        int64_t len = voba_array_len(a_patterns);
        for(int64_t i = 0; i < len; ++i){
            voba_value_t pattern = voba_array_at(a_patterns,i);
            walk_pattern_env(pattern,env);
        }
        break;
    }
    default:
        assert(0&&"never goes here");
    }
    return;
}



