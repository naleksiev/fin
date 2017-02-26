/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_ast.h"
#include "fin_ctx.h"
#include "fin_lex.h"
#include "fin_str.h"
#include <assert.h>

static fin_ast_expr_t* fin_ast_parse_expr(fin_ctx_t* ctx, fin_lex_t* lex, fin_ast_expr_t* expr);
static fin_ast_stmt_t* fin_ast_parse_stmt(fin_ctx_t* ctx, fin_lex_t* lex);

inline static fin_str_t* fin_str_from_lex(fin_ctx_t* ctx, fin_lex_str_t lex_str) {
    return fin_str_create(ctx, lex_str.cstr, lex_str.len);
}

static void fin_ast_expect(fin_lex_t* lex, fin_lex_type_t type) {
    if (fin_lex_get_type(lex) == type) {
        fin_lex_next(lex);
        return;
    }
    assert(0);
}

static fin_ast_type_ref_t* fin_ast_parse_type_ref(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_type_ref_t* type = (fin_ast_type_ref_t*)ctx->alloc(NULL, sizeof(fin_ast_type_ref_t));
    type->module = NULL;
    type->name = fin_str_from_lex(ctx, fin_lex_consume_name(lex));
    if (fin_lex_match(lex, fin_lex_type_dot)) {
        type->module = type->name;
        type->name = fin_str_from_lex(ctx, fin_lex_consume_name(lex));
    }
    return type;
}

static fin_ast_arg_expr_t* fin_ast_parse_arg_expr(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_expr_t* expr = fin_ast_parse_expr(ctx, lex, NULL);
    fin_ast_arg_expr_t* arg_expr = (fin_ast_arg_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_arg_expr_t));
    arg_expr->base.type = fin_ast_expr_type_arg;
    arg_expr->expr = expr;
    arg_expr->next = NULL;
    return arg_expr;
}

static fin_ast_expr_t* fin_ast_parse_id_expr(fin_ctx_t* ctx, fin_lex_t* lex, fin_ast_expr_t* primary) {
    fin_ast_id_expr_t* id_expr = (fin_ast_id_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_id_expr_t));
    id_expr->base.type = fin_ast_expr_type_id;
    id_expr->primary = primary;
    id_expr->name = fin_str_from_lex(ctx, fin_lex_consume_name(lex));
    if (fin_lex_match(lex, fin_lex_type_dot))
        return fin_ast_parse_id_expr(ctx, lex, &id_expr->base);
    return &id_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_invoke_expr(fin_ctx_t* ctx, fin_lex_t* lex, fin_ast_expr_t* id) {
    fin_ast_invoke_expr_t* invoke_expr = (fin_ast_invoke_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_invoke_expr_t));
    invoke_expr->base.type = fin_ast_expr_type_invoke;
    invoke_expr->id = id;
    fin_ast_expect(lex, fin_lex_type_l_paren);
    fin_ast_arg_expr_t** tail = &invoke_expr->args;
    *tail = NULL;
    while (!fin_lex_match(lex, fin_lex_type_r_paren)) {
        if (invoke_expr->args)
            fin_ast_expect(lex, fin_lex_type_comma);
        *tail = fin_ast_parse_arg_expr(ctx, lex);
        tail = &(*tail)->next;
    }
    return &invoke_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_const_expr(fin_ctx_t* ctx, fin_lex_t* lex) {
    if (fin_lex_get_type(lex) == fin_lex_type_bool) {
        fin_ast_bool_expr_t* bool_expr = (fin_ast_bool_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_bool_expr_t));
        bool_expr->base.type = fin_ast_expr_type_bool;
        bool_expr->value = fin_lex_consume_bool(lex);
        return &bool_expr->base;
    }
    else if (fin_lex_get_type(lex) == fin_lex_type_int) {
        fin_ast_int_expr_t* int_expr = (fin_ast_int_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_int_expr_t));
        int_expr->base.type = fin_ast_expr_type_int;
        int_expr->value = fin_lex_consume_int(lex);
        return &int_expr->base;
    }
    else if (fin_lex_get_type(lex) == fin_lex_type_float) {
        fin_ast_float_expr_t* float_expr = (fin_ast_float_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_float_expr_t));
        float_expr->base.type = fin_ast_expr_type_float;
        float_expr->value = fin_lex_consume_float(lex);
        return &float_expr->base;
    }
    else if (fin_lex_get_type(lex) == fin_lex_type_string) {
        fin_ast_str_expr_t* str_expr = (fin_ast_str_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_str_expr_t));
        str_expr->base.type = fin_ast_expr_type_str;
        str_expr->value = fin_str_from_lex(ctx, fin_lex_consume_string(lex));
        return &str_expr->base;
    }
    else {
        assert(0);
        return NULL;
    }
}

