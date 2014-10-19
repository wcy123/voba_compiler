#include <voba/include/value.h>
#include <voba/core/builtin.h>
#include <exec_once.h>
#include "match.h"

DEFINE_CLS(sizeof(match_t),match);
DEFINE_CLS(sizeof(rule_t),rule);
DEFINE_CLS(sizeof(pattern_t),pattern);
voba_value_t make_match()
{
    voba_value_t ret = VOBA_NIL;
    return ret;
}
voba_value_t make_rule()
{
    voba_value_t ret = VOBA_NIL;
    return ret;
}
voba_value_t make_pattern()
{
    voba_value_t ret = VOBA_NIL;
    return ret;
}

EXEC_ONCE_START;


