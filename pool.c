#include <stdlib.h>
#include "utils.h"
#include <stdio.h>

typedef struct pool Pool;

typedef struct pool {
    int size;
    int* tags;
    void** values;
} Pool;


u_int8_t peek(u_int8_t** content, const int* cursor){
    return (*content)[*cursor];
}

u_int8_t consume(u_int8_t** content, int* cursor){
    return (*content)[(*cursor)++];
}

int load_int(u_int8_t** content, int* cursor){
    int i = ((consume(content, cursor) & 0xFF) << 24) |
        ((consume(content, cursor) & 0xFF) << 16) |
        ((consume(content, cursor) & 0xFF) << 8) |
        ((consume(content, cursor) & 0xFF));
    return i;
}


char* load_string(u_int8_t** content, int* cursor){
    int length = load_int(content, cursor);

    char* str = malloc((sizeof(char)*length)+1);
    for (int i = 0; i < length; i++){
        str[i] = (char) consume(content, cursor);
    }
    str[length] = '\0';
    return str;
}

Pool* load_pool(u_int8_t** content, int* cursor){
    Pool* pool = malloc(sizeof(Pool));
    u_int8_t size = consume(content, cursor);

    pool->size = size;

    pool->tags = malloc(sizeof(int) * size);
    pool->values = malloc(sizeof(void*) * size);

    for (int i = 0; i < size; i++){
        u_int8_t tag = consume(content, cursor);
        pool->tags[i] = tag;
        switch (tag) {
            case 0:
            case 1:
                pool->values[i] = (void*)(long)load_int(content, cursor);
                break;

            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
                pool->values[i] = load_string(content, cursor);
                break;

            default:
                error("unsupported tag type in constant-pool");
        }
    }

    return pool;
}
