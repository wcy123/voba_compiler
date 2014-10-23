#include <voba_str.h>
#define EXEC_ONCE_TU_NAME "c_backend"
#include <exec_once.h>
#include <voba/include/value.h>
#include <voba/core/builtin.h>
#include "c_backend.h"

static
DEFINE_CLS(sizeof(c_backend_t),c_backend);

static int is_c_backend(voba_value_t x)
{
    return voba_get_class(x) == voba_cls_c_backend;
}

voba_value_t make_c_backend()
{
    voba_value_t ret = voba_make_user_data(voba_cls_c_backend);
    c_backend_t * r = C_BACKEND(ret);
    r->decl = voba_str_empty();
    r->start = voba_str_empty();
    r->impl = voba_str_empty();
    return ret;
}
VOBA_FUNC static voba_value_t to_string_c_backend(voba_value_t self, voba_value_t args);
EXEC_ONCE_PROGN{
    voba_gf_add_class(voba_symbol_value(s_to_string),
                      voba_cls_c_backend,
                      voba_make_func(to_string_c_backend));
}
VOBA_FUNC static voba_value_t to_string_c_backend(voba_value_t self, voba_value_t args)
{
    voba_str_t * ret = voba_str_empty();
    VOBA_DEF_ARG(bk,args,0,is_c_backend);
    ret = voba_strcat(ret, C_BACKEND(bk)->decl);
    ret = voba_strcat(ret, C_BACKEND(bk)->impl);
    ret = voba_strcat(ret, VOBA_CONST_CHAR("EXEC_ONCE_PROGN {\n"));
    ret = voba_strcat(ret, C_BACKEND(bk)->start);
    ret = voba_strcat(ret, VOBA_CONST_CHAR("}\n"));
    return voba_make_string(ret); 
}



