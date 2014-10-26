#pragma once

typedef struct match_s {
    voba_value_t a_rules; // an array of rules;
}match_t;
#define MATCH(s)  VOBA_USER_DATA_AS(match_t *,s)
extern voba_value_t voba_cls_match;
voba_value_t make_match(voba_value_t a_rules);

typedef struct rule_s rule_t;
struct rule_s {
    voba_value_t pattern;
    voba_value_t a_ast_action; // an list of ast
    voba_value_t env; // pattern creates a lexical scope
};
#define RULE(s)  VOBA_USER_DATA_AS(rule_t *,s)
extern voba_value_t voba_cls_rule;
voba_value_t make_rule(voba_value_t p, voba_value_t a, voba_value_t e);

enum pattern_type_e {
    PATTERN_VALUE,
    PATTERN_VAR,
    PATTERN_APPLY,
    PATTERN_ELSE
};
typedef struct pattern_s pattern_t;
typedef struct pattern_value_s pattern_value_t;
typedef struct pattern_var_s pattern_var_t;
typedef struct pattern_apply_s pattern_apply_t;
struct pattern_value_s {
    voba_value_t a_ast_value;
};
struct pattern_var_s {
    voba_value_t var;
};
struct pattern_apply_s {
    voba_value_t ast_cls;
    voba_value_t a_patterns; // an array of pattern
};
struct pattern_s {
    enum pattern_type_e type;
    union {
        pattern_value_t value;
        pattern_var_t var;
        pattern_apply_t apply;
    }u;
};
#define PATTERN(s)  VOBA_USER_DATA_AS(pattern_t *,s)
extern voba_value_t voba_cls_pattern;
voba_value_t make_pattern_value(voba_value_t value);
voba_value_t make_pattern_var(voba_value_t value);
voba_value_t make_pattern_apply(voba_value_t cls, voba_value_t a_patterns);
voba_value_t make_pattern_else();
voba_value_t calculate_pattern_env(voba_value_t pattern, voba_value_t env);
/* Local Variables: */
/* mode:c */
/* coding: undecided-unix */
/* End: */
