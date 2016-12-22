#ifndef __FIN_STR_H__
#define __FIN_STR_H__

#include "fin_common.h"

typedef struct fin_str      fin_str;
typedef struct fin_str_pool fin_str_pool;

fin_str_pool* fin_str_pool_create(fin_alloc alloc);
void          fin_str_pool_destroy(fin_str_pool* pool);
fin_str*      fin_str_create(fin_str_pool* pool, const char* str, int32_t len);
void          fin_str_destroy(fin_str_pool* pool, fin_str* str);
const char*   fin_str_cstr(fin_str* str);
int32_t       fin_str_len(fin_str* str);

#endif //#ifndef __FIN_STR_H__
