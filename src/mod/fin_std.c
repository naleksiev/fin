/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_std.h"
#include "../fin_mod.h"
#include <math.h>
#include <stdio.h>
#include <inttypes.h>

static void fin_std_bool_and (fin_ctx* ctx, fin_val* args)  { args[0].b = args[0].b && args[1].b; }
static void fin_std_bool_or  (fin_ctx* ctx, fin_val* args)  { args[0].b = args[0].b || args[1].b; }

static void fin_std_int_pos (fin_ctx* ctx, fin_val* args)  { args[0].i = args[0].i; }
static void fin_std_int_neg (fin_ctx* ctx, fin_val* args)  { args[0].i = -args[0].i; }
static void fin_std_int_not (fin_ctx* ctx, fin_val* args)  { args[0].i = !args[0].i; }
static void fin_std_int_bnot(fin_ctx* ctx, fin_val* args)  { args[0].i = ~args[0].i; }
static void fin_std_int_inc (fin_ctx* ctx, fin_val* args)  { args[0].i = args[0].i + 1; }
static void fin_std_int_dec (fin_ctx* ctx, fin_val* args)  { args[0].i = args[0].i - 1; }
static void fin_std_int_add (fin_ctx* ctx, fin_val* args)  { args[0].i = args[0].i  + args[1].i; }
static void fin_std_int_sub (fin_ctx* ctx, fin_val* args)  { args[0].i = args[0].i  - args[1].i; }
static void fin_std_int_mul (fin_ctx* ctx, fin_val* args)  { args[0].i = args[0].i  * args[1].i; }
static void fin_std_int_div (fin_ctx* ctx, fin_val* args)  { args[0].i = args[0].i  / args[1].i; }
static void fin_std_int_mod (fin_ctx* ctx, fin_val* args)  { args[0].i = args[0].i  % args[1].i; }
static void fin_std_int_band(fin_ctx* ctx, fin_val* args)  { args[0].i = args[0].i  & args[1].i; }
static void fin_std_int_bor (fin_ctx* ctx, fin_val* args)  { args[0].i = args[0].i  | args[1].i; }
static void fin_std_int_xor (fin_ctx* ctx, fin_val* args)  { args[0].i = args[0].i  ^ args[1].i; }
static void fin_std_int_shl (fin_ctx* ctx, fin_val* args)  { args[0].i = args[0].i << args[1].i; }
static void fin_std_int_shr (fin_ctx* ctx, fin_val* args)  { args[0].i = args[0].i >> args[1].i; }
static void fin_std_int_lt  (fin_ctx* ctx, fin_val* args)  { args[0].b = args[0].i  < args[1].i; }
static void fin_std_int_leq (fin_ctx* ctx, fin_val* args)  { args[0].b = args[0].i <= args[1].i; }
static void fin_std_int_gt  (fin_ctx* ctx, fin_val* args)  { args[0].b = args[0].i  > args[1].i; }
static void fin_std_int_geq (fin_ctx* ctx, fin_val* args)  { args[0].b = args[0].i >= args[1].i; }
static void fin_std_int_eq  (fin_ctx* ctx, fin_val* args)  { args[0].b = args[0].i == args[1].i; }
static void fin_std_int_neq (fin_ctx* ctx, fin_val* args)  { args[0].b = args[0].i != args[1].i; }
static void fin_std_int_to_float(fin_ctx* ctx, fin_val* args) { args[0].f = (double)(args[0].i); }

static void fin_std_int_to_str(fin_ctx* ctx, fin_val* args) {
    char buffer[64];
    sprintf(buffer, "%" PRIu64, args[0].i);
    args[0].s = fin_str_create(ctx, buffer, -1);
}

static void fin_std_float_neg(fin_ctx* ctx, fin_val* args) { args[0].f = -args[0].f; }
static void fin_std_float_add(fin_ctx* ctx, fin_val* args) { args[0].f = args[0].f  + args[1].f; }
static void fin_std_float_sub(fin_ctx* ctx, fin_val* args) { args[0].f = args[0].f  - args[1].f; }
static void fin_std_float_mul(fin_ctx* ctx, fin_val* args) { args[0].f = args[0].f  * args[1].f; }
static void fin_std_float_div(fin_ctx* ctx, fin_val* args) { args[0].f = args[0].f  / args[1].f; }
static void fin_std_float_mod(fin_ctx* ctx, fin_val* args) { args[0].f = fmod(args[0].f, args[1].f); }
static void fin_std_float_lt (fin_ctx* ctx, fin_val* args) { args[0].b = args[0].f  < args[1].f; }
static void fin_std_float_leq(fin_ctx* ctx, fin_val* args) { args[0].b = args[0].f <= args[1].f; }
static void fin_std_float_gt (fin_ctx* ctx, fin_val* args) { args[0].b = args[0].f  > args[1].f; }
static void fin_std_float_geq(fin_ctx* ctx, fin_val* args) { args[0].b = args[0].f >= args[1].f; }
static void fin_std_float_eq (fin_ctx* ctx, fin_val* args) { args[0].b = args[0].f == args[1].f; }
static void fin_std_float_neq(fin_ctx* ctx, fin_val* args) { args[0].b = args[0].f != args[1].f; }
static void fin_std_float_to_int(fin_ctx* ctx, fin_val* args) { args[0].i = (int64_t)(args[0].f); }

