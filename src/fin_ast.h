/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_AST_H
#define FIN_AST_H

#include "fin_str.h"

typedef struct fin_ast_type_ref_t {
    fin_str_t* module;
    fin_str_t* name;
} fin_ast_type_ref_t;

typedef enum fin_ast_expr_type_t {
    fin_ast_expr_type_id,
    fin_ast_expr_type_bool,
    fin_ast_expr_type_int,
    fin_ast_expr_type_float,
    fin_ast_expr_type_str,
    fin_ast_expr_type_str_interp,
    fin_ast_expr_type_unary,
    fin_ast_expr_type_binary,
    fin_ast_expr_type_cond,
    fin_ast_expr_type_arg,
    fin_ast_expr_type_invoke,
    fin_ast_expr_type_init,
    fin_ast_expr_type_assign,
} fin_ast_expr_type_t;

typedef struct fin_ast_expr_t {
    fin_ast_expr_type_t type;
} fin_ast_expr_t;

typedef struct fin_ast_id_expr_t {
    fin_ast_expr_t  base;
    fin_ast_expr_t* primary;
    fin_str_t*      name;
} fin_ast_id_expr_t;

typedef struct fin_ast_bool_expr_t {
    fin_ast_expr_t base;
    bool           value;
} fin_ast_bool_expr_t;

typedef struct fin_ast_int_expr_t {
    fin_ast_expr_t base;
    int64_t        value;
} fin_ast_int_expr_t;

typedef struct fin_ast_float_expr_t {
    fin_ast_expr_t base;
    double         value;
} fin_ast_float_expr_t;

typedef struct fin_ast_str_exp_t {
    fin_ast_expr_t base;
    fin_str_t*     value;
} fin_ast_str_expr_t;

typedef struct fin_ast_str_interp_exp_t {
    fin_ast_expr_t                   base;
    fin_ast_expr_t*                  expr;
    struct fin_ast_str_interp_exp_t* next;
} fin_ast_str_interp_expr_t;

typedef enum fin_ast_unary_type_t {
    fin_ast_unary_type_pos,
    fin_ast_unary_type_neg,
    fin_ast_unary_type_not,
    fin_ast_unary_type_bnot,
    fin_ast_unary_type_inc,
    fin_ast_unary_type_dec,
} fin_ast_unary_type_t;

typedef struct fin_ast_unary_expr_t {
    fin_ast_expr_t       base;
    fin_ast_unary_type_t op;
    fin_ast_expr_t*      expr;
} fin_ast_unary_expr_t;

typedef enum fin_ast_binary_type_t {
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
} fin_ast_binary_type_t;

typedef struct fin_ast_binary_expr_t {
    fin_ast_expr_t        base;
    fin_ast_binary_type_t op;
    fin_ast_expr_t*       lhs;
    fin_ast_expr_t*       rhs;
} fin_ast_binary_expr_t;

typedef struct fin_ast_cond_expr_t {
    fin_ast_expr_t  base;
    fin_ast_expr_t* cond;
    fin_ast_expr_t* true_expr;
    fin_ast_expr_t* false_expr;
} fin_ast_cond_expr_t;

typedef struct fin_ast_ref_expr_t {
    fin_ast_expr_t base;
    fin_str_t*     name;
} fin_ast_ref_expr_t;

typedef struct fin_ast_arg_expr_t {
    fin_ast_expr_t             base;
    fin_ast_expr_t*            expr;
    struct fin_ast_arg_expr_t* next;
} fin_ast_arg_expr_t;

typedef enum fin_ast_assign_type_t {
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
} fin_ast_assign_type_t;

typedef struct fin_ast_invoke_expr_t {
    fin_ast_expr_t      base;
    fin_ast_expr_t*     id;
    fin_ast_arg_expr_t* args;
} fin_ast_invoke_expr_t;

typedef struct fin_ast_init_expr_t {
    fin_ast_expr_t      base;
    fin_ast_arg_expr_t* args;
} fin_ast_init_expr_t;

