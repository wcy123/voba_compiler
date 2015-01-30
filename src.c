#define EXEC_ONCE_TU_NAME "voba.compiler.src"
#include <exec_once.h>
#include <voba/value.h>
#include "src.h"
VOBA_DEF_CLS(sizeof(src_t),src);
voba_value_t make_src(voba_str_t* filename, voba_str_t* c)
{
    voba_value_t ret = voba_make_user_data(voba_cls_src);
    src_t * p_src  = SRC(ret);
    p_src->filename = filename;
    p_src->content = c;
    return ret;
}
