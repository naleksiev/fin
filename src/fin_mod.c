/*
 * Copyright 2016 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_mod.h"
#include "fin_ctx.h"
#include "fin_ast.h"
#include "fin_op.h"
#include "fin_lex.h"
#include "fin_str.h"

#define FIN_LOG(...) printf(__VA_ARGS__)
//#define FIN_LOG(...)

typedef struct fin_mod_type {
    fin_str* name;
    int32_t  fields;
} fin_mod_type;

typedef struct fin_mod_compiler {
    fin_mod*      mod;
    fin_ast_func* func;
    uint8_t*      code;
    uint8_t*      code_begin;
    uint8_t*      code_end;
    uint8_t       code_storage[1024];
    fin_str*      locals[256];
    int32_t       locals_count;
    fin_str*      params[32];
    int32_t       params_count;
} fin_mod_compiler;

static fin_str* fin_mod_resolve_type(fin_ctx* ctx, fin_mod_compiler* cmp, fin_ast_expr* expr, fin_ast_expr* primary);

static void fin_mod_ensure(fin_mod_compiler* cmp, int32_t size) {
    assert(size > cmp->code - cmp->code_end);
}

static void fin_mod_emit_uint8(fin_mod_compiler* cmp, uint8_t val) {
    fin_mod_ensure(cmp, 1);
    *cmp->code++ = val;
}

static void fin_mod_emit_uint16(fin_mod_compiler* cmp, uint16_t val) {
    fin_mod_ensure(cmp, 2);
    *cmp->code++ = val & 0xFF;
    *cmp->code++ = (val >> 8) & 0xFF;
}

static int16_t fin_mod_const_idx(fin_mod_compiler* cmp, fin_val val) {
    fin_mod* mod = cmp->mod;
    for (int32_t i=0; i<mod->consfin_count; i++) {
        if (fin_val_equal(val, mod->consts[i]))
            return i;
    }
    mod->consts[mod->consfin_count] = val;
    return mod->consfin_count++;
}

static int16_t fin_mod_bind_idx(fin_mod_compiler* cmp, fin_str* sign) {
    fin_mod* mod = cmp->mod;
    for (int32_t i=0; i<mod->binds_count; i++) {
        if (sign == mod->binds[i].sign)
            return i;
    }
    mod->binds[mod->binds_count].sign = sign;
    mod->binds[mod->binds_count].func = NULL;
    return mod->binds_count++;
}

static fin_mod_func* fin_mod_find_func(fin_ctx* ctx, fin_mod* mod, fin_str* sign) {
    for (int32_t i=0; i<mod->funcs_count; i++) {
        if (mod->funcs[i].sign == sign)
            return &mod->funcs[i];
    }
    fin_mod* m = ctx->mod;
    while (m) {
        for (int32_t i=0; i<m->funcs_count; i++) {
            if (m->funcs[i].sign == sign)
                return &m->funcs[i];
        }
        m = m->next;
    }
    return NULL;
}

static fin_mod* fin_mod_resolve_module(fin_ctx* ctx, fin_mod* mod, fin_str* id) {
    if (mod->name == id)
        return mod;
    for (fin_mod* m = ctx->mod; m; m = m->next)
        if (m->name == id)
            return m;
    return NULL;
}

static int32_t fin_mod_resolve_local(fin_mod_compiler* cmp, fin_str* id) {
    for (int32_t i=0; i<cmp->locals_count; i++)
        if (cmp->locals[i] == id)
            return i;
    return -1;
}

static int32_t fin_mod_resolve_arg(fin_mod_compiler* cmp, fin_str* id) {
    for (int32_t i=0; i<cmp->params_count; i++)
        if (cmp->params[i] == id)
            return i;
    return -1;
}

static fin_str* fin_mod_invoke_get_signature(fin_ctx* ctx, fin_mod_compiler* cmp, fin_ast_invoke_expr* expr, fin_ast_expr* primary) {
    fin_ast_id_expr* identifier = expr->id;

    assert(primary == NULL || primary->type == fin_ast_expr_type_id);
    fin_ast_id_expr* id_expr = (fin_ast_id_expr*)primary;

    char signature[128];
    signature[0] = '\0';
    if (id_expr) {
        strcat(signature, fin_str_cstr(id_expr->name));
        strcat(signature, ".");
    }
    strcat(signature, fin_str_cstr(identifier->name));

    strcat(signature, "(");
    for (fin_ast_arg_expr* e = expr->args; e; e = e->next) {
        if (e != expr->args)
            strcat(signature, ",");
        strcat(signature, fin_str_cstr(fin_mod_resolve_type(ctx, cmp, &e->base, primary)));
    }
    strcat(signature, ")");

    return fin_str_create(ctx->pool, signature, -1);
}

static fin_str* fin_mod_binary_get_signature(fin_ctx* ctx, fin_mod_compiler* cmp, fin_ast_binary_expr* expr) {
    char sign[128];
    sign[0] = '\0';
    switch (expr->op) {
        case fin_ast_binary_type_add:  strcat(sign, "op_add");  break;
        case fin_ast_binary_type_sub:  strcat(sign, "op_sub");  break;
        case fin_ast_binary_type_mul:  strcat(sign, "op_mul");  break;
        case fin_ast_binary_type_div:  strcat(sign, "op_div");  break;
        case fin_ast_binary_type_mod:  strcat(sign, "op_mod");  break;
        case fin_ast_binary_type_shl:  strcat(sign, "op_shl");  break;
        case fin_ast_binary_type_shr:  strcat(sign, "op_shr");  break;
        case fin_ast_binary_type_ls:   strcat(sign, "op_lt");   break;
        case fin_ast_binary_type_leq:  strcat(sign, "op_leq");  break;
        case fin_ast_binary_type_gr:   strcat(sign, "op_gt");   break;
        case fin_ast_binary_type_geq:  strcat(sign, "op_geq");  break;
        case fin_ast_binary_type_eq:   strcat(sign, "op_eq");   break;
        case fin_ast_binary_type_neq:  strcat(sign, "op_neq");  break;
        case fin_ast_binary_type_band: strcat(sign, "op_band"); break;
        case fin_ast_binary_type_bor:  strcat(sign, "op_bor");  break;
        case fin_ast_binary_type_bxor: strcat(sign, "op_bxor"); break;
        case fin_ast_binary_type_and:  strcat(sign, "op_and");  break;
        case fin_ast_binary_type_or:   strcat(sign, "op_or");   break;
    }
    strcat(sign, "(");
    strcat(sign, fin_str_cstr(fin_mod_resolve_type(ctx, cmp, expr->lhs, NULL)));
    strcat(sign, ",");
    strcat(sign, fin_str_cstr(fin_mod_resolve_type(ctx, cmp, expr->rhs, NULL)));
    strcat(sign, ")");
    return fin_str_create(ctx->pool, sign, -1);
}

static fin_str* fin_mod_resolve_type(fin_ctx* ctx, fin_mod_compiler* cmp, fin_ast_expr* expr, fin_ast_expr* primary) {
    switch (expr->type) {
        case fin_ast_expr_type_id: {
            return fin_str_create(ctx->pool, "int", -1); // hack
        }
        case fin_ast_expr_type_member: {
            fin_ast_member_expr* member_expr = (fin_ast_member_expr*)expr;
            return fin_mod_resolve_type(ctx, cmp, member_expr->member, member_expr->primary);
        }
        case fin_ast_expr_type_bool: {
            return fin_str_create(ctx->pool, "bool", -1);
        }
        case fin_ast_expr_type_int: {
            return fin_str_create(ctx->pool, "int", -1);
        }
        case fin_ast_expr_type_float: {
            return fin_str_create(ctx->pool, "float", -1);
        }
        case fin_ast_expr_type_str: {
            return fin_str_create(ctx->pool, "string", -1);
        }
        case fin_ast_expr_type_unary: {
            fin_ast_unary_expr* un_expr = (fin_ast_unary_expr*)expr;
            return fin_mod_resolve_type(ctx, cmp, un_expr->expr, NULL);
        }
        case fin_ast_expr_type_binary: {
            fin_ast_binary_expr* bin_expr = (fin_ast_binary_expr*)expr;
            fin_str* sign = fin_mod_binary_get_signature(ctx, cmp, bin_expr);
            fin_mod_func* func = fin_mod_find_func(ctx, cmp->mod, sign);
            return func->ret_type;
        }
        case fin_ast_expr_type_cond: {
            fin_ast_cond_expr* cond_expr = (fin_ast_cond_expr*)expr;
            fin_str* true_type = fin_mod_resolve_type(ctx, cmp, cond_expr->true_expr, NULL);
            fin_str* false_type = fin_mod_resolve_type(ctx, cmp, cond_expr->false_expr, NULL);
            assert(true_type == false_type);
            return true_type;
        }
        case fin_ast_expr_type_arg: {
            fin_ast_arg_expr* arg_expr = (fin_ast_arg_expr*)expr;
            return fin_mod_resolve_type(ctx, cmp, arg_expr->expr, NULL);
        }
        case fin_ast_expr_type_invoke: {
            fin_ast_invoke_expr* invoke_expr = (fin_ast_invoke_expr*)expr;
            fin_str* sign = fin_mod_invoke_get_signature(ctx, cmp, invoke_expr, primary);
            fin_mod_func* func = fin_mod_find_func(ctx, cmp->mod, sign);
            return func->ret_type;
        }
        case fin_ast_expr_type_assign: {
            return fin_str_create(ctx->pool, "void", -1);
        }
    }
}

static void fin_mod_compile_expr(fin_ctx* ctx, fin_mod_compiler* cmp, fin_ast_expr* expr, fin_ast_expr* primary) {
    switch (expr->type) {
        case fin_ast_expr_type_id: {
            fin_ast_id_expr* id_expr = (fin_ast_id_expr*)expr;
            int32_t local_idx = fin_mod_resolve_local(cmp, id_expr->name);
            if (local_idx >= 0) {
                fin_mod_emit_uint8(cmp, fin_op_load_local);
                fin_mod_emit_uint8(cmp, local_idx);
                FIN_LOG("\tload_loc   %2d         // %s\n", local_idx, fin_str_cstr(id_expr->name));
                break;
            }
            int32_t param_idx = fin_mod_resolve_arg(cmp, id_expr->name);
            if (param_idx >= 0) {
                fin_mod_emit_uint8(cmp, fin_op_load_arg);
                fin_mod_emit_uint8(cmp, param_idx);
                FIN_LOG("\tload_arg   %2d         // %s\n", param_idx, fin_str_cstr(id_expr->name));
                break;
            }
            assert(0);
            break;
        }
        case fin_ast_expr_type_bool: {
            fin_ast_bool_expr* bool_expr = (fin_ast_bool_expr*)expr;
            fin_val val = { .b = bool_expr->value };
            int16_t idx = fin_mod_const_idx(cmp, val);
            fin_mod_emit_uint8(cmp, fin_op_load_const);
            fin_mod_emit_uint16(cmp, idx);
            FIN_LOG("\tload_const %2d         // %s\n", idx, bool_expr->value ? "true" : "false");
            break;
        }
        case fin_ast_expr_type_int: {
            fin_ast_int_expr* int_expr = (fin_ast_int_expr*)expr;
            fin_val val = { .i = int_expr->value };
            int16_t idx = fin_mod_const_idx(cmp, val);
            fin_mod_emit_uint8(cmp, fin_op_load_const);
            fin_mod_emit_uint16(cmp, idx);
            FIN_LOG("\tload_const %2d         // %d\n", idx, (int32_t)int_expr->value);
            break;
        }
        case fin_ast_expr_type_float: {
            fin_ast_float_expr* float_expr = (fin_ast_float_expr*)expr;
            fin_val val = { .f = float_expr->value };
            int16_t idx = fin_mod_const_idx(cmp, val);
            fin_mod_emit_uint8(cmp, fin_op_load_const);
            fin_mod_emit_uint16(cmp, idx);
            FIN_LOG("\tload_const %2d         // %f\n", idx, float_expr->value);
            break;
        }
        case fin_ast_expr_type_str: {
            fin_ast_str_expr* str_expr = (fin_ast_str_expr*)expr;
            fin_val val = { .s = str_expr->value };
            int16_t idx = fin_mod_const_idx(cmp, val);
            fin_mod_emit_uint8(cmp, fin_op_load_const);
            fin_mod_emit_uint16(cmp, idx);
            FIN_LOG("\tload_const %2d         // %s\n", idx, fin_str_cstr(str_expr->value));
            break;
        }
        case fin_ast_expr_type_unary: {
            FIN_LOG("\t<fin_ast_expr_type_unary>\n");
            break;
        }
        case fin_ast_expr_type_binary: {
            fin_ast_binary_expr* bin_expr = (fin_ast_binary_expr*)expr;
            fin_mod_compile_expr(ctx, cmp, bin_expr->lhs, NULL);
            fin_mod_compile_expr(ctx, cmp, bin_expr->rhs, NULL);

            fin_str* sign = fin_mod_binary_get_signature(ctx, cmp, bin_expr);
            int16_t idx = fin_mod_bind_idx(cmp, sign);
            fin_mod_emit_uint8(cmp, fin_op_call);
            fin_mod_emit_uint16(cmp, idx);
            FIN_LOG("\tcall       %2d         // %s\n", idx, fin_str_cstr(sign));
            break;
        }
        case fin_ast_expr_type_cond: {
            fin_ast_cond_expr* cond_expr = (fin_ast_cond_expr*)expr;
            fin_mod_compile_expr(ctx, cmp, cond_expr->cond, NULL);
            fin_mod_emit_uint8(cmp, fin_op_branch_if_n);
            uint8_t* lbl_else = cmp->code;
            fin_mod_emit_uint16(cmp, 0);
            FIN_LOG("\tbr_if_n     lbl_%d\n", (int32_t)(lbl_else - cmp->code_begin));
            fin_mod_compile_expr(ctx, cmp, cond_expr->true_expr, NULL);
            if (cond_expr->false_expr) {
                fin_mod_emit_uint8(cmp, fin_op_branch);
                uint8_t* lbl_end = cmp->code;
                fin_mod_emit_uint16(cmp, 0);
                FIN_LOG("\tbr          lbl_%d\n", (int32_t)(lbl_end - cmp->code_begin));
                FIN_LOG("lbl_%d:\n", (int32_t)(lbl_else - cmp->code_begin));
                uint16_t offset = cmp->code - lbl_else - 2;
                *lbl_else++ = offset & 0xFF;
                *lbl_else++ = (offset >> 8) & 0xFF;
                fin_mod_compile_expr(ctx, cmp, cond_expr->false_expr, NULL);
                FIN_LOG("lbl_%d:\n", (int32_t)(lbl_end - cmp->code_begin));
                offset = cmp->code - lbl_end - 2;
                *lbl_end++ = offset & 0xFF;
                *lbl_end++ = (offset >> 8) & 0xFF;
            }
            else {
                FIN_LOG("lbl_%d:\n", (int32_t)(lbl_else - cmp->code_begin));
                uint16_t offset = cmp->code - lbl_else - 2;
                *lbl_else++ = offset & 0xFF;
                *lbl_else++ = (offset >> 8) & 0xFF;
            }
            break;
        }
        case fin_ast_expr_type_arg: {
            fin_ast_arg_expr* arg_expr = (fin_ast_arg_expr*)expr;
            fin_mod_compile_expr(ctx, cmp, arg_expr->expr, NULL);
            break;
        }
        case fin_ast_expr_type_invoke: {
            fin_ast_invoke_expr* invoke_expr = (fin_ast_invoke_expr*)expr;

            //fin_mod_compile_expr(ctx, &invoke_expr->id->base);
            for (fin_ast_arg_expr* e = invoke_expr->args; e; e = e->next)
                fin_mod_compile_expr(ctx, cmp, &e->base, NULL);

            fin_str* sign = fin_mod_invoke_get_signature(ctx, cmp, invoke_expr, primary);

            int16_t idx = fin_mod_bind_idx(cmp, sign);
            fin_mod_emit_uint8(cmp, fin_op_call);
            fin_mod_emit_uint16(cmp, idx);
            FIN_LOG("\tcall       %2d         // %s\n", idx, fin_str_cstr(sign));
            break;
        }
        case fin_ast_expr_type_member: {
            fin_ast_member_expr* member_expr = (fin_ast_member_expr*)expr;
            fin_mod_compile_expr(ctx, cmp, member_expr->member, member_expr->primary);
            break;
        }
        case fin_ast_expr_type_assign: {
            assert(0);
            break;
        }
        default:
            assert(0);
    }
}

static void fin_mod_compile_stmt(fin_ctx* ctx, fin_mod_compiler* cmp, fin_ast_stmt* stmt) {
    switch (stmt->type) {
        case fin_ast_stmt_type_expr: {
            fin_ast_expr_stmt* expr_stmt = (fin_ast_expr_stmt*)stmt;
            fin_mod_compile_expr(ctx, cmp, expr_stmt->expr, NULL);
            break;
        }
        case fin_ast_stmt_type_ret: {
            fin_ast_ret_stmt* ret_stmt = (fin_ast_ret_stmt*)stmt;
            if (ret_stmt->expr)
                fin_mod_compile_expr(ctx, cmp, ret_stmt->expr, NULL);
            fin_mod_emit_uint8(cmp, fin_op_return);
            FIN_LOG("\tret\n");
            break;
        }
        case fin_ast_stmt_type_if: {
            fin_ast_if_stmt* if_stmt = (fin_ast_if_stmt*)stmt;
            fin_mod_compile_expr(ctx, cmp, if_stmt->cond, NULL);
            fin_mod_emit_uint8(cmp, fin_op_branch_if_n);
            uint8_t* lbl_else = cmp->code;
            fin_mod_emit_uint16(cmp, 0);
            FIN_LOG("\tbr_if_n     lbl_%d\n", (int32_t)(lbl_else - cmp->code_begin));
            fin_mod_compile_stmt(ctx, cmp, if_stmt->true_stmt);
            if (if_stmt->false_stmt) {
                fin_mod_emit_uint8(cmp, fin_op_branch);
                uint8_t* lbl_end = cmp->code;
                fin_mod_emit_uint16(cmp, 0);
                FIN_LOG("\tbr          lbl_%d\n", (int32_t)(lbl_end - cmp->code_begin));
                FIN_LOG("lbl_%d:\n", (int32_t)(lbl_else - cmp->code_begin));
                uint16_t offset = cmp->code - lbl_else - 2;
                *lbl_else++ = offset & 0xFF;
                *lbl_else++ = (offset >> 8) & 0xFF;
                fin_mod_compile_stmt(ctx, cmp, if_stmt->false_stmt);
                FIN_LOG("lbl_%d:\n", (int32_t)(lbl_end - cmp->code_begin));
                offset = cmp->code - lbl_end - 2;
                *lbl_end++ = offset & 0xFF;
                *lbl_end++ = (offset >> 8) & 0xFF;
            }
            else {
                FIN_LOG("lbl_%d:\n", (int32_t)(lbl_else - cmp->code_begin));
                uint16_t offset = cmp->code - lbl_else - 2;
                *lbl_else++ = offset & 0xFF;
                *lbl_else++ = (offset >> 8) & 0xFF;
            }
            break;
        }
        case fin_ast_stmt_type_while: {
            fin_ast_while_stmt* while_stmt = (fin_ast_while_stmt*)stmt;
            uint8_t* lbl_loop = cmp->code;
            FIN_LOG("lbl_%d:\n", (int32_t)(lbl_loop - cmp->code_begin));
            fin_mod_compile_expr(ctx, cmp, while_stmt->cond, NULL);
            fin_mod_emit_uint8(cmp, fin_op_branch_if_n);
            uint8_t* lbl_end = cmp->code;
            fin_mod_emit_uint16(cmp, 0);
            FIN_LOG("\tbr_if_n     lbl_%d\n", (int32_t)(lbl_end - cmp->code_begin));
            fin_mod_compile_stmt(ctx, cmp, while_stmt->stmt);
            uint16_t offset = lbl_loop - cmp->code - 3;
            fin_mod_emit_uint8(cmp, fin_op_branch);
            fin_mod_emit_uint16(cmp, offset);
            FIN_LOG("\tbr          lbl_%d\n", (int32_t)(lbl_loop - cmp->code_begin));
            FIN_LOG("lbl_%d:\n", (int32_t)(lbl_end - cmp->code_begin));
            offset = cmp->code - lbl_end - 2;
            *lbl_end++ = offset & 0xFF;
            *lbl_end++ = (offset >> 8) & 0xFF;
            break;
        }
        case fin_ast_stmt_type_decl: {
            fin_ast_decl_stmt* decl_stmt = (fin_ast_decl_stmt*)stmt;
            assert(fin_mod_resolve_local(cmp, decl_stmt->name) < 0);
            int32_t local_idx = cmp->locals_count++;
            cmp->locals[local_idx] = decl_stmt->name;
            if (decl_stmt->init) {
                fin_mod_compile_expr(ctx, cmp, decl_stmt->init, NULL);
                fin_mod_emit_uint8(cmp, fin_op_store_local);
                fin_mod_emit_uint8(cmp, local_idx);
                FIN_LOG("\tstore_loc  %2d\n", local_idx);
            }
            break;
        }
        case fin_ast_stmt_type_block: {
            fin_ast_block_stmt* block_stmt = (fin_ast_block_stmt*)stmt;
            for (fin_ast_stmt* s = block_stmt->stmts; s; s = s->next)
                fin_mod_compile_stmt(ctx, cmp, s);
            break;
        }
    }
}

static void fin_mod_compile_func(fin_mod_func* out_func, fin_ctx* ctx, fin_mod* mod, fin_ast_func* func) {
    FIN_LOG("\n");
    FIN_LOG("func %s\n", fin_str_cstr(out_func->sign));

    fin_mod_compiler cmp;
    cmp.mod = mod;
    cmp.func = func;
    cmp.code = cmp.code_storage;
    cmp.code_begin = cmp.code_storage;
    cmp.code_end = cmp.code_storage + sizeof(cmp.code_storage);
    cmp.locals_count = 0;
    cmp.params_count = 0;

    for (fin_ast_param* param = func->params; param; param = param->next)
        cmp.params[cmp.params_count++] = param->name;

    fin_mod_compile_stmt(ctx, &cmp, &func->block->base);
    if (cmp.code == cmp.code_begin || cmp.code[-1] != fin_op_return) {
        fin_mod_emit_uint8(&cmp, fin_op_return);
        FIN_LOG("\tret\n");
    }

    out_func->code_length = (int32_t)(cmp.code - cmp.code_begin);
    out_func->code = (uint8_t*)ctx->alloc(NULL, out_func->code_length);
    memcpy(out_func->code, cmp.code_begin, out_func->code_length);
    out_func->locals = cmp.locals_count;

    FIN_LOG("\n");
}

static void fin_mod_compile_type(fin_mod_type* out_type, fin_ast_type* type) {
    int32_t fields = 0;
    fin_ast_field* field = type->fields;
    while (field) {
        fields++;
        field = field->next;
    }

    *out_type = (fin_mod_type){ .name = type->name, .fields = fields };
}

static void fin_mod_register(fin_ctx* ctx, fin_mod* mod) {
    mod->next = ctx->mod;
    ctx->mod = mod;

    for (int32_t i=0; i<mod->binds_count; i++) {
        fin_mod* m = ctx->mod;
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

static void fin_mod_unregister(fin_ctx* ctx, fin_mod* mod) {

}

fin_mod* fin_mod_create(fin_ctx* ctx, const char* name, fin_mod_func_desc* descs, int32_t descs_count) {
    fin_str* mod_name = fin_str_create(ctx->pool, name, -1);

    fin_mod_func* funcs = (fin_mod_func*)ctx->alloc(NULL, sizeof(fin_mod_func) * descs_count);

    fin_mod* mod = (fin_mod*)ctx->alloc(NULL, sizeof(fin_mod));
    *mod = (fin_mod){ .name = mod_name, .funcs = funcs, .funcs_count = descs_count, .next = NULL };

    for (int32_t i=0; i<descs_count; i++) {
        funcs[i].mod = mod;
        funcs[i].func = descs[i].func;
        funcs[i].is_native = true;
        funcs[i].code = NULL;
        funcs[i].code_length = 0;
        funcs[i].args = 0;

        // ret_type name "(" (arg ("," arg)* )? ")"
        fin_lex* lex = fin_lex_create(ctx->alloc, ctx->pool, descs[i].sign);

        char signature[256];
        signature[0] = '\0';

        if (name != NULL && name[0] != '\0') {
            strcat(signature, name);
            strcat(signature, ".");
        }

        if (fin_lex_match(lex, fin_lex_type_void))
            funcs[i].ret_type = NULL;
        else
            funcs[i].ret_type = fin_lex_consume_name(lex);

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
        funcs[i].sign = fin_str_create(ctx->pool, signature, -1);

        fin_lex_destroy(lex);
    }


    fin_mod_register(ctx, mod);
    return mod;
}

fin_mod* fin_mod_compile(fin_ctx* ctx, const char* cstr) {
    fin_ast_module* module = fin_ast_parse(ctx->alloc, ctx->pool, cstr);

    fin_mod* mod = (fin_mod*)ctx->alloc(NULL, sizeof(fin_mod));
    mod->consts = (fin_val*)ctx->alloc(NULL, sizeof(fin_val) * 128);
    mod->consfin_count = 0;
    mod->binds = (fin_mod_func_bind*)ctx->alloc(NULL, sizeof(fin_mod_func_bind) * 128);
    mod->binds_count = 0;
    mod->types_count = 0;
    mod->funcs_count = 0;

    mod->next = NULL;

    for (fin_ast_type* type = module->types; type; type = type->next)
        mod->types_count++;
    for (fin_ast_func* func = module->funcs; func; func = func->next)
        mod->funcs_count++;

    if (mod->types_count) {
        mod->types = (fin_mod_type*)ctx->alloc(NULL, sizeof(fin_mod_type) * mod->types_count);

        int32_t idx = 0;
        for (fin_ast_type* type = module->types; type; type = type->next)
            fin_mod_compile_type(&mod->types[idx++], type);
    }

    if (mod->funcs_count) {
        mod->funcs = (fin_mod_func*)ctx->alloc(NULL, sizeof(fin_mod_func) * mod->funcs_count);

        int32_t idx = 0;
        for (fin_ast_func* func = module->funcs; func; func = func->next) {
            fin_mod_func* f = &mod->funcs[idx++];

            char signature[128];
            signature[0] = '\0';
            //strcat(signature, fin_str_cstr(func->ret->name));
            //strcat(signature, " ");
            strcat(signature, fin_str_cstr(func->name));
            strcat(signature, "(");
            uint8_t args = 0;
            for (fin_ast_param* param = func->params; param; param = param->next) {
                if (args)
                    strcat(signature, ",");
                strcat(signature, fin_str_cstr(param->type->name));
                args++;
            }
            strcat(signature, ")");

            f->mod = mod;
            f->sign = fin_str_create(ctx->pool, signature, -1);
            f->func = NULL;
            f->is_native = false;
            f->code = NULL;
            f->code_length = 0;
            f->args = args;
            f->ret_type = func->ret ? func->ret->name : NULL;
        }

        idx = 0;
        for (fin_ast_func* func = module->funcs; func; func = func->next)
            fin_mod_compile_func(&mod->funcs[idx++], ctx, mod, func);
    }

    fin_ast_destroy(module);

    mod->name = NULL;
    mod->entry = fin_mod_find_func(ctx, mod, fin_str_create(ctx->pool, "Main()", 6));
    fin_mod_register(ctx, mod);
    return mod;
}

void fin_mod_destroy(fin_ctx* ctx, fin_mod* mod) {
    fin_mod_unregister(ctx, mod);
    free(mod);
}