static void fin_std_float_to_str(fin_ctx* ctx, fin_val* args) {
    char buffer[64];
    sprintf(buffer, "%g", args[0].f);
    args[0].s = fin_str_create(ctx, buffer, -1);
}

static void fin_std_str_add(fin_ctx* ctx, fin_val* args) { args[0].s = fin_str_concat(ctx, args[0].s, args[1].s); }
static void fin_std_str_eq(fin_ctx* ctx, fin_val* args)  { args[0].b = (args[0].s == args[1].s); }
static void fin_std_str_neq(fin_ctx* ctx, fin_val* args) { args[0].b = (args[0].s != args[1].s); }

void fin_std_register(fin_ctx* ctx) {
    fin_mod_func_desc descs[] = {
        { "bool __op_and(bool,bool)", &fin_std_bool_and },
        { "bool __op_or(bool,bool)",  &fin_std_bool_or  },

        { "int __op_pos(int)",      &fin_std_int_pos  },
        { "int __op_neg(int)",      &fin_std_int_neg  },
        { "int __op_not(int)",      &fin_std_int_not  },
        { "int __op_bnot(int)",     &fin_std_int_bnot },
        { "int __op_inc(int)",      &fin_std_int_inc  },
        { "int __op_dec(int)",      &fin_std_int_dec  },
        { "int __op_add(int,int)",  &fin_std_int_add  },
        { "int __op_sub(int,int)",  &fin_std_int_sub  },
        { "int __op_mul(int,int)",  &fin_std_int_mul  },
        { "int __op_div(int,int)",  &fin_std_int_div  },
        { "int __op_mod(int,int)",  &fin_std_int_mod  },
        { "int __op_bor(int,int)",  &fin_std_int_bor  },
        { "int __op_bxor(int,int)", &fin_std_int_xor  },
        { "int __op_shl(int,int)",  &fin_std_int_shl  },
        { "int __op_shr(int,int)",  &fin_std_int_shr  },
        { "int __op_band(int,int)", &fin_std_int_band },
        { "bool __op_lt(int,int)",  &fin_std_int_lt   },
        { "bool __op_leq(int,int)", &fin_std_int_leq  },
        { "bool __op_gt(int,int)",  &fin_std_int_gt   },
        { "bool __op_geq(int,int)", &fin_std_int_geq  },
        { "bool __op_eq(int,int)",  &fin_std_int_eq   },
        { "bool __op_neq(int,int)", &fin_std_int_neq  },
        { "float float(int)",       &fin_std_int_to_float },
        { "string string(int)",     &fin_std_int_to_str },

        { "float __op_neg(float)",       &fin_std_float_neg },
        { "float __op_add(float,float)", &fin_std_float_add },
        { "float __op_sub(float,float)", &fin_std_float_sub },
        { "float __op_mul(float,float)", &fin_std_float_mul },
        { "float __op_div(float,float)", &fin_std_float_div },
        { "float __op_mod(float,float)", &fin_std_float_mod },
        { "bool __op_lt(float,float)",   &fin_std_float_lt  },
        { "bool __op_leq(float,float)",  &fin_std_float_leq },
        { "bool __op_gt(float,float)",   &fin_std_float_gt  },
        { "bool __op_geq(float,float)",  &fin_std_float_geq },
        { "bool __op_eq(float,float)",   &fin_std_float_eq  },
        { "bool __op_neq(float,float)",  &fin_std_float_neq },
        { "int int(float)",              &fin_std_float_to_int },
        { "string string(float)",        &fin_std_float_to_str },

        { "string __op_add(string,string)", &fin_std_str_add },
        { "bool __op_eq(string,string)",    &fin_std_str_eq  },
        { "bool __op_neq(string,string)",   &fin_std_str_neq },
    };
    fin_mod_create(ctx, "", descs, FIN_COUNT_OF(descs));
}