static fin_ast_expr_t* fin_ast_parse_str_interp_expr(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_str_interp_expr_t* expr = NULL;
    fin_ast_str_interp_expr_t** tail = &expr;
    fin_ast_expect(lex, fin_lex_type_quot);
    while (!fin_lex_match(lex, fin_lex_type_quot)) {
        fin_ast_expr_t* next = NULL;
        if (fin_lex_get_type(lex) == fin_lex_type_string)
            next = fin_ast_parse_const_expr(ctx, lex);
        else if (fin_lex_match(lex, fin_lex_type_l_str_interp)) {
            next = fin_ast_parse_expr(ctx, lex, NULL);
            fin_ast_expect(lex, fin_lex_type_r_str_interp);
        }
        *tail = (fin_ast_str_interp_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_str_interp_expr_t));
        (*tail)->base.type = fin_ast_expr_type_str_interp;
        (*tail)->expr = next;
        (*tail)->next = NULL;
        tail = &(*tail)->next;
    }
    return &expr->base;
}

static fin_ast_expr_t* fin_ast_parse_unary_expr(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_unary_type_t op;
    if (fin_lex_match(lex, fin_lex_type_plus))
        op = fin_ast_unary_type_pos;
    else if (fin_lex_match(lex, fin_lex_type_minus))
        op = fin_ast_unary_type_neg;
    else if (fin_lex_match(lex, fin_lex_type_bang))
        op = fin_ast_unary_type_not;
    else if (fin_lex_match(lex, fin_lex_type_tilde))
        op = fin_ast_unary_type_bnot;
    else if (fin_lex_match(lex, fin_lex_type_plus_plus))
        op = fin_ast_unary_type_inc;
    else if (fin_lex_match(lex, fin_lex_type_minus_minus))
        op = fin_ast_unary_type_dec;
    else if (fin_lex_get_type(lex) == fin_lex_type_l_paren)
        return fin_ast_parse_const_expr(ctx, lex);
    else if (fin_lex_get_type(lex) == fin_lex_type_int)
        return fin_ast_parse_const_expr(ctx, lex);
    else if (fin_lex_get_type(lex) == fin_lex_type_float)
        return fin_ast_parse_const_expr(ctx, lex);
    else if (fin_lex_get_type(lex) == fin_lex_type_bool)
        return fin_ast_parse_const_expr(ctx, lex);
    else if (fin_lex_get_type(lex) == fin_lex_type_string)
        return fin_ast_parse_const_expr(ctx, lex);
    else if (fin_lex_get_type(lex) == fin_lex_type_quot)
        return fin_ast_parse_str_interp_expr(ctx, lex);
    else {
        fin_ast_expr_t* id_expr = fin_ast_parse_id_expr(ctx, lex, NULL);
        return fin_lex_get_type(lex) == fin_lex_type_l_paren ? fin_ast_parse_invoke_expr(ctx, lex, id_expr) : id_expr;
    }

    fin_ast_expr_t* expr = fin_ast_parse_unary_expr(ctx, lex);
    fin_ast_unary_expr_t* un_expr = (fin_ast_unary_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_unary_expr_t));
    *un_expr = (fin_ast_unary_expr_t){ .base = (fin_ast_expr_t){ .type = fin_ast_expr_type_unary }, .op = op, .expr = expr };
    return &un_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_multiplicative_expr(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_expr_t* lhs = fin_ast_parse_unary_expr(ctx, lex);
    fin_ast_binary_type_t op;
    if (fin_lex_match(lex, fin_lex_type_star))
        op = fin_ast_binary_type_mul;
    else if (fin_lex_match(lex, fin_lex_type_slash))
        op = fin_ast_binary_type_div;
    else if (fin_lex_match(lex, fin_lex_type_percent))
        op = fin_ast_binary_type_mod;
    else
        return lhs;
    fin_ast_binary_expr_t* bin_expr = (fin_ast_binary_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_binary_expr_t));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_multiplicative_expr(ctx, lex);
    return &bin_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_additive_expr(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_expr_t* lhs = fin_ast_parse_multiplicative_expr(ctx, lex);
    fin_ast_binary_type_t op;
    if (fin_lex_match(lex, fin_lex_type_plus))
        op = fin_ast_binary_type_add;
    else if (fin_lex_match(lex, fin_lex_type_minus))
        op = fin_ast_binary_type_sub;
    else
        return lhs;
    fin_ast_binary_expr_t* bin_expr = (fin_ast_binary_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_binary_expr_t));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_additive_expr(ctx, lex);
    return &bin_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_shift_expr(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_expr_t* lhs = fin_ast_parse_additive_expr(ctx, lex);
    fin_ast_binary_type_t op;
    if (fin_lex_match(lex, fin_lex_type_lt_lt))
        op = fin_ast_binary_type_shl;
    else if (fin_lex_match(lex, fin_lex_type_gt_gt))
        op = fin_ast_binary_type_shr;
    else
        return lhs;
    fin_ast_binary_expr_t* bin_expr = (fin_ast_binary_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_binary_expr_t));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_shift_expr(ctx, lex);
    return &bin_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_relational_expr(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_expr_t* lhs = fin_ast_parse_shift_expr(ctx, lex);
    fin_ast_binary_type_t op;
    if (fin_lex_match(lex, fin_lex_type_lt))
        op = fin_ast_binary_type_ls;
    else if (fin_lex_match(lex, fin_lex_type_gt))
        op = fin_ast_binary_type_gr;
    else if (fin_lex_match(lex, fin_lex_type_lt_eq))
        op = fin_ast_binary_type_leq;
    else if (fin_lex_match(lex, fin_lex_type_gt_eq))
        op = fin_ast_binary_type_geq;
    else
        return lhs;
    fin_ast_binary_expr_t* bin_expr = (fin_ast_binary_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_binary_expr_t));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_shift_expr(ctx, lex);
    return &bin_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_equality_expr(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_expr_t* lhs = fin_ast_parse_relational_expr(ctx, lex);
    fin_ast_binary_type_t op;
    if (fin_lex_match(lex, fin_lex_type_eq_eq))
        op = fin_ast_binary_type_eq;
    else if (fin_lex_match(lex, fin_lex_type_bang_eq))
        op = fin_ast_binary_type_neq;
    else
        return lhs;
    fin_ast_binary_expr_t* bin_expr = (fin_ast_binary_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_binary_expr_t));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_equality_expr(ctx, lex);
    return &bin_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_and_expr(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_expr_t* lhs = fin_ast_parse_equality_expr(ctx, lex);
    fin_ast_binary_type_t op;
    if (fin_lex_match(lex, fin_lex_type_amp))
        op = fin_ast_binary_type_eq;
    else
        return lhs;
    fin_ast_binary_expr_t* bin_expr = (fin_ast_binary_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_binary_expr_t));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_and_expr(ctx, lex);
    return &bin_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_xor_expr(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_expr_t* lhs = fin_ast_parse_and_expr(ctx, lex);
    fin_ast_binary_type_t op;
    if (fin_lex_match(lex, fin_lex_type_caret))
        op = fin_ast_binary_type_bxor;
    else
        return lhs;
    fin_ast_binary_expr_t* bin_expr = (fin_ast_binary_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_binary_expr_t));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_xor_expr(ctx, lex);
    return &bin_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_or_expr(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_expr_t* lhs = fin_ast_parse_xor_expr(ctx, lex);
    fin_ast_binary_type_t op;
    if (fin_lex_match(lex, fin_lex_type_pipe))
        op = fin_ast_binary_type_bor;
    else
        return lhs;
    fin_ast_binary_expr_t* bin_expr = (fin_ast_binary_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_binary_expr_t));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_or_expr(ctx, lex);
    return &bin_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_cond_and_expr(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_expr_t* lhs = fin_ast_parse_or_expr(ctx, lex);
    fin_ast_binary_type_t op;
    if (fin_lex_match(lex, fin_lex_type_amp_amp))
        op = fin_ast_binary_type_and;
    else
        return lhs;
    fin_ast_binary_expr_t* bin_expr = (fin_ast_binary_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_binary_expr_t));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_cond_and_expr(ctx, lex);
    return &bin_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_cond_or_expr(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_expr_t* lhs = fin_ast_parse_cond_and_expr(ctx, lex);
    fin_ast_binary_type_t op;
    if (fin_lex_match(lex, fin_lex_type_pipe_pipe))
        op = fin_ast_binary_type_or;
    else
        return lhs;
    fin_ast_binary_expr_t* bin_expr = (fin_ast_binary_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_binary_expr_t));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_cond_or_expr(ctx, lex);
    return &bin_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_cond_expr(fin_ctx_t* ctx, fin_lex_t* lex, fin_ast_expr_t* cond) {
    fin_ast_cond_expr_t* cond_expr = (fin_ast_cond_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_cond_expr_t));
    cond_expr->base.type = fin_ast_expr_type_cond;
    cond_expr->cond = cond;
    fin_ast_expect(lex, fin_lex_type_question);
    cond_expr->true_expr = fin_ast_parse_cond_or_expr(ctx, lex);
    fin_ast_expect(lex, fin_lex_type_colon);
    cond_expr->false_expr = fin_ast_parse_cond_or_expr(ctx, lex);
    return &cond_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_init_expr(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_init_expr_t* init_expr = (fin_ast_init_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_init_expr_t));
    init_expr->base.type = fin_ast_expr_type_init;
    fin_ast_expect(lex, fin_lex_type_l_brace);
    fin_ast_arg_expr_t** tail = &init_expr->args;
    *tail = NULL;
    while (!fin_lex_match(lex, fin_lex_type_r_brace)) {
        if (init_expr->args)
            fin_ast_expect(lex, fin_lex_type_comma);
        *tail = fin_ast_parse_arg_expr(ctx, lex);
        tail = &(*tail)->next;
    }
    return &init_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_assign_expr(fin_ctx_t* ctx, fin_lex_t* lex, fin_ast_expr_t* lhs) {
    fin_ast_assign_expr_t* assign_expr = (fin_ast_assign_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_assign_expr_t));
    assign_expr->base.type = fin_ast_expr_type_assign;
    assign_expr->lhs = lhs;
    if (fin_lex_match(lex, fin_lex_type_eq))
        assign_expr->op = fin_ast_assign_type_assign;
    else if (fin_lex_match(lex, fin_lex_type_plus_eq))
        assign_expr->op = fin_ast_assign_type_add;
    else if (fin_lex_match(lex, fin_lex_type_minus_eq))
        assign_expr->op = fin_ast_assign_type_sub;
    else if (fin_lex_match(lex, fin_lex_type_star_eq))
        assign_expr->op = fin_ast_assign_type_mul;
    else if (fin_lex_match(lex, fin_lex_type_slash_eq))
        assign_expr->op = fin_ast_assign_type_div;
    else if (fin_lex_match(lex, fin_lex_type_percent_eq))
        assign_expr->op = fin_ast_assign_type_mod;
    else if (fin_lex_match(lex, fin_lex_type_amp_eq))
        assign_expr->op = fin_ast_assign_type_and;
    else if (fin_lex_match(lex, fin_lex_type_pipe_eq))
        assign_expr->op = fin_ast_assign_type_or;
    else if (fin_lex_match(lex, fin_lex_type_caret_eq))
        assign_expr->op = fin_ast_assign_type_xor;
    else if (fin_lex_match(lex, fin_lex_type_lt_lt_eq))
        assign_expr->op = fin_ast_assign_type_shl;
    else if (fin_lex_match(lex, fin_lex_type_gt_gt_eq))
        assign_expr->op = fin_ast_assign_type_shr;
    else
        assert(0);

    assign_expr->rhs = fin_ast_parse_expr(ctx, lex, NULL);
    return &assign_expr->base;
}