typedef struct fin_ast_assign_expr_t {
    fin_ast_expr_t        base;
    fin_ast_expr_t*       lhs;
    fin_ast_expr_t*       rhs;
    fin_ast_assign_type_t op;
} fin_ast_assign_expr_t;

typedef enum fin_ast_stmt_type_t {
    fin_ast_stmt_type_expr,
    fin_ast_stmt_type_ret,
    fin_ast_stmt_type_if,
    fin_ast_stmt_type_while,
    fin_ast_stmt_type_do,
    fin_ast_stmt_type_decl,
    fin_ast_stmt_type_block,
} fin_ast_stmt_type_t;

typedef struct fin_ast_stmt_t {
    fin_ast_stmt_type_t    type;
    struct fin_ast_stmt_t* next;
} fin_ast_stmt_t;

typedef struct fin_ast_expr_stmt_t {
    fin_ast_stmt_t  base;
    fin_ast_expr_t* expr;
} fin_ast_expr_stmt_t;

typedef struct fin_ast_ret_stmt_t {
    fin_ast_stmt_t  base;
    fin_ast_expr_t* expr;
} fin_ast_ret_stmt_t;

typedef struct fin_ast_if_stmt_t {
    fin_ast_stmt_t  base;
    fin_ast_expr_t* cond;
    fin_ast_stmt_t* true_stmt;
    fin_ast_stmt_t* false_stmt;
} fin_ast_if_stmt_t;

typedef struct fin_ast_while_stmt_t {
    fin_ast_stmt_t  base;
    fin_ast_expr_t* cond;
    fin_ast_stmt_t* stmt;
} fin_ast_while_stmt_t;

typedef struct fin_ast_do_stmt_t {
    fin_ast_stmt_t  base;
    fin_ast_stmt_t* stmt;
    fin_ast_expr_t* cond;
} fin_ast_do_stmt_t;

typedef struct fin_ast_decl_stmt_t {
    fin_ast_stmt_t      base;
    fin_ast_type_ref_t* type;
    fin_str_t*          name;
    fin_ast_expr_t*     init;
} fin_ast_decl_stmt_t;

typedef struct fin_ast_block_stmt_t {
    fin_ast_stmt_t  base;
    fin_ast_stmt_t* stmts;
} fin_ast_block_stmt_t;

typedef struct fin_ast_generic_t {
    fin_str_t*                name;
    struct fin_ast_generic_t* next;
} fin_ast_generic_t;

typedef struct fin_ast_enum_val_t {
    fin_str_t*                 name;
    fin_ast_expr_t*            expr;
    struct fin_ast_enum_val_t* next;
} fin_ast_enum_val_t;

typedef struct fin_ast_enum_t {
    fin_str_t*             name;
    fin_ast_enum_val_t*    values;
    struct fin_ast_enum_t* next;
} fin_ast_enum_t;

typedef struct fin_ast_field_t {
    fin_str_t*              name;
    fin_ast_type_ref_t*     type;
    struct fin_ast_field_t* next;
} fin_ast_field_t;

typedef struct fin_ast_param_t {
    fin_str_t*              name;
    fin_ast_type_ref_t*     type;
    struct fin_ast_param_t* next;
} fin_ast_param_t;

typedef struct fin_ast_func_t {
    fin_str_t*             name;
    fin_ast_type_ref_t*    ret;
    fin_ast_param_t*       params;
    fin_ast_block_stmt_t*  block;
    fin_ast_generic_t*     generics;
    struct fin_ast_func_t* next;
} fin_ast_func_t;

typedef struct fin_ast_type_t {
    fin_str_t*             name;
    fin_ast_field_t*       fields;
    fin_ast_generic_t*     generics;
    struct fin_ast_type_t* next;
} fin_ast_type_t;

typedef struct fin_ast_module_t {
    fin_ctx_t*      ctx;
    fin_str_t*      name;
    fin_ast_type_t* types;
    fin_ast_enum_t* enums;
    fin_ast_func_t* funcs;
} fin_ast_module_t;

fin_ast_module_t* fin_ast_parse(fin_ctx_t* ctx, const char* str);
void              fin_ast_destroy(fin_ast_module_t* mod);

#endif // #ifndef FIN_AST_H
