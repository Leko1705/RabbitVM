#include "utils.h"
#include "stdlib.h"

typedef struct v_function {
    char* name;
    u_int8_t locals;
    u_int8_t op_stack;
    u_int8_t** instructions;
} V_Function;

typedef struct v_method_table {
    u_int8_t size;
    char** names;
    int* addresses;
} V_Method_Table;

typedef struct type {
    u_int8_t size;
    char* name;
    V_Method_Table* v_methods;
} Type;


typedef struct r_object R_Object;

typedef struct r_object {
    Type* type;
    void** content;
} R_Object;

typedef struct context Context;

u_int8_t get_main_address(Context* ctx);

Context* init_components(char* file_name);

void clean_up(Context* ctx);

int get_pool_tag(Context* ctx, int idx);

void* get_pool_value(Context* ctx, int idx);

void* load_local(Context* ctx, int idx);

void store_local(Context* ctx, int idx, void* value);

void* op_stack_pop(Context* ctx);

void op_stack_push(Context* ctx, void* value);

char* curr_func_name(Context* ctx);

u_int8_t* fetch(Context* ctx);

void push_frame(Context* ctx, char* name, int locals, int op_stack, u_int8_t** instructions);

void pop_frame(Context* ctx);

bool frame_stack_is_empty(Context* ctx);

int get_frame_stack_size(Context* ctx);

void jump_to(Context* ctx, int address);

void set_line(Context* ctx, int line);

int get_curr_line(Context* ctx);
