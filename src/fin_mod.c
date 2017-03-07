/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_mod.h"
#include "fin_ctx.h"
#include "fin_ast.h"
#include "fin_op.h"
#include "fin_lex.h"
#include "fin_str.h"
#include "fin_obj.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#if FIN_ASM
#   define FIN_LOG(...) printf(__VA_ARGS__)
#else
#   define FIN_LOG(...)
#endif

typedef struct fin_mod_local_t {
    fin_str_t* name;
    fin_str_t* type;
    uint8_t    idx;
    bool       is_param;
} fin_mod_local_t;

typedef struct fin_mod_code_t {
    uint8_t*      top;
    uint8_t*      begin;
    uint8_t*      end;
    uint8_t       storage[256];
} fin_mod_code_t;

typedef struct fin_mod_field_t {
    fin_str_t* name;
    fin_str_t* type;
} fin_mod_field_t;

typedef struct fin_mod_type_t {
    fin_str_t*       name;
    fin_mod_field_t* fields;
    int32_t          fields_count;
} fin_mod_type_t;

typedef struct fin_mod_compiler_t {
    fin_mod_t*      mod;
    fin_ast_func_t* func;
    fin_str_t*      ret_type;
    fin_mod_code_t  code;
    fin_mod_local_t locals[256];
    uint8_t         scopes[256];
    uint8_t         locals_count;
    uint8_t         params_count;
    uint8_t         scopes_count;
} fin_mod_compiler_t;

static void fin_mod_scope_begin(fin_mod_compiler_t* cmp) {
    cmp->scopes[cmp->scopes_count++] = cmp->locals_count;
}

static void fin_mod_scope_end(fin_mod_compiler_t* cmp) {
    cmp->locals_count = cmp->scopes[--cmp->scopes_count];
}

static void fin_mod_code_init(fin_mod_code_t* code) {
    code->top   = code->storage;
    code->begin = code->storage;
    code->end   = code->storage + sizeof(code->storage);
}

static void fin_mod_code_reset(fin_ctx_t* ctx, fin_mod_code_t* code) {
    if (code->begin != code->storage)
        ctx->alloc(code->begin, 0);
    fin_mod_code_init(code);
}

static void fin_mod_code_ensure(fin_ctx_t* ctx, fin_mod_code_t* code, int32_t size) {
    if (code->top + size >= code->end) {
        int32_t length;
        uint8_t* buffer;
        if (code->begin == code->storage) {
            length = sizeof(code->storage) * 2;
            buffer = ctx->alloc(NULL, length);
            memcpy(buffer, code->begin, code->top - code->begin);
        }
        else {
            length = (int32_t)(code->end - code->begin) * 2;
            buffer = ctx->alloc(code->begin, length);
        }
        code->top   = buffer + (code->top - code->begin);
        code->begin = buffer;
        code->end   = buffer + length;
    }
}

static void fin_mod_code_emit_uint8(fin_ctx_t* ctx, fin_mod_code_t* code, uint8_t val) {
    fin_mod_code_ensure(ctx, code, 1);
    *code->top++ = val;
}

static void fin_mod_code_emit_uint16(fin_ctx_t* ctx, fin_mod_code_t* code, uint16_t val) {
    fin_mod_code_ensure(ctx, code, 2);
    *code->top++ = val & 0xFF;
    *code->top++ = (val >> 8) & 0xFF;
}

static fin_str_t* fin_mod_resolve_type(fin_ctx_t* ctx, fin_mod_compiler_t* cmp, fin_ast_expr_t* expr);

static int16_t fin_mod_const_idx(fin_mod_compiler_t* cmp, fin_val_t val) {
    fin_mod_t* mod = cmp->mod;
    for (int32_t i=0; i<mod->consts_count; i++) {
        if (val.i == mod->consts[i].i)
            return i;
    }
    mod->consts[mod->consts_count] = val;
    return mod->consts_count++;
}

static int16_t fin_mod_bind_idx(fin_mod_compiler_t* cmp, fin_str_t* sign) {
    fin_mod_t* mod = cmp->mod;
    for (int32_t i=0; i<mod->binds_count; i++) {
        if (sign == mod->binds[i].sign)
            return i;
    }
    mod->binds[mod->binds_count].sign = fin_str_clone(sign);
    mod->binds[mod->binds_count].func = NULL;
    return mod->binds_count++;
}

static fin_mod_type_t* fin_mod_find_type(fin_ctx_t* ctx, fin_mod_t* mod, fin_str_t* name) {
    for (int32_t i=0; i<mod->types_count; i++) {
        if (mod->types[i].name == name)
            return &mod->types[i];
    }
    fin_mod_t* m = ctx->mod;
    while (m) {
        for (int32_t i=0; i<m->types_count; i++) {
            if (m->types[i].name == name)
                return &m->types[i];
        }
        m = m->next;
    }
    return NULL;
}

static fin_mod_func_t* fin_mod_find_func(fin_ctx_t* ctx, fin_mod_t* mod, fin_str_t* sign) {
    for (int32_t i=0; i<mod->funcs_count; i++) {
        if (mod->funcs[i].sign == sign)
            return &mod->funcs[i];
    }
    fin_mod_t* m = ctx->mod;
    while (m) {
        for (int32_t i=0; i<m->funcs_count; i++) {
            if (m->funcs[i].sign == sign)
                return &m->funcs[i];
        }
        m = m->next;
    }
    return NULL;
}

