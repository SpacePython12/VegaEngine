#ifndef VEGA_TYPES_H
#define VEGA_TYPES_H 1

#ifdef __cplusplus
#define cextern extern "C"
#else
#define cextern extern
#endif

#include <stdint.h>
#include <stdbool.h>

typedef uint8_t u8;
typedef int8_t s8;
typedef uint16_t u16;
typedef int16_t s16;
typedef uint32_t u32;
typedef int32_t s32;
typedef uint64_t u64;
typedef int64_t s64;

typedef u8 bool8;

typedef unsigned int uint;

typedef uint ubitfield;
typedef int bitfield;

#endif