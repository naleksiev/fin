/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_ast.h"
#include "fin_val.h"
#include "fin_lex.h"
#include "fin_str.h"

static fin_ast_expr* fin_ast_parse_expr(fin_alloc alloc, fin_lex* lex);
static fin_ast_stmt* fin_ast_parse_stmt(fin_alloc alloc, fin_lex* lex);

static void fin_ast_expect(fin_lex* lex, fin_lex_type type) {
    if (fin_lex_get_type(lex) == type) {
        fin_lex_next(lex);
        return;
    }
    assert(0);
}

static fin_ast_type_ref* fin_ast_parse_type_ref(fin_alloc alloc, fin_lex* lex) {
    fin_ast_type_ref* type = (fin_ast_type_ref*)alloc(NULL, sizeof(fin_ast_type_ref));
    type->module = NULL;
    type->name = fin_str_clone(fin_lex_consume_name(lex));
    if (fin_lex_match(lex, fin_lex_type_dot)) {
        type->module = type->name;
        type->name = fin_str_clone(fin_lex_consume_name(lex));
    }
    return type;
}

static fin_ast_arg_expr* fin_ast_parse_arg_expr(fin_alloc alloc, fin_lex* lex) {
    fin_ast_expr* expr = fin_ast_parse_expr(alloc, lex);
    fin_ast_arg_expr* arg_expr = (fin_ast_arg_expr*)alloc(NULL, sizeof(fin_ast_arg_expr));
    arg_expr->base.type = fin_ast_expr_type_arg;
    arg_expr->expr = expr;
    arg_expr->next = NULL;
    return arg_expr;
}

static fin_ast_expr* fin_ast_parse_id_expr(fin_alloc alloc, fin_lex* lex) {
    fin_ast_id_expr* id_expr = (fin_ast_id_expr*)alloc(NULL, sizeof(fin_ast_id_expr));
    id_expr->base.type = fin_ast_expr_type_id;
    id_expr->name = fin_str_clone(fin_lex_consume_name(lex));
    return &id_expr->base;
}

static fin_ast_expr* fin_ast_parse_member_expr(fin_alloc alloc, fin_lex* lex, fin_ast_expr* primary) {
    fin_ast_member_expr* member_expr = (fin_ast_member_expr*)alloc(NULL, sizeof(fin_ast_member_expr));
    member_expr->base.type = fin_ast_expr_type_member;
    member_expr->primary = primary;
    fin_ast_expect(lex, fin_lex_type_dot);
    member_expr->member = fin_ast_parse_expr(alloc, lex);
    return &member_expr->base;
}

static fin_ast_expr* fin_ast_parse_invoke_expr(fin_alloc alloc, fin_lex* lex, fin_ast_expr* id) {
    assert(id->type == fin_ast_expr_type_id);
    fin_ast_invoke_expr* invoke_expr = (fin_ast_invoke_expr*)alloc(NULL, sizeof(fin_ast_invoke_expr));
    invoke_expr->base.type = fin_ast_expr_type_invoke;
    invoke_expr->id = (fin_ast_id_expr*)id;
    fin_ast_expect(lex, fin_lex_type_l_paren);
    fin_ast_arg_expr** tail = &invoke_expr->args;
    *tail = NULL;
    while (!fin_lex_match(lex, fin_lex_type_r_paren)) {
        if (invoke_expr->args)
            fin_ast_expect(lex, fin_lex_type_comma);
        *tail = fin_ast_parse_arg_expr(alloc, lex);
        tail = &(*tail)->next;
    }
    return &invoke_expr->base;
}

