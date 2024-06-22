#include "env.h"
#include "utils.h"
#include "stdio.h"
#include "rni.h"
#include <string.h>

#define MAX_RECURSION_LIMIT 300000

static char* ARRAY_NAME = "arr";

enum Opcode {
    PUSH_NULL,
    PUSH_INT,
    LOAD_CONST,

    LOAD_LOCAl,
    STORE_LOCAL,

    NEW,
    FREE,
    NULL_CHECK,
    CHECK_CAST,
    I2F,
    F2I,

    MAKE_ARRAY,
    READ_ARRAY,
    WRITE_ARRAY,

    GET_FIELD,
    PUT_FIELD,

    INVOKE_VIRTUAL,
    INVOKE_TEMPLATE,
    INVOKE_NATIVE,
    RETURN,

    DUP,
    SWAP,
    POP,

    NOT,
    NEG,

    ADD_I,
    SUB_I,
    MUL_I,
    MOD,
    AND,
    OR,
    AND_BIT,
    OR_BIT,
    XOR,
    SHIFT_AL,
    SHIFT_AR,
    ADD_F,
    SUB_F,
    MUL_F,
    DIV,
    EQUALS,
    NOT_EQUALS,
    LESS,
    GREATER,
    LESS_EQ,
    GREATER_EQ,


    GOTO,
    BRANCH_NOT_ZERO,
    BRANCH_ZERO,

    NEW_LINE,

};


void new_obj(Context* ctx, int addr){
    Type* type = get_pool_value(ctx, addr);
    R_Object * obj = malloc(sizeof(R_Object));
    obj->type = type;
    obj->content = malloc(sizeof(void*) * type->size);
    op_stack_push(ctx, obj);
}

void get_field(Context* ctx, int addr){
    R_Object* obj = op_stack_pop(ctx);
    op_stack_push(ctx, obj->content[addr]);
}

void put_field(Context* ctx, int addr){
    R_Object* obj = op_stack_pop(ctx);
    void* value = op_stack_pop(ctx);
    obj->content[addr] = value;
}

void report_cast_failed(Context* ctx, Type* req, Type* giv){
    fprintf(stderr, "%s%s%s%s%s%s%s%d%s",
            "can not cast ", giv->name, " to ", req->name, " (in ", curr_func_name(ctx), ": line ", get_curr_line(ctx), ")");
    clean_up(ctx);
    exit(-1);
}

void check_cast(Context* ctx, int addr){
    R_Object* obj = op_stack_pop(ctx);

    Type* giv_type = obj->type;
    Type* req_type = get_pool_value(ctx, addr);

    if(req_type->size != giv_type->size)
        report_cast_failed(ctx, req_type, giv_type);

    if (strcmp(giv_type->name, req_type->name) != 0)
        report_cast_failed(ctx, req_type, giv_type);

    op_stack_push(ctx, obj);
}


void null_check(Context* ctx){
    void* value = op_stack_pop(ctx);
    if (value == NULL) {
        fprintf(stderr, "%s%s%s%d%s", "null pointer error (in ", curr_func_name(ctx), ": line ", get_curr_line(ctx), ")");
        clean_up(ctx);
        exit(-1);
    }
    op_stack_push(ctx, value);
}

void dup(Context* ctx){
    void* value = op_stack_pop(ctx);
    op_stack_push(ctx, value);
    op_stack_push(ctx, value);
}

void swap(Context* ctx){
    void* old_top = op_stack_pop(ctx);
    void* new_top = op_stack_pop(ctx);
    op_stack_push(ctx, old_top);
    op_stack_push(ctx, new_top);
}


void invoke_virtual(Context* ctx, u_int8_t pool_addr, int argc) {
    if(get_frame_stack_size(ctx) == MAX_RECURSION_LIMIT){
        fprintf(stderr, "%s%s%s%d%s",
                "too many recursions (in ", curr_func_name(ctx), ": line ", get_curr_line(ctx), ")");
        clean_up(ctx);
        exit(-1);
    }
    V_Function* v_func = get_pool_value(ctx, pool_addr);
    void* args[argc];
    for (int i = 0; i < argc; i++) args[i] = op_stack_pop(ctx);
    push_frame(ctx, v_func->name, v_func->locals, v_func->op_stack, v_func->instructions);
    for (int i = argc-1; i >= 0; i--) op_stack_push(ctx, args[i]);
}

