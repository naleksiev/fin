/*
 * Copyright 2016 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef __FIN_AST_H__
#define __FIN_AST_H__

#include "fin_common.h"

typedef struct fin_ctx      fin_ctx;
typedef struct fin_str      fin_str;
typedef struct fin_str_pool fin_str_pool;
typedef struct fin_ast_expr fin_ast_expr;

typedef struct fin_ast_type_ref {
    fin_str* module;
    fin_str* name;
} fin_ast_type_ref;

typedef enum fin_ast_expr_type {
    fin_ast_expr_type_id,
    fin_ast_expr_type_member,
    fin_ast_expr_type_bool,
    fin_ast_expr_type_int,
    fin_ast_expr_type_float,
    fin_ast_expr_type_str,
    fin_ast_expr_type_unary,
    fin_ast_expr_type_binary,
    fin_ast_expr_type_cond,
    fin_ast_expr_type_arg,
    fin_ast_expr_type_invoke,
    fin_ast_expr_type_assign,
} fin_ast_expr_type;

typedef struct fin_ast_expr {
    fin_ast_expr_type type;
} fin_ast_expr;

typedef struct fin_ast_id_expr {
    fin_ast_expr base;
    fin_str*     name;
} fin_ast_id_expr;

typedef struct fin_ast_member_expr {
    fin_ast_expr base;
    fin_ast_expr* primary;
    fin_ast_expr* member;
} fin_ast_member_expr;

typedef struct fin_ast_bool_expr {
    fin_ast_expr base;
    bool         value;
} fin_ast_bool_expr;

typedef struct fin_ast_int_expr {
    fin_ast_expr base;
    int64_t      value;
} fin_ast_int_expr;

typedef struct fin_ast_float_expr {
    fin_ast_expr base;
    double       value;
} fin_ast_float_expr;

typedef struct fin_ast_str_exp {
    fin_ast_expr base;
    fin_str*     value;
} fin_ast_str_expr;

typedef enum fin_ast_unary_type {
    fin_ast_unary_type_pos,
    fin_ast_unary_type_neg,
    fin_ast_unary_type_not,
    fin_ast_unary_type_bnot,
    fin_ast_unary_type_inc,
    fin_ast_unary_type_dec,
} fin_ast_unary_type;

typedef struct fin_ast_unary_expr {
    fin_ast_expr       base;
    fin_ast_unary_type op;
    fin_ast_expr*      expr;
} fin_ast_unary_expr;

typedef enum fin_ast_binary_type {
    fin_ast_binary_type_add,
    fin_ast_binary_type_sub,
    fin_ast_binary_type_mul,
    fin_ast_binary_type_div,
    fin_ast_binary_type_mod,
    fin_ast_binary_type_shl,
    fin_ast_binary_type_shr,
    fin_ast_binary_type_ls,
    fin_ast_binary_type_leq,
    fin_ast_binary_type_gr,
    fin_ast_binary_type_geq,
    fin_ast_binary_type_eq,
    fin_ast_binary_type_neq,
    fin_ast_binary_type_band,
    fin_ast_binary_type_bor,
    fin_ast_binary_type_bxor,
    fin_ast_binary_type_and,
    fin_ast_binary_type_or,
} fin_ast_binary_type;

typedef struct fin_ast_binary_expr {
    fin_ast_expr        base;
    fin_ast_binary_type op;
    fin_ast_expr*       lhs;
    fin_ast_expr*       rhs;
} fin_ast_binary_expr;

typedef struct fin_ast_cond_expr {
    fin_ast_expr  base;
    fin_ast_expr* cond;
    fin_ast_expr* true_expr;
    fin_ast_expr* false_expr;
} fin_ast_cond_expr;

typedef struct fin_ast_ref_expr {
    fin_ast_expr base;
    fin_str* name;
} fin_ast_ref_expr;

typedef struct fin_ast_arg_expr {
    fin_ast_expr             base;
    fin_ast_expr*            expr;
    struct fin_ast_arg_expr* next;
} fin_ast_arg_expr;

typedef enum fin_ast_assign_type {
    fin_ast_assign_type_assign,
    fin_ast_assign_type_add,
    fin_ast_assign_type_sub,
    fin_ast_assign_type_mul,
    fin_ast_assign_type_div,
    fin_ast_assign_type_mod,
    fin_ast_assign_type_and,
    fin_ast_assign_type_or,
    fin_ast_assign_type_xor,
    fin_ast_assign_type_shl,
    fin_ast_assign_type_shr,
} fin_ast_assign_type;

typedef struct fin_ast_assign_expr {
    fin_ast_expr        base;
    fin_ast_expr*       lhs;
    fin_ast_expr*       rhs;
    fin_ast_assign_type op;
} fin_ast_assign_expr;

typedef struct fin_ast_invoke_expr {
    fin_ast_expr      base;
    fin_ast_id_expr*  id;
    fin_ast_arg_expr* args;
} fin_ast_invoke_expr;

typedef enum fin_ast_stmt_type {
    fin_ast_stmt_type_expr,
    fin_ast_stmt_type_ret,
    fin_ast_stmt_type_if,
    fin_ast_stmt_type_while,
    fin_ast_stmt_type_decl,
    fin_ast_stmt_type_block,
} fin_ast_stmt_type;

typedef struct fin_ast_stmt {
    fin_ast_stmt_type    type;
    struct fin_ast_stmt* next;
} fin_ast_stmt;

typedef struct fin_ast_expr_stmt {
    fin_ast_stmt  base;
    fin_ast_expr* expr;
} fin_ast_expr_stmt;

typedef struct fin_ast_ret_stmt {
    fin_ast_stmt  base;
    fin_ast_expr* expr;
} fin_ast_ret_stmt;

typedef struct fin_ast_if_stmt {
    fin_ast_stmt  base;
    fin_ast_expr* cond;
    fin_ast_stmt* true_stmt;
    fin_ast_stmt* false_stmt;
} fin_ast_if_stmt;

typedef struct fin_ast_while_stmt {
    fin_ast_stmt  base;
    fin_ast_expr* cond;
    fin_ast_stmt* stmt;
} fin_ast_while_stmt;

typedef struct fin_ast_decl_stmt {
    fin_ast_stmt      base;
    fin_ast_type_ref* type;
    fin_str*          name;
    fin_ast_expr*     init;
} fin_ast_decl_stmt;

typedef struct fin_ast_block_stmt {
    fin_ast_stmt  base;
    fin_ast_stmt* stmts;
} fin_ast_block_stmt;

typedef struct fin_ast_enum_val {
    fin_str*      name;
    fin_ast_expr* init;
} fin_ast_enum_val;

typedef struct fin_ast_enum {
    fin_str*          name;
    fin_ast_enum_val* values;
} fin_ast_enum;

typedef struct fin_ast_field {
    fin_str*              name;
    fin_ast_type_ref*     type;
    struct fin_ast_field* next;
} fin_ast_field;

typedef struct fin_ast_param {
    fin_str*          name;
    fin_ast_type_ref*     type;
    struct fin_ast_param* next;
} fin_ast_param;

typedef struct fin_ast_func {
    fin_str*             name;
    fin_ast_type_ref*    ret;
    fin_ast_param*       params;
    fin_ast_block_stmt*  block;
    struct fin_ast_func* next;
} fin_ast_func;

typedef struct fin_ast_type {
    fin_str*             name;
    fin_ast_field*       fields;
    struct fin_ast_type* next;
} fin_ast_type;

typedef struct fin_ast_module {
    fin_str*      name;
    fin_ast_type* types;
    fin_ast_func* funcs;
} fin_ast_module;

fin_ast_module* fin_ast_parse(fin_alloc alloc, fin_str_pool* pool, const char* str);
void            fin_ast_destroy(fin_ast_module* module);

#endif // #ifndef __FIN_AST_H__