static fin_ast_expr* fin_ast_parse_const_expr(fin_alloc alloc, fin_lex* lex) {
    if (fin_lex_get_type(lex) == fin_lex_type_bool) {
        fin_ast_bool_expr* bool_expr = (fin_ast_bool_expr*)alloc(NULL, sizeof(fin_ast_bool_expr));
        bool_expr->base.type = fin_ast_expr_type_bool;
        bool_expr->value = fin_lex_consume_bool(lex);
        return &bool_expr->base;
    }
    else if (fin_lex_get_type(lex) == fin_lex_type_int) {
        fin_ast_int_expr* int_expr = (fin_ast_int_expr*)alloc(NULL, sizeof(fin_ast_int_expr));
        int_expr->base.type = fin_ast_expr_type_int;
        int_expr->value = fin_lex_consume_int(lex);
        return &int_expr->base;
    }
    else if (fin_lex_get_type(lex) == fin_lex_type_float) {
        fin_ast_float_expr* float_expr = (fin_ast_float_expr*)alloc(NULL, sizeof(fin_ast_float_expr));
        float_expr->base.type = fin_ast_expr_type_float;
        float_expr->value = fin_lex_consume_float(lex);
        return &float_expr->base;
    }
    else if (fin_lex_get_type(lex) == fin_lex_type_string) {
        fin_ast_str_expr* str_expr = (fin_ast_str_expr*)alloc(NULL, sizeof(fin_ast_str_expr));
        str_expr->base.type = fin_ast_expr_type_str;
        str_expr->value = fin_str_clone(fin_lex_consume_string(lex));
        return &str_expr->base;
    }
    else {
        assert(0);
        return NULL;
    }
}

static fin_ast_expr* fin_ast_parse_unary_expr(fin_alloc alloc, fin_lex* lex) {
    fin_ast_unary_type op;
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
        return fin_ast_parse_const_expr(alloc, lex);
    else if (fin_lex_get_type(lex) == fin_lex_type_int)
        return fin_ast_parse_const_expr(alloc, lex);
    else if (fin_lex_get_type(lex) == fin_lex_type_float)
        return fin_ast_parse_const_expr(alloc, lex);
    else if (fin_lex_get_type(lex) == fin_lex_type_string)
        return fin_ast_parse_const_expr(alloc, lex);
    else if (fin_lex_get_type(lex) == fin_lex_type_bool)
        return fin_ast_parse_const_expr(alloc, lex);
    else {
        fin_ast_expr* id_expr = fin_ast_parse_id_expr(alloc, lex);
        return fin_lex_get_type(lex) == fin_lex_type_l_paren ? fin_ast_parse_invoke_expr(alloc, lex, id_expr) : id_expr;
    }

    fin_ast_expr* expr = fin_ast_parse_unary_expr(alloc, lex);
    fin_ast_unary_expr* un_expr = (fin_ast_unary_expr*)alloc(NULL, sizeof(fin_ast_unary_expr));
    *un_expr = (fin_ast_unary_expr){ .base = (fin_ast_expr){ .type = fin_ast_expr_type_unary }, .op = op, .expr = expr };
    return &un_expr->base;
}

static fin_ast_expr* fin_ast_parse_multiplicative_expr(fin_alloc alloc, fin_lex* lex) {
    fin_ast_expr* lhs = fin_ast_parse_unary_expr(alloc, lex);
    fin_ast_binary_type op;
    if (fin_lex_match(lex, fin_lex_type_star))
        op = fin_ast_binary_type_mul;
    else if (fin_lex_match(lex, fin_lex_type_slash))
        op = fin_ast_binary_type_div;
    else if (fin_lex_match(lex, fin_lex_type_percent))
        op = fin_ast_binary_type_mod;
    else
        return lhs;
    fin_ast_binary_expr* bin_expr = (fin_ast_binary_expr*)alloc(NULL, sizeof(fin_ast_binary_expr));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_unary_expr(alloc, lex);
    return &bin_expr->base;
}

static fin_ast_expr* fin_ast_parse_additive_expr(fin_alloc alloc, fin_lex* lex) {
    fin_ast_expr* lhs = fin_ast_parse_multiplicative_expr(alloc, lex);
    fin_ast_binary_type op;
    if (fin_lex_match(lex, fin_lex_type_plus))
        op = fin_ast_binary_type_add;
    else if (fin_lex_match(lex, fin_lex_type_minus))
        op = fin_ast_binary_type_sub;
    else
        return lhs;
    fin_ast_binary_expr* bin_expr = (fin_ast_binary_expr*)alloc(NULL, sizeof(fin_ast_binary_expr));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_multiplicative_expr(alloc, lex);
    return &bin_expr->base;
}