static int32_t fin_mod_resolve_field(fin_ctx_t* ctx, fin_mod_t* mod, fin_str_t* type_name, fin_str_t* field_name) {
    fin_mod_type_t* type = fin_mod_find_type(ctx, mod, type_name);
    for (int32_t i=0; i<type->fields_count; i++) {
        if (type->fields[i].name == field_name)
            return i;
    }
    return -1;
}

static fin_mod_local_t* fin_mod_resolve_local(fin_mod_compiler_t* cmp, fin_str_t* id) {
    for (int32_t i=0; i<cmp->locals_count; i++)
        if (cmp->locals[i].name == id)
            return &cmp->locals[i];
    return NULL;
}

static fin_str_t* fin_mod_invoke_get_signature(fin_ctx_t* ctx, fin_mod_compiler_t* cmp, fin_ast_invoke_expr_t* expr) {
    char signature[128];
    signature[0] = '\0';
    if (expr->id->type == fin_ast_expr_type_id) {
        fin_ast_id_expr_t* id_expr = (fin_ast_id_expr_t*)expr->id;
        if (id_expr->primary && id_expr->primary->type == fin_ast_expr_type_id) {
            fin_ast_id_expr_t* prim_id_expr = (fin_ast_id_expr_t*)id_expr->primary;
            strcat(signature, fin_str_cstr(prim_id_expr->name));
            strcat(signature, ".");
        }
        strcat(signature, fin_str_cstr(id_expr->name));
    }
    strcat(signature, "(");
    for (fin_ast_arg_expr_t* e = expr->args; e; e = e->next) {
        if (e != expr->args)
            strcat(signature, ",");
        fin_str_t* arg_type = fin_mod_resolve_type(ctx, cmp, &e->base);
        strcat(signature, fin_str_cstr(arg_type));
        fin_str_destroy(ctx, arg_type);
    }
    strcat(signature, ")");
    return fin_str_create(ctx, signature, -1);
}

static fin_str_t* fin_mod_unary_get_signature(fin_ctx_t* ctx, fin_mod_compiler_t* cmp, fin_ast_unary_expr_t* unary_expr) {
    char sign[128];
    sign[0] = '\0';
    switch (unary_expr->op) {
        case fin_ast_unary_type_pos:  strcat(sign, "__op_pos"); break;
        case fin_ast_unary_type_neg:  strcat(sign, "__op_neg"); break;
        case fin_ast_unary_type_not:  strcat(sign, "__op_not"); break;
        case fin_ast_unary_type_bnot: strcat(sign, "__op_bnot"); break;
        case fin_ast_unary_type_inc:  strcat(sign, "__op_inc"); break;
        case fin_ast_unary_type_dec:  strcat(sign, "__op_dec"); break;
    }
    strcat(sign, "(");
    strcat(sign, fin_str_cstr(fin_mod_resolve_type(ctx, cmp, unary_expr->expr)));
    strcat(sign, ")");
    return fin_str_create(ctx, sign, -1);
}

static fin_str_t* fin_mod_binary_get_signature(fin_ctx_t* ctx, fin_mod_compiler_t* cmp, fin_ast_binary_expr_t* bin_expr) {
    char sign[128];
    sign[0] = '\0';
    switch (bin_expr->op) {
        case fin_ast_binary_type_add:  strcat(sign, "__op_add");  break;
        case fin_ast_binary_type_sub:  strcat(sign, "__op_sub");  break;
        case fin_ast_binary_type_mul:  strcat(sign, "__op_mul");  break;
        case fin_ast_binary_type_div:  strcat(sign, "__op_div");  break;
        case fin_ast_binary_type_mod:  strcat(sign, "__op_mod");  break;
        case fin_ast_binary_type_shl:  strcat(sign, "__op_shl");  break;
        case fin_ast_binary_type_shr:  strcat(sign, "__op_shr");  break;
        case fin_ast_binary_type_ls:   strcat(sign, "__op_lt");   break;
        case fin_ast_binary_type_leq:  strcat(sign, "__op_leq");  break;
        case fin_ast_binary_type_gr:   strcat(sign, "__op_gt");   break;
        case fin_ast_binary_type_geq:  strcat(sign, "__op_geq");  break;
        case fin_ast_binary_type_eq:   strcat(sign, "__op_eq");   break;
        case fin_ast_binary_type_neq:  strcat(sign, "__op_neq");  break;
        case fin_ast_binary_type_band: strcat(sign, "__op_band"); break;
        case fin_ast_binary_type_bor:  strcat(sign, "__op_bor");  break;
        case fin_ast_binary_type_bxor: strcat(sign, "__op_bxor"); break;
        case fin_ast_binary_type_and:  strcat(sign, "__op_and");  break;
        case fin_ast_binary_type_or:   strcat(sign, "__op_or");   break;
    }
    strcat(sign, "(");
    fin_str_t* lhs_type = fin_mod_resolve_type(ctx, cmp, bin_expr->lhs);
    strcat(sign, fin_str_cstr(lhs_type));
    fin_str_destroy(ctx, lhs_type);
    strcat(sign, ",");
    fin_str_t* rhs_type = fin_mod_resolve_type(ctx, cmp, bin_expr->rhs);
    strcat(sign, fin_str_cstr(rhs_type));
    fin_str_destroy(ctx, rhs_type);
    strcat(sign, ")");
    return fin_str_create(ctx, sign, -1);
}

