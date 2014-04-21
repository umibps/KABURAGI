// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdio.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <ctype.h>
#include "cstl/map.h"
#include "application.h"
#include "memory.h"
#include "utils.h"

#ifndef M_PI
# define M_PI 3.1415926535897932384626433832795
#endif

CSTL_MAP_INTERFACE(STR_STR_MAP, char*, char*)
CSTL_MAP_IMPLEMENT(STR_STR_MAP, char*, char*, strcmp)

CSTL_MAP_INTERFACE(STR_PTR_MAP, char*, void*)
CSTL_MAP_IMPLEMENT(STR_PTR_MAP, char*, void*, strcmp)

extern int PointerCompare(void* p1, void* p2);
CSTL_MAP_INTERFACE(PTR_PTR_MAP, void*, void*)
CSTL_MAP_IMPLEMENT(PTR_PTR_MAP, void*, void*, PointerCompare)

int FuzzyZero(float value)
{
	return fabs(value) < FLT_EPSILON;
}

int ExtendedFuzzyZero(float value)
{
	return fabs(value) < FLT_EPSILON * 300;
}

int OutCode(const float p[3], const float half_extent[3])
{
	int ret = 0;
	if(p[0] < - half_extent[0])
	{
		ret |= 0x01;
	}
	else if(p[0] > half_extent[0])
	{
		ret |= 0x08;
	}
	if(p[1] < - half_extent[1])
	{
		ret |= 0x02;
	}
	else if(p[1] > half_extent[1])
	{
		ret |= 0x10;
	}
	if(p[2] < - half_extent[2])
	{
		ret |= 0x04;
	}
	else if(p[2] > half_extent[2])
	{
		ret |= 0x20;
	}

	return ret;
}

int CompareInt32(int32* num1, int32* num2)
{
	return *num1 < *num2;
}

int CompareUint32(uint32* num1, uint32* num2)
{
	return *num1 < *num2;
}

int PointerCompare(void* p1, void* p2)
{
	return (int)p1 - (int)p2;
}

void MultiQuaternion(float* q1, float* q2)
{
	float q[4];

	q[0] = q1[3]*q2[0] + q1[0]*q2[3] + q1[1]*q2[2] - q1[2]*q2[1];
	q[1] = q1[3]*q2[1] + q1[1]*q2[3] + q1[2]*q2[0] - q1[0]*q2[2];
	q[2] = q1[3]*q2[2] + q1[2]*q2[3] + q1[0]*q2[1] - q1[1]*q2[0];
	q1[3] = q1[3]*q2[3] - q1[0]*q2[0] - q1[1]*q2[1] - q1[2]*q2[2];

	q1[0] = q[0];
	q1[1] = q[1];
	q1[2] = q[2];
}

void Quaternion(float* r, float* q)
{
	float xx = q[0] * q[0] * 2.0f;
	float yy = q[1] * q[1] * 2.0f;
	float zz = q[2] * q[2] * 2.0f;
	float xy = q[0] * q[1] * 2.0f;
	float yz = q[1] * q[2] * 2.0f;
	float zx = q[2] * q[0] * 2.0f;
	float xw = q[0] * q[3] * 2.0f;
	float yw = q[1] * q[3] * 2.0f;
	float zw = q[2] * q[3] * 2.0f;
	float r1[16]={1.0f - yy - zz, xy + zw, zx - yw, 0.0f,
		xy - zw, 1.0f - zz - xx, yz + xw, 0.0f,
		zx + yw, yz - xw, 1.0f - xx - yy, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
	};
	COPY_MATRIX4x4(r, r1);
}

INLINE void PlusVector3(float* vector1, const float* vector2)
{
	vector1[0] = vector1[0] + vector2[0];
	vector1[1] = vector1[1] + vector2[1];
	vector1[2] = vector1[2] + vector2[2];
}

INLINE void MulVector3(float* vector1, const float* vector2)
{
	vector1[0] = vector1[0] * vector2[0];
	vector1[1] = vector1[1] * vector2[1];
	vector1[2] = vector1[2] * vector2[2];
}

INLINE void ScaleVector3(float* vector, float scalar)
{
	vector[0] *= scalar;
	vector[1] *= scalar;
	vector[2] *= scalar;
}

INLINE int CompareVector3(float* vector1, float* vector2)
{
	return FuzzyZero(vector1[0] - vector2[0])
		&& FuzzyZero(vector1[1] - vector2[1]) && FuzzyZero(vector1[2] - vector2[2]);
}

INLINE void PlusVector4(float* vector1, const float* vector2)
{
	vector1[0] = vector1[0] + vector2[0];
	vector1[1] = vector1[1] + vector2[1];
	vector1[2] = vector1[2] + vector2[2];
	vector1[3] = vector1[3] + vector2[3];
}

INLINE void SetInterpolateVector3(float* vector, float* v1, float* v2, float rt)
{
	float s = 1.0f - rt;
	vector[0] = s * v1[0] + rt * v2[0];
	vector[1] = s * v1[1] + rt * v2[1];
	vector[2] = s * v1[2] + rt * v2[2];
}

INLINE void MulVector4(float* vector1, const float* vector2)
{
	vector1[0] = vector1[0] * vector2[0];
	vector1[1] = vector1[1] * vector2[1];
	vector1[2] = vector1[2] * vector2[2];
	vector1[3] = vector1[3] * vector2[3];
}

