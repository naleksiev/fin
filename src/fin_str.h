/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_STR_H
#define FIN_STR_H

#include <fin/fin.h>

typedef struct fin_str_pool fin_str_pool;

fin_str_pool* fin_str_pool_create(fin_alloc alloc);
void          fin_str_pool_destroy(fin_str_pool* pool);

#endif //#ifndef FIN_STR_H

