#include <voba/include/value.h>
#include <voba/core/builtin.h>
#include <exec_once.h>
#include "match.h"
#include "env.h"
DEFINE_CLS(sizeof(match_t),match);
DEFINE_CLS(sizeof(rule_t),rule);
DEFINE_CLS(sizeof(pattern_t),pattern);
voba_value_t make_match(voba_value_t a_rules)
{
    voba_value_t ret = voba_make_user_data(voba_cls_match,sizeof(match_t));
    match_t * p_match = MATCH(ret);
    p_match->a_rules = a_rules;
    return ret;
}
voba_value_t make_rule(voba_value_t p, voba_value_t a, voba_value_t e)
{
    voba_value_t ret = voba_make_user_data(voba_cls_rule,sizeof(rule_t));
    rule_t * p_rule = RULE(ret);
    p_rule->pattern = p;
    p_rule->a_ast_action = a;
    p_rule->env = e;
    return ret;
}
voba_value_t make_pattern_constant(voba_value_t syn_value)
{
    voba_value_t ret = voba_make_user_data(voba_cls_pattern,sizeof(pattern_t));
    pattern_t * p_pattern = PATTERN(ret);
    p_pattern->type = PATTERN_CONSTANT;
    p_pattern->u.constant.syn_value = syn_value;
    return ret;
}
voba_value_t make_pattern_var(voba_value_t var)
{
    voba_value_t ret = voba_make_user_data(voba_cls_pattern,sizeof(pattern_t));
    pattern_t * p_pattern = PATTERN(ret);
    p_pattern->type = PATTERN_VAR;
    p_pattern->u.var.var = var;
    return ret;

}
voba_value_t make_pattern_apply(voba_value_t cls, voba_value_t a_patterns)
{
    voba_value_t ret = voba_make_user_data(voba_cls_pattern,sizeof(pattern_t));
    pattern_t * p_pattern = PATTERN(ret);
    p_pattern->type = PATTERN_APPLY;
    p_pattern->u.apply.cls = cls;
    p_pattern->u.apply.a_patterns = a_patterns;
    return ret;

}
voba_value_t calculate_pattern_env(voba_value_t pattern, voba_value_t env)
{
    voba_value_t ret = VOBA_NIL;
    ret = make_env();
    ENV(ret)->parent = env;
    return ret;
}
EXEC_ONCE_START;


