#include <voba/include/value.h>
#define EXEC_ONCE_TU_NAME "compiler"
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
    VOBA_DEF_ARG4(content, args, 0, voba_is_string);
    VOBA_DEF_ARG4(filename, args, 1, voba_is_string);
    int error = 0;
    voba_value_t module = VOBA_NIL;
    voba_value_t syn = src2syn(content,filename,&module, &error);
    if(error){
        fprintf(stderr,__FILE__ ":%d:[%s] syntax error %d errors \n", __LINE__, __FUNCTION__,error);
        return syn;
    }
    voba_value_t toplevel = syn2ast(syn,module,&error);
    if(error){
        fprintf(stderr,__FILE__ ":%d:[%s] total %d errors \n", __LINE__, __FUNCTION__,error);
        return toplevel;
    }
    voba_value_t c = ast2c(TOPLEVEL_ENV(toplevel));
    return c;
}
voba_value_t voba_init(voba_value_t this_module)
{
    exec_once_init();
    return VOBA_NIL;
}