INLINE void MulMatrix3x3(float matrix1[9], const float matrix2[9])
{
	float ret[9];
	ret[0] = matrix1[0] * matrix2[0] + matrix1[1] * matrix2[3] + matrix1[2] * matrix2[6];
	ret[1] = matrix1[0] * matrix2[1] + matrix1[1] * matrix2[4] + matrix1[2] * matrix2[7];
	ret[2] = matrix1[0] * matrix2[2] + matrix1[1] * matrix2[5] + matrix1[2] * matrix2[8];
	ret[3] = matrix1[3] * matrix2[0] + matrix1[4] * matrix2[3] + matrix1[5] * matrix2[6];
	ret[4] = matrix1[3] * matrix2[1] + matrix1[4] * matrix2[4] + matrix1[5] * matrix2[7];
	ret[5] = matrix1[3] * matrix2[2] + matrix1[4] * matrix2[5] + matrix1[5] * matrix2[8];
	ret[6] = matrix1[6] * matrix2[0] + matrix1[7] * matrix2[3] + matrix1[8] * matrix2[6];
	ret[7] = matrix1[6] * matrix2[1] + matrix1[7] * matrix2[4] + matrix1[8] * matrix2[7];
	ret[8] = matrix1[6] * matrix2[2] + matrix1[7] * matrix2[5] + matrix1[8] * matrix2[8];

	COPY_MATRIX3x3(matrix1, ret);
}

INLINE void MulMatrixVector3(float vector[3], float matrix[9])
{
	float ret[3];
	ret[0] = matrix[0] * vector[0] + matrix[1] * vector[1] + matrix[2] * vector[2];
	ret[1] = matrix[3] * vector[0] + matrix[4] * vector[1] + matrix[5] * vector[2];
	ret[2] = matrix[6] * vector[0] + matrix[7] * vector[1] + matrix[8] * vector[2];

	COPY_VECTOR3(vector, ret);
}

INLINE float CalcDeterminMatrix3x3(const float matrix[9])
{
	float det = matrix[0] * matrix[4] * matrix[8];
	det += matrix[3]*matrix[7]*matrix[2];
	det += matrix[6]*matrix[1]*matrix[5];
	det -= matrix[6]*matrix[4]*matrix[2];
	det -= matrix[3]*matrix[1]*matrix[8];
	det -= matrix[0]*matrix[7]*matrix[5];

	return det;
}

INLINE void RotateMatrix3x3(float matrix[9], const float quaternion[4])
{
	float d = QuaternionLength2(quaternion);
	float s = 2.0f / d;
	float xs = quaternion[0] * s;
	float ys = quaternion[1] * s;
	float zs = quaternion[2] * s;
	float wx = quaternion[3] * xs;
	float wy = quaternion[3] * ys;
	float wz = quaternion[3] * zs;
	float xx = quaternion[0] * xs;
	float xy = quaternion[0] * ys;
	float xz = quaternion[0] * zs;
	float yy = quaternion[1] * ys;
	float yz = quaternion[1] * zs;
	float zz = quaternion[2] * zs;

	matrix[0] = 1.0f - (yy + zz);
	matrix[1] = xy - wz;
	matrix[2] = xz + wy;
	matrix[3] = xy + wz;
	matrix[4] = 1.0f - (xx + zz);
	matrix[5] = yz - wx;
	matrix[6] = xz - wy;
	matrix[7] = yz + wx;
	matrix[8] = 1.0f - (xx + yy);
}

INLINE void InvertMatrix3x3(float matrix[9], float invert[9])
{
	float div = 1 / CalcDeterminMatrix3x3(matrix);
	float inv[9];

	inv[0] = (matrix[4]*matrix[8]-matrix[5]*matrix[7])*div;
	inv[1] = (matrix[2]*matrix[7]-matrix[1]*matrix[8])*div;
	inv[2] = (matrix[1]*matrix[5]-matrix[2]*matrix[4])*div;
	inv[3] = (matrix[5]*matrix[6]-matrix[3]*matrix[8])*div;
	inv[4] = (matrix[0]*matrix[8]-matrix[2]*matrix[6])*div;
	inv[5] = (matrix[2]*matrix[3]-matrix[0]*matrix[5])*div;
	inv[6] = (matrix[3]*matrix[7]-matrix[4]*matrix[6])*div;
	inv[7] = (matrix[1]*matrix[6]-matrix[0]*matrix[7])*div;
	inv[8] = (matrix[0]*matrix[4]-matrix[1]*matrix[3])*div;

	COPY_MATRIX3x3(invert, inv);
}

void Matrix3x3SetEulerZYX(
	float* matrix,
	float x,
	float y,
	float z
)
{
	float ci = cosf(x);
	float cj = cosf(y);
	float ch = cosf(z);
	float si = sinf(x);
	float sj = sinf(y);
	float sh = sinf(z);
	float cc = ci * ch;
	float cs = ci * sh;
	float sc = si * ch;
	float ss = si * sh;

	matrix[0] = cj * ch;
	matrix[1] = sj * sc - cs;
	matrix[2] = sj * cc + ss;
	matrix[3] = cj * sh;
	matrix[4] = sj * ss + cc;
	matrix[5] = sj * cs - sc;
	matrix[6] = -sj;
	matrix[7] = cj * si;
	matrix[8] = cj * ci;
}

INLINE void LoadIdentityMatrix4x4(float matrix[16])
{
	const float set_data[] = IDENTITY_MATRIX;
	COPY_MATRIX4x4(matrix, set_data);
}

