#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <exec_once.h>
#include <voba/value.h>
#include <voba/module.h>
int main(int argc, char *argv[])
{
    exec_once_init();
    voba_value_t symbols [] = {0};
    voba_import_module(argv[1],"__main__",voba_make_tuple(symbols));
    exit(EXIT_SUCCESS);
}
