#pragma once
#define YYLTYPE_IS_DECLARED
struct YYLTYPE
{
  uint32_t start_pos;
  uint32_t end_pos;
};
typedef struct YYLTYPE YYLTYPE;
#define DEFINE_SOURCE_LOCATION                                          \
    voba_value_t source_info;                                           \
    uint32_t start_pos;                                                 \
    uint32_t end_pos;

typedef struct syntax_s {
    DEFINE_SOURCE_LOCATION
    voba_value_t v;
} syntax_t;
#define SYNTAX(s) VOBA_USER_DATA_AS(syntax_t *,s)
extern voba_value_t voba_cls_syn;
#include "parser.h"
static inline
voba_value_t make_source_info(voba_value_t filename, voba_value_t c)
{
    return voba_make_pair(filename,c);
}
static inline
void attach_source_info(voba_value_t syn /*syntax object*/
                        , voba_value_t source_info)
{
    SYNTAX(syn)->source_info = source_info;
    if(voba_is_array(SYNTAX(syn)->v)){
        for(size_t n = 0 ; n < voba_array_len(SYNTAX(syn)->v); ++n){
            attach_source_info(voba_array_at(SYNTAX(syn)->v,n),source_info);
        }
    }
}
inline static
voba_value_t syntax_source_filename(voba_value_t syn)
{
    return voba_head(SYNTAX(syn)->source_info);
}
inline static
voba_value_t syntax_source_content(voba_value_t syn)
{
    return voba_tail(SYNTAX(syn)->source_info);
}
static inline
void get_line_column(voba_str_t* c, uint32_t pos, uint32_t* line, uint32_t *col)
{
    *line = 1;
    *col = 0;
    for(uint32_t i = 0; i < pos && i < c->len; ++i){
        if(c->data[i] == '\n'){
            ++ *line;
            *col = 0;
        }else{
            ++ *col;
        }
    }
}
static inline void syn_get_line_column(int start, voba_value_t syn,uint32_t * line, uint32_t * col)
{
    uint32_t pos = start?SYNTAX(syn)->start_pos: SYNTAX(syn)->end_pos;
    voba_str_t * c = voba_value_to_str(syntax_source_content(syn));
    get_line_column(c,pos,line,col);
}
inline
static void syntax_loc(voba_value_t a , YYLTYPE* b)
{
    SYNTAX(a)->start_pos = b->start_pos;
    SYNTAX(a)->end_pos = b->end_pos;
}
inline
static voba_value_t make_syntax(voba_value_t v, YYLTYPE* b)
{
    voba_value_t ret = voba_make_user_data(voba_cls_syn, sizeof(syntax_t));
    SYNTAX(ret)->v = v;
    SYNTAX(ret)->start_pos = b->start_pos;
    SYNTAX(ret)->end_pos = b->end_pos;
    SYNTAX(ret)->source_info = VOBA_NIL;
    return ret;
}


