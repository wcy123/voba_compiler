#include <voba/include/value.h>
#include <voba/core/builtin.h>
#include <exec_once.h>
#include "match.h"

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
voba_value_t make_pattern()
{
    voba_value_t ret = VOBA_NIL;
    return ret;
}
voba_value_t calculate_pattern_env(voba_value_t pattern, voba_value_t env)
{
    voba_value_t ret = VOBA_NIL;
    ret = env;
    return ret;
}
EXEC_ONCE_START;


