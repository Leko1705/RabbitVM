
#include <stdio.h>
#include <stdlib.h>
#include "utils.h"
#include "pool.h"
#include "env.h"
#include "string.h"

void read_file(char* file_name, u_int8_t** content, int* cursor){
    FILE *fileptr;
    u_int8_t *buffer;
    long filelen;

    fileptr = fopen(file_name, "rb");  // Open the file in binary mode
    if (fileptr == NULL){
        error("file not found");
        return;
    }
    fseek(fileptr, 0, SEEK_END);          // Jump to the end of the file
    filelen = ftell(fileptr);             // Get the current byte offset in the file
    rewind(fileptr);                      // Jump back to the beginning of the file

    buffer = (u_int8_t *)malloc(filelen * sizeof(u_int8_t)); // Enough memory for the file
    fread(buffer, filelen, 1, fileptr); // Read in the entire file
    fclose(fileptr); // Close the file

    *content = buffer;
    *cursor = 0;
}

void check_magic(u_int8_t** content_ptr, int* cursor){
    u_int8_t* content = *content_ptr;

    u_int8_t first = 0xDE;
    u_int8_t second = 0xAD;
    if(content[0] != first && content[1] != second){
        error("invalid magic number");
    }
    *cursor += 2;
}


typedef struct loaded {
    int main_addr;
    Pool* pool;
} Loaded;


Loaded* init_loaded_struct(int main_addr, Pool* pool){
    Loaded* loaded = malloc(sizeof(Loaded));
    loaded->main_addr = main_addr;
    loaded->pool = pool;
    return loaded;
}

u_int8_t** load_instructions(u_int8_t** content, int* cursor){
    int instruction_amount = load_int(content, cursor);
    u_int8_t** instructions = malloc(sizeof(u_int8_t*) * instruction_amount);

    for (int i = 0; i < instruction_amount; i++){
        u_int8_t cmd_size = consume(content, cursor);
        u_int8_t* instruction = malloc(sizeof(u_int8_t) * cmd_size);
        for (int j = 0; j < cmd_size; j++){
            instruction[j] = consume(content, cursor);
        }
        instructions[i] = instruction;
    }

    return instructions;
}

void put_pool(char* name, void* putted, Pool* pool, u_int8_t tag){
    int pool_size = pool->size;

    for (int i = 0; i < pool_size; i++){
        u_int8_t curr_tag = pool->tags[i];

        if(curr_tag == tag){
            char* curr_pool_val = pool->values[i];

            if (strcmp(curr_pool_val, name) == 0){
                free(curr_pool_val);
                pool->values[i] = putted;
                break;
            }
        }
    }
}

void load_functions(Pool* pool, u_int8_t** content, int* cursor){
    u_int8_t amount = consume(content, cursor);

    for (int i = 0; i < amount; i++){
        V_Function* function = malloc(sizeof(V_Function));

        function->name = load_string(content, cursor);
        function->op_stack = consume(content, cursor);
        function->locals = consume(content, cursor);
        function->instructions = load_instructions(content, cursor);

        put_pool(function->name, function, pool, 3);
    }
}

void load_structs(Pool* pool, u_int8_t** content, int* cursor){
    u_int8_t amount = consume(content, cursor);
    for (int i = 0; i < amount; i++){
        Type* type = malloc(sizeof(Type));

        type->name = load_string(content, cursor);
        type->size = consume(content, cursor);

        u_int8_t method_cnt = consume(content, cursor);

        if (method_cnt != 0) {
            type->v_methods = malloc(sizeof(V_Method_Table));
            type->v_methods->size = method_cnt;

            type->v_methods->names = malloc(sizeof(char *) * method_cnt);
            type->v_methods->addresses = malloc(sizeof(int) * method_cnt);

            for (int j = 0; j < method_cnt; j++) {
                char *name = load_string(content, cursor);
                u_int8_t address = consume(content, cursor);
                type->v_methods->names[j] = name;
                type->v_methods->addresses[j] = address;
            }

        }
        else {
            type->v_methods = NULL;
        }


        put_pool(type->name, type, pool, 5);
    }
}

Loaded* load(char* file_name){
    u_int8_t* content;
    int cursor;
    read_file(file_name, &content, &cursor);
    check_magic(&content, &cursor);

    int major = load_int(&content, &cursor);
    int minor = load_int(&content, &cursor);

    if (minor < 1) error("unsupported minor version");
    if (major > 1) error("unsupported minor version");

    int main_addr = content[cursor++];

    Pool* pool = load_pool(&content, &cursor);
    load_functions(pool, &content, &cursor);
    load_structs(pool, &content, &cursor);

    free(content);
    return init_loaded_struct(main_addr, pool);
}


void free_pool(Pool* pool){
    for (int i = 0; i < pool->size; ++i) {
        u_int8_t tag = pool->tags[i];
        switch (tag) {
            case 2:
            case 4:
            case 6:
                free(pool->values[i]);
                break;

            case 3:
            {
                V_Function* func = pool->values[i];
                free(func->name);
                free(func->instructions);
                free(func);
            }
                break;

            case 5:
            {
                Type* type = pool->values[i];
                free(type->name);
                for (int j = 0; j < type->v_methods->size; ++j) {
                    free(type->v_methods->names[i]);
                }
                free(type->v_methods->addresses);
                free(type->v_methods);
                free(type);
            }
                break;

            default:
                break;
        }
    }
}

