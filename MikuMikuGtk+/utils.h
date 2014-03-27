#ifndef _INCLUDED_UTILS_H_
#define _INCLUDED_UTILS_H_

#include <stdio.h>
#include <assimp/mesh.h>
#include "types.h"
#include "system_depends.h"
#include "ght_hash_table.h"

#define DEFAULT_BUFFER_SIZE 64

#if defined(DEBUG) || defined(_DEBUG)
# include <gtk/gtk.h>
# define DEBUG_PRINT(FORMAT, ...) g_print((FORMAT), __VA_ARGS__)
#else
# define DEBUG_PRINT(FORMAT, ...)
#endif

#ifndef INLINE
# ifdef _MSC_VER
#  define INLINE __inline
# else
#  define INLINE inline
# endif
#endif

#define CLAMPED(A, LB, UB) ((A) = (A) < (LB) ? (LB) : ((UB) < (A) ? (UB) : (A)))

#define BOOL_COMPARE(B1, B2) ((B1) && (B2) || !(B1) && !(B2))

#define CHECK_BOUND(DATA, MINIMUM, MAXIMUM) ((DATA) >= (MINIMUM) && (DATA) <= (MAXIMUM))

#define SET_POSITION(DST, SRC) \
	(DST)[0] = (SRC)[0],	\
	(DST)[1] = (SRC)[1],	\
	(DST)[2] = - (SRC)[2]

#define SET_ROTATION(DST, SRC) \
	(DST)[0] = (SRC)[0],	\
	(DST)[1] = - (SRC)[1],	\
	(DST)[2] = - (SRC)[2],	\
	(DST)[3] = (SRC)[3]

#define COPY_VECTOR3(DEST, SRC) \
	(DEST)[0] = (SRC)[0], \
	(DEST)[1] = (SRC)[1], \
	(DEST)[2] = (SRC)[2]

#define COPY_VECTOR4(DEST, SRC) \
	(DEST)[0] = (SRC)[0], \
	(DEST)[1] = (SRC)[1], \
	(DEST)[2] = (SRC)[2], \
	(DEST)[3] = (SRC)[3]

#define LOAD_IDENTITY_QUATERNION(QUAT) \
	(QUAT)[0] = 0,	\
	(QUAT)[1] = 0,	\
	(QUAT)[2] = 0,	\
	(QUAT)[3] = 1

#define IDENTITY_MATRIX {1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1}

#define IDENTITY_MATRIX3x3 {1,0,0,0,1,0,0,0,1}

#define COPY_MATRIX3x3(DEST, SRC) \
	(DEST)[0] = (SRC)[0], \
	(DEST)[1] = (SRC)[1], \
	(DEST)[2] = (SRC)[2], \
	(DEST)[3] = (SRC)[3], \
	(DEST)[4] = (SRC)[4], \
	(DEST)[5] = (SRC)[5], \
	(DEST)[6] = (SRC)[6], \
	(DEST)[7] = (SRC)[7], \
	(DEST)[8] = (SRC)[8]

#define COPY_MATRIX4x4(DEST, SRC) \
	(DEST)[0] = (SRC)[0], \
	(DEST)[1] = (SRC)[1], \
	(DEST)[2] = (SRC)[2], \
	(DEST)[3] = (SRC)[3], \
	(DEST)[4] = (SRC)[4], \
	(DEST)[5] = (SRC)[5], \
	(DEST)[6] = (SRC)[6], \
	(DEST)[7] = (SRC)[7], \
	(DEST)[8] = (SRC)[8], \
	(DEST)[9] = (SRC)[9], \
	(DEST)[10] = (SRC)[10], \
	(DEST)[11] = (SRC)[11], \
	(DEST)[12] = (SRC)[12], \
	(DEST)[13] = (SRC)[13], \
	(DEST)[14] = (SRC)[14], \
	(DEST)[15] = (SRC)[15] \

#define LOAD_IDENTITY_MATRIX3x3(MATRIX) \
	(MATRIX)[0] = 1, \
	(MATRIX)[1] = 0, \
	(MATRIX)[2] = 0, \
	(MATRIX)[3] = 0, \
	(MATRIX)[4] = 1, \
	(MATRIX)[5] = 0, \
	(MATRIX)[6] = 0, \
	(MATRIX)[7] = 0, \
	(MATRIX)[8] = 1

#define LOAD_IDENTITY_MATRIX4x4(MATRIX) \
	(MATRIX)[0] = 1, \
	(MATRIX)[1] = 0, \
	(MATRIX)[2] = 0, \
	(MATRIX)[3] = 0, \
	(MATRIX)[4] = 0, \
	(MATRIX)[5] = 1, \
	(MATRIX)[6] = 0, \
	(MATRIX)[7] = 0, \
	(MATRIX)[8] = 0, \
	(MATRIX)[9] = 0, \
	(MATRIX)[10] = 1, \
	(MATRIX)[11] = 0, \
	(MATRIX)[12] = 0, \
	(MATRIX)[13] = 0, \
	(MATRIX)[14] = 0, \
	(MATRIX)[15] = 1

