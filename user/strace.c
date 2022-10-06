#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    int i;
    char *newArgs[MAXARG];

    if(argc < 3){
        fprintf(2, "Too few Arguments\n");
        return 1;
    }

    if (trace(atoi(argv[1])) < 0) {
        fprintf(2, "Trace failed\n");
        return 1;
    }
    
    for(i = 2; i < argc && i < MAXARG; i++){
        newArgs[i-2] = argv[i];
    }
    exec(newArgs[0], newArgs);
    return 0;
}