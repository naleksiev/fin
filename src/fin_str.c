/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_str.h"
#include "fin_ctx.h"
#include <string.h>

typedef struct fin_str_t {
    int32_t ref;
    int32_t len;
    int32_t slot;
    char    cstr[1];
} fin_str_t;

typedef struct fin_str_entry_t {
    int32_t    hash;
    fin_str_t* str;
} fin_str_entry_t;

typedef struct fin_str_pool_t {
    fin_str_entry_t* entries;
    int32_t          capacity;
    int32_t          count;
    fin_alloc        alloc;
} fin_str_pool_t;

static int32_t fin_str_hash(const char* cstr, int32_t* len) {
    const int32_t prime = 16777619;
    const int32_t basis = 2166136261;
    int32_t hash = basis;
    const char* iter = cstr;
    const char* term = iter + *len;
    while (*iter && iter != term) {
        hash ^= *iter++;
        hash *= prime;
    }
    *len = (int32_t)(iter - cstr);
    return hash;
}

static void fin_str_insert(fin_str_pool_t* pool, int32_t hash, fin_str_t* str) {
    int32_t slot = (uint32_t)hash % pool->capacity;
    int32_t end_slot = slot;
    do {
        fin_str_entry_t* entry = &pool->entries[slot];
        if (entry->hash == 0 || entry->str == NULL) {
            entry->hash = hash;
            entry->str = str;
            str->slot = slot;
            pool->count++;
            break;
        }
        slot = (slot + 1) % pool->capacity;
    } while (slot != end_slot);
}

void fin_str_resize(fin_str_pool_t* pool, int32_t capacity) {
    fin_str_pool_t new_pool;
    new_pool.entries = (fin_str_entry_t*)pool->alloc(NULL, sizeof(fin_str_entry_t) * capacity);
    new_pool.capacity = capacity;
    new_pool.count = 0;
    new_pool.alloc = pool->alloc;
    for (int32_t i=0; i<capacity; i++) {
        new_pool.entries[i].hash = 0;
        new_pool.entries[i].str = NULL;
    }
    for (int32_t i=0; i<pool->capacity; i++)
        if (pool->entries[i].str != 0)
            fin_str_insert(&new_pool, pool->entries[i].hash, pool->entries[i].str);
    pool->alloc(pool->entries, 0);
    *pool = new_pool;
}

fin_str_pool_t* fin_str_pool_create(fin_alloc alloc) {
    fin_str_pool_t* pool = (fin_str_pool_t*)alloc(NULL, sizeof(fin_str_pool_t));
    pool->entries = NULL;
    pool->capacity = 0;
    pool->count = 0;
    pool->alloc = alloc;
    return pool;
}

void fin_str_pool_destroy(fin_str_pool_t* pool) {
    if (pool->entries)
        pool->alloc(pool->entries, 0);
    pool->alloc(pool, 0);
}

fin_str_t* fin_str_create(fin_ctx_t* ctx, const char* cstr, int32_t len) {
    fin_str_pool_t* pool = ctx->pool;
    if (cstr == NULL || cstr[0] == '\0' || len == 0)
        return NULL;

    int32_t hash = fin_str_hash(cstr, &len);
    if (pool->capacity) {
        int32_t slot = (uint32_t)hash % pool->capacity;
        int32_t end_slot = slot;
        do {
            fin_str_entry_t* entry = &pool->entries[slot];
            if (entry->hash == 0 && entry->str == NULL)
                break;
            if (entry->hash == hash && entry->str->len == len) {
                if (strncmp(entry->str->cstr, cstr, len) == 0) {
                    entry->str->ref++;
                    return entry->str;
                }
            }
            slot = (slot + 1) % pool->capacity;
        } while (slot != end_slot);
    }
    if (pool->capacity == 0)
        fin_str_resize(pool, 16);
    else if (pool->count + 1 > pool->capacity * 3 / 4)
        fin_str_resize(pool, pool->capacity * 2);
    fin_str_t* str = (fin_str_t*)pool->alloc(NULL, sizeof(fin_str_t) + len);
    str->ref = 1;
    str->len = len;
    strncpy(str->cstr, cstr, len);
    str->cstr[len] = '\0';
    fin_str_insert(pool, hash, str);
    return str;
}

void fin_str_destroy(fin_ctx_t* ctx, fin_str_t* str) {
    fin_str_pool_t* pool = ctx->pool;
    if (--str->ref == 0) {
        pool->entries[str->slot].str = NULL;
        pool->entries[str->slot].hash = 1;
        pool->alloc(str, 0);
    }
}

fin_str_t* fin_str_clone(fin_str_t* str) {
    str->ref++;
    return str;
}

fin_str_t* fin_str_concat(fin_ctx_t* ctx, fin_str_t* a, fin_str_t* b) {
    char* buffer = ctx->alloc(NULL, a->len + b->len + 1);
    strncpy(buffer, a->cstr, a->len);
    strncpy(buffer + a->len, b->cstr, b->len);
    buffer[a->len + b->len] = '\0';
    fin_str_t* result = fin_str_create(ctx, buffer, a->len + b->len);
    ctx->alloc(buffer, 0);
    return result;
}

fin_str_t* fin_str_join(fin_ctx_t* ctx, fin_str_t* arr, int32_t count) {
    int32_t length = 0;
    for (int32_t i=0; i<count; i++)
        length += fin_str_len(arr + i);
    char* buffer = ctx->alloc(NULL, length + 1);
    length = 0;
    for (int32_t i=0; i<count; i++) {
        if (!(arr+i))
            continue;
        strncpy(buffer, (arr+i)->cstr, (arr+i)->len);
        length += (arr+i)->len;
    }
    fin_str_t* result = fin_str_create(ctx, buffer, length);
    ctx->alloc(buffer, 0);
    return result;
}

const char* fin_str_cstr(fin_str_t* str) {
    return str ? str->cstr : "";
}

int32_t fin_str_len(fin_str_t* str) {
    return str ? str->len : 0;
}