static fin_ast_expr_t* fin_ast_parse_expr(fin_ctx_t* ctx, fin_lex_t* lex, fin_ast_expr_t* expr) {
    if (!expr)
        expr = fin_ast_parse_cond_or_expr(ctx, lex);
    switch (fin_lex_get_type(lex)) {
        case fin_lex_type_question:
            return fin_ast_parse_cond_expr(ctx, lex, expr);
        case fin_lex_type_dot:
            return fin_ast_parse_id_expr(ctx, lex, expr);
        case fin_lex_type_l_paren:
            return fin_ast_parse_invoke_expr(ctx, lex, expr);
        default:
            return expr;
    }
}

static fin_ast_stmt_t* fin_ast_parse_if_stmt(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_expect(lex, fin_lex_type_if);
    fin_ast_expect(lex, fin_lex_type_l_paren);
    fin_ast_expr_t* cond = fin_ast_parse_expr(ctx, lex, NULL);
    fin_ast_expect(lex, fin_lex_type_r_paren);
    fin_ast_stmt_t* true_stmt = fin_ast_parse_stmt(ctx, lex); fin_ast_stmt_t* false_stmt = NULL;
    if (fin_lex_match(lex, fin_lex_type_else))
        false_stmt = fin_ast_parse_stmt(ctx, lex);
    fin_ast_if_stmt_t* stmt = (fin_ast_if_stmt_t*)ctx->alloc(NULL, sizeof(fin_ast_if_stmt_t));
    *stmt = (fin_ast_if_stmt_t){ .base = (fin_ast_stmt_t){ .type = fin_ast_stmt_type_if, .next = NULL }, .cond = cond, .true_stmt = true_stmt, .false_stmt = false_stmt };
    return &stmt->base;
}

