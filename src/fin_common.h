/*
 * Copyright 2016-2017 Nikolay Aleksiev. All rights reserved.
 * License: https://github.com/naleksiev/fin/blob/master/LICENSE
 */

#ifndef FIN_COMMON_H
#define FIN_COMMON_H

#if defined(_MSC_VER)
#   define FIN_CONFIG_COMPUTED_GOTO 0
#   define _CRT_SECURE_NO_WARNINGS
#   define inline __forceinline
#endif

#ifndef FIN_CONFIG_COMPUTED_GOTO
#   define FIN_CONFIG_COMPUTED_GOTO 1
#endif

#define FIN_COUNT_OF(x) (sizeof(x) / sizeof((x)[0]))

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef void* (*fin_alloc)(void* ptr, unsigned int size);

#endif //#ifndef FIN_COMMON_H

