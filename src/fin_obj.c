/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_obj.h"

fin_obj_t* fin_obj_create(fin_alloc alloc, fin_val_t* fields, int32_t fields_count) {
    fin_obj_t* obj = (fin_obj_t*)alloc(NULL, sizeof(fin_obj_t*) + sizeof(fin_val_t) * fields_count);
    obj->ref = 1;
    for (int32_t i=0; i<fields_count; i++)
        obj->fields[i] = fields[i];
    return obj;
}

void fin_obj_inc_ref(fin_obj_t* obj) {
    obj->ref++;
}

void fin_obj_dec_ref(fin_alloc alloc, fin_obj_t* obj) {
    if (--obj->ref == 0)
        alloc(obj, 0);
}

void fin_obj_destroy(fin_alloc alloc, fin_obj_t* obj) {
    if (obj)
        alloc(obj, 0);
}