static fin_ast_expr* fin_ast_parse_shift_expr(fin_alloc alloc, fin_lex* lex) {
    fin_ast_expr* lhs = fin_ast_parse_additive_expr(alloc, lex);
    fin_ast_binary_type op;
    if (fin_lex_match(lex, fin_lex_type_lt_lt))
        op = fin_ast_binary_type_shl;
    else if (fin_lex_match(lex, fin_lex_type_gt_gt))
        op = fin_ast_binary_type_shr;
    else
        return lhs;
    fin_ast_binary_expr* bin_expr = (fin_ast_binary_expr*)alloc(NULL, sizeof(fin_ast_binary_expr));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_additive_expr(alloc, lex);
    return &bin_expr->base;
}

static fin_ast_expr* fin_ast_parse_relational_expr(fin_alloc alloc, fin_lex* lex) {
    fin_ast_expr* lhs = fin_ast_parse_shift_expr(alloc, lex);
    fin_ast_binary_type op;
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
    fin_ast_binary_expr* bin_expr = (fin_ast_binary_expr*)alloc(NULL, sizeof(fin_ast_binary_expr));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_shift_expr(alloc, lex);
    return &bin_expr->base;
}

static fin_ast_expr* fin_ast_parse_equality_expr(fin_alloc alloc, fin_lex* lex) {
    fin_ast_expr* lhs = fin_ast_parse_relational_expr(alloc, lex);
    fin_ast_binary_type op;
    if (fin_lex_match(lex, fin_lex_type_eq_eq))
        op = fin_ast_binary_type_eq;
    else if (fin_lex_match(lex, fin_lex_type_bang_eq))
        op = fin_ast_binary_type_neq;
    else
        return lhs;
    fin_ast_binary_expr* bin_expr = (fin_ast_binary_expr*)alloc(NULL, sizeof(fin_ast_binary_expr));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_relational_expr(alloc, lex);
    return &bin_expr->base;
}

static fin_ast_expr* fin_ast_parse_and_expr(fin_alloc alloc, fin_lex* lex) {
    fin_ast_expr* lhs = fin_ast_parse_equality_expr(alloc, lex);
    fin_ast_binary_type op;
    if (fin_lex_match(lex, fin_lex_type_amp))
        op = fin_ast_binary_type_eq;
    else
        return lhs;
    fin_ast_binary_expr* bin_expr = (fin_ast_binary_expr*)alloc(NULL, sizeof(fin_ast_binary_expr));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_equality_expr(alloc, lex);
    return &bin_expr->base;
}

static fin_ast_expr* fin_ast_parse_xor_expr(fin_alloc alloc, fin_lex* lex) {
    fin_ast_expr* lhs = fin_ast_parse_and_expr(alloc, lex);
    fin_ast_binary_type op;
    if (fin_lex_match(lex, fin_lex_type_caret))
        op = fin_ast_binary_type_bxor;
    else
        return lhs;
    fin_ast_binary_expr* bin_expr = (fin_ast_binary_expr*)alloc(NULL, sizeof(fin_ast_binary_expr));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_and_expr(alloc, lex);
    return &bin_expr->base;
}

static fin_ast_expr* fin_ast_parse_or_expr(fin_alloc alloc, fin_lex* lex) {
    fin_ast_expr* lhs = fin_ast_parse_xor_expr(alloc, lex);
    fin_ast_binary_type op;
    if (fin_lex_match(lex, fin_lex_type_pipe))
        op = fin_ast_binary_type_bor;
    else
        return lhs;
    fin_ast_binary_expr* bin_expr = (fin_ast_binary_expr*)alloc(NULL, sizeof(fin_ast_binary_expr));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_xor_expr(alloc, lex);
    return &bin_expr->base;
}

static fin_ast_expr* fin_ast_parse_cond_and_expr(fin_alloc alloc, fin_lex* lex) {
    fin_ast_expr* lhs = fin_ast_parse_or_expr(alloc, lex);
    fin_ast_binary_type op;
    if (fin_lex_match(lex, fin_lex_type_amp_amp))
        op = fin_ast_binary_type_and;
    else
        return lhs;
    fin_ast_binary_expr* bin_expr = (fin_ast_binary_expr*)alloc(NULL, sizeof(fin_ast_binary_expr));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_or_expr(alloc, lex);
    return &bin_expr->base;
}

