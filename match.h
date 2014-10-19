#pragma once

typedef struct match_s {
    voba_value_t a_rules; // an array of rules;
}match_t;
#define MATCH(s)  VOBA_USER_DATA_AS(match_t *,s)
extern voba_value_t voba_cls_match;
voba_value_t make_match();

typedef struct rule_s rule_t;
struct rule_s {
    voba_value_t pattern; 
    voba_value_t a_ast_action; // an list of ast
};
#define RULE(s)  VOBA_USER_DATA_AS(rule_t *,s)
extern voba_value_t voba_cls_rule;
voba_value_t make_rule();

enum pattern_type_e {
    PATTERN_CONSTANT,
    PATTERN_VAR,
    PATTERN_APPLY
};
typedef struct pattern_s pattern_t;
typedef struct pattern_constant_s pattern_constant_t;
typedef struct pattern_var_s pattern_var_t;
typedef struct pattern_apply_s pattern_apply_t;
struct pattern_constant_s {
    voba_value_t value;
};
struct pattern_var_s {
    voba_value_t var;
};
struct pattern_apply_s {
    voba_value_t cls;
    voba_value_t a_patterns; // an array of pattern
};
struct pattern_s {
    enum pattern_type_e type;
    union {
        pattern_constant_t constant;
        pattern_var_t var;
        pattern_apply_t apply;
    }u;
};
#define PATTERN(s)  VOBA_USER_DATA_AS(pattern_t *,s)
extern voba_value_t voba_cls_pattern;
voba_value_t make_pattern();


/* Local Variables: */
/* mode:c */
/* coding: undecided-unix */
/* End: */
