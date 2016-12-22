#include "fin_std.h"
#include "fin_mod.h"

static void fin_std_int_add(fin_val* args) { args[0].i = args[0].i  + args[1].i; }
static void fin_std_int_sub(fin_val* args) { args[0].i = args[0].i  - args[1].i; }
static void fin_std_int_mul(fin_val* args) { args[0].i = args[0].i  * args[1].i; }
static void fin_std_int_div(fin_val* args) { args[0].i = args[0].i  / args[1].i; }
static void fin_std_int_mod(fin_val* args) { args[0].i = args[0].i  % args[1].i; }
static void fin_std_int_and(fin_val* args) { args[0].i = args[0].i  & args[1].i; }
static void fin_std_int_or (fin_val* args) { args[0].i = args[0].i  | args[1].i; }
static void fin_std_int_xor(fin_val* args) { args[0].i = args[0].i  ^ args[1].i; }
static void fin_std_int_shl(fin_val* args) { args[0].i = args[0].i << args[1].i; }
static void fin_std_int_shr(fin_val* args) { args[0].i = args[0].i >> args[1].i; }
static void fin_std_int_lt (fin_val* args) { args[0].b = args[0].i  < args[1].i; }
static void fin_std_int_leq(fin_val* args) { args[0].b = args[0].i <= args[1].i; }
static void fin_std_int_gt (fin_val* args) { args[0].b = args[0].i  > args[1].i; }
static void fin_std_int_geq(fin_val* args) { args[0].b = args[0].i >= args[1].i; }
static void fin_std_int_eq (fin_val* args) { args[0].b = args[0].i == args[1].i; }
static void fin_std_int_neq(fin_val* args) { args[0].b = args[0].i != args[1].i; }

static void fin_std_str_eq(fin_val* args)  { args[0].b = (args[0].s == args[1].s); }
static void fin_std_str_neq(fin_val* args) { args[0].b = (args[0].s != args[1].s); }

static void fin_std_float_to_int(fin_val* args) { args[0].i = (int64_t)(args[0].f); }
static void fin_std_int_to_float(fin_val* args) { args[0].f = (double)(args[0].i); }

void fin_std_register(fin_ctx* ctx) {
    fin_mod_func_desc descs[] = {
        { "int op_add(int,int)",  &fin_std_int_add },
        { "int op_sub(int,int)",  &fin_std_int_sub },
        { "int op_mul(int,int)",  &fin_std_int_mul },
        { "int op_div(int,int)",  &fin_std_int_div },
        { "int op_mod(int,int)",  &fin_std_int_mod },
        { "int op_bor(int,int)",  &fin_std_int_or  },
        { "int op_bxor(int,int)", &fin_std_int_xor },
        { "int op_shl(int,int)",  &fin_std_int_shl },
        { "int op_shr(int,int)",  &fin_std_int_shr },
        { "int op_band(int,int)", &fin_std_int_and },
        { "bool op_lt(int,int)",  &fin_std_int_lt  },
        { "bool op_leq(int,int)", &fin_std_int_leq },
        { "bool op_gt(int,int)",  &fin_std_int_gt  },
        { "bool op_geq(int,int)", &fin_std_int_geq },
        { "bool op_eq(int,int)",  &fin_std_int_eq  },
        { "bool op_neq(int,int)", &fin_std_int_neq },

        { "bool op_eq(string,string)",  &fin_std_str_eq  },
        { "bool op_neq(string,string)", &fin_std_str_neq },

        { "float float(int)", &fin_std_int_to_float },
        { "int int(float)", &fin_std_float_to_int },
    };
    fin_mod_create(ctx, "", descs, FIN_COUNT_OF(descs));
}
