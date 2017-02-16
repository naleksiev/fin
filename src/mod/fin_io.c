/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_io.h"
#include "../fin_mod.h"
#include <stdio.h>

static void fin_io_write(fin_ctx* ctx, const fin_val* args, fin_val* ret) {
    printf("%s", fin_str_cstr(args[0].s));
}

static void fin_io_write_line(fin_ctx* ctx, const fin_val* args, fin_val* ret) {
    printf("%s\n", fin_str_cstr(args[0].s));
}

static void fin_io_file_write(fin_ctx* ctx, const fin_val* args, fin_val* ret) {
    const char* file = fin_str_cstr(args[0].s);
    if (!file)
        return;
    FILE* fp = fopen(file, "wb");
    if (!fp)
        return;
    fputs(fin_str_cstr(args[1].s), fp);
    fclose(fp);
}

void fin_io_register(fin_ctx* ctx) {
    fin_mod_func_desc descs[] = {
        { "void Write(string)", &fin_io_write },
        { "void WriteLine(string)", &fin_io_write_line },
        { "void FileWrite(string,string)", &fin_io_file_write },
    };

    fin_mod_create(ctx, "io", descs, FIN_COUNT_OF(descs));
}