void invoke_template(Context* ctx, u_int8_t pool_addr, int argc){
    char* name = get_pool_value(ctx, pool_addr);
    R_Object* obj = op_stack_pop(ctx);
    Type* type = obj->type;
    if(type->v_methods == NULL) {
        fprintf(stderr, "%s%s%s%s%s%d%s",
                "can not find implementation of '", name, "' (in ", type->name, ": line ",get_curr_line(ctx), ")");
        clean_up(ctx);
        exit(-1);
    }

    int addr = -1;
    for (int i = 0; i < type->v_methods->size; i++){
        if (strcmp(name, type->v_methods->names[i]) == 0){
            addr = type->v_methods->addresses[i];
            break;
        }
    }

    if(addr == -1) {
        fprintf(stderr, "%s%s%s%s%s%d%s",
                "can not find implementation of '", name, "' (in ", type->name, ": line ",get_curr_line(ctx), ")");
        clean_up(ctx);
        exit(-1);
    }

    invoke_virtual(ctx, addr, argc);
}

void invoke_native(Context* ctx, u_int8_t pool_addr, int argc) {
    char* name = get_pool_value(ctx, pool_addr);
    int req_argc = rni_argc_of(name);
    if (argc != req_argc){
        fprintf(stderr, "%s%s%s%s%s", "invalid argument count for native function '", name, "' (in ", curr_func_name(ctx), ")");
        clean_up(ctx);
        exit(-1);
    }
    void* args[argc];
    for (int  i = 0; i < argc; i++) args[i] = op_stack_pop(ctx);
    void* res = rni_invoke(name, args);
    op_stack_push(ctx, res);
}

void return_virtual(Context* ctx){
    void* return_value = op_stack_pop(ctx);
    pop_frame(ctx);
    if (!frame_stack_is_empty(ctx))
        op_stack_push(ctx, return_value);
}


Type* make_array_type(){
    Type* type = malloc(sizeof(Type));
    type->name = ARRAY_NAME;
    type->v_methods = NULL;
    type->size = 0;
    return type;
}

void make_array(Context* ctx, int size){
    R_Object* arr = malloc(sizeof(R_Object));

    arr->content = malloc(sizeof(size+1));
    arr->type = make_array_type();
    arr->content[0] = (void*)(long)size;
    for (int  i = 0; i < size; i++){
        arr->content[i+1] = op_stack_pop(ctx);
    }
    op_stack_push(ctx, arr);
}

void check_bounds(Context* ctx, int idx, int size){
    if(idx < 0 || idx >= size){
        fprintf(stderr, "%s%d%s%d%s%s%s%d%s",
                "index ", idx, " out of bounds for array length ", size,
                " (in ", curr_func_name(ctx), ": line ", get_curr_line(ctx), ")");
        clean_up(ctx);
        exit(-1);
    }
}

void load_array(Context* ctx, int idx){
    R_Object * array = op_stack_pop(ctx);
    check_bounds(ctx, idx, (int)(long)array->content[0]);
    op_stack_push(ctx, array->content[idx+1]);
}

void store_array(Context* ctx, int idx){
    R_Object * array = op_stack_pop(ctx);
    check_bounds(ctx, idx, (int)(long)array->content[0]);
    void* value = op_stack_pop(ctx);
    array->content[idx+1] = value;
}

void load_const(Context* ctx, u_int8_t addr){
    int tag = get_pool_tag(ctx, addr);

    switch (tag) {

        case 0:
        case 1:
        case 2:
            op_stack_push(ctx, get_pool_value(ctx, addr));
            break;

        default:
            break;
    }
}

void jump_branch(Context* ctx, int address, u_int8_t when){
    int b = ((int)(long)op_stack_pop(ctx));
    if (b == when) jump_to(ctx, address);
}

int int_from_2_bytes(u_int8_t b1, u_int8_t b2){
    return ((b1 & 0xff) << 8) | (b2 & 0xff);
}

