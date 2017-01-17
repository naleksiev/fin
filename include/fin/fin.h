/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_H
#define FIN_H

#ifdef __cplusplus
extern "C" {
#endif

// alloc:   ptr == NULL, size > 0
// realloc: ptr != NULL, size > 0
// free:    ptr != NULL, size == 0
typedef void* (*fin_alloc)(void* ptr, unsigned int size);

typedef struct fin_ctx fin_ctx;

fin_ctx* fin_ctx_create_default();
fin_ctx* fin_ctx_create(fin_alloc alloc);
void     fin_ctx_destroy(fin_ctx* ctx);
void     fin_ctx_eval_str(fin_ctx* ctx, const char* cstr);
void     fin_ctx_eval_file(fin_ctx* ctx, const char* path);

#ifdef __cplusplus
}
#endif

#endif //#ifndef FIN_H