#define SET_MAX(DST, SRC) \
	if((DST) < (SRC))	\
		(DST) = (SRC)

#define SET_MIN(DST, SRC) \
	if((DST) > (SRC))	\
	(DST) = (SRC)

typedef struct _BYTE_ARRAY
{
	uint8 *buffer;
	size_t num_data;
	size_t buffer_size;
	size_t block_size;
} BYTE_ARRAY;

typedef struct _WORD_ARRAY
{
	uint16 *buffer;
	size_t num_data;
	size_t buffer_size;
	size_t block_size;
} WORD_ARRAY;

typedef struct _UINT32_ARRAY
{
	uint32 *buffer;
	size_t num_data;
	size_t buffer_size;
	size_t block_size;
} UINT32_ARRAY;

typedef struct _POINTER_ARRAY
{
	void **buffer;
	size_t num_data;
	size_t buffer_size;
	size_t block_size;
} POINTER_ARRAY;

typedef struct _STRUCT_ARRAY
{
	uint8 *buffer;
	size_t num_data;
	size_t buffer_size;
	size_t block_size;
	size_t data_size;
} STRUCT_ARRAY;

#ifdef __cplusplus
extern "C" {
#endif

extern BYTE_ARRAY* ByteArrayNew(size_t block_size);
extern void ByteArrayAppend(BYTE_ARRAY* barray, uint8 data);
extern void ByteArrayDesteroy(BYTE_ARRAY** barray);

extern WORD_ARRAY* WordArrayNew(size_t block_size);
extern void WordArrayAppend(WORD_ARRAY* warray, uint16 data);
extern void WordArrayDestroy(WORD_ARRAY** warray);

extern UINT32_ARRAY* Uint32ArrayNew(size_t block_size);
extern void Uint32ArrayAppend(UINT32_ARRAY* uarray, uint32 data);
extern void Uint32ArrayDestroy(UINT32_ARRAY** uarray);

extern POINTER_ARRAY* PointerArrayNew(size_t block_size);
extern void PointerArrayRelease(
	POINTER_ARRAY* pointer_array,
	void (*destroy_func)(void*)
);
extern void PointerArrayDestroy(
	POINTER_ARRAY** pointer_array,
	void (*destroy_func)(void*)
);
void PointerArrayAppend(POINTER_ARRAY* pointer_array, void* data);
extern void PointerArrayRemoveByIndex(
	POINTER_ARRAY* pointer_array,
	size_t index,
	void (*destroy_func)(void*)
);
extern void PointerArrayRemoveByData(
	POINTER_ARRAY* pointer_array,
	void* data,
	void (*destroy_func)(void*)
);
extern int PointerArrayDoesCointainData(POINTER_ARRAY* pointer_array, void* data);

extern STRUCT_ARRAY* StructArrayNew(size_t data_size, size_t block_size);
extern void StructArrayDestroy(
	STRUCT_ARRAY** struct_array,
	void (*destroy_func)(void*)
);
extern void StructArrayAppend(STRUCT_ARRAY* struct_array, void* data);
extern void* StructArrayReserve(STRUCT_ARRAY* struct_array);
extern void StructArrayResize(STRUCT_ARRAY* struct_array, size_t new_size);
extern void StructArrayRemoveByIndex(
	STRUCT_ARRAY* struct_array,
	size_t index,
	void (*destroy_func)(void*)
);

extern int FuzzyZero(float value);
extern int ExtendedFuzzyZero(float value);

extern int OutCode(const float p[3], const float half_extent[3]);

extern int CompareInt32(int32* num1, int32* num2);
extern int CompareUint32(uint32* num1, uint32* num2);

extern void MultiQuaternion(float* q1, float* q2);

extern INLINE void PlusVector3(float* vector1, const float* vector2);
extern INLINE void MulVector3(float* vector1, const float* vector2);
extern INLINE void ScaleVector3(float* vector, float scalar);
extern INLINE int CompareVector3(float* vector1, float* vector2);
extern INLINE void PlusVector4(float* vector1, const float* vector2);
extern INLINE void SetInterpolateVector3(float* vector, float* v1, float* v2, float rt);
extern INLINE void MulVector4(float* vector1, const float* vector2);
extern INLINE void MulMatrix3x3(float matrix1[9], const float matrix2[9]);
extern INLINE void RotateMatrix3x3(float matrix[9], const float quaternion[4]);
extern INLINE void InvertMatrix3x3(float matrix[9], float invert[9]);
extern INLINE void MulMatrixVector3(float vector[3], float matrix[9]);
extern void Matrix3x3SetEulerZYX(
	float* matrix,
	float x,
	float y,
	float z
);
extern INLINE void LoadIdentityMatrix4x4(float matrix[16]);
extern INLINE void MulMatrix4x4(const float matrix1[16], const float matrix2[16], float result[16]);
extern INLINE void MulMatrixVector4(const float matrix[16], const float vector[4], float result[4]);
extern INLINE void MulMatrixVector4Transpose(const float matrix[16], const float vector[4], float result[4]);
extern INLINE void MulMatrixScalar4x4(float matrix[16], float scale);
extern INLINE int InvertMatrix4x4(float matrix[16], float invert_matrix[16]);
extern void TransposeMatrix4x4(float matrix[16], float transpose[16]);
extern INLINE void ScaleMatrix4x4(float matrix[16], float scale[3]);
extern void RotateMatrix4x4(float matrix[16], float rotate[3]);
extern void Matrix3x3SetRotation(float matrix[9], float quaternion[4]);
extern void Matrix3x3GetEulerZYX(
	float matrix[9],
	float* yaw,
	float* pitch,
	float* roll
);

extern void Quaternion(float* r, float* q);

extern int CheckHasExtension(const char* name);

extern int CheckHasAnyExtension(const char** names);

extern float Dot3DVector(const float* vector1, const float* vector2);

extern float Length3DVector(const float* vector);

extern void Normalize3DVector(float* vector);

extern void SafeNormalize3DVector(float* vector);

extern void Cross3DVector(float* result, const float* vector1, const float* vector2);

INLINE float QuaternionDot(const float* q1, const float* q2);

INLINE float QuaternionLength2(const float* quaternion);

extern void QuaternionSlerp(float* result, const float* q1, const float* q2, const float t);

extern void QuaternionSetRotation(float* quaternion, const float* axis, const float angle);

extern void QuaternionSetEuler(float* quaternion, float yaw, float pitch, float roll);

extern void QuaternionSetEulerZYX(float* quaternion, float yaw, float pitch, float roll);

extern void QuaternionNormalize(float* quaternion);

extern void Perspective(float matrix[16], float fovy, float aspect, float z_near, float z_far);

extern void Unprojection(
	float vector[3],
	const float window[3],
	const float model[16],
	const float projection[16],
	const float viewport[4]
);

extern void InfinitePerspective(float* matrix, float fovy, float aspect, float z_near);

extern void Ortho(float matrix[16], float left, float right, float bottom, float top, float near_plane, float far_plane);

extern void RectangleSetTopLeft(float rectangle[4], float left, float top);
extern void RectangleSetSize(float rectangle[4], float width, float height);
extern int RectangleContainsPosition(float rectangle[4], float x, float y);

extern void SetVertices2D(float vertices[8], float rect[4]);

extern int aiMeshHasTextureCoords(const struct aiMesh* mesh, unsigned int index);

/**************************************
* StringCompareIgnoreCaseä÷êî         *
* ëÂï∂éö/è¨ï∂éöÇñ≥éãÇµÇƒï∂éöóÒÇî‰är *
* à¯êî                                *
* str1	: î‰ärï∂éöóÒ1                 *
* str2	: î‰ärï∂éöóÒ2                 *
* ï‘ÇËíl                              *
*	ï∂éöóÒÇÃç∑(ìØï∂éöóÒÇ»ÇÁ0)         *
**************************************/
extern int StringCompareIgnoreCase(const char* str1, const char* str2);

extern int StringNumCompareIgnoreCase(const char* str1, const char* str2, int num);

extern const char* StringStringIgnoreCase(const char* str, const char* compare);

extern int GetTextFromStream(char* stream, char** text);

extern int GetSignedValue(uint8* stream, int length);

extern INLINE void ClampByteData(uint8* a, const uint8 lb, const uint8 ub);

extern char** SplitString(char* str, const char* delim, int* num_strings);

extern ght_uint32_t GetStringHash(ght_hash_key_t* key);

extern void HashTableReleaseAll(ght_hash_table_t* table, void (*destroy_func)(void*));

extern void DummyFuncNoReturn(void* dummy);
extern void DummyFuncNoReturn2(void* dummy1, void* dummy2);
extern int DummyFuncZeroReturn(void* dummy);
extern int DummyFuncTrueReturn(void* dummy);
extern int DummyFuncMinusOneReturn(void* dummy);
extern float DummyFuncFloatZeroReturn(void* dummy);
extern float DummyFuncFloatOneReturn(void* dummy);
extern FLOAT_T DummyFuncFloatTOneReturn(void* dummy);
extern void DummyFuncGetZeroVector3(void* dummy, float* vector);
extern void DummyFuncGetZeroVector4(void* dummy, float* vector);
extern void DummyFuncGetWhiteColor(void* dummy,float* color);

#ifdef __cplusplus
};
#endif

#endif	// #ifndef _INCLUDED_UTILS_H_
