/* Copyright 2014 Google Inc. All Rights Reserved. */

/*------------------------------------------------------------------------------
--
--  Description : Basic type definitions.
--
------------------------------------------------------------------------------*/

#ifndef BASETYPE_H_INCLUDED
#define BASETYPE_H_INCLUDED

#define VOLATILE    volatile

#ifdef __linux__    /* typedefs for Linux */

#include <stddef.h> /* for size_t, NULL, etc. */

typedef unsigned char u8;
typedef signed char i8;
typedef unsigned short u16;
typedef signed short i16;
typedef unsigned int u32;
typedef signed int i32;
typedef long i64;
typedef unsigned long u64;

#if defined(_WIN64)
typedef unsigned long long addr_t;
#else
typedef unsigned long addr_t;
#endif

typedef long off64_t;
typedef size_t    ptr_t;

#ifndef __cplusplus
#ifndef BOOL_DEFINED
#define BOOL_DEFINED
typedef enum {
        false   = 0,
        true    = 1
} bool;
#endif
#endif

#else /* __symbian__ or __win__ or whatever, customize it to suit well */

#ifndef _SIZE_T_DEFINED
typedef unsigned int size_t;

#define _SIZE_T_DEFINED
#endif

#ifndef NULL
#ifdef  __cplusplus
#define NULL    0
#else /*  */
#define NULL    ((void *)0)
#endif /*  */
#endif

typedef unsigned char u8;
typedef signed char i8;
typedef unsigned short u16;
typedef signed short i16;
typedef unsigned int u32;
typedef signed int i32;
#if INTPTR_MAX == INT64_MAX
typedef unsigned long u64;
#else
typedef unsigned long long u64;
#endif

#ifndef __cplusplus
typedef enum {
        false   = 0,
        true    = 1
} bool;
#endif

#endif

#if defined(VC1SWDEC_16BIT) || defined(MP4ENC_ARM11)
typedef unsigned short u16x;
typedef signed short i16x;
#else
typedef unsigned int u16x;
typedef signed int i16x;
#endif

#endif /* BASETYPE_H_INCLUDED */
