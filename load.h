#include "pool.h"

typedef struct loaded {
    int main_addr;
    Pool* pool;
} Loaded;

Loaded* load(char* file_name);

void free_pool(Pool* pool);