INLINE void MulMatrix4x4(const float matrix1[16], const float matrix2[16], float result[16])
{
	float ret[16];
	ret[0] = matrix1[0] * matrix2[0] + matrix1[1] * matrix2[4]
		+ matrix1[2] * matrix2[8] + matrix1[3] * matrix2[12];
	ret[1] = matrix1[0] * matrix2[1] + matrix1[1] * matrix2[5]
		+ matrix1[2] * matrix2[9] + matrix1[3] * matrix2[13];
	ret[2] = matrix1[0] * matrix2[2] + matrix1[1] * matrix2[6]
		+ matrix1[2] * matrix2[10] + matrix1[3] * matrix2[14];
	ret[3] = matrix1[0] * matrix2[3] + matrix1[1] * matrix2[7]
		+ matrix1[2] * matrix2[11] + matrix1[3] * matrix2[15];
	ret[4] = matrix1[4] * matrix2[0] + matrix1[5] * matrix2[4]
		+ matrix1[6] * matrix2[8] + matrix1[7] * matrix2[12];
	ret[5] = matrix1[4] * matrix2[1] + matrix1[5] * matrix2[5]
		+ matrix1[6] * matrix2[9] + matrix1[7] * matrix2[13];
	ret[6] = matrix1[4] * matrix2[2] + matrix1[5] * matrix2[6]
		+ matrix1[6] * matrix2[10] + matrix1[7] * matrix2[14];
	ret[7] = matrix1[4] * matrix2[3] + matrix1[5] * matrix2[7]
		+ matrix1[6] * matrix2[11] + matrix1[7] * matrix2[15];
	ret[8] = matrix1[8] * matrix2[0] + matrix1[9] * matrix2[4]
		+ matrix1[10] * matrix2[8] + matrix1[11] * matrix2[12];
	ret[9] = matrix1[8] * matrix2[1] + matrix1[9] * matrix2[5]
		+ matrix1[10] * matrix2[9] + matrix1[11] * matrix2[13];
	ret[10] = matrix1[8] * matrix2[2] + matrix1[9] * matrix2[6]
		+ matrix1[10] * matrix2[10] + matrix1[11] * matrix2[14];
	ret[11] = matrix1[8] * matrix2[3] + matrix1[9] * matrix2[7]
		+ matrix1[10] * matrix2[11] + matrix1[11] * matrix2[15];
	ret[12] = matrix1[12] * matrix2[0] + matrix1[13] * matrix2[4]
		+ matrix1[14] * matrix2[8] + matrix1[15] * matrix2[12];
	ret[13] = matrix1[12] * matrix2[1] + matrix1[13] * matrix2[5]
		+ matrix1[14] * matrix2[9] + matrix1[15] * matrix2[13];
	ret[14] = matrix1[12] * matrix2[2] + matrix1[13] * matrix2[6]
		+ matrix1[14] * matrix2[10] + matrix1[15] * matrix2[14];
	ret[15] = matrix1[12] * matrix2[3] + matrix1[13] * matrix2[7]
		+ matrix1[14] * matrix2[11] + matrix1[15] * matrix2[15];

	COPY_MATRIX4x4(result, ret);
}

INLINE void MulMatrixVector4(const float matrix[16], const float vector[4], float result[4])
{
	float ret[4];

	ret[0] = matrix[0] * vector[0] + matrix[1] * vector[1] + matrix[2] * vector[2] + matrix[3] * vector[3];
	ret[1] = matrix[4] * vector[0] + matrix[5] * vector[1] + matrix[6] * vector[2] + matrix[7] * vector[3];
	ret[2] = matrix[8] * vector[0] + matrix[9] * vector[1] + matrix[10] * vector[2] + matrix[11] * vector[3];
	ret[3] = matrix[12] * vector[0] + matrix[13] * vector[1] + matrix[14] * vector[2] + matrix[15] * vector[3];

	COPY_VECTOR4(result, ret);
}

INLINE void MulMatrixVector4Transpose(const float matrix[16], const float vector[4], float result[4])
{
	float ret[4];

	ret[0] = matrix[0] * vector[0] + matrix[4] * vector[1] + matrix[8] * vector[2] + matrix[12] * vector[3];
	ret[1] = matrix[1] * vector[0] + matrix[5] * vector[1] + matrix[9] * vector[2] + matrix[13] * vector[3];
	ret[2] = matrix[2] * vector[0] + matrix[6] * vector[1] + matrix[10] * vector[2] + matrix[14] * vector[3];
	ret[3] = matrix[3] * vector[0] + matrix[7] * vector[1] + matrix[11] * vector[2] + matrix[15] * vector[3];

	COPY_VECTOR4(result, ret);
}

INLINE float CalcDetermineMatrix4x4(float matrix[16])
{
	return matrix[0]*matrix[5]*matrix[10]*matrix[15]+matrix[0]*matrix[6]*matrix[11]*matrix[13]+matrix[0]*matrix[7]*matrix[9]*matrix[14]
			+matrix[1]*matrix[4]*matrix[11]*matrix[14]+matrix[1]*matrix[6]*matrix[8]*matrix[15]+matrix[1]*matrix[7]*matrix[10]*matrix[12]
			+matrix[2]*matrix[4]*matrix[9]*matrix[15]+matrix[2]*matrix[5]*matrix[11]*matrix[12]+matrix[2]*matrix[7]*matrix[8]*matrix[13]
			+matrix[3]*matrix[4]*matrix[10]*matrix[13]+matrix[3]*matrix[5]*matrix[8]*matrix[14]+matrix[3]*matrix[6]*matrix[9]*matrix[12]
			-matrix[0]*matrix[5]*matrix[11]*matrix[14]-matrix[0]*matrix[6]*matrix[9]*matrix[15]-matrix[0]*matrix[7]*matrix[10]*matrix[13]
			-matrix[1]*matrix[4]*matrix[10]*matrix[15]-matrix[1]*matrix[6]*matrix[11]*matrix[12]-matrix[1]*matrix[7]*matrix[8]*matrix[14]
			-matrix[2]*matrix[4]*matrix[11]*matrix[13]-matrix[2]*matrix[5]*matrix[8]*matrix[15]-matrix[2]*matrix[7]*matrix[9]*matrix[12]
			-matrix[3]*matrix[4]*matrix[9]*matrix[14]-matrix[3]*matrix[5]*matrix[10]*matrix[12]-matrix[3]*matrix[6]*matrix[8]*matrix[13];
}

