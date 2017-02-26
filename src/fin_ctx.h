/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_CTX_H
#define FIN_CTX_H

#include <fin/fin.h>

typedef struct fin_mod_t      fin_mod_t;
typedef struct fin_str_pool_t fin_str_pool_t;

typedef struct fin_ctx_t {
    fin_alloc       alloc;
    fin_str_pool_t* pool;
    fin_mod_t*      mod;
} fin_ctx_t;

fin_ctx_t* fin_ctx_create(fin_alloc alloc);
fin_ctx_t* fin_ctx_create_default();
void       fin_ctx_destroy(fin_ctx_t* ctx);
void       fin_ctx_eval_str(fin_ctx_t* ctx, const char* cstr);
void       fin_ctx_eval_file(fin_ctx_t* ctx, const char* path);

#endif //#ifndef FIN_CTX_H

