#include "kernel/param.h"
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if(argc < 2){
        fprintf(2, "Too few Arguments\n");
        return 1;
    }

    if (set_priority(atoi(argv[1]),atoi(argv[2])) < 0) {
        fprintf(2, "Set Priority failed\n");
        return 1;
    }
    
    return 0;
}