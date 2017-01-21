/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_H
#define FIN_H

#if defined(_MSC_VER)
#   define FIN_CONFIG_COMPUTED_GOTO 0
#   define _CRT_SECURE_NO_WARNINGS
#   define inline __forceinline
#endif

#ifndef FIN_CONFIG_COMPUTED_GOTO
#   define FIN_CONFIG_COMPUTED_GOTO 1
#endif

#ifndef NULL
#   define NULL ((void*)0)
#endif

#define FIN_COUNT_OF(x) (sizeof(x) / sizeof((x)[0]))

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// alloc:   ptr == NULL, size > 0
// realloc: ptr != NULL, size > 0
// free:    ptr != NULL, size == 0
typedef void* (*fin_alloc)(void* ptr, unsigned int size);

typedef struct fin_str fin_str;
typedef struct fin_obj fin_obj;
typedef struct fin_ctx fin_ctx;

typedef union fin_val {
    bool            b;
    int64_t         i;
    double          f;
    struct fin_str* s;
    struct fin_obj* o;
} fin_val;

fin_str*    fin_str_create(fin_ctx* ctx, const char* str, int32_t len);
void        fin_str_destroy(fin_ctx* ctx, fin_str* str);
fin_str*    fin_str_clone(fin_str* str);
fin_str*    fin_str_concat(fin_ctx* ctx, fin_str* a, fin_str* b);
const char* fin_str_cstr(fin_str* str);
int32_t     fin_str_len(fin_str* str);

fin_ctx* fin_ctx_create_default();
fin_ctx* fin_ctx_create(fin_alloc alloc);
void     fin_ctx_destroy(fin_ctx* ctx);
void     fin_ctx_eval_str(fin_ctx* ctx, const char* cstr);
void     fin_ctx_eval_file(fin_ctx* ctx, const char* path);

#ifdef __cplusplus
}
#endif

#endif //#ifndef FIN_H

