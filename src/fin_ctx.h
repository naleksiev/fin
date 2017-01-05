/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef __FIN_CTX_H__
#define __FIN_CTX_H__

#include "fin_common.h"

typedef struct fin_mod      fin_mod;
typedef struct fin_str_pool fin_str_pool;

typedef void* (*fin_ctx_alloc)(void* ptr, unsigned int size);

typedef struct fin_ctx {
    fin_ctx_alloc alloc;
    fin_mod*      mod;
    fin_str_pool* pool;
} fin_ctx;

fin_ctx* fin_ctx_create(fin_ctx_alloc alloc);
fin_ctx* fin_ctx_create_default();
void     fin_ctx_destroy(fin_ctx* ctx);
void     fin_ctx_eval_str(fin_ctx* ctx, const char* cstr);
void     fin_ctx_eval_file(fin_ctx* ctx, const char* path);

#endif //#ifndef __FIN_CTX_H__