static fin_str_t* fin_mod_resolve_type(fin_ctx_t* ctx, fin_mod_compiler_t* cmp, fin_ast_expr_t* expr) {
    switch (expr->type) {
        case fin_ast_expr_type_init:
        case fin_ast_expr_type_assign:
            return NULL;
        case fin_ast_expr_type_bool:
            return fin_str_create(ctx, "bool", -1);
        case fin_ast_expr_type_int:
            return fin_str_create(ctx, "int", -1);
        case fin_ast_expr_type_float:
            return fin_str_create(ctx, "float", -1);
        case fin_ast_expr_type_str:
        case fin_ast_expr_type_str_interp:
            return fin_str_create(ctx, "string", -1);
        case fin_ast_expr_type_id: {
            fin_ast_id_expr_t* id_expr = (fin_ast_id_expr_t*)expr;
            if (id_expr->primary) {
                fin_str_t* type_name = fin_mod_resolve_type(ctx, cmp, id_expr->primary);
                fin_mod_type_t* type = fin_mod_find_type(ctx, cmp->mod, type_name);
                fin_str_destroy(ctx, type_name);
                for (int32_t i=0; i<type->fields_count; i++) {
                    if (type->fields[i].name == id_expr->name)
                        return fin_str_clone(type->fields[i].type);
                }
                printf("Unresolved field %s\n", fin_str_cstr(id_expr->name));
                assert(0);
            }
            else {
                fin_mod_local_t* local = fin_mod_resolve_local(cmp, id_expr->name);
                assert(local);
                return fin_str_clone(local->type);
            }
        }
        case fin_ast_expr_type_unary: {
            fin_ast_unary_expr_t* unary_expr = (fin_ast_unary_expr_t*)expr;
            fin_str_t* sign = fin_mod_unary_get_signature(ctx, cmp, unary_expr);
            fin_mod_func_t* func = fin_mod_find_func(ctx, cmp->mod, sign);
            fin_str_destroy(ctx, sign);
            return fin_str_clone(func->ret_type);
        }
        case fin_ast_expr_type_binary: {
            fin_ast_binary_expr_t* bin_expr = (fin_ast_binary_expr_t*)expr;
            fin_str_t* sign = fin_mod_binary_get_signature(ctx, cmp, bin_expr);
            fin_mod_func_t* func = fin_mod_find_func(ctx, cmp->mod, sign);
            fin_str_destroy(ctx, sign);
            return fin_str_clone(func->ret_type);
        }
        case fin_ast_expr_type_cond: {
            fin_ast_cond_expr_t* cond_expr = (fin_ast_cond_expr_t*)expr;
            fin_str_t* true_type = fin_mod_resolve_type(ctx, cmp, cond_expr->true_expr);
            fin_str_t* false_type = fin_mod_resolve_type(ctx, cmp, cond_expr->false_expr);
            assert(true_type == false_type);
            fin_str_destroy(ctx, false_type);
            return true_type;
        }
        case fin_ast_expr_type_arg: {
            fin_ast_arg_expr_t* arg_expr = (fin_ast_arg_expr_t*)expr;
            return fin_mod_resolve_type(ctx, cmp, arg_expr->expr);
        }
        case fin_ast_expr_type_invoke: {
            fin_ast_invoke_expr_t* invoke_expr = (fin_ast_invoke_expr_t*)expr;
            fin_str_t* sign = fin_mod_invoke_get_signature(ctx, cmp, invoke_expr);
            fin_mod_func_t* func = fin_mod_find_func(ctx, cmp->mod, sign);
            fin_str_destroy(ctx, sign);
            return fin_str_clone(func->ret_type);
        }
    }
}

