/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef __FIN_VAL_H__
#define __FIN_VAL_H__

#include "fin_common.h"

typedef struct fin_str fin_str;
typedef struct fin_obj fin_obj;
typedef struct fin_map fin_map;

typedef union fin_val {
    bool     b;
    int64_t  i;
    double   f;
    fin_str* s;
    fin_obj* o;
} fin_val;

inline static bool fin_val_equal(fin_val a, fin_val b) { return a.i == b.i; }

#endif //#ifndef __FIN_VAL_H__

