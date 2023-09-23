#ifndef _I386_TYPES_H
#define _I386_TYPES_H


#define NULL ((void *)0)
#define BITS_PER_LONG 32

typedef enum
{
    FALSE = 0,
    TRUE = 1
} bool;

typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;
typedef unsigned int __u32;
typedef signed char int8_t;
typedef signed short int int16_t;
typedef signed int int32_t;
typedef signed long long int int64_t;
typedef unsigned char uint8_t;
typedef unsigned short int uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long long int uint64_t;

#endif