static void fin_mod_compile_expr(fin_ctx_t* ctx, fin_mod_compiler_t* cmp, fin_ast_expr_t* expr) {
    switch (expr->type) {
        case fin_ast_expr_type_id: {
            fin_ast_id_expr_t* id_expr = (fin_ast_id_expr_t*)expr;
            if (id_expr->primary) {
                fin_mod_compile_expr(ctx, cmp, id_expr->primary);
                fin_str_t* type_name = fin_mod_resolve_type(ctx, cmp, id_expr->primary);
                int32_t field_idx = fin_mod_resolve_field(ctx, cmp->mod, type_name, id_expr->name);
                fin_str_destroy(ctx, type_name);
                if (field_idx >= 0) {
                    fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_load_field);
                    fin_mod_code_emit_uint8(ctx, &cmp->code, field_idx);
                    FIN_LOG("\tload_fld   %2d         // %s\n", field_idx, fin_str_cstr(id_expr->name));
                    break;
                }
            }
            else {
                fin_mod_local_t* local = fin_mod_resolve_local(cmp, id_expr->name);
                assert(local);
                fin_mod_code_emit_uint8(ctx, &cmp->code, local->is_param ? fin_op_load_arg : fin_op_load_local);
                fin_mod_code_emit_uint8(ctx, &cmp->code, local->idx);
                FIN_LOG("\t%s   %2d         // %s\n",
                    local->is_param ? "load_arg" : "load_loc", local->idx, fin_str_cstr(local->name));
            }
            break;
        }
        case fin_ast_expr_type_bool: {
            fin_ast_bool_expr_t* bool_expr = (fin_ast_bool_expr_t*)expr;
            fin_val_t val = { .b = bool_expr->value };
            int16_t idx = fin_mod_const_idx(cmp, val);
            fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_load_const);
            fin_mod_code_emit_uint16(ctx, &cmp->code, idx);
            FIN_LOG("\tload_const %2d         // %s\n", idx, bool_expr->value ? "true" : "false");
            break;
        }
        case fin_ast_expr_type_int: {
            fin_ast_int_expr_t* int_expr = (fin_ast_int_expr_t*)expr;
            fin_val_t val = { .i = int_expr->value };
            int16_t idx = fin_mod_const_idx(cmp, val);
            fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_load_const);
            fin_mod_code_emit_uint16(ctx, &cmp->code, idx);
            FIN_LOG("\tload_const %2d         // %d\n", idx, (int32_t)int_expr->value);
            break;
        }
        case fin_ast_expr_type_float: {
            fin_ast_float_expr_t* float_expr = (fin_ast_float_expr_t*)expr;
            fin_val_t val = { .f = float_expr->value };
            int16_t idx = fin_mod_const_idx(cmp, val);
            fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_load_const);
            fin_mod_code_emit_uint16(ctx, &cmp->code, idx);
            FIN_LOG("\tload_const %2d         // %f\n", idx, float_expr->value);
            break;
        }
        case fin_ast_expr_type_str: {
            fin_ast_str_expr_t* str_expr = (fin_ast_str_expr_t*)expr;
            fin_val_t val = { .s = str_expr->value };
            if (val.s)
                val.s = fin_str_clone(val.s);
            int16_t idx = fin_mod_const_idx(cmp, val);
            fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_load_const);
            fin_mod_code_emit_uint16(ctx, &cmp->code, idx);
            FIN_LOG("\tload_const %2d         // \"%s\"\n", idx, fin_str_cstr(str_expr->value));
            break;
        }
        case fin_ast_expr_type_str_interp: {
            fin_ast_str_interp_expr_t* interp_expr = (fin_ast_str_interp_expr_t*)expr;
            fin_mod_compile_expr(ctx, cmp, interp_expr->expr);
            fin_str_t* type = fin_mod_resolve_type(ctx, cmp, interp_expr->expr);
            if (strcmp(fin_str_cstr(type), "string") != 0) {
                char signature[256];
                signature[0] = '\0';
                strcat(signature, "string(");
                strcat(signature, fin_str_cstr(type));
                strcat(signature, ")");
                fin_str_t* sign = fin_str_create(ctx, signature, -1);
                int16_t idx = fin_mod_bind_idx(cmp, sign);
                fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_call);
                fin_mod_code_emit_uint16(ctx, &cmp->code, idx);
                FIN_LOG("\tcall       %2d         // %s\n", idx, fin_str_cstr(sign));
                fin_str_destroy(ctx, sign);
            }
            fin_str_destroy(ctx, type);
            if (interp_expr->next) {
                fin_mod_compile_expr(ctx, cmp, &interp_expr->next->base);
                fin_str_t* sign = fin_str_create(ctx, "__op_add(string,string)", -1);
                int16_t idx = fin_mod_bind_idx(cmp, sign);
                fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_call);
                fin_mod_code_emit_uint16(ctx, &cmp->code, idx);
                FIN_LOG("\tcall       %2d         // %s\n", idx, fin_str_cstr(sign));
                fin_str_destroy(ctx, sign);
            }
            break;
        }
        case fin_ast_expr_type_unary: {
            fin_ast_unary_expr_t* unary_expr = (fin_ast_unary_expr_t*)expr;
            fin_mod_compile_expr(ctx, cmp, unary_expr->expr);

            fin_str_t* sign = fin_mod_unary_get_signature(ctx, cmp, unary_expr);
            int16_t idx = fin_mod_bind_idx(cmp, sign);
            fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_call);
            fin_mod_code_emit_uint16(ctx, &cmp->code, idx);
            FIN_LOG("\tcall       %2d         // %s\n", idx, fin_str_cstr(sign));
            fin_str_destroy(ctx, sign);
            break;
        }
        case fin_ast_expr_type_binary: {
            fin_ast_binary_expr_t* bin_expr = (fin_ast_binary_expr_t*)expr;
            fin_mod_compile_expr(ctx, cmp, bin_expr->lhs);
            fin_mod_compile_expr(ctx, cmp, bin_expr->rhs);

            fin_str_t* sign = fin_mod_binary_get_signature(ctx, cmp, bin_expr);
            int16_t idx = fin_mod_bind_idx(cmp, sign);
            fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_call);
            fin_mod_code_emit_uint16(ctx, &cmp->code, idx);
            FIN_LOG("\tcall       %2d         // %s\n", idx, fin_str_cstr(sign));
            fin_str_destroy(ctx, sign);
            break;
        }
        case fin_ast_expr_type_cond: {
            fin_ast_cond_expr_t* cond_expr = (fin_ast_cond_expr_t*)expr;
            fin_mod_compile_expr(ctx, cmp, cond_expr->cond);
            fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_branch_if_n);
            uint8_t* lbl_else = cmp->code.top;
            fin_mod_code_emit_uint16(ctx, &cmp->code, 0);
            FIN_LOG("\tbr_if_n     lbl_%d\n", (int32_t)(lbl_else - cmp->code.begin));
            fin_mod_compile_expr(ctx, cmp, cond_expr->true_expr);
            if (cond_expr->false_expr) {
                fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_branch);
                uint8_t* lbl_end = cmp->code.top;
                fin_mod_code_emit_uint16(ctx, &cmp->code, 0);
                FIN_LOG("\tbr          lbl_%d\n", (int32_t)(lbl_end - cmp->code.begin));
                FIN_LOG("lbl_%d:\n", (int32_t)(lbl_else - cmp->code.begin));
                uint16_t offset = cmp->code.top - lbl_else - 2;
                *lbl_else++ = offset & 0xFF;
                *lbl_else++ = (offset >> 8) & 0xFF;
                fin_mod_compile_expr(ctx, cmp, cond_expr->false_expr);
                FIN_LOG("lbl_%d:\n", (int32_t)(lbl_end - cmp->code.begin));
                offset = cmp->code.top - lbl_end - 2;
                *lbl_end++ = offset & 0xFF;
                *lbl_end++ = (offset >> 8) & 0xFF;
            }
            else {
                FIN_LOG("lbl_%d:\n", (int32_t)(lbl_else - cmp->code.begin));
                uint16_t offset = cmp->code.top - lbl_else - 2;
                *lbl_else++ = offset & 0xFF;
                *lbl_else++ = (offset >> 8) & 0xFF;
            }
            break;
        }
        case fin_ast_expr_type_arg: {
            fin_ast_arg_expr_t* arg_expr = (fin_ast_arg_expr_t*)expr;
            fin_mod_compile_expr(ctx, cmp, arg_expr->expr);
            break;
        }
        case fin_ast_expr_type_invoke: {
            fin_ast_invoke_expr_t* invoke_expr = (fin_ast_invoke_expr_t*)expr;
            for (fin_ast_arg_expr_t* e = invoke_expr->args; e; e = e->next)
                fin_mod_compile_expr(ctx, cmp, &e->base);
            fin_str_t* sign = fin_mod_invoke_get_signature(ctx, cmp, invoke_expr);
            int16_t idx = fin_mod_bind_idx(cmp, sign);
            fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_call);
            fin_mod_code_emit_uint16(ctx, &cmp->code, idx);
            FIN_LOG("\tcall       %2d         // %s\n", idx, fin_str_cstr(sign));
            fin_str_destroy(ctx, sign);
            break;
        }
        case fin_ast_expr_type_assign: {
            fin_ast_assign_expr_t* assign_expr = (fin_ast_assign_expr_t*)expr;
            assert(assign_expr->op == fin_ast_assign_type_assign); // rest not supported yet
            assert(assign_expr->lhs->type == fin_ast_expr_type_id);
            fin_ast_id_expr_t* id_expr = (fin_ast_id_expr_t*)assign_expr->lhs;
            if (id_expr->primary)
                fin_mod_compile_expr(ctx, cmp, id_expr->primary);
            fin_mod_compile_expr(ctx, cmp, assign_expr->rhs);
            if (id_expr->primary) {
                fin_str_t* type_name = fin_mod_resolve_type(ctx, cmp, id_expr->primary);
                int32_t field_idx = fin_mod_resolve_field(ctx, cmp->mod, type_name, id_expr->name);
                fin_str_destroy(ctx, type_name);
                if (field_idx >= 0) {
                    fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_store_field);
                    fin_mod_code_emit_uint8(ctx, &cmp->code, field_idx);
                    FIN_LOG("\tstore_fld  %2d         // %s\n", field_idx, fin_str_cstr(id_expr->name));
                    break;
                }
            }
            else {
                fin_mod_local_t* local = fin_mod_resolve_local(cmp, id_expr->name);
                assert(local);
                fin_mod_code_emit_uint8(ctx, &cmp->code, local->is_param ? fin_op_store_arg : fin_op_store_local);
                fin_mod_code_emit_uint8(ctx, &cmp->code, local->idx);
                FIN_LOG("\t%s  %2d         // %s\n",
                local->is_param ? "store_arg" : "store_loc", local->idx, fin_str_cstr(local->name));
            }
            break;
        }
        default:
            assert(0);
    }
}

