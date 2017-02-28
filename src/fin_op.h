/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_OP_H
#define FIN_OP_H

typedef enum fin_op_t {
    fin_op_load_const,
    fin_op_load_arg,
    fin_op_store_arg,
    fin_op_load_local,
    fin_op_store_local,
    fin_op_load_field,
    fin_op_store_field,
    fin_op_call,
    fin_op_branch,
    fin_op_branch_if,
    fin_op_branch_if_n,
    fin_op_return,
    fin_op_pop,
    fin_op_new,
} fin_op_t_t;

#endif //#ifndef FIN_OP_H