static fin_ast_expr* fin_ast_parse_cond_or_expr(fin_alloc alloc, fin_lex* lex) {
    fin_ast_expr* lhs = fin_ast_parse_cond_and_expr(alloc, lex);
    fin_ast_binary_type op;
    if (fin_lex_match(lex, fin_lex_type_pipe_pipe))
        op = fin_ast_binary_type_or;
    else
        return lhs;
    fin_ast_binary_expr* bin_expr = (fin_ast_binary_expr*)alloc(NULL, sizeof(fin_ast_binary_expr));
    bin_expr->base.type = fin_ast_expr_type_binary;
    bin_expr->op = op;
    bin_expr->lhs = lhs;
    bin_expr->rhs = fin_ast_parse_cond_and_expr(alloc, lex);
    return &bin_expr->base;
}

static fin_ast_expr* fin_ast_parse_cond_expr(fin_alloc alloc, fin_lex* lex, fin_ast_expr* cond) {
    fin_ast_cond_expr* cond_expr = (fin_ast_cond_expr*)alloc(NULL, sizeof(fin_ast_cond_expr));
    cond_expr->base.type = fin_ast_expr_type_cond;
    cond_expr->cond = cond;
    fin_ast_expect(lex, fin_lex_type_question);
    cond_expr->true_expr = fin_ast_parse_cond_or_expr(alloc, lex);
    fin_ast_expect(lex, fin_lex_type_colon);
    cond_expr->false_expr = fin_ast_parse_cond_or_expr(alloc, lex);
    return &cond_expr->base;
}

static fin_ast_expr* fin_ast_parse_assign_expr(fin_alloc alloc, fin_lex* lex, fin_ast_expr* lhs) {
    fin_ast_assign_expr* assign_expr = (fin_ast_assign_expr*)alloc(NULL, sizeof(fin_ast_assign_expr));
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
    assign_expr->rhs = fin_ast_parse_expr(alloc, lex);
    return &assign_expr->base;
}

static fin_ast_expr* fin_ast_parse_expr(fin_alloc alloc, fin_lex* lex) {
    fin_ast_expr* expr = fin_ast_parse_cond_or_expr(alloc, lex);
    switch (fin_lex_get_type(lex)) {
        case fin_lex_type_question:
            return fin_ast_parse_cond_expr(alloc, lex, expr);
        case fin_lex_type_dot:
            return fin_ast_parse_member_expr(alloc, lex, expr);
        case fin_lex_type_l_paren:
            return fin_ast_parse_invoke_expr(alloc, lex, expr);
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
            return fin_ast_parse_assign_expr(alloc, lex, expr);
        default:
            return expr;
    }
}

static fin_ast_stmt* fin_ast_parse_if_stmt(fin_alloc alloc, fin_lex* lex) {
    fin_ast_expect(lex, fin_lex_type_if);
    fin_ast_expect(lex, fin_lex_type_l_paren);
    fin_ast_expr* cond = fin_ast_parse_expr(alloc, lex);
    fin_ast_expect(lex, fin_lex_type_r_paren);
    fin_ast_stmt* true_stmt = fin_ast_parse_stmt(alloc, lex); fin_ast_stmt* false_stmt = NULL;
    if (fin_lex_match(lex, fin_lex_type_else))
        false_stmt = fin_ast_parse_stmt(alloc, lex);
    fin_ast_if_stmt* stmt = (fin_ast_if_stmt*)alloc(NULL, sizeof(fin_ast_if_stmt));
    *stmt = (fin_ast_if_stmt){ .base = (fin_ast_stmt){ .type = fin_ast_stmt_type_if, .next = NULL }, .cond = cond, .true_stmt = true_stmt, .false_stmt = false_stmt };
    return &stmt->base;
}

