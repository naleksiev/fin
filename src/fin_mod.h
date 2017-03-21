/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_MOD_H
#define FIN_MOD_H

#include <fin/fin.h>

typedef struct fin_ast_module_t fin_ast_module_t;
typedef struct fin_ctx_t        fin_ctx_t;
typedef struct fin_vm_t         fin_vm_t;
typedef struct fin_mod_t        fin_mod_t;
typedef struct fin_mod_type_t   fin_mod_type_t;

typedef struct fin_mod_func_t {
    fin_mod_t*  mod;
    fin_str_t*  ret_type;
    fin_str_t*  sign;
    void        (*func)(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* res);
    bool        is_native;
    uint8_t*    code;
    int32_t     code_length;
    uint8_t     args;
    uint8_t     locals;
} fin_mod_func_t;

typedef struct fin_mod_func_desc_t {
    char* sign;
    void  (*func)(fin_ctx_t* ctx, const fin_val_t*, fin_val_t* res);
} fin_mod_func_desc_t;

typedef struct fin_mod_func_bind_t {
    fin_str_t*      sign;
    fin_mod_func_t* func;
} fin_mod_func_bind_t;

typedef struct fin_mod_t {
    fin_str_t*           name;
    fin_mod_type_t*      types;
    fin_mod_func_t*      funcs;
    fin_val_t*           consts;
    fin_mod_func_bind_t* binds;
    int32_t              types_count;
    int32_t              funcs_count;
    int32_t              consts_count;
    int32_t              binds_count;
    fin_mod_func_t*      entry;
    struct fin_mod_t*    next;
} fin_mod_t;

fin_mod_t* fin_mod_create(fin_ctx_t* ctx, const char* name, fin_mod_func_desc_t* descs, int32_t binds_count);
fin_mod_t* fin_mod_compile(fin_ctx_t* ctx, const char* cstr);
void       fin_mod_destroy(fin_ctx_t* ctx, fin_mod_t* mod);

#endif //#ifndef FIN_MOD_H
