/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_MOD_H
#define FIN_MOD_H

#include <fin/fin.h>

typedef struct fin_ast_module fin_ast_module;
typedef struct fin_ctx        fin_ctx;
typedef struct fin_vm         fin_vm;
typedef struct fin_mod        fin_mod;
typedef struct fin_mod_type   fin_mod_type;

typedef struct fin_mod_func {
    fin_mod*    mod;
    fin_str*    ret_type;
    fin_str*    sign;
    void        (*func)(fin_ctx* ctx, const fin_val* args, fin_val* res);
    bool        is_native;
    uint8_t*    code;
    int32_t     code_length;
    uint8_t     args;
    uint8_t     locals;
} fin_mod_func;

typedef struct fin_mod_func_desc {
    char* sign;
    void  (*func)(fin_ctx* ctx, const fin_val*, fin_val* res);
} fin_mod_func_desc;

typedef struct fin_mod_func_bind {
    fin_str*      sign;
    fin_mod_func* func;
} fin_mod_func_bind;

typedef struct fin_mod {
    fin_str*           name;
    fin_mod_type*      types;
    fin_mod_func*      funcs;
    fin_val*           consts;
    fin_mod_func_bind* binds;
    int32_t            types_count;
    int32_t            funcs_count;
    int32_t            consts_count;
    int32_t            binds_count;
    fin_mod_func*      entry;
    struct fin_mod*    next;
} fin_mod;

fin_mod* fin_mod_create(fin_ctx* ctx, const char* name, fin_mod_func_desc* descs, int32_t binds_count);
fin_mod* fin_mod_compile(fin_ctx* ctx, const char* cstr);
void    fin_mod_destroy(fin_ctx* ctx, fin_mod* mod);

#endif //#ifndef FIN_MOD_H

