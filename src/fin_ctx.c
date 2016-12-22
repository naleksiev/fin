/*
 * Copyright 2016 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_ctx.h"
#include "fin_mod.h"
#include "fin_val.h"
#include "fin_vm.h"
#include "fin_io.h"
#include "fin_math.h"
#include "fin_time.h"
#include "fin_std.h"
#include "fin_str.h"

static void* fin_allocator(void* ptr, unsigned int size) {
    if (ptr) {
        if (size) {
            return realloc(ptr, size);
        }
        else {
            free(ptr);
            return NULL;
        }
    }
    else if (size) {
        return malloc(size);
    }
    return NULL;
}

fin_ctx* fin_ctx_create(fin_alloc alloc) {
    fin_ctx* ctx = (fin_ctx*)alloc(NULL, sizeof(fin_ctx));
    ctx->alloc = alloc;
    ctx->pool = fin_str_pool_create(alloc);
    ctx->mod = NULL;
    fin_io_register(ctx); // this should be optional
    fin_math_register(ctx); // this should be optional
    fin_time_register(ctx); // this should be optional
    fin_std_register(ctx); // this should be optional
    return ctx;
}

fin_ctx* fin_ctx_create_default() {
    return fin_ctx_create(&fin_allocator);
}

void fin_ctx_destroy(fin_ctx* ctx) {
    fin_str_pool_destroy(ctx->pool);
    ctx->alloc(ctx, 0);
}

void fin_ctx_eval_str(fin_ctx* ctx, const char* cstr) {
    fin_mod* mod = fin_mod_compile(ctx, cstr);
    if (mod) {
        if (mod->entry) {
            fin_vm* vm = fin_vm_create(ctx);
            fin_vm_invoke(vm, mod->entry);
            fin_vm_destroy(vm);
        }
    }
}

void fin_ctx_eval_file(fin_ctx* ctx, const char* path) {
    FILE* fp = fopen(path, "rb");
    assert(fp);

    fseek(fp, 0, SEEK_END);
    int32_t file_size = (int32_t)ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char* buffer = (char*)ctx->alloc(NULL, file_size + 1);
    assert(buffer);
    int32_t read = (int32_t)fread(buffer, 1, file_size, fp);
    buffer[file_size] = '\0';
    assert(read == file_size);
    fclose(fp);

    fin_ctx_eval_str(ctx, buffer);

    free(buffer);
}