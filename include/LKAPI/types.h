#ifndef KRAW_TYPES_H
#define KRAW_TYPES_H

/*
 * Freestanding integer type definitions for x86_64
 */
typedef unsigned char      u8;
typedef unsigned short     u16;
typedef unsigned int       u32;
typedef unsigned long long u64;

typedef signed char        i8;
typedef signed short       i16;
typedef signed int         i32;
typedef signed long long   i64;

typedef u64                usize;
typedef i64                isize;
typedef u64                uintptr_t;
typedef i64                intptr_t;

#define NULL ((void*)0)

#endif /* KRAW_TYPES_H */
