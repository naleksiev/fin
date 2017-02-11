/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#include "fin_obj.h"

fin_obj* fin_obj_create(fin_alloc alloc, int32_t fields) {
    fin_obj* obj = (fin_obj*)alloc(NULL, sizeof(fin_obj*) + sizeof(fin_val) * fields);
    obj->ref = 1;
    for (int32_t i=0; i<fields; i++)
        obj->fields[i].i = 0;
    return obj;
}

void fin_obj_inc_ref(fin_obj* obj) {
    obj->ref++;
}

void fin_obj_dec_ref(fin_alloc alloc, fin_obj* obj) {
    if (--obj->ref == 0)
        alloc(obj, 0);
}

void fin_obj_destroy(fin_alloc alloc, fin_obj* obj) {
    if (obj)
        alloc(obj, 0);
}

