/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_VM_H
#define FIN_VM_H

#include <fin/fin.h>

typedef struct fin_vm       fin_vm;
typedef struct fin_mod_func fin_mod_func;

fin_vm* fin_vm_create(fin_ctx* ctx);
void    fin_vm_destroy(fin_vm* vm);
void    fin_vm_invoke(fin_vm* vm, fin_mod_func* func);

#endif //#ifndef FIN_VM_H