INLINE int InvertMatrix4x4(float matrix[16], float invert_matrix[16])
{
	float determine = CalcDetermineMatrix4x4(matrix);
	if(fabsf(determine) < FLT_MIN * 100)
	{
		return 1;
	}
	else
	{
		float inv_determine = 1.0f/determine;
		float temp_matrix[16];

		temp_matrix[0]  = inv_determine*(matrix[5]*matrix[10]*matrix[15]+matrix[6]*matrix[11]*matrix[13]+matrix[7]*matrix[9]*matrix[14]-matrix[5]*matrix[11]*matrix[14]-matrix[6]*matrix[9]*matrix[15]-matrix[7]*matrix[10]*matrix[13]);
		temp_matrix[1]  = inv_determine*(matrix[1]*matrix[11]*matrix[14]+matrix[2]*matrix[9]*matrix[15]+matrix[3]*matrix[10]*matrix[13]-matrix[1]*matrix[10]*matrix[15]-matrix[2]*matrix[11]*matrix[13]-matrix[3]*matrix[9]*matrix[14]);
		temp_matrix[2]  = inv_determine*(matrix[1]*matrix[6]*matrix[15]+matrix[2]*matrix[7]*matrix[13]+matrix[3]*matrix[5]*matrix[14]-matrix[1]*matrix[7]*matrix[14]-matrix[2]*matrix[5]*matrix[15]-matrix[3]*matrix[6]*matrix[13]);
		temp_matrix[3]  = inv_determine*(matrix[1]*matrix[7]*matrix[10]+matrix[2]*matrix[5]*matrix[11]+matrix[3]*matrix[6]*matrix[9]-matrix[1]*matrix[6]*matrix[11]-matrix[2]*matrix[7]*matrix[9]-matrix[3]*matrix[5]*matrix[10]);

		temp_matrix[4]  = inv_determine*(matrix[4]*matrix[11]*matrix[14]+matrix[6]*matrix[8]*matrix[15]+matrix[7]*matrix[10]*matrix[12]-matrix[4]*matrix[10]*matrix[15]-matrix[6]*matrix[11]*matrix[12]-matrix[7]*matrix[8]*matrix[14]);
		temp_matrix[5]  = inv_determine*(matrix[0]*matrix[10]*matrix[15]+matrix[2]*matrix[11]*matrix[12]+matrix[3]*matrix[8]*matrix[14]-matrix[0]*matrix[11]*matrix[14]-matrix[2]*matrix[8]*matrix[15]-matrix[3]*matrix[10]*matrix[12]);
		temp_matrix[6]  = inv_determine*(matrix[0]*matrix[7]*matrix[14]+matrix[2]*matrix[4]*matrix[15]+matrix[3]*matrix[6]*matrix[12]-matrix[0]*matrix[6]*matrix[15]-matrix[2]*matrix[7]*matrix[12]-matrix[3]*matrix[4]*matrix[14]);
		temp_matrix[7]  = inv_determine*(matrix[0]*matrix[6]*matrix[11]+matrix[2]*matrix[7]*matrix[8]+matrix[3]*matrix[4]*matrix[10]-matrix[0]*matrix[7]*matrix[10]-matrix[2]*matrix[4]*matrix[11]-matrix[3]*matrix[6]*matrix[8]);

		temp_matrix[8]  = inv_determine*(matrix[4]*matrix[9]*matrix[15]+matrix[5]*matrix[11]*matrix[12]+matrix[7]*matrix[8]*matrix[13]-matrix[4]*matrix[11]*matrix[13]-matrix[5]*matrix[8]*matrix[15]-matrix[7]*matrix[9]*matrix[12]);
		temp_matrix[9]  = inv_determine*(matrix[0]*matrix[11]*matrix[13]+matrix[1]*matrix[8]*matrix[15]+matrix[3]*matrix[9]*matrix[12]-matrix[0]*matrix[9]*matrix[15]-matrix[1]*matrix[11]*matrix[12]-matrix[3]*matrix[8]*matrix[13]);
		temp_matrix[10] = inv_determine*(matrix[0]*matrix[5]*matrix[15]+matrix[1]*matrix[7]*matrix[12]+matrix[3]*matrix[4]*matrix[13]-matrix[0]*matrix[7]*matrix[13]-matrix[1]*matrix[4]*matrix[15]-matrix[3]*matrix[5]*matrix[12]);
		temp_matrix[11] = inv_determine*(matrix[0]*matrix[7]*matrix[9]+matrix[1]*matrix[4]*matrix[11]+matrix[3]*matrix[5]*matrix[8]-matrix[0]*matrix[5]*matrix[11]-matrix[1]*matrix[7]*matrix[8]-matrix[3]*matrix[4]*matrix[9]);

		temp_matrix[12] = inv_determine*(matrix[4]*matrix[10]*matrix[13]+matrix[5]*matrix[8]*matrix[14]+matrix[6]*matrix[9]*matrix[12]-matrix[4]*matrix[9]*matrix[14]-matrix[5]*matrix[10]*matrix[12]-matrix[6]*matrix[8]*matrix[13]);
		temp_matrix[13] = inv_determine*(matrix[0]*matrix[9]*matrix[14]+matrix[1]*matrix[10]*matrix[12]+matrix[2]*matrix[8]*matrix[13]-matrix[0]*matrix[10]*matrix[13]-matrix[1]*matrix[8]*matrix[14]-matrix[2]*matrix[9]*matrix[12]);
		temp_matrix[14] = inv_determine*(matrix[0]*matrix[6]*matrix[13]+matrix[1]*matrix[4]*matrix[14]+matrix[2]*matrix[5]*matrix[12]-matrix[0]*matrix[5]*matrix[14]-matrix[1]*matrix[6]*matrix[12]-matrix[2]*matrix[4]*matrix[13]);
		temp_matrix[15] = inv_determine*(matrix[0]*matrix[5]*matrix[10]+matrix[1]*matrix[6]*matrix[8]+matrix[2]*matrix[4]*matrix[9]-matrix[0]*matrix[6]*matrix[9]-matrix[1]*matrix[4]*matrix[10]-matrix[2]*matrix[5]*matrix[8]);

		COPY_MATRIX4x4(invert_matrix, temp_matrix);
	}

	return 0;
}

