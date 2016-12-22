/*
 * Copyright 2016 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef __FIN_H__
#define __FIN_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct fin_ctx fin_ctx;

typedef void* (*fin_alloc)(void* ptr, unsigned int size);

fin_ctx* fin_ctx_create_default();
fin_ctx* fin_ctx_create(fin_alloc alloc);
void     fin_ctx_destroy(fin_ctx* ctx);
void     fin_ctx_eval_str(fin_ctx* ctx, const char* cstr);
void     fin_ctx_eval_file(fin_ctx* ctx, const char* path);

#ifdef __cplusplus
}
#endif

#endif //#ifndef __FIN_H__

