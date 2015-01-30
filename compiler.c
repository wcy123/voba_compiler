#include <voba/value.h>
#define EXEC_ONCE_TU_NAME "voba.compiler"
#include "compiler.h"
#include "syn.h"
#include "ast.h"
#include "c_backend.h"

#include "src2syn.h"
#include "syn2ast.h"
#include "ast2c.h"

VOBA_FUNC static voba_value_t compile(voba_value_t self, voba_value_t args);
EXEC_ONCE_PROGN{voba_symbol_set_value(s_compile, voba_make_func(compile));}
VOBA_FUNC static voba_value_t compile(voba_value_t self, voba_value_t args)
{
    VOBA_ASSERT_N_ARG( args, 0);
    voba_value_t  content = voba_tuple_at( args, 0);
    VOBA_ASSERT_ARG_ISA( content,voba_cls_str, 0);

    VOBA_ASSERT_N_ARG( args, 1);
    voba_value_t  filename = voba_tuple_at( args, 1);
    VOBA_ASSERT_ARG_ISA( filename,voba_cls_str, 1);

    voba_value_t ret  = VOBA_NIL;
    uint32_t error = 0;
    voba_value_t module = VOBA_NIL;
    voba_value_t syn = src2syn(content,filename,&module, &error);
    if(error == 0){
        voba_value_t toplevel = syn2ast(syn,module);
        if(error == 0){
            voba_value_t c = ast2c(toplevel);
            error = TOPLEVEL_ENV(toplevel)->n_of_errors;
            if(error == 0){
                ret = c;
            }else{
                fprintf(stderr,__FILE__ ":%d:[%s] total %d errors \n", __LINE__, __FUNCTION__,error);
            }
        }else{
            fprintf(stderr,__FILE__ ":%d:[%s] total %d errors \n", __LINE__, __FUNCTION__,error);
        }
    }else{
        fprintf(stderr,__FILE__ ":%d:[%s] syntax error %d errors \n", __LINE__, __FUNCTION__,error);
    }
    return ret;
}
voba_value_t voba_init(voba_value_t this_module)
{
    return VOBA_NIL;
}
