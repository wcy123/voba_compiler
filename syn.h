#pragma once
#define YYLTYPE_IS_DECLARED
struct YYLTYPE
{
  uint32_t start_pos;
  uint32_t end_pos;
};
typedef struct YYLTYPE YYLTYPE;
typedef struct syntax_s {
    uint32_t start_pos;
    uint32_t end_pos;
    voba_value_t src;
    voba_value_t v;
} syntax_t;
#define SYNTAX(s) VOBA_USER_DATA_AS(syntax_t *,s)
extern voba_value_t voba_cls_syn;

void attach_src(voba_value_t syn, voba_value_t src);
voba_value_t make_syntax(voba_value_t v, uint32_t start, uint32_t end);
voba_value_t make_syn_const(voba_value_t value);
void syn_get_line_column(int start, voba_value_t syn,uint32_t * line, uint32_t * col);
//voba_value_t syn_new1(voba_value_t v, voba_value_t syn);
//voba_value_t syn_new2(voba_value_t v, voba_value_t syn1,voba_value_t syn2);