static fin_ast_stmt_t* fin_ast_parse_while_stmt(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_expect(lex, fin_lex_type_while);
    fin_ast_expect(lex, fin_lex_type_l_paren);
    fin_ast_expr_t* cond = fin_ast_parse_expr(ctx, lex, NULL);
    fin_ast_expect(lex, fin_lex_type_r_paren);
    fin_ast_stmt_t* stmt = fin_ast_parse_stmt(ctx, lex);
    fin_ast_while_stmt_t* while_stmt = (fin_ast_while_stmt_t*)ctx->alloc(NULL, sizeof(fin_ast_while_stmt_t));
    *while_stmt = (fin_ast_while_stmt_t){ .base = (fin_ast_stmt_t){ .type = fin_ast_stmt_type_while, .next = NULL }, .cond = cond, .stmt = stmt };
    return &while_stmt->base;
}

static fin_ast_stmt_t* fin_ast_parse_for_stmt(fin_ctx_t* ctx, fin_lex_t* lex) {
    return NULL;
}

static fin_ast_stmt_t* fin_ast_parse_decl_stmt(fin_ctx_t* ctx, fin_lex_t* lex, fin_ast_type_ref_t* type) {
    fin_ast_decl_stmt_t* stmt = (fin_ast_decl_stmt_t*)ctx->alloc(NULL, sizeof(fin_ast_decl_stmt_t));
    stmt->base.type = fin_ast_stmt_type_decl;
    stmt->base.next = NULL;
    stmt->type = type;
    stmt->name = fin_str_from_lex(ctx, fin_lex_consume_name(lex));
    stmt->init = NULL;
    if (fin_lex_match(lex, fin_lex_type_eq)) {
        if (fin_lex_get_type(lex) == fin_lex_type_l_brace)
            stmt->init = fin_ast_parse_init_expr(ctx, lex);
        else
            stmt->init = fin_ast_parse_expr(ctx, lex, NULL);
    }
    fin_ast_expect(lex, fin_lex_type_semicolon);
    return &stmt->base;
}

