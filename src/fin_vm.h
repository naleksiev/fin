/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_VM_H
#define FIN_VM_H

#include <fin/fin.h>

typedef struct fin_vm_t       fin_vm_t;
typedef struct fin_mod_func_t fin_mod_func_t;

fin_vm_t* fin_vm_create(fin_ctx_t* ctx);
void      fin_vm_destroy(fin_vm_t* vm);
void      fin_vm_invoke(fin_vm_t* vm, fin_mod_func_t* func);

#endif //#ifndef FIN_VM_H
