/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_OBJ_H
#define FIN_OBJ_H

#include <fin/fin.h>

typedef struct fin_obj_t {
    int32_t   ref;
    fin_val_t fields[0];
} fin_obj_t;

fin_obj_t* fin_obj_create(fin_alloc alloc, fin_val_t* fields, int32_t fields_count);
void       fin_obj_inc_ref(fin_obj_t* obj);
void       fin_obj_dec_ref(fin_alloc alloc, fin_obj_t* obj);

#endif //#ifndef FIN_OBJ_H
