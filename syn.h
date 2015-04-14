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
voba_value_t make_syn_const(const char * filename,
			    int line,
			    const char * src,
			    int src_length,	
			    voba_value_t value);
void syn_get_line_column(int start, voba_value_t syn,uint32_t * line, uint32_t * col);
voba_value_t syn_new1(voba_value_t v, voba_value_t syn);
voba_value_t syn_new2(voba_value_t v, voba_value_t syn1,voba_value_t syn2);
/** @brief return a new syntax object */
voba_value_t syn_backquote(voba_value_t n, ...);

#define MAKE_SYN_CONST(v) make_syn_const(__FILE__,__LINE__,#v,sizeof(#v), (v))
#define SYN_SYMBOL(x) MAKE_SYN_CONST(voba_make_symbol(voba_str_from_cstr(#x) ,VOBA_NIL))
#define SYN_NIL (MAKE_SYN_CONST(VOBA_NIL))