static fin_ast_stmt_t* fin_ast_parse_ret_stmt(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_ret_stmt_t* stmt = (fin_ast_ret_stmt_t*)ctx->alloc(NULL, sizeof(fin_ast_ret_stmt_t));
    stmt->base.type = fin_ast_stmt_type_ret;
    stmt->base.next = NULL;
    stmt->expr = NULL;
    fin_ast_expect(lex, fin_lex_type_return);
    if (fin_lex_get_type(lex) == fin_lex_type_l_brace)
        stmt->expr = fin_ast_parse_init_expr(ctx, lex);
    else if (fin_lex_get_type(lex) != fin_lex_type_semicolon)
        stmt->expr = fin_ast_parse_expr(ctx, lex, NULL);
    fin_ast_expect(lex, fin_lex_type_semicolon);
    return &stmt->base;
}

static fin_ast_stmt_t* fin_ast_parse_expr_stmt(fin_ctx_t* ctx, fin_lex_t* lex, fin_ast_expr_t* expr) {
    switch (fin_lex_get_type(lex)) {
        case fin_lex_type_eq:
        case fin_lex_type_plus_eq:
        case fin_lex_type_minus_eq:
        case fin_lex_type_star_eq:
        case fin_lex_type_slash_eq:
        case fin_lex_type_percent_eq:
        case fin_lex_type_amp_eq:
        case fin_lex_type_pipe_eq:
        case fin_lex_type_caret_eq:
        case fin_lex_type_lt_lt_eq:
        case fin_lex_type_gt_gt_eq:
            expr = fin_ast_parse_assign_expr(ctx, lex, expr);
        default:
            break;
    }
    fin_ast_expr_stmt_t* stmt = (fin_ast_expr_stmt_t*)ctx->alloc(NULL, sizeof(fin_ast_expr_stmt_t));
    stmt->base.type = fin_ast_stmt_type_expr;
    stmt->base.next = NULL;
    stmt->expr = expr;
    fin_ast_expect(lex, fin_lex_type_semicolon);
    return &stmt->base;
}

static fin_ast_block_stmt_t* fin_ast_parse_block_stmt(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_block_stmt_t* block = (fin_ast_block_stmt_t*)ctx->alloc(NULL, sizeof(fin_ast_block_stmt_t));
    block->base.type = fin_ast_stmt_type_block;
    block->base.next = NULL;
    fin_ast_expect(lex, fin_lex_type_l_brace);
    fin_ast_stmt_t** tail = &block->stmts;
    *tail = NULL;
    while (!fin_lex_match(lex, fin_lex_type_r_brace)) {
        *tail = fin_ast_parse_stmt(ctx, lex);
        tail = &(*tail)->next;
    }
    return block;
}

static fin_ast_stmt_t* fin_ast_parse_expr_or_decl_stmt(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_str_t* id1 = fin_str_from_lex(ctx, fin_lex_consume_name(lex));
    fin_str_t* id2 = NULL;
    if (fin_lex_match(lex, fin_lex_type_dot))
        id2 = fin_str_from_lex(ctx, fin_lex_consume_name(lex));
    if (fin_lex_get_type(lex) == fin_lex_type_name) {
        fin_ast_type_ref_t* type = (fin_ast_type_ref_t*)ctx->alloc(NULL, sizeof(fin_ast_type_ref_t));
        type->module = id2 ? id1 : NULL;
        type->name = id2 ? id2 : id1;
        return fin_ast_parse_decl_stmt(ctx, lex, type);
    }
    fin_ast_id_expr_t* id1_expr = (fin_ast_id_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_id_expr_t));
    id1_expr->base.type = fin_ast_expr_type_id;
    id1_expr->primary = NULL;
    id1_expr->name = id1;
    fin_ast_expr_t* expr = &id1_expr->base;
    if (id2) {
        fin_ast_id_expr_t* id2_expr = (fin_ast_id_expr_t*)ctx->alloc(NULL, sizeof(fin_ast_id_expr_t));
        id2_expr->base.type = fin_ast_expr_type_id;
        id2_expr->primary = expr;
        id2_expr->name = id2;
        expr = &id2_expr->base;
    }
    expr = fin_ast_parse_expr(ctx, lex, expr);
    return fin_ast_parse_expr_stmt(ctx, lex, expr);
}

