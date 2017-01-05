/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_lex.h"
#include "fin_val.h"
#include "fin_str.h"

static const int32_t fin_lex_max_cache = 4;

static const struct {
    const char* token;
    fin_lex_type type;
} fin_lex_keywords [] = {
    { "void", fin_lex_type_void },
    { "import", fin_lex_type_import },
    { "if", fin_lex_type_if },
    { "else", fin_lex_type_else },
    { "for", fin_lex_type_for },
    { "do", fin_lex_type_do },
    { "while", fin_lex_type_while },
    { "continue", fin_lex_type_continue },
    { "break", fin_lex_type_break },
    { "return", fin_lex_type_return },
    { "struct", fin_lex_type_struct },
};

typedef struct fin_lex_token {
    fin_lex_type type;
    const char*  cstr;
    int32_t      len;
    int32_t      line;
} fin_lex_token;

typedef struct fin_lex_state {
    const char*   cstr;
    int32_t       line;
    fin_lex_token token;
} fin_lex_state;

typedef struct fin_lex {
    fin_alloc     alloc;
    fin_str_pool* pool;
    const char*   cstr;
    int32_t       line;
    fin_lex_token token;
    fin_lex_state state;
} fin_lex_;

static bool fin_lex_is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool fin_lex_is_name(char c) {
    return c == '_' ||
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z');
}

static bool fin_lex_match_char(fin_lex_* lex, char c) {
    if (*lex->cstr == c) {
        lex->cstr++;
        return true;
    }
    return false;
}

static void fin_lex_skip_line_comment(fin_lex_* lex) {
    while (*lex->cstr != '\n' && *lex->cstr != '\0')
        lex->cstr++;
}

static void fin_lex_skip_block_comment(fin_lex_* lex) {
    while (lex->cstr[0] != '\0') {
        if (lex->cstr[0] == '*' && lex->cstr[1] == '/') {
            lex->cstr += 2;
            return;
        }
        if (lex->cstr[0] == '\n')
            lex->line++;
        lex->cstr++;
    }
}

static fin_lex_token fin_lex_create_token(fin_lex_* lex, fin_lex_type type) {
    fin_lex_token token;
    token.type = type;
    token.cstr = NULL;
    token.len = 0;
    token.line = lex->line;
    return token;
}

static fin_lex_token fin_lex_create_number_token(fin_lex_* lex) {
    fin_lex_token token;
    token.cstr = lex->cstr - 1;
    token.line = lex->line;

    bool is_float = false;
    while (fin_lex_is_digit(*lex->cstr))
        lex->cstr++;
    if (fin_lex_match_char(lex, '.'))
        is_float = true;
    while (fin_lex_is_digit(*lex->cstr))
        lex->cstr++;

    token.type = is_float ? fin_lex_type_float : fin_lex_type_int;
    return token;
}

static fin_lex_token fin_lex_create_string_token(fin_lex_* lex) {
    fin_lex_token token;
    token.type = fin_lex_type_string;
    token.cstr = lex->cstr;
    token.line = lex->line;
    token.len = 0;

    char c;
    while ((c = *lex->cstr++) != '\0') {
        if (c == '"') {
            token.len = (int32_t)(lex->cstr - token.cstr - 1);
            return token;
        }
        if (c == '\\')
            lex->cstr++;
    }
    //fin_lex_create_error_token(lex, "Unterminated string.");
    return fin_lex_create_token(lex, fin_lex_type_error);
}

static fin_lex_token fin_lex_create_name_token(fin_lex_* lex) {
    fin_lex_token token;
    token.cstr = lex->cstr - 1;
    token.line = lex->line;

    while (fin_lex_is_name(*lex->cstr) || fin_lex_is_digit(*lex->cstr))
        lex->cstr++;
    token.len = (int32_t)(lex->cstr - token.cstr);

    for (int32_t i=0; i<FIN_COUNT_OF(fin_lex_keywords); i++) {
        if (strncmp(fin_lex_keywords[i].token, token.cstr, token.len) == 0) {
            token.type = fin_lex_keywords[i].type;
            return token;
        }
    }

    token.type = fin_lex_type_name;
    return token;
}

