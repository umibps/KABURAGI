#ifndef _INCLUDE_TYPES_H_
#define _INCLUDE_TYPES_H_

#include <stdio.h>
#include <string.h>

#if defined(_WIN32) && !defined(__GNUC__)

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
typedef float VECTOR3[3];
typedef float VECTOR4[4];
typedef float QUATERNION[4];

#define IDENTITY_QUATERNION {0, 0, 0, 1}

#define TRANSFORM_SIZE (sizeof(float)*4*(3+1))

typedef float QUAD_WORD[4];

typedef union _TYPE_UNION
{
	long lvalue;
	double dvalue;
	int ivalue;
	float fvalue;
} TYPE_UNION;

typedef struct _VERTEX_C4UB_V3F
{
	uint8 color[4];
	float position[3];
} VERTEX_C4UB_V3F;

typedef struct _VERTEX_C3F_V3F
{
	float color[3];
	float position[3];
} VERTEX_C3F_V3F;

#define DIV_PIXEL (0.00392157f)

#define ROOT2 1.4142135623730950488016887242097

#define WHITE_TEXTURE_NAME "WhiteTexture"

typedef size_t (*stream_func)(void*, size_t, size_t, void*);
typedef int (*seek_func)(void*, long, int);

#ifndef CSTL_MAP_H_INCLUDED

#define CSTL_MAGIC(x)

#define CSTL_TYPE_IMPLEMENT(Name, KeyType, ValueType)	\
\
typedef struct Name##RBTree Name##RBTree;\
	struct Name##RBTree {\
	struct Name##RBTree *parent;\
	struct Name##RBTree *left;\
	struct Name##RBTree *right;\
	int color;\
	KeyType key;\
	ValueType value;\
	CSTL_MAGIC(struct Name##RBTree *magic;)\
};\
/*! \
 * \brief set/mapç\ë¢ëÃ\
 */\
struct Name {\
	Name##RBTree *tree;\
	size_t size;\
	CSTL_MAGIC(Name *magic;)\
};

CSTL_TYPE_IMPLEMENT(STR_STR_MAP, char*, char*)
typedef struct STR_STR_MAP STR_STR_MAP;
typedef struct STR_STR_MAPRBTree *STR_STR_MAPIterator;

CSTL_TYPE_IMPLEMENT(STR_PTR_MAP, char*, void*)
typedef struct STR_PTR_MAP STR_PTR_MAP;
typedef struct STR_PTR_MAPRBTree *STR_PTR_MAPIterator;

CSTL_TYPE_IMPLEMENT(PTR_PTR_MAP, void*, void*)
typedef struct PTR_PTR_MAP PTR_PTR_MAP;
typedef struct PTR_PTR_MAPRBTree *PTR_PTR_MAPIterator;

#endif

#endif	// #ifndef _INCLUDE_TYPES_H_
