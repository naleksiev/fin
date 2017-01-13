/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_LEX_H
#define FIN_LEX_H

#include "fin_common.h"
#include "fin_str.h"

typedef enum fin_lex_type {
    fin_lex_type_bool,
    fin_lex_type_int,
    fin_lex_type_float,
    fin_lex_type_string,
    fin_lex_type_name,

    fin_lex_type_dot,
    fin_lex_type_comma,
    fin_lex_type_question,
    fin_lex_type_colon,
    fin_lex_type_semicolon,
    fin_lex_type_caret,
    fin_lex_type_tilde,
    fin_lex_type_amp,
    fin_lex_type_eq,
    fin_lex_type_bang,
    fin_lex_type_plus,
    fin_lex_type_minus,
    fin_lex_type_star,
    fin_lex_type_slash,
    fin_lex_type_pipe,
    fin_lex_type_percent,
    fin_lex_type_plus_plus,
    fin_lex_type_minus_minus,
    fin_lex_type_pipe_pipe,
    fin_lex_type_amp_amp,
    fin_lex_type_eq_eq,
    fin_lex_type_bang_eq,
    fin_lex_type_plus_eq,
    fin_lex_type_minus_eq,
    fin_lex_type_star_eq,
    fin_lex_type_slash_eq,
    fin_lex_type_percent_eq,
    fin_lex_type_amp_eq,
    fin_lex_type_pipe_eq,
    fin_lex_type_caret_eq,
    fin_lex_type_lt,
    fin_lex_type_lt_lt,
    fin_lex_type_lt_eq,
    fin_lex_type_lt_lt_eq,
    fin_lex_type_gt,
    fin_lex_type_gt_gt,
    fin_lex_type_gt_eq,
    fin_lex_type_gt_gt_eq,
    fin_lex_type_l_paren,
    fin_lex_type_r_paren,
    fin_lex_type_l_bracket,
    fin_lex_type_r_bracket,
    fin_lex_type_l_brace,
    fin_lex_type_r_brace,

    fin_lex_type_void,
    fin_lex_type_import,
    fin_lex_type_if,
    fin_lex_type_else,
    fin_lex_type_for,
    fin_lex_type_do,
    fin_lex_type_while,
    fin_lex_type_continue,
    fin_lex_type_break,
    fin_lex_type_return,
    fin_lex_type_struct,

    fin_lex_type_error,
    fin_lex_type_eof,
} fin_lex_type;

typedef struct fin_lex fin_lex;

fin_lex*     fin_lex_create(fin_alloc alloc, fin_str_pool* pool, const char* cstr);
void         fin_lex_destroy(fin_lex* lex);
void         fin_lex_store(fin_lex* lex);
void         fin_lex_restore(fin_lex* lex);
void         fin_lex_next(fin_lex* lex);
bool         fin_lex_match(fin_lex* lex, fin_lex_type type);
fin_lex_type fin_lex_get_type(fin_lex* lex);
bool         fin_lex_consume_bool(fin_lex* lex);
int64_t      fin_lex_consume_int(fin_lex* lex);
double       fin_lex_consume_float(fin_lex* lex);
fin_str*     fin_lex_consume_string(fin_lex* lex);
fin_str*     fin_lex_consume_name(fin_lex* lex);
void         fin_lex_consume_name_to(fin_lex* lex, char* buffer);

#endif //#ifndef FIN_LEX_H

