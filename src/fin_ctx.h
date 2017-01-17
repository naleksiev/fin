/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_CTX_H
#define FIN_CTX_H

#include <fin/fin.h>

struct fin_mod;
struct fin_str_pool;

typedef struct fin_ctx {
    fin_alloc            alloc;
    struct fin_str_pool* pool;
    struct fin_mod*      mod;
} fin_ctx;

fin_ctx* fin_ctx_create(fin_alloc alloc);
fin_ctx* fin_ctx_create_default();
void     fin_ctx_destroy(fin_ctx* ctx);
void     fin_ctx_eval_str(fin_ctx* ctx, const char* cstr);
void     fin_ctx_eval_file(fin_ctx* ctx, const char* path);

#endif //#ifndef FIN_CTX_H

