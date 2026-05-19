#ifndef MCSOS_TYPES_H
#define MCSOS_TYPES_H

#ifdef __STDC_HOSTED__
/* Host environment — pakai stdint.h sistem */
#include <stdint.h>
#include <stddef.h>
typedef int bool;
#define true  1
#define false 0
#ifndef NULL
#define NULL ((void *)0)
#endif

#else
/* Freestanding environment — definisi manual */
typedef __SIZE_TYPE__      size_t;
typedef unsigned char      uint8_t;
typedef unsigned short     uint16_t;
typedef unsigned int       uint32_t;
typedef unsigned long long uint64_t;
typedef long long          int64_t;
typedef uint64_t           uintptr_t;
typedef int                bool;
#define true  1
#define false 0
#ifndef NULL
#define NULL ((void *)0)
#endif
#define UINT8_MAX   0xffU
#define UINT16_MAX  0xffffU
#define UINT32_MAX  0xffffffffU
#define UINT64_MAX  0xffffffffffffffffULL
#define UINTPTR_MAX UINT64_MAX
#define SIZE_MAX    UINT64_MAX
#endif /* __STDC_HOSTED__ */

#endif /* MCSOS_TYPES_H */
