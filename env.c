#include "stdlib.h"
#include "load.h"
#include "utils.h"
#include <stdio.h>

typedef struct frame Frame;

typedef struct frame {
    char* name;
    void** locals;
    void** op_stack;
    Frame* prev;
    int sp;
    int ip;
    u_int8_t** instructions;
    int line;
} Frame;


typedef struct context {
    Loaded* areas;
    Frame* top_frame;
    int call_stack_size;
} Context;

Context* init_components(char* file_name) {
    Loaded *loaded = load(file_name);
    Context *ctx = malloc(sizeof(Context));
    ctx->areas = loaded;
    ctx->top_frame = NULL;
    return ctx;
}

u_int8_t get_main_address(Context* ctx){
    return ctx->areas->main_addr;
}


int get_pool_tag(Context* ctx, int idx){
    return ctx->areas->pool->tags[idx];
}

void* get_pool_value(Context* ctx, int idx){
    return ctx->areas->pool->values[idx];
}

void* load_local(Context* ctx, int idx){
    return ctx->top_frame->locals[idx];
}

void store_local(Context* ctx, int idx, void* value){
    ctx->top_frame->locals[idx] = value;
}

void* op_stack_pop(Context* ctx){
    return ctx->top_frame->op_stack[--ctx->top_frame->sp];
}

void op_stack_push(Context* ctx, void* value){
    ctx->top_frame->op_stack[ctx->top_frame->sp++] = value;
}

char* curr_func_name(Context* ctx){
    return ctx->top_frame->name;
}

u_int8_t* fetch(Context* ctx){
    return ctx->top_frame->instructions[ctx->top_frame->ip++];
}


Frame* init_frame(char* name, int locals, int op_stack, u_int8_t** instructions){
    Frame* new_frame = malloc(sizeof(Frame));
    new_frame->name = name;
    new_frame->sp = 0;
    new_frame->ip = 0;
    new_frame->locals = malloc(sizeof(void*) * locals);
    new_frame->op_stack = malloc(sizeof(void*) * op_stack);
    new_frame->instructions = instructions;
    new_frame->line = -1;
    return new_frame;
}

void push_frame(Context* ctx, char* name, int locals, int op_stack, u_int8_t** instructions){
    Frame* new_top = init_frame(name, locals, op_stack, instructions);
    Frame* old_top = ctx->top_frame;
    new_top->prev = old_top;
    ctx->top_frame = new_top;
    ctx->call_stack_size++;
}

void pop_frame(Context* ctx){
    Frame* top = ctx->top_frame;
    ctx->top_frame = top->prev;
    free(top->locals);
    free(top->op_stack);
    free(top);
    ctx->call_stack_size--;
}

bool frame_stack_is_empty(Context* ctx){
    return ctx->top_frame == NULL;
}

int get_frame_stack_size(Context* ctx){
    return ctx->call_stack_size;
}

void jump_to(Context* ctx, int address){
    ctx->top_frame->ip = address;
}

void set_line(Context* ctx, int line){
    ctx->top_frame->line = line;
}

int get_curr_line(Context* ctx){
    return ctx->top_frame->line;
}


void clean_up(Context* ctx){
    while (ctx->top_frame != NULL){
        pop_frame(ctx);
    }
    free_pool(ctx->areas->pool);
    free(ctx->areas);
    free(ctx);
}