static void fin_mod_compile_init_expr(fin_ctx_t* ctx, fin_mod_compiler_t* cmp, fin_ast_init_expr_t* expr, fin_str_t* type_name) {
    fin_mod_type_t* type = fin_mod_find_type(ctx, cmp->mod, type_name);
    assert(type);
    int32_t args_count = 0;
    for (fin_ast_arg_expr_t* e = expr->args; e; e = e->next)
        args_count++;
    assert(args_count == type->fields_count);
    int32_t idx = 0;
    for (fin_ast_arg_expr_t* e = expr->args; e; e = e->next) {
        fin_str_t* arg_type = fin_mod_resolve_type(ctx, cmp, e->expr);
        assert(arg_type == type->fields[idx].type);
        fin_str_destroy(ctx, arg_type);
        fin_mod_compile_expr(ctx, cmp, e->expr);
        idx++;
    }
    fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_new);
    fin_mod_code_emit_uint8(ctx, &cmp->code, type->fields_count);
    FIN_LOG("\tnew        %2d         // %s\n", type->fields_count, fin_str_cstr(type->name));
}

static void fin_mod_compile_stmt(fin_ctx_t* ctx, fin_mod_compiler_t* cmp, fin_ast_stmt_t* stmt) {
    switch (stmt->type) {
        case fin_ast_stmt_type_expr: {
            fin_ast_expr_stmt_t* expr_stmt = (fin_ast_expr_stmt_t*)stmt;
            fin_mod_compile_expr(ctx, cmp, expr_stmt->expr);
            break;
        }
        case fin_ast_stmt_type_ret: {
            fin_ast_ret_stmt_t* ret_stmt = (fin_ast_ret_stmt_t*)stmt;
            if (ret_stmt->expr) {
                if (ret_stmt->expr->type == fin_ast_expr_type_init)
                    fin_mod_compile_init_expr(ctx, cmp, (fin_ast_init_expr_t*)ret_stmt->expr, cmp->ret_type);
                else
                    fin_mod_compile_expr(ctx, cmp, ret_stmt->expr);
            }
            fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_return);
            FIN_LOG("\tret\n");
            break;
        }
        case fin_ast_stmt_type_if: {
            fin_ast_if_stmt_t* if_stmt = (fin_ast_if_stmt_t*)stmt;
            fin_mod_compile_expr(ctx, cmp, if_stmt->cond);
            fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_branch_if_n);
            uint8_t* lbl_else = cmp->code.top;
            fin_mod_code_emit_uint16(ctx, &cmp->code, 0);
            FIN_LOG("\tbr_if_n     lbl_%d\n", (int32_t)(lbl_else - cmp->code.begin));
            fin_mod_scope_begin(cmp);
            fin_mod_compile_stmt(ctx, cmp, if_stmt->true_stmt);
            fin_mod_scope_end(cmp);
            if (if_stmt->false_stmt) {
                fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_branch);
                uint8_t* lbl_end = cmp->code.top;
                fin_mod_code_emit_uint16(ctx, &cmp->code, 0);
                FIN_LOG("\tbr          lbl_%d\n", (int32_t)(lbl_end - cmp->code.begin));
                FIN_LOG("lbl_%d:\n", (int32_t)(lbl_else - cmp->code.begin));
                uint16_t offset = cmp->code.top - lbl_else - 2;
                *lbl_else++ = offset & 0xFF;
                *lbl_else++ = (offset >> 8) & 0xFF;
                fin_mod_scope_begin(cmp);
                fin_mod_compile_stmt(ctx, cmp, if_stmt->false_stmt);
                fin_mod_scope_end(cmp);
                FIN_LOG("lbl_%d:\n", (int32_t)(lbl_end - cmp->code.begin));
                offset = cmp->code.top - lbl_end - 2;
                *lbl_end++ = offset & 0xFF;
                *lbl_end++ = (offset >> 8) & 0xFF;
            }
            else {
                FIN_LOG("lbl_%d:\n", (int32_t)(lbl_else - cmp->code.begin));
                uint16_t offset = cmp->code.top - lbl_else - 2;
                *lbl_else++ = offset & 0xFF;
                *lbl_else++ = (offset >> 8) & 0xFF;
            }
            break;
        }
        case fin_ast_stmt_type_while: {
            fin_ast_while_stmt_t* while_stmt = (fin_ast_while_stmt_t*)stmt;
            fin_mod_scope_begin(cmp);
            uint8_t* lbl_loop = cmp->code.top;
            FIN_LOG("lbl_%d:\n", (int32_t)(lbl_loop - cmp->code.begin));
            fin_mod_compile_expr(ctx, cmp, while_stmt->cond);
            fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_branch_if_n);
            uint8_t* lbl_end = cmp->code.top;
            fin_mod_code_emit_uint16(ctx, &cmp->code, 0);
            FIN_LOG("\tbr_if_n     lbl_%d\n", (int32_t)(lbl_end - cmp->code.begin));
            fin_mod_scope_begin(cmp);
            fin_mod_compile_stmt(ctx, cmp, while_stmt->stmt);
            fin_mod_scope_end(cmp);
            uint16_t offset = lbl_loop - cmp->code.top - 3;
            fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_branch);
            fin_mod_code_emit_uint16(ctx, &cmp->code, offset);
            FIN_LOG("\tbr          lbl_%d\n", (int32_t)(lbl_loop - cmp->code.begin));
            FIN_LOG("lbl_%d:\n", (int32_t)(lbl_end - cmp->code.begin));
            offset = cmp->code.top - lbl_end - 2;
            *lbl_end++ = offset & 0xFF;
            *lbl_end++ = (offset >> 8) & 0xFF;
            fin_mod_scope_end(cmp);
            break;
        }
        case fin_ast_stmt_type_do: {
            fin_ast_do_stmt_t* do_stmt = (fin_ast_do_stmt_t*)stmt;
            uint8_t* lbl_loop = cmp->code.top;
            FIN_LOG("lbl_%d:\n", (int32_t)(lbl_loop - cmp->code.begin));
            fin_mod_scope_begin(cmp);
            fin_mod_compile_stmt(ctx, cmp, do_stmt->stmt);
            fin_mod_scope_end(cmp);
            fin_mod_compile_expr(ctx, cmp, do_stmt->cond);
            fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_branch_if);
            FIN_LOG("\tbr_if       lbl_%d\n", (int32_t)(lbl_loop - cmp->code.begin));
            uint16_t offset = lbl_loop - cmp->code.top - 2;
            fin_mod_code_emit_uint16(ctx, &cmp->code, offset);
            break;
        }
        case fin_ast_stmt_type_decl: {
            fin_ast_decl_stmt_t* decl_stmt = (fin_ast_decl_stmt_t*)stmt;
            assert(!fin_mod_resolve_local(cmp, decl_stmt->name));
            fin_mod_local_t* local = &cmp->locals[cmp->locals_count++];
            local->name = decl_stmt->name;
            local->type = decl_stmt->type->name;
            local->idx = cmp->locals_count - cmp->params_count - 1;
            local->is_param = false;
            if (decl_stmt->init) {
                if (decl_stmt->init->type == fin_ast_expr_type_init)
                    fin_mod_compile_init_expr(ctx, cmp, (fin_ast_init_expr_t*)decl_stmt->init, decl_stmt->type->name);
                else
                    fin_mod_compile_expr(ctx, cmp, decl_stmt->init);
                fin_mod_code_emit_uint8(ctx, &cmp->code, fin_op_store_local);
                fin_mod_code_emit_uint8(ctx, &cmp->code, local->idx);
                FIN_LOG("\tstore_loc  %2d\n", local->idx);
            }
            break;
        }
        case fin_ast_stmt_type_block: {
            fin_ast_block_stmt_t* block_stmt = (fin_ast_block_stmt_t*)stmt;
            fin_mod_scope_begin(cmp);
            for (fin_ast_stmt_t* s = block_stmt->stmts; s; s = s->next)
                fin_mod_compile_stmt(ctx, cmp, s);
            fin_mod_scope_end(cmp);
            break;
        }
    }
}

