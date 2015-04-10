#include <voba/value.h>
#include <libgen.h>
#define EXEC_ONCE_TU_NAME "voba.compiler"
#include "compiler.h"
#include "syn.h"
#include "ast.h"
#include "c_backend.h"
#include "src2syn.h"
#include "syn2ast.h"
#include "ast2c.h"
static inline voba_value_t mydirname(voba_value_t filename1);
VOBA_FUNC static voba_value_t compile(voba_value_t fun, voba_value_t args, voba_value_t* next_fun, voba_value_t next_args[]);
EXEC_ONCE_PROGN{voba_symbol_set_value(s_compile, voba_make_func(compile));}
VOBA_FUNC static voba_value_t compile(voba_value_t fun, voba_value_t args, voba_value_t* next_fun, voba_value_t next_args[])
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
        voba_value_t toplevel = create_toplevel_env(module);
        TOPLEVEL_ENV(toplevel)->file_dirname = mydirname(filename);
        TOPLEVEL_ENV(toplevel)->full_file_name = filename;
        syn2ast(syn,module,toplevel);
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
static inline voba_value_t mydirname(voba_value_t filename1)
{
    voba_value_t ret = VOBA_NIL;
    uint32_t i = 0, j = 0;
    voba_str_t* filename = voba_value_to_str(filename1);
    const char sep = 
#ifdef _WIN32        
        '\\'
#else
        '/'
#endif
        ;
    for(i = 0 ; i < filename->len; ++i){
        j = filename->len - i - 1;
        if( filename->data[j] == sep){
            break;
        }
    }
    if(i == filename->len){
        ret = voba_make_string(voba_str_from_cstr("."));
    }else{
        ret = voba_make_string(voba_substr(filename,0,j));
    }
    return ret;
}
voba_value_t voba_init(voba_value_t this_module)
{
    return VOBA_NIL;
}