static fin_ast_stmt_t* fin_ast_parse_stmt(fin_ctx_t* ctx, fin_lex_t* lex) {
    switch (fin_lex_get_type(lex)) {
        case fin_lex_type_l_brace:
            return &fin_ast_parse_block_stmt(ctx, lex)->base;
        case fin_lex_type_if:
            return fin_ast_parse_if_stmt(ctx, lex);
        //case fin_lex_type_switch:
        //    return fin_ast_parse_switch_stmt(lex);
        case fin_lex_type_while:
            return fin_ast_parse_while_stmt(ctx, lex);
        case fin_lex_type_for:
            return fin_ast_parse_for_stmt(ctx, lex);
//        case fin_lex_type_break:
//            return fin_ast_parse_break_stmt(lex);
//        case fin_lex_type_continue:
//            return fin_ast_parse_continue_stmt(lex);
        case fin_lex_type_return:
            return fin_ast_parse_ret_stmt(ctx, lex);
        case fin_lex_type_name:
            return fin_ast_parse_expr_or_decl_stmt(ctx, lex);
        default:
            assert(0);
    }
}

static fin_ast_param_t* fin_ast_parse_param(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_param_t* param = (fin_ast_param_t*)ctx->alloc(NULL, sizeof(fin_ast_param_t));
    param->type = fin_ast_parse_type_ref(ctx, lex);
    param->name = fin_str_from_lex(ctx, fin_lex_consume_name(lex));
    param->next = NULL;
    return param;
}

static fin_ast_generic_t* fin_ast_parse_generics(fin_ctx_t* ctx, fin_lex_t* lex) {
    if (!fin_lex_match(lex, fin_lex_type_lt))
        return NULL;
    fin_ast_generic_t*  gen  = NULL;
    fin_ast_generic_t** tail = &gen;
    while (true) {
        *tail = (fin_ast_generic_t*)ctx->alloc(NULL, sizeof(fin_ast_generic_t));
        (*tail)->name = fin_str_from_lex(ctx, fin_lex_consume_name(lex));
        tail = &(*tail)->next;
        if (!fin_lex_match(lex, fin_lex_type_comma))
            break;
    }
    fin_ast_expect(lex, fin_lex_type_gt);
    return gen;
}

static fin_ast_func_t* fin_ast_parse_func(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_func_t* func = (fin_ast_func_t*)ctx->alloc(NULL, sizeof(fin_ast_func_t));
    func->ret = fin_ast_parse_type_ref(ctx, lex);
    func->name = fin_str_from_lex(ctx, fin_lex_consume_name(lex));
    func->generics = fin_ast_parse_generics(ctx, lex);
    fin_ast_expect(lex, fin_lex_type_l_paren);
    fin_ast_param_t** tail = &func->params;
    *tail = NULL;
    while (!fin_lex_match(lex, fin_lex_type_r_paren)) {
        if (func->params)
            fin_ast_expect(lex, fin_lex_type_comma);
        *tail = fin_ast_parse_param(ctx, lex);
        tail = &(*tail)->next;
    }
    func->block = fin_ast_parse_block_stmt(ctx, lex);
    func->next = NULL;
    return func;
}

static struct fin_ast_enum_val_t* fin_ast_parse_enum_val(fin_ctx_t* ctx, fin_lex_t* lex) {
    struct fin_ast_enum_val_t* val = (fin_ast_enum_val_t*)ctx->alloc(NULL, sizeof(fin_ast_enum_val_t));
    val->name = fin_str_from_lex(ctx, fin_lex_consume_name(lex));
    val->expr = fin_lex_match(lex, fin_lex_type_eq) ? fin_ast_parse_expr(ctx, lex, NULL) : NULL;
    val->next = NULL;
    return val;
}

static fin_ast_enum_t* fin_ast_parse_enum(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_enum_t* e = (fin_ast_enum_t*)ctx->alloc(NULL, sizeof(fin_ast_enum_t));
    fin_ast_expect(lex, fin_lex_type_enum);
    e->name = fin_str_from_lex(ctx, fin_lex_consume_name(lex));
    fin_ast_expect(lex, fin_lex_type_l_brace);
    fin_ast_enum_val_t** val_tail = &e->values;
    *val_tail = NULL;
    while (!fin_lex_match(lex, fin_lex_type_r_brace)) {
        if (e->values)
            fin_ast_expect(lex, fin_lex_type_comma);
        *val_tail = fin_ast_parse_enum_val(ctx, lex);
        val_tail = &(*val_tail)->next;
    }
    e->next = NULL;
    return e;
}

static fin_ast_field_t* fin_ast_parse_field(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_field_t* field = (fin_ast_field_t*)ctx->alloc(NULL, sizeof(fin_ast_field_t));
    field->type = fin_ast_parse_type_ref(ctx, lex);
    field->name = fin_str_from_lex(ctx, fin_lex_consume_name(lex));
    fin_ast_expect(lex, fin_lex_type_semicolon);
    field->next = NULL;
    return field;
}