static void fin_mod_compile_func(fin_mod_func_t* out_func, fin_ctx_t* ctx, fin_mod_t* mod, fin_ast_func_t* func) {
    FIN_LOG("\n");
    FIN_LOG("func %s\n", fin_str_cstr(out_func->sign));

    fin_mod_compiler_t cmp;
    cmp.mod = mod;
    cmp.func = func;
    cmp.locals_count = 0;
    cmp.params_count = 0;
    cmp.scopes_count = 0;
    cmp.ret_type = fin_str_clone(out_func->ret_type);
    fin_mod_code_init(&cmp.code);

    for (fin_ast_param_t* param = func->params; param; param = param->next) {
        fin_mod_local_t* l = &cmp.locals[cmp.locals_count++];
        l->name = param->name;
        l->type = param->type->name;
        l->idx = cmp.params_count++;
        l->is_param = true;
    }

    fin_mod_compile_stmt(ctx, &cmp, &func->block->base);
    if (cmp.code.top == cmp.code.begin || cmp.code.top[-1] != fin_op_return) {
        fin_mod_code_emit_uint8(ctx, &cmp.code, fin_op_return);
        FIN_LOG("\tret\n");
    }

    out_func->code_length = (int32_t)(cmp.code.top - cmp.code.begin);
    out_func->code = (uint8_t*)ctx->alloc(NULL, out_func->code_length);
    memcpy(out_func->code, cmp.code.begin, out_func->code_length);
    out_func->locals = cmp.locals_count - cmp.params_count;

    fin_mod_code_reset(ctx, &cmp.code);

    FIN_LOG("\n");
}