static fin_ast_stmt* fin_ast_parse_while_stmt(fin_alloc alloc, fin_lex* lex) {
    fin_ast_expect(lex, fin_lex_type_while);
    fin_ast_expect(lex, fin_lex_type_l_paren);
    fin_ast_expr* cond = fin_ast_parse_expr(alloc, lex);
    fin_ast_expect(lex, fin_lex_type_r_paren);
    fin_ast_stmt* stmt = fin_ast_parse_stmt(alloc, lex);
    fin_ast_while_stmt* while_stmt = (fin_ast_while_stmt*)alloc(NULL, sizeof(fin_ast_while_stmt));
    *while_stmt = (fin_ast_while_stmt){ .base = (fin_ast_stmt){ .type = fin_ast_stmt_type_while, .next = NULL }, .cond = cond, .stmt = stmt };
    return &while_stmt->base;
}

static fin_ast_stmt* fin_ast_parse_for_stmt(fin_alloc alloc, fin_lex* lex) {
    return NULL;
}

static fin_ast_stmt* fin_ast_parse_decl_stmt(fin_alloc alloc, fin_lex* lex, fin_ast_type_ref* type) {
    fin_ast_decl_stmt* stmt = (fin_ast_decl_stmt*)alloc(NULL, sizeof(fin_ast_decl_stmt));
    stmt->base.type = fin_ast_stmt_type_decl;
    stmt->base.next = NULL;
    stmt->type = type;
    stmt->name = fin_str_clone(fin_lex_consume_name(lex));
    stmt->init = fin_lex_match(lex, fin_lex_type_eq) ? fin_ast_parse_expr(alloc, lex) : NULL;
    fin_ast_expect(lex, fin_lex_type_semicolon);
    return &stmt->base;
}

static fin_ast_stmt* fin_ast_parse_ret_stmt(fin_alloc alloc, fin_lex* lex) {
    fin_ast_ret_stmt* stmt = (fin_ast_ret_stmt*)alloc(NULL, sizeof(fin_ast_ret_stmt));
    stmt->base.type = fin_ast_stmt_type_ret;
    stmt->base.next = NULL;
    stmt->expr = NULL;
    fin_ast_expect(lex, fin_lex_type_return);
    if (fin_lex_match(lex, fin_lex_type_semicolon))
        return &stmt->base;
    stmt->expr = fin_ast_parse_expr(alloc, lex);
    fin_ast_expect(lex, fin_lex_type_semicolon);
    return &stmt->base;
}

static fin_ast_stmt* fin_ast_parse_expr_stmt(fin_alloc alloc, fin_lex* lex, fin_ast_expr* expr) {
    fin_ast_expr_stmt* stmt = (fin_ast_expr_stmt*)alloc(NULL, sizeof(fin_ast_expr_stmt));
    stmt->base.type = fin_ast_stmt_type_expr;
    stmt->base.next = NULL;
    stmt->expr = expr;
    fin_ast_expect(lex, fin_lex_type_semicolon);
    return &stmt->base;
}

static fin_ast_block_stmt* fin_ast_parse_block_stmt(fin_alloc alloc, fin_lex* lex) {
    fin_ast_block_stmt* block = (fin_ast_block_stmt*)alloc(NULL, sizeof(fin_ast_block_stmt));
    block->base.type = fin_ast_stmt_type_block;
    block->base.next = NULL;
    fin_ast_expect(lex, fin_lex_type_l_brace);
    fin_ast_stmt** tail = &block->stmts;
    *tail = NULL;
    while (!fin_lex_match(lex, fin_lex_type_r_brace)) {
        *tail = fin_ast_parse_stmt(alloc, lex);
        tail = &(*tail)->next;
    }
    return block;
}

