#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#define EXEC_ONCE_TU_NAME "voba"
#define EXEC_ONCE_DEPENDS {"voba.module"}
#include <voba/value.h>
///@todo clean up this messy dependency.
#include "../voba_builtin/sys/sys.h"
static voba_value_t get_argv(int argc, char * argv[])
{
    voba_value_t * p = (voba_value_t*)GC_MALLOC(sizeof(voba_value_t)*(argc+2));
    assert(p);
    p[0] = argc;
    for(int i = 0; i < argc; i ++){
        p[i+1] = voba_make_string(voba_str_from_cstr(argv[i]));
    }
    p[argc+1] = VOBA_BOX_END;
    return voba_make_tuple(p);
}
int main(int argc, char *argv[])
{
    void *handle;
    exec_once_init();
    voba_symbol_set_value(s_argv,get_argv(argc,argv));
    ///@todo, clean up hard coded file name
    handle = dlopen("./build/lib/voba/core/./libprelude.so", RTLD_LAZY|RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    exec_once_init();
    dlclose(handle);    
    exit(EXIT_SUCCESS);
}