void TransposeMatrix4x4(float matrix[16], float transpose[16])
{
	float temp_matrix[16];

	temp_matrix[0] = matrix[0];
	temp_matrix[1] = matrix[4];
	temp_matrix[2] = matrix[8];
	temp_matrix[3] = matrix[12];
	temp_matrix[4] = matrix[1];
	temp_matrix[5] = matrix[5];
	temp_matrix[6] = matrix[9];
	temp_matrix[7] = matrix[13];
	temp_matrix[8] = matrix[2];
	temp_matrix[9] = matrix[6];
	temp_matrix[10] = matrix[10];
	temp_matrix[11] = matrix[14];
	temp_matrix[12] = matrix[3];
	temp_matrix[13] = matrix[7];
	temp_matrix[14] = matrix[11];
	temp_matrix[15] = matrix[15];

	COPY_MATRIX4x4(transpose, temp_matrix);
}

INLINE void ScaleMatrix4x4(float matrix[16], float scale[3])
{
	float m[16];

	m[0] = scale[0];
	m[1] = 0;
	m[2] = 0;
	m[3] = 0;
	m[4] = 0;
	m[5] = scale[1];
	m[6] = 0;
	m[7] = 0;
	m[8] = 0;
	m[9] = 0;
	m[10] = scale[2];
	m[11] = 0;
	m[12] = 0;
	m[13] = 0;
	m[14] = 0;
	m[15] = 1;

	MulMatrix4x4(matrix, m, matrix);
}

void RotateMatrix4x4(float matrix[16], float rotate[3])
{
	float m[16];
	float radian;
	float sin_value, cos_value;

	radian = 6.283185308f / (360.0f / rotate[1]);
	sin_value = sinf(radian);
	cos_value = cosf(radian);

	m[0] = cos_value;
	m[1] = 0;
	m[2] = - sin_value;
	m[3] = 0;
	m[4] = 0;
	m[5] = 1;
	m[6] = 0;
	m[7] = 0;
	m[8] = sin_value;
	m[9] = 0;
	m[10] = cos_value;
	m[11] = 0;
	m[12] = 0;
	m[13] = 0;
	m[14] = 0;
	m[15] = 1;

	MulMatrix4x4(matrix, m, matrix);
}

void Matrix3x3SetRotation(float matrix[9], float quaternion[4])
{
	float d = QuaternionLength2(quaternion);
	float s = 2.0f / d;
	float xs = quaternion[0] * s;
	float ys = quaternion[1] * s;
	float zs = quaternion[2] * s;
	float wx = quaternion[3] * xs;
	float wy = quaternion[3] * ys;
	float wz = quaternion[3] * zs;
	float xx = quaternion[0] * xs;
	float xy = quaternion[0] * ys;
	float xz = quaternion[0] * zs;
	float yy = quaternion[1] * ys;
	float yz = quaternion[1] * zs;
	float zz = quaternion[2] * zs;

	matrix[0] = 1 - (yy + zz);
	matrix[1] = xy - wz;
	matrix[2] = xz + wy;
	matrix[3] = xy + wz;
	matrix[4] = 1 - (xx + zz);
	matrix[5] = yz - wx;
	matrix[6] = xz - wy;
	matrix[7] = yz + wx;
	matrix[8] = 1 - (xx + yy);
}

typedef struct _MATRIX_EULER
{
	float yaw;
	float pitch;
	float roll;
} MATRIX_EULER;

void Matrix3x3GetEulerZYX(
	float matrix[9],
	float* yaw,
	float* pitch,
	float* roll
)
{
	MATRIX_EULER euler_out;
	MATRIX_EULER euler_out2;

	if(fabsf(matrix[6]) >= 1)
	{
		float delta;

		euler_out.yaw = 0;
		euler_out2.yaw = 0;

		delta = atan2f(matrix[0], matrix[2]);
		if(matrix[6] > 0)
		{
			euler_out.pitch = (float)M_PI / 2.0f;
			euler_out2.pitch = (float)M_PI / 2.0f;
			euler_out.roll = euler_out.pitch + delta;
			euler_out2.roll = euler_out.pitch + delta;
		}
		else
		{
			euler_out.pitch = - (float)M_PI / 2.0f;
			euler_out2.pitch = - (float)M_PI / 2.0f;
			euler_out.roll = - euler_out.pitch + delta;
			euler_out2.roll = - euler_out.pitch + delta;
		}
	}
	else
	{
		float pitch_cos;
		float pitch2_cos;
		euler_out.pitch = - asinf(matrix[6]);
		euler_out2.pitch = (float)M_PI - euler_out.pitch;
		pitch_cos  = cosf(euler_out.pitch);
		pitch2_cos = cosf(euler_out2.pitch);

		euler_out.roll = atan2f(matrix[7] / pitch_cos,
			matrix[8] / pitch_cos);
		euler_out2.roll = atan2f(matrix[7] / pitch2_cos,
			matrix[8] / pitch2_cos);

		euler_out.yaw = atan2f(matrix[3] / pitch_cos,
			matrix[0] / pitch_cos);
		euler_out2.yaw = atan2f(matrix[3] / pitch2_cos,
			matrix[0] / pitch2_cos);
	}

	*yaw = euler_out.yaw;
	*pitch = euler_out.pitch;
	*roll = euler_out.roll;
}