static fin_ast_stmt* fin_ast_parse_stmt(fin_alloc alloc, fin_lex* lex) {
    switch (fin_lex_get_type(lex)) {
        case fin_lex_type_l_brace:
            return &fin_ast_parse_block_stmt(alloc, lex)->base;
        case fin_lex_type_if:
            return fin_ast_parse_if_stmt(alloc, lex);
        //case fin_lex_type_switch:
        //    return fin_ast_parse_switch_stmt(lex);
        case fin_lex_type_while:
            return fin_ast_parse_while_stmt(alloc, lex);
        case fin_lex_type_for:
            return fin_ast_parse_for_stmt(alloc, lex);
//        case fin_lex_type_break:
//            return fin_ast_parse_break_stmt(lex);
//        case fin_lex_type_continue:
//            return fin_ast_parse_continue_stmt(lex);
        case fin_lex_type_return:
            return fin_ast_parse_ret_stmt(alloc, lex);
        case fin_lex_type_name: {
            fin_lex_store(lex);
            fin_ast_type_ref* type = fin_ast_parse_type_ref(alloc, lex);
            if (fin_lex_get_type(lex) == fin_lex_type_name)
                return fin_ast_parse_decl_stmt(alloc, lex, type);
            fin_lex_restore(lex);
            fin_ast_expr* expr = fin_ast_parse_expr(alloc, lex);
            return fin_ast_parse_expr_stmt(alloc, lex, expr);
        }
        default:
            assert(0);
    }
}

static fin_ast_param* fin_ast_parse_param(fin_alloc alloc, fin_lex* lex) {
    fin_ast_param* param = (fin_ast_param*)alloc(NULL, sizeof(fin_ast_param));
    param->type = fin_ast_parse_type_ref(alloc, lex);
    param->name = fin_str_clone(fin_lex_consume_name(lex));
    param->next = NULL;
    return param;
}

static fin_ast_func* fin_ast_parse_func(fin_alloc alloc, fin_lex* lex, fin_ast_type_ref* type, fin_str* name) {
    fin_ast_func* func = (fin_ast_func*)alloc(NULL, sizeof(fin_ast_func));
    func->ret = type;
    func->name = fin_str_clone(name);
    fin_ast_expect(lex, fin_lex_type_l_paren);
    fin_ast_param** tail = &func->params;
    *tail = NULL;
    while (!fin_lex_match(lex, fin_lex_type_r_paren)) {
        if (func->params)
            fin_ast_expect(lex, fin_lex_type_comma);
        *tail = fin_ast_parse_param(alloc, lex);
        tail = &(*tail)->next;
    }
    func->block = fin_ast_parse_block_stmt(alloc, lex);
    func->next = NULL;
    return func;
}

static fin_ast_field* fin_lex_parse_field(fin_alloc alloc, fin_lex* lex) {
    fin_ast_field* field = (fin_ast_field*)alloc(NULL, sizeof(fin_ast_field));
    field->type = fin_ast_parse_type_ref(alloc, lex);
    field->name = fin_str_clone(fin_lex_consume_name(lex));
    fin_ast_expect(lex, fin_lex_type_semicolon);
    field->next = NULL;
    return field;
}

static fin_ast_type* fin_lex_parse_struct(fin_alloc alloc, fin_lex* lex) {
    fin_ast_type* type = (fin_ast_type*)alloc(NULL, sizeof(fin_ast_type));
    fin_ast_expect(lex, fin_lex_type_struct);
    type->name = fin_str_clone(fin_lex_consume_name(lex));
    fin_ast_expect(lex, fin_lex_type_l_brace);
    fin_ast_field** field_tail = &type->fields;
    *field_tail = NULL;
    while (!fin_lex_match(lex, fin_lex_type_r_brace)) {
        *field_tail = fin_lex_parse_field(alloc, lex);
        field_tail = &(*field_tail)->next;
    }
    type->next = NULL;
    return type;
}

fin_ast_module* fin_ast_parse(fin_alloc alloc, fin_str_pool* pool, const char* str) {
    fin_lex* lex = fin_lex_create(alloc, pool, str);
    fin_str* name = fin_str_create(pool, "<noname>", -1);

/*
    if (fin_lex_match(&lex, fin_lex_type_module)) {

    }
*/

    while (fin_lex_match(lex, fin_lex_type_import)) {

    }

    fin_ast_func*  funcs = NULL;
    fin_ast_func** func_tail = &funcs;

    fin_ast_type*  types = NULL;
    fin_ast_type** type_tail = &types;

    while (fin_lex_get_type(lex) != fin_lex_type_eof) {
        if (fin_lex_get_type(lex) == fin_lex_type_struct) {
            *type_tail = fin_lex_parse_struct(alloc, lex);
            type_tail = &(*type_tail)->next;
        }
        fin_ast_type_ref* type = fin_ast_parse_type_ref(alloc, lex);
        fin_str* name = fin_str_clone(fin_lex_consume_name(lex));
        if (fin_lex_get_type(lex) == fin_lex_type_l_paren) {
            *func_tail = fin_ast_parse_func(alloc, lex, type, name);
            func_tail = &(*func_tail)->next;

        }
    }

    fin_lex_destroy(lex);

    fin_ast_module* module = (fin_ast_module*)alloc(NULL, sizeof(fin_ast_module));
    module->alloc = alloc;
    module->pool = pool;
    module->name = name;
    module->types = NULL;
    module->funcs = funcs;
    return module;
}

