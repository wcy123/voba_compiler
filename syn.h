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
    if(voba_is_a(SYNTAX(syn)->v,voba_cls_array)){
        for(size_t n = 0 ; n < voba_array_len(SYNTAX(syn)->v); ++n){
            attach_source_info(voba_array_at(SYNTAX(syn)->v,n),source_info);
        }
    }
}