int CheckHasExtension(const char* name)
{
	const char *version = glGetString(GL_VERSION);
	const char *p;

	p = version;
	while(*p != '\0')
	{
		if(isalnum(*p) != 0)
		{
			if(*p - '0' >= 3)
			{
				return GL_TRUE;
			}
			else
			{
				break;
			}
		}
	}

	if(name != NULL)
	{
		if(glewGetExtension(name) != GL_FALSE)
		{
			return GL_TRUE;
		}
	}

	return 0;
}

int CheckHasAnyExtension(const char** names)
{
	const char *version = glGetString(GL_VERSION);
	const char *p;
	int i;

	p = version;
	while(*p != '\0')
	{
		if(isalnum(*p) != 0)
		{
			if(*p - '0' >= 3)
			{
				return GL_TRUE;
			}
			else
			{
				break;
			}
		}
	}

	if(names != NULL)
	{
		for(i=0; names[i] != NULL; i++)
		{
			if(glewGetExtension(names[i]) != GL_FALSE)
			{
				return GL_TRUE;
			}
		}
	}

	return 0;
}

float Dot3DVector(const float* vector1, const float* vector2)
{
	return vector1[0] * vector2[0] + vector1[1] * vector2[1] + vector1[2] * vector2[2];
}

float Length2_3DVector(const float* vector)
{
	return Dot3DVector(vector, vector);
}

float Length3DVector(const float* vector)
{
	return sqrtf(Length2_3DVector(vector));
}

void Normalize3DVector(float* vector)
{
	float length = Length3DVector(vector);

	vector[0] /= length;
	vector[1] /= length;
	vector[2] /= length;
}

void Absolute3DVector(const float* vector, float* result)
{
	result[0] = fabsf(vector[0]);
	result[1] = fabsf(vector[1]);
	result[2] = fabsf(vector[2]);
}

int MaximumAxis3DVector(const float* vector)
{
	return vector[0] < vector[1] ? (vector[1] < vector[2] ? 2 : 1) : (vector[0] < vector[2] ? 2 : 0);
}

void SafeNormalize3DVector(float* vector)
{
	float abs_vector[3];
	int max_index;
	Absolute3DVector(vector, abs_vector);
	max_index = MaximumAxis3DVector(abs_vector);
	if(abs_vector[max_index] > 0)
	{
		float div;
		div = abs_vector[max_index];
		vector[0] /= div;
		vector[1] /= div;
		vector[2] /= div;
		div = Length3DVector(vector);
		vector[0] /= div;
		vector[1] /= div;
		vector[2] /= div;
		/*
		div = 1.0f / abs_vector[max_index];
		vector[0] *= div;
		vector[1] *= div;
		vector[2] *= div;
		div = 1.0f / Length3DVector(vector);
		vector[0] *= div;
		vector[1] *= div;
		vector[2] *= div;
		*/

		return;
	}

	vector[0] = 1;
	vector[1] = 0;
	vector[2] = 0;
}

void Cross3DVector(float* result, const float* vector1, const float* vector2)
{
	float local[2];
	local[0] = vector1[1] * vector2[2] - vector1[2] * vector2[1];
	local[1] = vector1[2] * vector2[0] - vector1[0] * vector2[2];
	result[2] = vector1[0] * vector2[1] - vector1[1] * vector2[0];
	result[0] = local[0];
	result[1] = local[1];
}

INLINE float QuaternionDot(const float* q1, const float* q2)
{
	return q1[0]*q2[0] + q1[1]*q2[1] + q1[2]*q2[2] + q1[3]*q2[3];
}

INLINE float QuaternionLength2(const float* quaternion)
{
	return QuaternionDot(quaternion, quaternion);
}

void QuaternionSlerp(float* result, const float* q1, const float* q2, const float t)
{
	float magnitude = sqrtf(
		QuaternionLength2(q1) * QuaternionLength2(q2));
	float product = QuaternionDot(q1, q2) / magnitude;

	if(fabs(product) < 1.0f)
	{
		const float sign = (float)((product < 0) ? -1 : 1);
		const float theta = acosf(sign * product);
		const float s1 = sinf(sign * t * theta);
		const float d = 1.0f / sinf(theta);
		const float s0 = sinf((1.0f - t) * theta);
		float local[3];

		local[0] = (q1[0] * s0 + q2[0] * s1) * d;
		local[1] = (q1[1] * s0 + q2[1] * s1) * d;
		local[2] = (q1[2] * s0 + q2[2] * s1) * d;
		result[3] = (q1[3] * s0 + q2[3] * s1) * d;
		result[0] = local[0];
		result[1] = local[1];
		result[2] = local[2];
	}
}

void QuaternionSetRotation(float* quaternion, const float* axis, const float angle)
{
	float d = Length3DVector(axis);
	float sin_value = sinf(angle * 0.5f) / d;
	quaternion[0] = axis[0] * sin_value;
	quaternion[1] = axis[1] * sin_value;
	quaternion[2] = axis[2] * sin_value;
	quaternion[3] = cosf(angle * 0.5f);
}