static void fin_mod_compile_type(fin_ctx_t* ctx, fin_ast_type_t* type, fin_mod_type_t* dest_type) {
    int32_t fields = 0;
    fin_ast_field_t* field = type->fields;
    while (field) {
        fields++;
        field = field->next;
    }
    dest_type->name = fin_str_clone(type->name);
    dest_type->fields_count = fields;
    dest_type->fields = (fin_mod_field_t*)ctx->alloc(NULL, sizeof(fin_mod_field_t) * fields);

    field = type->fields;
    fin_mod_field_t* f = dest_type->fields;
    while (field) {
        f->name = fin_str_clone(field->name);
        f->type = fin_str_clone(field->type->name);
        field = field->next;
        f++;
    }
}

static void fin_mod_register(fin_ctx_t* ctx, fin_mod_t* mod) {
    mod->next = ctx->mod;
    ctx->mod = mod;

    for (int32_t i=0; i<mod->binds_count; i++) {
        fin_mod_t* m = ctx->mod;
        while (m) {
            for (int32_t f=0; f<m->funcs_count; f++) {
                if (m->funcs[f].sign == mod->binds[i].sign) {
                    mod->binds[i].func = &m->funcs[f];
                    break;
                }
            }
            if (mod->binds[i].func)
                break;
            m = m->next;
        }
        if (!mod->binds[i].func) {
            printf("Unresolved function %s\n", fin_str_cstr(mod->binds[i].sign));
            assert(mod->binds[i].func);
        }
    }
}

fin_mod_t* fin_mod_create(fin_ctx_t* ctx, const char* name, fin_mod_func_desc_t* descs, int32_t descs_count) {
    fin_mod_func_t* funcs = (fin_mod_func_t*)ctx->alloc(NULL, sizeof(fin_mod_func_t) * descs_count);

    fin_mod_t* mod = (fin_mod_t*)ctx->alloc(NULL, sizeof(fin_mod_t));
    mod->name = fin_str_create(ctx, name, -1);
    mod->types = NULL;
    mod->funcs = funcs;
    mod->binds = NULL;
    mod->consts = NULL;
    mod->types_count = 0;
    mod->funcs_count = descs_count;
    mod->binds_count = 0;
    mod->consts_count = 0;
    mod->next = NULL;

    for (int32_t i=0; i<descs_count; i++) {
        funcs[i].mod = mod;
        funcs[i].func = descs[i].func;
        funcs[i].is_native = true;
        funcs[i].code = NULL;
        funcs[i].code_length = 0;
        funcs[i].args = 0;

        // ret_type name "(" arg? ("," arg)* ")"
        fin_lex_t* lex = fin_lex_create(ctx->alloc, descs[i].sign);

        char signature[256];
        signature[0] = '\0';

        if (name != NULL && name[0] != '\0') {
            strcat(signature, name);
            strcat(signature, ".");
        }

        if (fin_lex_match(lex, fin_lex_type_void))
            funcs[i].ret_type = NULL;
        else {
            fin_lex_str_t lex_str = fin_lex_consume_name(lex);
            funcs[i].ret_type = fin_str_create(ctx, lex_str.cstr, lex_str.len);
        }

        fin_lex_consume_name_to(lex, signature + strlen(signature));

        fin_lex_match(lex, fin_lex_type_l_paren);
        strcat(signature, "(");
        while (!fin_lex_match(lex, fin_lex_type_r_paren)) {
            if (funcs[i].args) {
                fin_lex_match(lex, fin_lex_type_comma);
                strcat(signature, ",");
            }
            fin_lex_consume_name_to(lex, signature + strlen(signature));
            funcs[i].args++;
        }
        strcat(signature, ")");
        funcs[i].sign = fin_str_create(ctx, signature, -1);

        fin_lex_destroy(ctx->alloc, lex);
    }

    fin_mod_register(ctx, mod);
    return mod;
}

