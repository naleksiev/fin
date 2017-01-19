/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_io.h"
#include "../fin_mod.h"
#include <stdio.h>

static void fin_io_write(fin_ctx* ctx, fin_val* args) {
    printf("%s", fin_str_cstr(args[0].s));
}

static void fin_io_write_line(fin_ctx* ctx, fin_val* args) {
    printf("%s\n", fin_str_cstr(args[0].s));
}

static void fin_io_write_line_int(fin_ctx* ctx, fin_val* args) {
    printf("%d\n", (int32_t)args[0].i);
}

static void fin_io_write_line_float(fin_ctx* ctx, fin_val* args) {
    printf("%f\n", args[0].f);
}

void fin_io_register(fin_ctx* ctx) {
    fin_mod_func_desc descs[] = {
        { "void Write(string)", &fin_io_write },
        { "void WriteLine(string)", &fin_io_write_line },
        { "void WriteLine(int)", &fin_io_write_line_int},
        { "void WriteLine(float)", &fin_io_write_line_float},
    };

    fin_mod_create(ctx, "io", descs, FIN_COUNT_OF(descs));
}

