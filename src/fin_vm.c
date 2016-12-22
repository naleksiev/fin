/*
 * Copyright 2016 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_vm.h"
#include "fin_ctx.h"
#include "fin_val.h"
#include "fin_obj.h"
#include "fin_op.h"
#include "fin_mod.h"

#if FIN_CONFIG_COMPUTED_GOTO
    #define FIN_VM_NEXT()       goto *goto_table[*ip++]
    #define FIN_VM_LOOP_BEGIN() static void* goto_table[] = { \
                                    &&fin_op_load_const,      \
                                    &&fin_op_load_arg,        \
                                    &&fin_op_load_local,      \
                                    &&fin_op_store_local,     \
                                    &&fin_op_load_field,      \
                                    &&fin_op_store_field,     \
                                    &&fin_op_call,            \
                                    &&fin_op_branch,          \
                                    &&fin_op_branch_if_n,     \
                                    &&fin_op_return,          \
                                    &&fin_op_pop              \
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

typedef struct fin_vm_stack {
    fin_val* top;
    fin_val* begin;
    fin_val* end;
    fin_val  stash[64];
} fin_vm_stack;

typedef struct fin_vm_fiber {
    fin_vm_stack      stack;
} fin_vm_fiber;

typedef struct fin_vm {
    fin_vm_fiber fib; // ????
} fin_vm;

void fin_vm_invoke_int(fin_mod_func* func, fin_val* stack);

void fin_vm_interpret(fin_mod* mod, uint8_t* ip, fin_val* stack, fin_val* args, int32_t locals) {
    fin_val* top = stack + locals;

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
        FIN_VM_OP(fin_op_load_local) {
            *top++ = stack[*ip++];
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_store_local) {
            stack[*ip++] = *--top;
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_load_field) {
            top[-1] = top[-1].o->fields[*ip++];
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_store_field) {
            top[-2].o->fields[*ip++] = top[-1];
            top -= 2;
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_call) {
            int32_t idx = *ip++;
            idx |= *ip++ << 8;
            fin_mod_func* func = mod->binds[idx].func;
            fin_vm_invoke_int(func, top);
            top -= func->args - (func->ret_type ? 1 : 0);
            FIN_VM_NEXT();
        }
        FIN_VM_OP(fin_op_branch) {
            int16_t offset = *ip++;
            offset |= *ip++ << 8;
            ip += offset;
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
            args[0] = top[-1];
            return;
        }
        FIN_VM_OP(fin_op_pop) {
            top--;
            FIN_VM_NEXT();
        }
    }
    FIN_VM_LOOP_END();
}

fin_vm* fin_vm_create(fin_ctx* ctx) {
    fin_vm* vm = (fin_vm*)ctx->alloc(NULL, sizeof(fin_vm));
    vm->fib.stack.begin = vm->fib.stack.stash;
    vm->fib.stack.top = vm->fib.stack.begin;
    return vm;
}

inline void fin_vm_invoke_int(fin_mod_func* func, fin_val* stack) {
    if (func->is_native)
        (*func->func)(stack - func->args);
    else
        fin_vm_interpret(func->mod, func->code, stack, stack - func->args, func->locals);
}

void fin_vm_invoke(fin_vm* vm, fin_mod_func* func) {
    fin_vm_invoke_int(func, vm->fib.stack.top);
}

void fin_vm_destroy(fin_vm* vm) {
    free(vm);
}
