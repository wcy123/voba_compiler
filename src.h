#pragma once

typedef struct src_s {
    voba_str_t* filename;
    voba_str_t* content;
} src_t;
#define SRC(s) VOBA_USER_DATA_AS(src_t *,s)
extern voba_value_t voba_cls_src;

voba_value_t make_src(voba_str_t* filename, voba_str_t* c);



