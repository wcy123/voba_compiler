#include <stdlib.h>
#define EXEC_ONCE_TU_NAME "voba.compiler.syn"
#include <voba/value.h>
#include "voba_str.h"
#include "parser.syn.h"
#include "ast.h"
#include "exec_once.h"
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
    if(0)ret = voba_vstrcat(ret,
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
        ret = voba_strcat(ret, voba_value_to_str(voba_apply(voba_gf_to_string,
                                                            voba_make_tuple(args))));
        ret = voba_strcat_cstr(ret,"");
    }
    return ret;
}
VOBA_FUNC static voba_value_t str_syn(voba_value_t fun, voba_value_t args, voba_value_t* next_fun, voba_value_t next_args[])
{
    VOBA_ASSERT_N_ARG( args, 0); voba_value_t  syn = voba_tuple_at( args, 0);
    VOBA_ASSERT_ARG_ISA( syn,voba_cls_syn, 0);
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
voba_value_t make_syn_const(const char * filename,
			    int line,
			    const char * src,
			    int src_length,
			    voba_value_t value)
{
    voba_value_t si = VOBA_NIL;
    voba_value_t ret = VOBA_NIL;
    si = make_src(
        voba_str_from_cstr(filename),
        voba_str_from_cstr(src));
    ret = make_syntax(value,0,(uint32_t)src_length-1);
    attach_src(ret,si);
    return ret;
}
voba_value_t syn_new1(voba_value_t v, voba_value_t syn)
{
    assert(voba_is_a(syn,voba_cls_syn));
    voba_value_t ret = make_syntax(VOBA_NIL,0,0);
    syntax_t * p_dst = SYNTAX(ret);
    syntax_t * p_src = SYNTAX(syn);
    p_dst->start_pos = p_src->start_pos;
    p_dst->end_pos = p_src->end_pos;
    p_dst->src = p_src->src;
    p_dst->v = v;
    return ret;
}
voba_value_t syn_new2(voba_value_t v, voba_value_t syn1,voba_value_t syn2)
{
    assert(voba_is_a(syn1,voba_cls_syn));
    voba_value_t ret = make_syntax(VOBA_NIL,0,0);
    syntax_t * p_dst = SYNTAX(ret);
    syntax_t * p_src1 = SYNTAX(syn1);
    syntax_t * p_src2 = SYNTAX(syn2);
    p_dst->start_pos = p_src1->start_pos;
    p_dst->end_pos = p_src2->end_pos;
    p_dst->src = p_src1->src;
    p_dst->v = v;
    return ret;
}

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
    voba_gf_add_class(voba_gf_to_string, voba_cls_syn, voba_make_func(str_syn));
}

voba_value_t syn_backquote(voba_value_t n, ...)
{
    voba_value_t ret = voba_make_array_0();
    voba_value_t syn_start = SYN_NIL;
    voba_value_t syn_end = SYN_NIL;
    va_list ap;
    va_start(ap,n);
    for(voba_value_t i = 0; i < n ; ++i){
	voba_value_t v = va_arg(ap,voba_value_t);
	if(voba_is_a(v,voba_cls_syn)){
	    if(i==0) syn_start = v;
	    syn_end = v;
	    ret = voba_array_push(ret,v);
	}else if(voba_is_a(v,voba_cls_array)){
	    int64_t len = voba_array_len(v);
	    if(i==0 && len > 0 ) syn_start = voba_array_at(v,0);
	    if(len > 0) syn_end = voba_array_at(v,len-1);
	    ret = voba_array_concat(ret,v);
	}else{
	    assert(0&&"never goes here");
	}
    }
    va_end(ap);
    return syn_new2(ret,syn_start,syn_end);
}
voba_value_t syn_unlist(voba_value_t v)
{
    return SYNTAX(v)->v;
}