void QuaternionSetEuler(float* quaternion, float yaw, float pitch, float roll)
{
	float half_yaw = yaw * 0.5f;
	float half_pitch = pitch * 0.5f;
	float half_roll = roll * 0.5f;
	float cos_yaw = cosf(half_yaw);
	float sin_yaw = sinf(half_yaw);
	float cos_pitch = cosf(half_pitch);
	float sin_pitch = sinf(half_pitch);
	float cos_roll = cosf(half_roll);
	float sin_roll = sinf(half_roll);

	quaternion[0] = cos_roll * sin_pitch * cos_yaw + sin_roll * cos_pitch * sin_yaw;
	quaternion[1] = cos_roll * cos_pitch * sin_yaw - sin_roll * sin_pitch * cos_yaw;
	quaternion[2] = cos_roll * cos_pitch * cos_yaw - cos_roll * sin_pitch * sin_yaw;
	quaternion[3] = cos_roll * cos_pitch * cos_yaw + sin_roll * sin_pitch * sin_yaw;
}

void QuaternionSetEulerZYX(float* quaternion, float yaw, float pitch, float roll)
{
	float half_yaw = yaw * 0.5f;
	float half_pitch = pitch * 0.5f;
	float half_roll = roll * 0.5f;
	float cos_yaw = cosf(half_yaw);
	float sin_yaw = sinf(half_yaw);
	float cos_pitch = cosf(half_pitch);
	float sin_pitch = sinf(half_pitch);
	float cos_roll = cosf(half_roll);
	float sin_roll = sinf(half_roll);

	quaternion[0] = sin_roll * cos_pitch * cos_yaw - cos_roll * sin_pitch * sin_yaw;
	quaternion[1] = cos_roll * sin_pitch * cos_yaw + sin_roll * cos_pitch * sin_yaw;
	quaternion[2] = cos_roll * cos_pitch * sin_yaw - sin_roll * sin_pitch * cos_yaw;
	quaternion[3] = cos_roll * cos_pitch * cos_yaw + sin_roll * sin_pitch * sin_yaw;
}

void QuaternionNormalize(float* quaternion)
{
	float length = sqrtf(QuaternionLength2(quaternion));
	quaternion[0] /= length;
	quaternion[1] /= length;
	quaternion[2] /= length;
	quaternion[3] /= length;
}

void Perspective(float matrix[16], float fovy, float aspect, float z_near, float z_far)
{
	const float rad = fovy * (float)M_PI / 180.0f;
	float range = tanf(rad * 0.5f) * z_near;
	float left = range * aspect;
	float right = left;
	float bottom = - range;
	float top = range;

	left = - left;

	matrix[0] = (2 * z_near) / (right - left);
	matrix[1] = 0;
	matrix[2] = 0;
	matrix[3] = 0;
	matrix[4] = 0;
	matrix[5] = (2 * z_near) / (top - bottom);
	matrix[6] = 0;
	matrix[7] = 0;
	matrix[8] = 0;
	matrix[9] = 0;
	matrix[10] = - (z_far + z_near) / (z_far - z_near);
	matrix[11] = - 1;
	matrix[12] = 0;
	matrix[13] = 0;
	matrix[14] = - (2 * z_far * z_near) / (z_far - z_near);
	matrix[15] = 0;
}

void Unprojection(
	float vector[3],
	const float window[3],
	const float model[16],
	const float projection[16],
	const float viewport[4]
)
{
	float inverse[16];
	float local_vector[4];
	float object[4];

	MulMatrix4x4(model, projection, inverse);
	InvertMatrix4x4(inverse, inverse);
	local_vector[0] = ((window[0] - viewport[0]) / viewport[2]) * 2 - 1;
	local_vector[1] = ((window[1] - viewport[1]) / viewport[3]) * 2 - 1;
	local_vector[2] = window[2] * 2 - 1;
	local_vector[3] = 1;

	MulMatrixVector4Transpose(inverse, local_vector, object);
	object[0] /= object[3];
	object[1] /= object[3];
	object[2] /= object[3];

	COPY_VECTOR3(vector, object);
}

void InfinitePerspective(float* matrix, float fovy, float aspect, float z_near)
{
	float left, right;
	float top, bottom;

	top = z_near * tanf(fovy * (float)M_PI / 360.0f);
	bottom = - top;
	left = bottom * aspect;
	right = top * aspect;

	matrix[0] = (2.0f * z_near) / (right - left);
	matrix[1] = 0;
	matrix[2] = 0;
	matrix[3] = 0;

	matrix[4] = 0;
	matrix[5] = (2.0f * z_near) / (top - bottom);
	//matrix[9] = 0;
	matrix[6] = 0;
	matrix[7] = 0;

	matrix[8] = 0;
	matrix[9] = 0;
	matrix[10] = -1;
	matrix[11] = -1;

	matrix[12] = 0;
	matrix[13] = 0;
	matrix[14] = -2 * z_near;
	matrix[15] = 0;
}

void Ortho(float matrix[16], float left, float right, float bottom, float top, float near_plane, float far_plane)
{
	float width = right - left;
	float inv_height = top - bottom;
	float clip = far_plane - near_plane;

	if(left == right || bottom == top || near_plane == far_plane)
	{
		return;
	}

	matrix[0] = 2.0f / width;
	matrix[1] = 0.0f;
	matrix[2] = 0.0f;
	matrix[3] = 0.0f;
	matrix[4] = 0.0f;
	matrix[5] = 2.0f / inv_height;
	matrix[6] = 0.0f;
	matrix[7] = 0.0f;
	matrix[8] = 0.0f;
	matrix[9] = 0.0f;
	matrix[10] = - 2.0f / clip;
	matrix[11] = 0.0f;
	matrix[12] = - (left + right) / width;
	matrix[13] = - (top + bottom) / inv_height;
	matrix[14] = - (near_plane + far_plane) / clip;
	matrix[15] = 1.0f;
}

