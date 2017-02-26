/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include <fin/fin.h>

int main(int argc, const char* argv[]) {
    fin_ctx_t* ctx = fin_ctx_create_default();
    if (argc == 1)
        fin_ctx_eval_str(ctx, "void Main() { io.WriteLine(\"Hello, world!\"); }");
    else
        fin_ctx_eval_file(ctx, argv[1]);
    fin_ctx_destroy(ctx);
}

