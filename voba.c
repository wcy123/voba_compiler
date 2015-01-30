#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <exec_once.h>

int main(int argc, char *argv[])
{
    void *handle;
    handle = dlopen(argv[1], RTLD_LAZY|RTLD_GLOBAL);
    if (!handle) {
        fprintf(stderr, "%s\n", dlerror());
        exit(EXIT_FAILURE);
    }
    exec_once_init();
    dlclose(handle);    
    exit(EXIT_SUCCESS);
}
