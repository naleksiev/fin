/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef __FIN_VM_H__
#define __FIN_VM_H__

#include "fin_common.h"
#include "fin_val.h"

typedef struct fin_vm       fin_vm;
typedef struct fin_mod_func fin_mod_func;

fin_vm* fin_vm_create(fin_alloc alloc);
void    fin_vm_destroy(fin_vm* vm);
void    fin_vm_invoke(fin_vm* vm, fin_mod_func* func);

#endif //#ifndef __FIN_VM_H__

