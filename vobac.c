#include <stdio.h>
#include <stdlib.h>
#define EXEC_ONCE_TU_NAME "test_parser"
#include <exec_once.h>
#include <voba/value.h>
#include <voba/module.h>
#include <voba/core/builtin.h>
#include <voba/core/compiler.h>


int main(int argc, char *argv[])
{
    exec_once_init();
    if(argc <= 1) {
        fprintf(stderr,"%s filename\n",argv[0]);
        return 1;
    }
    FILE * fp = fopen(argv[1],"r");
    if(fp == NULL){
        char buf[1024];
        snprintf(buf,1024,"cannot open file %s\n",argv[1]);
        perror(buf);
        return 1;
    }
    voba_str_t * ss = voba_str_from_FILE(fp);
    if(ss == NULL){
        fprintf(stderr,__FILE__ ":%d:[%s] cannot open file.\n", __LINE__, __FUNCTION__);
        return 1;
    }
    fclose(fp);
    voba_value_t args [] = {
        2,
        voba_make_string(ss),
        voba_make_string(voba_str_from_cstr(argv[1]))
    };
    voba_value_t c = voba_apply(voba_symbol_value(s_compile), voba_make_tuple(args));
    args[0]=1;
    args[1] = c;
    voba_apply(voba_symbol_value(s_print),voba_make_tuple(args));
    return 0;
}
