/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_time.h"
#include "../fin_mod.h"
#include <time.h>

static void fin_time_clock(fin_val* args) {
    args[0].f = (double)clock() / CLOCKS_PER_SEC;
}

void fin_time_register(fin_ctx* ctx) {
    fin_mod_func_desc descs[] = {
        { "float Clock()", & fin_time_clock},
    };

    fin_mod_create(ctx, "time", descs, FIN_COUNT_OF(descs));
}

