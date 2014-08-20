#pragma once

typedef struct syntax_s {
    voba_value_t v;
    unsigned int first_line;
    unsigned int first_column;
    unsigned int last_line;
    unsigned int last_column;
} syntax_t;
#define SYNTAX(s) VOBA_USER_DATA_AS(syntax_t *,s)
extern voba_value_t voba_cls_syn;
#include "parser.h"
inline
static void syntax_loc(voba_value_t a , YYLTYPE *b)
{
    SYNTAX(a)->first_line = b->first_line;
    SYNTAX(a)->first_column = b->first_column;
    SYNTAX(a)->last_line = b->last_line;
    SYNTAX(a)->last_column = b->last_column;
}
inline
static voba_value_t make_syntax(voba_value_t v, YYLTYPE * b)
{
    voba_value_t ret = voba_make_user_data(voba_cls_syn, sizeof(syntax_t));
    SYNTAX(ret)->v = v;
    SYNTAX(ret)->first_line = b->first_line;
    SYNTAX(ret)->first_column = b->first_column;
    SYNTAX(ret)->last_line = b->last_line;
    SYNTAX(ret)->last_column = b->last_column;
    return ret;
}


