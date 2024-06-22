#include <stdlib.h>

typedef struct pool Pool;

typedef struct pool {
    int size;
    int* tags;
    void** values;
} Pool;


Pool* load_pool(u_int8_t** content, int* cursor);

u_int8_t peek(u_int8_t** content, const int* cursor);

u_int8_t consume(u_int8_t** content, int* cursor);

int load_int(u_int8_t** content, int* cursor);

char* load_string(u_int8_t** content, int* cursor);
