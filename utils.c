#include "stdio.h"
#include "stdlib.h"

void error(char* msg){
    fprintf(stderr, "%s", msg);
    exit(0);
}