static fin_ast_type_t* fin_ast_parse_type(fin_ctx_t* ctx, fin_lex_t* lex) {
    fin_ast_type_t* type = (fin_ast_type_t*)ctx->alloc(NULL, sizeof(fin_ast_type_t));
    fin_ast_expect(lex, fin_lex_type_struct);
    type->name = fin_str_from_lex(ctx, fin_lex_consume_name(lex));
    type->generics = fin_ast_parse_generics(ctx, lex);
    fin_ast_expect(lex, fin_lex_type_l_brace);
    fin_ast_field_t** field_tail = &type->fields;
    *field_tail = NULL;
    while (!fin_lex_match(lex, fin_lex_type_r_brace)) {
        *field_tail = fin_ast_parse_field(ctx, lex);
        field_tail = &(*field_tail)->next;
    }
    type->next = NULL;
    return type;
}

fin_ast_module_t* fin_ast_parse(fin_ctx_t* ctx, const char* str) {
    fin_lex_t* lex = fin_lex_create(ctx->alloc, str);
    fin_str_t* name = fin_str_create(ctx, "<noname>", -1);

/*
    if (fin_lex_match(&lex, fin_lex_type_module)) {
    }
    while (fin_lex_match(lex, fin_lex_type_import)) {
    }
*/

    fin_ast_func_t*  funcs = NULL;
    fin_ast_func_t** func_tail = &funcs;

    fin_ast_enum_t*  enums = NULL;
    fin_ast_enum_t** enum_tail = &enums;

    fin_ast_type_t*  types = NULL;
    fin_ast_type_t** type_tail = &types;

    while (fin_lex_get_type(lex) != fin_lex_type_eof) {
        if (fin_lex_get_type(lex) == fin_lex_type_struct) {
            *type_tail = fin_ast_parse_type(ctx, lex);
            type_tail = &(*type_tail)->next;
        }
        else  if (fin_lex_get_type(lex) == fin_lex_type_enum) {
            *enum_tail = fin_ast_parse_enum(ctx, lex);
            enum_tail = &(*enum_tail)->next;
        }
        else {
            *func_tail = fin_ast_parse_func(ctx, lex);
            func_tail = &(*func_tail)->next;
        }
    }

    fin_lex_destroy(ctx->alloc, lex);

    fin_ast_module_t* module = (fin_ast_module_t*)ctx->alloc(NULL, sizeof(fin_ast_module_t));
    module->ctx = ctx;
    module->name = name;
    module->types = types;
    module->enums = enums;
    module->funcs = funcs;
    return module;
}

static void fin_ast_type_ref_destroy(fin_ast_module_t* mod, fin_ast_type_ref_t* type) {
    fin_str_destroy(mod->ctx, type->name);
    if (type->module)
        fin_str_destroy(mod->ctx, type->module);
    mod->ctx->alloc(type, 0);
}

static void fin_ast_expr_destroy(fin_ast_module_t* mod, fin_ast_expr_t* expr) {
    if (!expr)
        return;
    switch (expr->type) {
        case fin_ast_expr_type_id: {
            fin_ast_id_expr_t* id_expr = (fin_ast_id_expr_t*)expr;
            fin_ast_expr_destroy(mod, id_expr->primary);
            fin_str_destroy(mod->ctx, id_expr->name);
            break;
        }
        case fin_ast_expr_type_bool: {
            break;
        }
        case fin_ast_expr_type_int: {
            break;
        }
        case fin_ast_expr_type_float: {
            break;
        }
        case fin_ast_expr_type_str: {
            fin_ast_str_expr_t* str_expr = (fin_ast_str_expr_t*)expr;
            if (str_expr->value)
                fin_str_destroy(mod->ctx, str_expr->value);
            break;
        }
        case fin_ast_expr_type_str_interp: {
            fin_ast_str_interp_expr_t* interp_expr = (fin_ast_str_interp_expr_t*)expr;
            fin_ast_expr_destroy(mod, &interp_expr->next->base);
            fin_ast_expr_destroy(mod, interp_expr->expr);
            break;
        }
        case fin_ast_expr_type_unary: {
            fin_ast_unary_expr_t* un_expr = (fin_ast_unary_expr_t*)expr;
            fin_ast_expr_destroy(mod, un_expr->expr);
            break;
        }
        case fin_ast_expr_type_binary: {
            fin_ast_binary_expr_t* bin_expr = (fin_ast_binary_expr_t*)expr;
            fin_ast_expr_destroy(mod, bin_expr->lhs);
            fin_ast_expr_destroy(mod, bin_expr->rhs);
            break;
        }
        case fin_ast_expr_type_cond: {
            fin_ast_cond_expr_t* cond_expr = (fin_ast_cond_expr_t*)expr;
            fin_ast_expr_destroy(mod, cond_expr->cond);
            fin_ast_expr_destroy(mod, cond_expr->true_expr);
            if (cond_expr->false_expr)
                fin_ast_expr_destroy(mod, cond_expr->false_expr);
            break;
        }
        case fin_ast_expr_type_arg: {
            fin_ast_arg_expr_t* arg_expr = (fin_ast_arg_expr_t*)expr;
            fin_ast_expr_destroy(mod, &arg_expr->next->base);
            fin_ast_expr_destroy(mod, arg_expr->expr);
            break;
        }
        case fin_ast_expr_type_invoke: {
            fin_ast_invoke_expr_t* invoke_expr = (fin_ast_invoke_expr_t*)expr;
            fin_ast_expr_destroy(mod, invoke_expr->id);
            fin_ast_expr_destroy(mod, &invoke_expr->args->base);
            break;
        }
        case fin_ast_expr_type_init: {
            fin_ast_init_expr_t* init_expr = (fin_ast_init_expr_t*)expr;
            fin_ast_expr_destroy(mod, &init_expr->args->base);
            break;
        }
        case fin_ast_expr_type_assign: {
            fin_ast_assign_expr_t* assign_expr = (fin_ast_assign_expr_t*)expr;
            fin_ast_expr_destroy(mod, assign_expr->lhs);
            fin_ast_expr_destroy(mod, assign_expr->rhs);
            break;
        }
    }
    mod->ctx->alloc(expr, 0);
}