static void fin_ast_expr_destroy(fin_ast_module* mod, fin_ast_expr* expr) {
    if (!expr)
        return;
    switch (expr->type) {
        case fin_ast_expr_type_id: {
            fin_ast_id_expr* id_expr = (fin_ast_id_expr*)expr;
            fin_str_destroy(mod->pool, id_expr->name);
            break;
        }
        case fin_ast_expr_type_member: {
            fin_ast_member_expr* member_expr = (fin_ast_member_expr*)expr;
            fin_ast_expr_destroy(mod, member_expr->primary);
            fin_ast_expr_destroy(mod, member_expr->member);
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
            fin_ast_str_expr* str_expr = (fin_ast_str_expr*)expr;
            fin_str_destroy(mod->pool, str_expr->value);
            break;
        }
        case fin_ast_expr_type_unary: {
            break;
        }
        case fin_ast_expr_type_binary: {
            break;
        }
        case fin_ast_expr_type_cond: {
            break;
        }
        case fin_ast_expr_type_arg: {
            fin_ast_arg_expr* arg_expr = (fin_ast_arg_expr*)expr;
            fin_ast_expr_destroy(mod, &arg_expr->next->base);
            fin_ast_expr_destroy(mod, arg_expr->expr);
            break;
        }
        case fin_ast_expr_type_invoke: {
            fin_ast_invoke_expr* invoke_expr = (fin_ast_invoke_expr*)expr;
            fin_ast_expr_destroy(mod, &invoke_expr->args->base);
            break;
        }
        case fin_ast_expr_type_assign: {
            break;
        }
    }
    mod->alloc(expr, 0);
}

static void fin_ast_stmt_destroy(fin_ast_module* mod, fin_ast_stmt* stmt) {
    if (stmt)
        return;
    fin_ast_stmt_destroy(mod, stmt->next);
    switch (stmt->type) {
        case fin_ast_stmt_type_expr: {
            fin_ast_expr_stmt* expr_stmt = (fin_ast_expr_stmt*)stmt;
            fin_ast_expr_destroy(mod, expr_stmt->expr);
            break;
        }
        case fin_ast_stmt_type_ret: {
            break;
        }
        case fin_ast_stmt_type_if: {
            break;
        }
        case fin_ast_stmt_type_while: {
            break;
        }
        case fin_ast_stmt_type_decl: {
            break;
        }
        case fin_ast_stmt_type_block: {
            fin_ast_block_stmt* block_stmt = (fin_ast_block_stmt*)stmt;
            fin_ast_stmt_destroy(mod, block_stmt->stmts);
            break;
        }
    }
    mod->alloc(stmt, 0);
}

static void fin_ast_param_destroy(fin_ast_module* mod, fin_ast_param* param) {
    if (!param)
        return;
    fin_ast_param_destroy(mod, param->next);
    fin_str_destroy(mod->pool, param->name);
    mod->alloc(param, 0);
}

static void fin_ast_func_destroy(fin_ast_module* mod, fin_ast_func* func) {
    if (!func)
        return;
    fin_ast_func_destroy(mod, func->next);
    fin_str_destroy(mod->pool, func->name);
    fin_ast_param_destroy(mod, func->params);
    fin_ast_stmt_destroy(mod, &func->block->base);
    mod->alloc(func, 0);
}

void fin_ast_destroy(fin_ast_module* mod) {
    fin_ast_func_destroy(mod, mod->funcs);
    fin_str_destroy(mod->pool, mod->name);
    mod->alloc(mod, 0);
}

