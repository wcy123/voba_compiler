#include <stdlib.h>
#define EXEC_ONCE_TU_NAME "syn"
#include <voba/include/value.h>
#include "voba_str.h"
#include "parser.syn.h"
#include "ast.h"
#include "exec_once.h"
#include <voba/core/builtin.h>
#include "ast.h"
VOBA_DEF_CLS(sizeof(syntax_t),syn)



static inline voba_str_t* indent(voba_str_t * s, int level)
{
    return voba_strcat(s,voba_str_from_char(' ',level * 4));
}
static voba_str_t* dump_location(voba_value_t syn, int level)
{
    if(level > 10){
        exit(1);
    }
    voba_str_t * ret = voba_str_empty();
    ret = indent(ret,level);
    uint32_t start_line;
    uint32_t end_line;
    uint32_t start_col;
    uint32_t end_col;
    syn_get_line_column(1,syn,&start_line,&start_col);
    syn_get_line_column(0,syn,&end_line,&end_col);    
    ret = voba_vstrcat(ret,
                       VOBA_CONST_CHAR("(L"),
                       voba_str_fmt_uint32_t(start_line,10),
                       VOBA_CONST_CHAR(",C"),
                       voba_str_fmt_uint32_t(start_col,10),
                       VOBA_CONST_CHAR(")-(L"),
                       voba_str_fmt_uint32_t(end_line,10),
                       VOBA_CONST_CHAR(",C"),
                       voba_str_fmt_uint32_t(end_col,10),
                       VOBA_CONST_CHAR("):"),NULL);
    if(voba_is_a(SYNTAX(syn)->v,voba_cls_array)){
        ret = voba_strcat(ret,VOBA_CONST_CHAR("[\n"));
        for(size_t n = 0 ; n < voba_array_len(SYNTAX(syn)->v); ++n){
            ret = voba_strcat(ret,dump_location(voba_array_at(SYNTAX(syn)->v,n), level+1));
        }
        ret = indent(ret,level);
        ret = voba_strcat(ret,VOBA_CONST_CHAR("]"));
    }else{
        voba_value_t args[] = {1, SYNTAX(syn)->v};
        ret = voba_strcat(ret, voba_value_to_str(voba_apply(voba_symbol_value(s_str),
                                                            voba_make_array(args))));
        ret = voba_strcat_cstr(ret,"");
    }
    return ret;
}
VOBA_FUNC static voba_value_t str_syn(voba_value_t self, voba_value_t args)
{
    VOBA_ASSERT_N_ARG( args, 0); voba_value_t  syn = voba_array_at( args, 0);
VOBA_ASSERT_CLS( syn,voba_cls_syn, 0);
;
    return voba_make_string(dump_location(syn,0));
}

void attach_src(voba_value_t syn, voba_value_t src)
{
    SYNTAX(syn)->src = src;
    if(voba_is_a(SYNTAX(syn)->v,voba_cls_array)){
        for(size_t n = 0 ; n < voba_array_len(SYNTAX(syn)->v); ++n){
            attach_src(voba_array_at(SYNTAX(syn)->v,n),src);
        }
    }
}
voba_value_t make_syn_const(voba_value_t value)
{
    static YYLTYPE s = { 0,0};
    voba_value_t si = VOBA_NIL;
    voba_value_t ret = VOBA_NIL;
    si = make_src(
        VOBA_CONST_CHAR(__FILE__),
        VOBA_CONST_CHAR("nil"));
    ret = make_syntax(value,s.start_pos,s.end_pos);
    attach_src(ret,si);
    return ret;
}
/* voba_value_t syn_new1(voba_value_t v, voba_value_t syn) */
/* { */
/*     assert(voba_is_a(syn,voba_cls_syn)); */
/*     voba_value_t ret = make_syntax(VOBA_NIL,NULL); */
/*     syn_t * p_dst = SYNTAX(ret); */
/*     syn_t * p_src = SYNTAX(syn); */
/*     p_dst->start_pos = p_src->start_pos; */
/*     p_dst->end_pos = p_src->end_pos; */
/*     p_dst->src = p_src->src; */
/*     p_dst->v = v; */
/* } */
/* voba_value_t syn_new2(voba_value_t v, voba_value_t syn1,voba_value_t syn2) */
/* { */
/*     assert(voba_is_a(syn,voba_cls_syn)); */
/*     voba_value_t ret = make_syntax(VOBA_NIL,NULL); */
/*     syn_t * p_dst = SYNTAX(ret); */
/*     syn_t * p_src1 = SYNTAX(syn1); */
/*     syn_t * p_src2 = SYNTAX(syn2); */
/*     p_dst->start_pos = p_src1->start_pos; */
/*     p_dst->end_pos = p_src2->end_pos; */
/*     p_dst->src = p_src1->src; */
/*     p_dst->v = v; */
/* } */

voba_value_t make_syntax(voba_value_t v, uint32_t start_pos, uint32_t end_pos)
{
    voba_value_t ret = voba_make_user_data(voba_cls_syn);
    SYNTAX(ret)->v = v;
    SYNTAX(ret)->start_pos = start_pos;
    SYNTAX(ret)->end_pos = end_pos;
    SYNTAX(ret)->src = VOBA_NIL;
    return ret;
}
void syn_get_line_column(int start, voba_value_t syn,uint32_t * line, uint32_t * col)
{
    uint32_t pos = start?SYNTAX(syn)->start_pos: SYNTAX(syn)->end_pos;
    voba_str_t * c = SRC(SYNTAX(syn)->src)->content;
    get_line_column(c,pos,line,col);
}

EXEC_ONCE_PROGN{
    voba_gf_add_class(voba_symbol_value(s_str), voba_cls_syn, voba_make_func(str_syn));
}

