/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_math.h"
#include "fin_mod.h"
#include <math.h>

static void fin_math_abs_int(fin_val* args)    { if (args[0].i < 0) args[0].i = -args[0].i; }
static void fin_math_abs_float(fin_val* args)  { args[0].f = fabs(args[0].f); }
static void fin_math_ceiling(fin_val* args)    { args[0].f = ceil(args[0].f); }
static void fin_math_floor(fin_val* args)      { args[0].f = floor(args[0].f); }
static void fin_math_log(fin_val* args)        { args[0].f = log(args[0].f); }
static void fin_math_log2(fin_val* args)       { args[0].f = log2(args[0].f); }
static void fin_math_log10(fin_val* args)      { args[0].f = log10(args[0].f); }
static void fin_math_max_int(fin_val* args)    { args[0].i = args[0].i > args[1].i ? args[0].i : args[1].i; }
static void fin_math_max_float(fin_val* args)  { args[0].f = args[0].f > args[1].f ? args[0].f : args[1].f; }
static void fin_math_min_int(fin_val* args)    { args[0].i = args[0].i < args[1].i ? args[0].i : args[1].i; }
static void fin_math_min_float(fin_val* args)  { args[0].f = args[0].f < args[1].f ? args[0].f : args[1].f; }
static void fin_math_pow(fin_val* args)        { args[0].f = pow(args[0].f, args[1].f); }
static void fin_math_round(fin_val* args)      { args[0].f = round(args[0].f); }
static void fin_math_sign_int(fin_val* args)   { args[0].i = args[0].i < 0 ? -1 : 1; }
static void fin_math_sign_float(fin_val* args) { args[0].f = args[0].f < 0.0 ? -1.0 : 1.0; }
static void fin_math_sqrt(fin_val* args)       { args[0].f = sqrt(args[0].f); }

static void fin_math_acos(fin_val* args)  { args[0].f = acos(args[0].f); }
static void fin_math_asin(fin_val* args)  { args[0].f = asin(args[0].f); }
static void fin_math_atan(fin_val* args)  { args[0].f = atan(args[0].f); }
static void fin_math_atan2(fin_val* args) { args[0].f = atan2(args[0].f, args[1].f); }
static void fin_math_cos(fin_val* args)   { args[0].f = cos(args[0].f); }
static void fin_math_sin(fin_val* args)   { args[0].f = sin(args[0].f); }
static void fin_math_tan(fin_val* args)   { args[0].f = tan(args[0].f); }
static void fin_math_acosh(fin_val* args) { args[0].f = acosh(args[0].f); }
static void fin_math_asinh(fin_val* args) { args[0].f = asinh(args[0].f); }
static void fin_math_atanh(fin_val* args) { args[0].f = atanh(args[0].f); }
static void fin_math_cosh(fin_val* args)  { args[0].f = cosh(args[0].f); }
static void fin_math_sinh(fin_val* args)  { args[0].f = sinh(args[0].f); }
static void fin_math_tanh(fin_val* args)  { args[0].f = tanh(args[0].f); }

void fin_math_register(fin_ctx* ctx) {
    fin_mod_func_desc descs[] = {
        { "int Abs(int)", & fin_math_abs_int },
        { "float Abs(float)", & fin_math_abs_float },
        { "float Ceiling(float)", & fin_math_ceiling },
        { "float Floor(float)", & fin_math_floor },
        { "float Log(float)", & fin_math_log },
        { "float Log2(float)", & fin_math_log2 },
        { "float Log10(float)", & fin_math_log10 },
        { "int Max(int)", & fin_math_max_int },
        { "float Max(float)", & fin_math_max_float },
        { "int Min(int)", & fin_math_min_int },
        { "float Min(float)", & fin_math_min_float },
        { "float Pow(float)", & fin_math_pow },
        { "float Round(float)", & fin_math_round },
        { "int Sign(int)", & fin_math_sign_int },
        { "float Sign(float)", & fin_math_sign_float },
        { "float Sqrt(float)", & fin_math_sqrt },

        { "float ACos(float)", & fin_math_acos },
        { "float ASin(float)", & fin_math_asin },
        { "float ATan(float)", & fin_math_atan },
        { "float ATan2(float,float)", & fin_math_atan2 },
        { "float Cos(float)", & fin_math_cos },
        { "float Sin(float)", & fin_math_sin },
        { "float Tan(float)", & fin_math_tan },
        { "float ACosH(float)", & fin_math_acosh },
        { "float ASinH(float)", & fin_math_asinh },
        { "float ATanH(float)", & fin_math_atanh },
        { "float CosH(float)", & fin_math_cosh },
        { "float SinH(float)", & fin_math_sinh },
        { "float TanH(float)", & fin_math_tanh },
    };

    fin_mod_create(ctx, "math", descs, FIN_COUNT_OF(descs));
}