fin_mod_t* fin_mod_compile(fin_ctx_t* ctx, const char* cstr) {
    fin_ast_module_t* module = fin_ast_parse(ctx, cstr);

    fin_mod_t* mod = (fin_mod_t*)ctx->alloc(NULL, sizeof(fin_mod_t));
    mod->types = NULL;
    mod->funcs = NULL;
    mod->consts = (fin_val_t*)ctx->alloc(NULL, sizeof(fin_val_t) * 128);
    mod->binds = (fin_mod_func_bind_t*)ctx->alloc(NULL, sizeof(fin_mod_func_bind_t) * 128);
    mod->types_count = 0;
    mod->funcs_count = 0;
    mod->consts_count = 0;
    mod->binds_count = 0;

    mod->next = NULL;

    for (fin_ast_type_t* type = module->types; type; type = type->next)
        mod->types_count++;
    for (fin_ast_func_t* func = module->funcs; func; func = func->next)
        mod->funcs_count++;

    if (mod->types_count) {
        mod->types = (fin_mod_type_t*)ctx->alloc(NULL, sizeof(fin_mod_type_t) * mod->types_count);

        int32_t idx = 0;
        for (fin_ast_type_t* type = module->types; type; type = type->next)
            fin_mod_compile_type(ctx, type, &mod->types[idx++]);
    }

    if (mod->funcs_count) {
        mod->funcs = (fin_mod_func_t*)ctx->alloc(NULL, sizeof(fin_mod_func_t) * mod->funcs_count);

        int32_t idx = 0;
        for (fin_ast_func_t* func = module->funcs; func; func = func->next) {
            fin_mod_func_t* f = &mod->funcs[idx++];

            char signature[128];
            signature[0] = '\0';
            //strcat(signature, fin_str_cstr(func->ret->name));
            //strcat(signature, " ");
            strcat(signature, fin_str_cstr(func->name));
            strcat(signature, "(");
            uint8_t args = 0;
            for (fin_ast_param_t* param = func->params; param; param = param->next) {
                if (args)
                    strcat(signature, ",");
                strcat(signature, fin_str_cstr(param->type->name));
                args++;
            }
            strcat(signature, ")");

            f->mod = mod;
            f->sign = fin_str_create(ctx, signature, -1);
            f->func = NULL;
            f->is_native = false;
            f->code = NULL;
            f->code_length = 0;
            f->args = args;
            f->ret_type = func->ret ? func->ret->name : NULL;
        }

        idx = 0;
        for (fin_ast_func_t* func = module->funcs; func; func = func->next)
            fin_mod_compile_func(&mod->funcs[idx++], ctx, mod, func);
    }

    fin_ast_destroy(module);

    mod->name = NULL;
    fin_str_t* entry_name = fin_str_create(ctx, "Main()", 6);
    mod->entry = fin_mod_find_func(ctx, mod, entry_name);
    fin_str_destroy(ctx, entry_name);
    fin_mod_register(ctx, mod);
    return mod;
}

void fin_mod_destroy(fin_ctx_t* ctx, fin_mod_t* mod) {
    if (mod->name)
        fin_str_destroy(ctx, mod->name);
    for (int32_t i=0; i<mod->types_count; i++) {
        fin_mod_type_t* type = &mod->types[i];
        fin_str_destroy(ctx, type->name);
        for (int32_t j=0; j<type->fields_count; j++) {
            fin_str_destroy(ctx, type->fields[j].name);
            fin_str_destroy(ctx, type->fields[j].type);
        }
        ctx->alloc(type->fields, 0);
    }
    for (int32_t i=0; i<mod->funcs_count; i++) {
        fin_mod_func_t* func = &mod->funcs[i];
        if (func->ret_type)
            fin_str_destroy(ctx, func->ret_type);
        ctx->alloc(func->code, 0);
        fin_str_destroy(ctx, func->sign);
    }
    if (mod->binds) {
        for (int32_t i=0; i<mod->binds_count; i++) {
            fin_mod_func_bind_t* bind = &mod->binds[i];
            fin_str_destroy(ctx, bind->sign);
        }
        ctx->alloc(mod->binds, 0);
    }
    if (mod->consts)
        ctx->alloc(mod->consts, 0);
    if (mod->funcs)
        ctx->alloc(mod->funcs, 0);
    if (mod->types)
        ctx->alloc(mod->types, 0);
    ctx->alloc(mod, 0);
}

