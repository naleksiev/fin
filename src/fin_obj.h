/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_OBJ_H
#define FIN_OBJ_H

#include <fin/fin.h>

typedef struct fin_obj {
    int32_t ref;
    fin_val fields[0];
} fin_obj;

fin_obj* fin_obj_create(fin_alloc alloc, int32_t fields);
void     fin_obj_inc_ref(fin_obj* obj);
void     fin_obj_dec_ref(fin_alloc alloc, fin_obj* obj);

#endif //#ifndef FIN_OBJ_H

