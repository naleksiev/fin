/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_STR_H
#define FIN_STR_H

#include <fin/fin.h>

typedef struct fin_str      fin_str;
typedef struct fin_str_pool fin_str_pool;

fin_str_pool* fin_str_pool_create(fin_alloc alloc);
void          fin_str_pool_destroy(fin_str_pool* pool);
fin_str*      fin_str_create(fin_str_pool* pool, const char* str, int32_t len);
fin_str*      fin_str_clone(fin_str* str);
void          fin_str_destroy(fin_str_pool* pool, fin_str* str);
const char*   fin_str_cstr(fin_str* str);
int32_t       fin_str_len(fin_str* str);

#endif //#ifndef FIN_STR_H

