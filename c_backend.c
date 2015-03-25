#define EXEC_ONCE_TU_NAME "voba.compiler.c_backend"
#include <exec_once.h>
#include <voba/value.h>
#include "c_backend.h"

static
VOBA_DEF_CLS(sizeof(c_backend_t),c_backend);


voba_value_t make_c_backend()
{
    voba_value_t ret = voba_make_user_data(voba_cls_c_backend);
    c_backend_t * r = C_BACKEND(ret);
    r->decl = voba_str_empty();
    r->start = voba_str_empty();
    r->impl = voba_str_empty();
    r->it = NULL;
    r->latest_for_end_label= NULL;
    r->latest_for_final= NULL;
    return ret;
}
VOBA_FUNC static voba_value_t str_c_backend(voba_value_t self, voba_value_t args);
EXEC_ONCE_PROGN{
    voba_gf_add_class(voba_gf_to_string,
                      voba_cls_c_backend,
                      voba_make_func(str_c_backend));
}
VOBA_FUNC static voba_value_t str_c_backend(voba_value_t self, voba_value_t args)
{
    voba_str_t * ret = voba_str_empty();
    VOBA_ASSERT_N_ARG(args,0); voba_value_t bk = voba_tuple_at(args,0);
    VOBA_ASSERT_ARG_ISA(bk,voba_cls_c_backend,0);

    ret = voba_strcat(ret, C_BACKEND(bk)->decl);
    ret = voba_strcat(ret, C_BACKEND(bk)->impl);
    ret = voba_strcat(ret, VOBA_CONST_CHAR("EXEC_ONCE_PROGN {\n"));
    ret = voba_strcat(ret, C_BACKEND(bk)->start);
    ret = voba_strcat(ret, VOBA_CONST_CHAR("}\n"));
    return voba_make_string(ret); 
}



