#pragma once

#define ARRAY_LENGTH(x)   sizeof(x)/sizeof(x[0])
#define SET_BIT(x, pos)   x |=  (1 << (u32)pos)
#define CLEAR_BIT(x, pos) x &= ~(1 << (u32)pos)
#define IS_BIT(x, pos)    (((x)>>((u32)pos)) & 1)

typedef unsigned char           u8;
typedef unsigned short          u16;
typedef unsigned int            u32;
typedef unsigned long long int  u64;
typedef signed char             s8;
typedef signed short            s16;
typedef signed int              s32;
typedef signed long long int    s64;
typedef unsigned char           b8;
typedef unsigned short          b16;
typedef unsigned int            b32;
typedef unsigned long long int  b64;
typedef float                   f32;
typedef double                  f64;

#if(XE_DBG)
void debugUnreachable(char *file, u32 line) {
    printf("\n[ERROR] unreachable area reached: %s(%d)", file, line);
};
#define DEBUG_UNREACHABLE debugUnreachable(__FILE__, __LINE__);
#else
#define DEBUG_UNREACHABLE
#endif

//@NOTE: these are defined in the cmd line scripts
//#define XE_DBG
//#define XE_TEST
//#define MSVC_COMPILER
//#define CLANG_COMPILER
//#define XE_PLAT_WIN
//#define XE_PLAT_LIN