void free_op(Context* ctx){
    void* ptr = op_stack_pop(ctx);
    if (ptr != NULL)
        free(ptr);
}



float interpret_float(int i){
    union {
        int i;
        float f;
    } converter;
    converter.i = i;
    return converter.f;
}

int interpret_int(float f){
    union {
        int i;
        float f;
    } converter;
    converter.f = f;
    return converter.i;
}

void not(Context* ctx){
    int* value = op_stack_pop(ctx);
    value = (void*)(((long)value) ^ -1);
    op_stack_push(ctx, value);
}

void negate(Context* ctx){
    int* value = op_stack_pop(ctx);
    value = (void*)(((long)value) * -1);
    op_stack_push(ctx, value);
}

void operate_f(Context* ctx, float (*operation_func)(float, float)){
    float left_val = interpret_float((int)(long)op_stack_pop(ctx));
    float right_val = interpret_float((int)(long)op_stack_pop(ctx));
    float res = operation_func(left_val, right_val);
    op_stack_push(ctx, (void*)(long)interpret_int(res));
}

void f2i(Context* ctx){
    float f_val = interpret_float((int)(long)op_stack_pop(ctx));
    int i_val = (int) f_val;
    op_stack_push(ctx, (void*)(long)(i_val));
}

void i2f(Context* ctx){
    int i_val = (int)(long)op_stack_pop(ctx);
    float f_val = (float) i_val;
    op_stack_push(ctx, (void*)(long)interpret_int(f_val));
}

float add_f(float f1, float f2){
    return f1 + f2;
}

float sub_f(float f1, float f2){
    return f1 - f2;
}

float mul_f(float f1, float f2){
    return f1 * f2;
}

float div_f(float f1, float f2){
    return f1 / f2;
}

