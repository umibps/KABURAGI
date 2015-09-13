#ifndef _INCLUDE_TYPES_H_
#define _INCLUDE_TYPES_H_

#include <stdio.h>

#if defined(_MSC_VER)

typedef char int8;
typedef short int16;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef long int32;
typedef unsigned long uint32;
typedef long long int int64;
typedef unsigned long long int uint64;

#else

# if !defined(_TIFFCONF_)

#  include <stdint.h>

typedef char int8;
typedef short int16;
typedef unsigned char uint8;
typedef unsigned short uint16;
typedef int32_t int32;
typedef uint32_t uint32;
typedef long long int int64;
typedef unsigned long long int uint64;

# endif

#endif

typedef double FLOAT_T;

#define MAXIMUM(A, B) (((A) > (B)) ? (A) : (B))
#define MINIMUM(A, B) (((A) < (B)) ? (A) : (B))

#define DIV_PIXEL ((FLOAT_T)0.00392157)

#define ROOT2 1.4142135623730950488016887242097

typedef size_t (*stream_func_t)(void*, size_t, size_t, void*);
typedef int (*seek_func_t)(void*, long, int);

#ifdef EXTERN
# undef EXTERN
#endif

#ifdef _MSC_VER
# ifdef __cplusplus
#  define EXTERN extern "C" __declspec(dllexport)
# else
#  define EXTERN extern __declspec(dllexport)
# endif
#else
# define EXTERN extern
#endif

#ifndef INLINE
# ifdef _MSC_VER
#  define INLINE __inline
# else
#  define INLINE inline
# endif
#endif

#endif	// #ifndef _INCLUDE_TYPES_H_