static void fin_lex_skip_whitespaces(fin_lex_* lex) {
    while (*lex->cstr == ' ' || *lex->cstr == '\t' || *lex->cstr == '\r')
        lex->cstr++;
};

static fin_lex_token fin_lex_next_token(fin_lex_* lex) {
    if (*lex->cstr == '\0')
        return fin_lex_create_token(lex, fin_lex_type_eof);

    while (true) {
        char c = *lex->cstr++;
        switch (c) {
            case '\0': return fin_lex_create_token(lex, fin_lex_type_eof);
            case '(': return fin_lex_create_token(lex, fin_lex_type_l_paren);
            case ')': return fin_lex_create_token(lex, fin_lex_type_r_paren);
            case '[': return fin_lex_create_token(lex, fin_lex_type_l_bracket);
            case ']': return fin_lex_create_token(lex, fin_lex_type_r_bracket);
            case '{': return fin_lex_create_token(lex, fin_lex_type_l_brace);
            case '}': return fin_lex_create_token(lex, fin_lex_type_r_brace);
            case '.': return fin_lex_create_token(lex, fin_lex_type_dot);
            case ',': return fin_lex_create_token(lex, fin_lex_type_comma);
            case '?': return fin_lex_create_token(lex, fin_lex_type_question);
            case ':': return fin_lex_create_token(lex, fin_lex_type_colon);
            case ';': return fin_lex_create_token(lex, fin_lex_type_semicolon);
            case '~': return fin_lex_create_token(lex, fin_lex_type_tilde);
            case '=': return fin_lex_create_token(lex, fin_lex_match_char(lex, '=') ? fin_lex_type_eq_eq : fin_lex_type_eq);
            case '!': return fin_lex_create_token(lex, fin_lex_match_char(lex, '=') ? fin_lex_type_bang_eq : fin_lex_type_bang);
            case '*': return fin_lex_create_token(lex, fin_lex_match_char(lex, '=') ? fin_lex_type_star_eq : fin_lex_type_star);
            case '%': return fin_lex_create_token(lex, fin_lex_match_char(lex, '=') ? fin_lex_type_percent_eq : fin_lex_type_percent);
            case '^': return fin_lex_create_token(lex, fin_lex_match_char(lex, '=') ? fin_lex_type_caret_eq : fin_lex_type_caret);
            case '+':
                return fin_lex_create_token(lex, fin_lex_match_char(lex, '+') ?
                    fin_lex_type_plus_plus :
                    fin_lex_match_char(lex, '=') ? fin_lex_type_plus_eq : fin_lex_type_plus);
            case '-':
                return fin_lex_create_token(lex, fin_lex_match_char(lex, '-') ?
                    fin_lex_type_minus_minus :
                    fin_lex_match_char(lex, '=') ? fin_lex_type_minus_eq : fin_lex_type_minus);
            case '|':
                return fin_lex_create_token(lex, fin_lex_match_char(lex, '|') ?
                    fin_lex_type_pipe_pipe :
                    fin_lex_match_char(lex, '=') ? fin_lex_type_pipe_eq : fin_lex_type_pipe);
            case '&':
                return fin_lex_create_token(lex, fin_lex_match_char(lex, '&') ?
                    fin_lex_type_amp_amp :
                    fin_lex_match_char(lex, '=') ? fin_lex_type_amp_eq : fin_lex_type_amp);
            case '<':
                return fin_lex_create_token(lex, fin_lex_match_char(lex, '<') ?
                    (fin_lex_match_char(lex, '=') ? fin_lex_type_lt_lt_eq : fin_lex_type_lt_lt) :
                    (fin_lex_match_char(lex, '=') ? fin_lex_type_lt_eq : fin_lex_type_lt));
            case '>':
                return fin_lex_create_token(lex, fin_lex_match_char(lex, '>') ?
                    (fin_lex_match_char(lex, '=') ? fin_lex_type_gt_gt_eq : fin_lex_type_gt_gt) :
                    (fin_lex_match_char(lex, '=') ? fin_lex_type_gt_eq : fin_lex_type_gt));
            case '/':
                if (fin_lex_match_char(lex, '/')) { fin_lex_skip_line_comment(lex); break; }
                if (fin_lex_match_char(lex, '*')) { fin_lex_skip_block_comment(lex); break; }
                return fin_lex_create_token(lex, fin_lex_match_char(lex, '=') ? fin_lex_type_slash_eq : fin_lex_type_slash);
            case ' ':
            case '\t':
            case '\r':
                fin_lex_skip_whitespaces(lex);
                break;
            case '\n':
                lex->line++;
                break;
            case '"':
                return fin_lex_create_string_token(lex);
            default:
                if (fin_lex_is_name(c))
                    return fin_lex_create_name_token(lex);
                if (fin_lex_is_digit(c))
                    return fin_lex_create_number_token(lex);
                //fin_lex_create_error_token(lex, "Unexpected character");
                return fin_lex_create_token(lex, fin_lex_type_error);
        }
    };
}

