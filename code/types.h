/* date = July 15th 2021 9:58 am */
#ifndef TYPES_H

#include <stdint.h>

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float r32;
typedef double r64;

typedef uintptr_t u_ptr;
typedef intptr_t s_ptr;

typedef s32 b32;
typedef size_t memory_index;

#define internal static
#define local_persist static
#define global_variable static

#define KiloBytes(num) num*1024
#define MegaBytes(num) KiloBytes(num)*1024
#define GigaBytes(num) MegaBytes(num)*1024

#define TYPES_H
#endif //TYPES_H