static void fin_ast_stmt_destroy(fin_ast_module_t* mod, fin_ast_stmt_t* stmt) {
    if (!stmt)
        return;
    fin_ast_stmt_destroy(mod, stmt->next);
    switch (stmt->type) {
        case fin_ast_stmt_type_expr: {
            fin_ast_expr_stmt_t* expr_stmt = (fin_ast_expr_stmt_t*)stmt;
            fin_ast_expr_destroy(mod, expr_stmt->expr);
            break;
        }
        case fin_ast_stmt_type_ret: {
            fin_ast_ret_stmt_t* ret_stmt = (fin_ast_ret_stmt_t*)stmt;
            fin_ast_expr_destroy(mod, ret_stmt->expr);
            break;
        }
        case fin_ast_stmt_type_if: {
            break;
        }
        case fin_ast_stmt_type_while: {
            fin_ast_while_stmt_t* while_stmt = (fin_ast_while_stmt_t*)stmt;
            fin_ast_expr_destroy(mod, while_stmt->cond);
            fin_ast_stmt_destroy(mod, while_stmt->stmt);
            break;
        }
        case fin_ast_stmt_type_decl: {
            fin_ast_decl_stmt_t* decl_stmt = (fin_ast_decl_stmt_t*)stmt;
            fin_ast_type_ref_destroy(mod, decl_stmt->type);
            fin_str_destroy(mod->ctx, decl_stmt->name);
            fin_ast_expr_destroy(mod, decl_stmt->init);
            break;
        }
        case fin_ast_stmt_type_block: {
            fin_ast_block_stmt_t* block_stmt = (fin_ast_block_stmt_t*)stmt;
            fin_ast_stmt_destroy(mod, block_stmt->stmts);
            break;
        }
    }
    mod->ctx->alloc(stmt, 0);
}

static void fin_ast_param_destroy(fin_ast_module_t* mod, fin_ast_param_t* param) {
    if (!param)
        return;
    fin_ast_param_destroy(mod, param->next);
    fin_ast_type_ref_destroy(mod, param->type);
    fin_str_destroy(mod->ctx, param->name);
    mod->ctx->alloc(param, 0);
}

static void fin_ast_func_destroy(fin_ast_module_t* mod, fin_ast_func_t* func) {
    if (!func)
        return;
    fin_ast_func_destroy(mod, func->next);
    fin_str_destroy(mod->ctx, func->name);
    fin_ast_param_destroy(mod, func->params);
    fin_ast_stmt_destroy(mod, &func->block->base);
    fin_ast_type_ref_destroy(mod, func->ret);
    mod->ctx->alloc(func, 0);
}

static void fin_ast_field_destroy(fin_ast_module_t* mod, fin_ast_field_t* field) {
    if (!field)
        return;
    fin_ast_field_destroy(mod, field->next);
    fin_str_destroy(mod->ctx, field->name);
    fin_ast_type_ref_destroy(mod, field->type);
    mod->ctx->alloc(field, 0);
}

static void fin_ast_type_destroy(fin_ast_module_t* mod, fin_ast_type_t* type) {
    if (!type)
        return;
    fin_ast_type_destroy(mod, type->next);
    fin_str_destroy(mod->ctx, type->name);
    fin_ast_field_destroy(mod, type->fields);
    mod->ctx->alloc(type, 0);
}

void fin_ast_destroy(fin_ast_module_t* mod) {
    fin_ast_func_destroy(mod, mod->funcs);
    fin_ast_type_destroy(mod, mod->types);
    fin_str_destroy(mod->ctx, mod->name);
    mod->ctx->alloc(mod, 0);
}