void RectangleSetTopLeft(float rectangle[4], float left, float top)
{
	rectangle[0] = left;
	rectangle[1] = top;
}

void RectangleSetSize(float rectangle[4], float width, float height)
{
	rectangle[2] = rectangle[0] + width;
	rectangle[3] = rectangle[1] + height;
}

int RectangleContainsPosition(float rectangle[4], float x, float y)
{
	return (x >= rectangle[0] && x <= rectangle[2]
		&& y >= rectangle[1] && y <= rectangle[3]);
}

void SetVertices2D(float vertices[8], float rect[4])
{
	vertices[0] = rect[0];
	vertices[1] = rect[1];
	vertices[2] = rect[2];
	vertices[3] = rect[1];
	vertices[4] = rect[2];
	vertices[5] = rect[3];
	vertices[6] = rect[0];
	vertices[7] = rect[3];
}

int GetTextFromStream(char* stream, char** text)
{
	int32 length;
	(void)memcpy(&length, stream, sizeof(length));
	if(length < 0)
	{
		return length;
	}
	stream += sizeof(length);
	*text = stream;

	return (int)length;
}

int GetSignedValue(uint8* stream, int length)
{
	switch(length)
	{
	case 1:
		return (int)((int8)*stream);
	case 2:
		{
			int16 *value = (int16*)stream;
			return (int)*value;
		}
	case 4:
		{
			int32 *value = (int32*)stream;
			return (int)*value;
		}
	case 8:
		{
			int64 *value = (int64*)stream;
			return (int)*value;
		}
	}

	return 0;
}

INLINE void ClampByteData(uint8* a, const uint8 lb, const uint8 ub)
{
	if(*a < lb)
	{
		*a = lb;
	}
	else if(*a > ub)
	{
		*a = ub;
	}
}

char** SplitString(char* str, const char* delim, int* num_strings)
{
	char **ret;
	char *token;
	POINTER_ARRAY *str_array;

	*num_strings = 0;
	token = strtok(str, delim);
	if(token == NULL)
	{
		return NULL;
	}

	str_array = PointerArrayNew(8);
	while(token != NULL)
	{
		PointerArrayAppend(str_array, token);
		token = strtok(NULL, delim);
	}

	ret = (char**)str_array->buffer;
	MEM_FREE_FUNC(str_array);
	return ret;
}

ght_uint32_t GetStringHash(ght_hash_key_t* key)
{
	static const unsigned int initial_fnv = 2166136261u;
	static const unsigned int fnv_multiple = 16777619u;

	unsigned int hash = initial_fnv;
	const char *str = (const char*)key->p_key;
	unsigned int i;

	for(i=0; i<key->i_size; i++)
	{
		hash = hash ^ (str[i]);
		hash = hash * fnv_multiple;
	}

	return (ght_uint32_t)hash;
}

void HashTableReleaseAll(ght_hash_table_t* table, void (*destroy_func)(void*))
{
	ght_iterator_t iterator;
	void *data;
	void *key;
	unsigned int key_size;

	if(destroy_func != NULL)
	{
		for(data = ght_first_keysize(table, &iterator, &key, &key_size); data != NULL;
			data = ght_next_keysize(table, &iterator, &key, &key_size))
		{
			destroy_func(ght_remove(table, key_size, key));
		}
	}
	else
	{
		for(data = ght_first_keysize(table, &iterator, &key, &key_size); data != NULL;
			data = ght_next_keysize(table, &iterator, &key, &key_size))
		{
			(void)ght_remove(table, key_size, key);
		}
	}
}

int aiMeshHasTextureCoords(const struct aiMesh* mesh, unsigned int index)
{
	if(index >= AI_MAX_NUMBER_OF_TEXTURECOORDS)
	{
		return FALSE;
	}
	return mesh->mTextureCoords[index] != NULL && mesh->mNumVertices > 0;
}

void DummyFuncNoReturn(void* dummy)
{
}

void DummyFuncNoReturn2(void* dummy1, void* dummy2)
{
}

int DummyFuncZeroReturn(void* dummy)
{
	return 0;
}

void* DummyFuncNullReturn(void* dummy)
{
	return NULL;
}

void* DummyFuncNullReturn2(void* dummy1, void* dummy2)
{
	return NULL;
}

int DummyFuncTrueReturn(void* dummy)
{
	return TRUE;
}

int DummyFuncMinusOneReturn(void* dummy)
{
	return -1;
}

float DummyFuncFloatZeroReturn(void* dummy)
{
	return 0.0f;
}

float DummyFuncFloatOneReturn(void* dummy)
{
	return 1.0f;
}

FLOAT_T DummyFuncFloatTOneReturn(void* dummy)
{
	return 1;
}

void DummyFuncGetZeroVector3(void* dummy, float* vector)
{
	vector[0] = vector[1] = vector[2] = 0;
}

void DummyFuncGetZeroVector4(void* dummy, float* vector)
{
	vector[0] = vector[1] = vector[2] = vector[3] = 0;
}

void DummyFuncGetIdentityQuaternion(void* dummy, float* quaternion)
{
	quaternion[0] = quaternion[1] = quaternion[2] = 0;
	quaternion[3] = 1;
}

void DummyFuncGetIdentityTransform(void* dummy, void* transform)
{
	BtTransformSetIdentity(transform);
}

void DummyFuncGetWhiteColor(void* dummy,float* color)
{
	color[0] = color[1] = color[2] = color[3] = 1;
}
