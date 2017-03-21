/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_math.h"
#include "../fin_mod.h"
#include <math.h>

static void fin_math_abs_int(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)    { if (args[0].i < 0) ret->i = -args[0].i; }
static void fin_math_abs_float(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)  { ret->f = fabs(args[0].f); }
static void fin_math_ceiling(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)    { ret->f = ceil(args[0].f); }
static void fin_math_floor(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)      { ret->f = floor(args[0].f); }
static void fin_math_log(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)        { ret->f = log(args[0].f); }
static void fin_math_log2(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)       { ret->f = log2(args[0].f); }
static void fin_math_log10(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)      { ret->f = log10(args[0].f); }
static void fin_math_max_int(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)    { ret->i = args[0].i > args[1].i ? args[0].i : args[1].i; }
static void fin_math_max_float(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)  { ret->f = args[0].f > args[1].f ? args[0].f : args[1].f; }
static void fin_math_min_int(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)    { ret->i = args[0].i < args[1].i ? args[0].i : args[1].i; }
static void fin_math_min_float(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)  { ret->f = args[0].f < args[1].f ? args[0].f : args[1].f; }
static void fin_math_pow(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)        { ret->f = pow(args[0].f, args[1].f); }
static void fin_math_round(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)      { ret->f = round(args[0].f); }
static void fin_math_sign_int(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)   { ret->i = args[0].i < 0 ? -1 : 1; }
static void fin_math_sign_float(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret) { ret->f = args[0].f < 0.0 ? -1.0 : 1.0; }
static void fin_math_sqrt(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)       { ret->f = sqrt(args[0].f); }

static void fin_math_acos(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)  { ret->f = acos(args[0].f); }
static void fin_math_asin(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)  { ret->f = asin(args[0].f); }
static void fin_math_atan(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)  { ret->f = atan(args[0].f); }
static void fin_math_atan2(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret) { ret->f = atan2(args[0].f, args[1].f); }
static void fin_math_cos(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)   { ret->f = cos(args[0].f); }
static void fin_math_sin(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)   { ret->f = sin(args[0].f); }
static void fin_math_tan(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)   { ret->f = tan(args[0].f); }
static void fin_math_acosh(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret) { ret->f = acosh(args[0].f); }
static void fin_math_asinh(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret) { ret->f = asinh(args[0].f); }
static void fin_math_atanh(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret) { ret->f = atanh(args[0].f); }
static void fin_math_cosh(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)  { ret->f = cosh(args[0].f); }
static void fin_math_sinh(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)  { ret->f = sinh(args[0].f); }
static void fin_math_tanh(fin_ctx_t* ctx, const fin_val_t* args, fin_val_t* ret)  { ret->f = tanh(args[0].f); }

void fin_math_register(fin_ctx_t* ctx) {
    fin_mod_func_desc_t descs[] = {
        { "int Abs(int)", &fin_math_abs_int },
        { "float Abs(float)", &fin_math_abs_float },
        { "float Ceiling(float)", &fin_math_ceiling },
        { "float Floor(float)", &fin_math_floor },
        { "float Log(float)", &fin_math_log },
        { "float Log2(float)", &fin_math_log2 },
        { "float Log10(float)", &fin_math_log10 },
        { "int Max(int)", &fin_math_max_int },
        { "float Max(float)", &fin_math_max_float },
        { "int Min(int)", &fin_math_min_int },
        { "float Min(float)", &fin_math_min_float },
        { "float Pow(float)", &fin_math_pow },
        { "float Round(float)", &fin_math_round },
        { "int Sign(int)", &fin_math_sign_int },
        { "float Sign(float)", &fin_math_sign_float },
        { "float Sqrt(float)", &fin_math_sqrt },

        { "float ACos(float)", &fin_math_acos },
        { "float ASin(float)", &fin_math_asin },
        { "float ATan(float)", &fin_math_atan },
        { "float ATan2(float,float)", &fin_math_atan2 },
        { "float Cos(float)", &fin_math_cos },
        { "float Sin(float)", &fin_math_sin },
        { "float Tan(float)", &fin_math_tan },
        { "float ACosH(float)", &fin_math_acosh },
        { "float ASinH(float)", &fin_math_asinh },
        { "float ATanH(float)", &fin_math_atanh },
        { "float CosH(float)", &fin_math_cosh },
        { "float SinH(float)", &fin_math_sinh },
        { "float TanH(float)", &fin_math_tanh },
    };

    fin_mod_create(ctx, "math", descs, FIN_COUNT_OF(descs));
}
