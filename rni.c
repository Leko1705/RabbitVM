#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include <string.h>

void* println(void* arg){
    if(arg == NULL)
        printf("%s\n", "null");
    else
        printf("%s\n", (char*)arg);
    return NULL;
}









bool string_equals(char* given, char* expected){
    return strcmp(given, expected) == 0;
}

int rni_argc_of(char* name){
    if(string_equals(name, "println"))
        return 1;

    fprintf(stderr, "%s%s%s", "can not find native function '", name, "'");
    exit(-1);
}

void* rni_invoke(char* name, void** args){

    if(string_equals(name, "println"))
        return println(args[0]);

    /* should never happen */
    error("assertion error: should never happen -> native function not found");
    return NULL;
}