void decode_and_execute(Context* ctx, u_int8_t* inst){
    u_int8_t opc = inst[0];

    switch (opc) {
        case PUSH_NULL:
            op_stack_push(ctx, NULL);
            break;

        case PUSH_INT:
            op_stack_push(ctx, (void*)(long)inst[1]);
            break;

        case LOAD_CONST:
            load_const(ctx, inst[1]);
            break;

        case LOAD_LOCAl:
            op_stack_push(ctx, load_local(ctx, inst[1]));
            break;

        case STORE_LOCAL:
            store_local(ctx, inst[1], op_stack_pop(ctx));
            break;

        case NULL_CHECK:
            null_check(ctx);
            break;

        case CHECK_CAST:
            check_cast(ctx, inst[1]);
            break;

        case F2I:
            f2i(ctx);
            break;

        case I2F:
            i2f(ctx);
            break;

        case MAKE_ARRAY:
            make_array(ctx, inst[1]);
            break;

        case READ_ARRAY:
            load_array(ctx, inst[1]);
            break;

        case WRITE_ARRAY:
            store_array(ctx, inst[1]);
            break;

        case NEW:
            new_obj(ctx, inst[1]);
            break;

        case FREE:
            free_op(ctx);
            break;

        case GET_FIELD:
            get_field(ctx, inst[1]);
            break;

        case PUT_FIELD:
            put_field(ctx, inst[1]);
            break;

        case INVOKE_VIRTUAL:
            invoke_virtual(ctx, inst[1], inst[2]);
            break;

        case INVOKE_TEMPLATE:
            invoke_template(ctx, inst[1], inst[2]);
            break;

        case INVOKE_NATIVE:
            invoke_native(ctx, inst[1], inst[2]);
            break;

        case RETURN:
            return_virtual(ctx);
            break;

        case DUP:
            dup(ctx);
            break;

        case SWAP:
            swap(ctx);
            break;

        case POP:
            op_stack_pop(ctx);
            break;

        case NOT:
            not(ctx);
            break;

        case NEG:
            negate(ctx);
            break;

        case GOTO:
            jump_to(ctx, int_from_2_bytes(inst[1], inst[2]));
            break;

        case BRANCH_ZERO:
            jump_branch(ctx, int_from_2_bytes(inst[1], inst[2]), 0);
            break;

        case BRANCH_NOT_ZERO:
            jump_branch(ctx, int_from_2_bytes(inst[1], inst[2]), 1);
            break;

        case NEW_LINE:
            set_line(ctx, int_from_2_bytes(inst[1], inst[2]));
            break;

        case ADD_I:
            op_stack_push(ctx, (void*)(long)(((int)(long) op_stack_pop(ctx)) + ((int)(long) op_stack_pop(ctx))));
            break;

        case SUB_I:
            op_stack_push(ctx, (void*)(long)(((int)(long) op_stack_pop(ctx)) - ((int)(long) op_stack_pop(ctx))));
            break;

        case MOD:
            op_stack_push(ctx, (void*)(long)(((int)(long) op_stack_pop(ctx)) % ((int)(long) op_stack_pop(ctx))));
            break;

        case MUL_I:
            op_stack_push(ctx, (void*)(long)(((int)(long) op_stack_pop(ctx)) * ((int)(long) op_stack_pop(ctx))));
            break;

        case AND:
            op_stack_push(ctx, (void*)(long)(((int)(long) op_stack_pop(ctx)) && ((int)(long) op_stack_pop(ctx))));
            break;

        case OR:
            op_stack_push(ctx, (void*)(long)(((int)(long) op_stack_pop(ctx)) || ((int)(long) op_stack_pop(ctx))));
            break;

        case AND_BIT:
            op_stack_push(ctx, (void*)(long)(((int)(long) op_stack_pop(ctx)) & ((int)(long) op_stack_pop(ctx))));
            break;

        case OR_BIT:
            op_stack_push(ctx, (void*)(long)(((int)(long) op_stack_pop(ctx)) | ((int)(long) op_stack_pop(ctx))));
            break;

        case XOR:
            op_stack_push(ctx, (void*)(long)(((int)(long) op_stack_pop(ctx)) ^ ((int)(long) op_stack_pop(ctx))));
            break;

        case SHIFT_AL:
            op_stack_push(ctx, (void*)(long)(((int)(long) op_stack_pop(ctx)) << ((int)(long) op_stack_pop(ctx))));
            break;

        case SHIFT_AR:
            op_stack_push(ctx, (void*)(long)(((int)(long) op_stack_pop(ctx)) >> ((int)(long) op_stack_pop(ctx))));
            break;

        case EQUALS:
            op_stack_push(ctx, (void*)(long)(op_stack_pop(ctx) == op_stack_pop(ctx)));
            break;

        case NOT_EQUALS:
            op_stack_push(ctx, (void*)(long)(op_stack_pop(ctx) != op_stack_pop(ctx)));
            break;

        case LESS:
            op_stack_push(ctx, (void*)(long)(((int)(long) op_stack_pop(ctx)) < ((int)(long) op_stack_pop(ctx))));
            break;

        case GREATER:
            op_stack_push(ctx, (void*)(long)(((int)(long) op_stack_pop(ctx)) > ((int)(long) op_stack_pop(ctx))));
            break;

        case LESS_EQ:
            op_stack_push(ctx, (void*)(long)(((int)(long) op_stack_pop(ctx)) <= ((int)(long) op_stack_pop(ctx))));
            break;

        case GREATER_EQ:
            op_stack_push(ctx, (void*)(long)(((int)(long) op_stack_pop(ctx)) >= ((int)(long) op_stack_pop(ctx))));
            break;

        case ADD_F:
            operate_f(ctx, &add_f);
            break;

        case SUB_F:
            operate_f(ctx, &sub_f);
            break;

        case MUL_F:
            operate_f(ctx, &mul_f);
            break;

        case DIV:
            operate_f(ctx, &div_f);
            break;

        default:
            fprintf(stderr, "%s%d", "unsupported opcode ", opc);
            exit(-1);
    }
}

void FDE_cycle(Context* ctx){
    while (!frame_stack_is_empty(ctx)){
        u_int8_t* instruction = fetch(ctx);
        decode_and_execute(ctx, instruction);
    }
}

int exec(char* file_name){
    Context* ctx = init_components(file_name);
    invoke_virtual(ctx, get_main_address(ctx), 0);
    FDE_cycle(ctx);
    clean_up(ctx);
    return 0;
}
