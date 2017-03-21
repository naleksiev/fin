/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_vm.h"
#include "fin_ctx.h"
#include "fin_obj.h"
#include "fin_op.h"
#include "fin_mod.h"
#include <assert.h>

#if FIN_CONFIG_COMPUTED_GOTO
    #define FIN_VM_NEXT()       goto *goto_table[*ip++]
    #define FIN_VM_LOOP_BEGIN() static void* goto_table[] = { \
                                    &&fin_op_load_const,      \
                                    &&fin_op_load_arg,        \
                                    &&fin_op_store_arg,       \
                                    &&fin_op_load_local,      \
                                    &&fin_op_store_local,     \
                                    &&fin_op_load_field,      \
                                    &&fin_op_store_field,     \
                                    &&fin_op_call,            \
                                    &&fin_op_branch,          \
                                    &&fin_op_branch_if,       \
                                    &&fin_op_branch_if_n,     \
                                    &&fin_op_return,          \
                                    &&fin_op_pop,             \
                                    &&fin_op_new              \
                                };                            \
                                FIN_VM_NEXT();
    #define FIN_VM_LOOP_END()
    #define FIN_VM_OP(op)       op:
#else
    #define FIN_VM_NEXT()       break
    #define FIN_VM_LOOP_BEGIN() while (true) {                \
                                   fin_op op = *ip++;         \
                                   switch (op)
    #define FIN_VM_LOOP_END()   }
    #define FIN_VM_OP(op)       case op:
#endif

typedef struct fin_vm_stack_t {
    fin_val_t* top;
    fin_val_t* begin;
    fin_val_t* end;
    fin_val_t  stash[64];
} fin_vm_stack_t;

typedef struct fin_vm_t {
    fin_vm_stack_t stack;
    fin_ctx_t*     ctx;
} fin_vm_t;

void fin_vm_invoke_int(fin_ctx_t* ctx, fin_mod_func_t* func, fin_val_t* stack);

void fin_vm_interpret(fin_ctx_t* ctx, fin_mod_func_t* func, fin_val_t* stack) {
    fin_mod_t* mod  = func->mod;
    fin_val_t* args = stack - func->args;
    fin_val_t* top  = stack + func->locals;
    uint8_t* ip   = func->code;

    FIN_VM_LOOP_BEGIN() {
        FIN_VM_OP(fin_op_load_const) {
            int32_t idx = *ip++;
            idx |= *ip++ << 8;
            *top++ = mod->consts[idx];
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_load_arg) {
            *top++ = args[*ip++];
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_store_arg) {
            args[*ip++] = *--top;
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_load_local) {
            *top++ = stack[*ip++];
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_store_local) {
            stack[*ip++] = *--top;
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_load_field) {
            if (top[-1].o)
                top[-1] = top[-1].o->fields[*ip++];
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_store_field) {
            if (top[-2].o)
                top[-2].o->fields[*ip++] = top[-1];
            top -= 2;
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_call) {
            int32_t idx = *ip++;
            idx |= *ip++ << 8;
            fin_mod_func_t* func = mod->binds[idx].func;
            fin_vm_invoke_int(ctx, func, top);
            top -= func->args - (func->ret_type ? 1 : 0);
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_branch) {
            int16_t offset = *ip++;
            offset |= *ip++ << 8;
            ip += offset;
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_branch_if) {
            if ((--top)->b) {
                int16_t offset = *ip++;
                offset |= *ip++ << 8;
                ip += offset;
            } else
                ip += 2;
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_branch_if_n) {
            if ((--top)->b)
                ip += 2;
            else {
                int16_t offset = *ip++;
                offset |= *ip++ << 8;
                ip += offset;
            }
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_return) {
            if (func->ret_type)
                args[0] = top[-1];
            return;
        }
        FIN_VM_OP(fin_op_pop) {
            top--;
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_new) {
            top -= *ip;
            top->o = fin_obj_create(ctx->alloc, top, *ip);
            top++;
            ip++;
            FIN_VM_NEXT();
        }
    }
    FIN_VM_LOOP_END();
}

fin_vm_t* fin_vm_create(fin_ctx_t* ctx) {
    fin_vm_t* vm = (fin_vm_t*)ctx->alloc(NULL, sizeof(fin_vm_t));
    vm->stack.top   = vm->stack.stash;
    vm->stack.begin = vm->stack.stash;
    vm->stack.end   = vm->stack.stash + FIN_COUNT_OF(vm->stack.stash);
    vm->ctx = ctx;
    return vm;
}

void fin_vm_destroy(fin_vm_t* vm) {
    vm->ctx->alloc(vm, 0);
}

inline void fin_vm_invoke_int(fin_ctx_t* ctx, fin_mod_func_t* func, fin_val_t* stack) {
    if (func->is_native) {
        fin_val_t result;
        (*func->func)(ctx, stack - func->args, &result);
        if (func->ret_type)
            stack[-func->args] = result;
    }
    else
        fin_vm_interpret(ctx, func, stack);
}

void fin_vm_invoke(fin_vm_t* vm, fin_mod_func_t* func) {
    fin_vm_invoke_int(vm->ctx, func, vm->stack.top);
    assert(vm->stack.top == vm->stack.begin);
}