fin_lex_* fin_lex_create(fin_alloc alloc, fin_str_pool* pool, const char* cstr) {
    fin_lex_* lex = (fin_lex_*)alloc(NULL, sizeof(fin_lex_));
    lex->alloc = alloc;
    lex->pool = pool; //REMOVE
    lex->cstr = cstr;
    lex->line = 1;
    fin_lex_next(lex);
    return lex;
}

void fin_lex_destroy(fin_lex_* lex) {
    lex->alloc(lex, 0);
}

void fin_lex_store(fin_lex* lex) {
    lex->state.cstr = lex->cstr;
    lex->state.line = lex->line;
    lex->state.token = lex->token;
}

void fin_lex_restore(fin_lex* lex) {
    lex->cstr = lex->state.cstr;
    lex->line = lex->state.line;
    lex->token = lex->state.token;
}

void fin_lex_next(fin_lex_* lex) {
    lex->token = fin_lex_next_token(lex);
}

bool fin_lex_match(fin_lex_* lex, fin_lex_type type) {
    if (lex->token.type == type) {
        fin_lex_next(lex);
        return true;
    }
    return false;
}

fin_lex_type fin_lex_get_type(fin_lex_* lex) {
    return lex->token.type;
}

bool fin_lex_consume_bool(fin_lex_* lex) {
    assert(0);
    return false;
}

int64_t fin_lex_consume_int(fin_lex_* lex) {
    int64_t value = strtoll(lex->token.cstr, NULL, 10);
    fin_lex_next(lex);
    return value;
}

double fin_lex_consume_float(fin_lex_* lex) {
    double value = strtod(lex->token.cstr, NULL);
    fin_lex_next(lex);
    return value;
}

fin_str* fin_lex_consume_string(fin_lex_* lex) {
//    char buffer[1024];
//    assert(lex->token.len < FIN_COUNT_OF(buffer));
    fin_str* value = fin_str_create(lex->pool, lex->token.cstr, lex->token.len);
    fin_lex_next(lex);
    return value;
}

fin_str* fin_lex_consume_name(fin_lex_* lex) {
    fin_str* value = fin_str_create(lex->pool, lex->token.cstr, lex->token.len);
    fin_lex_next(lex);
    return value;
}

void fin_lex_consume_name_to(fin_lex* lex, char* buffer) {
    strncpy(buffer, lex->token.cstr, lex->token.len);
    buffer[lex->token.len] = '\0';
    fin_lex_next(lex);
}

