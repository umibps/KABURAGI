#ifndef _INCLUDED_FRACTAL_POINT_H_
#define _INCLUDED_FRACTAL_POINT_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FRACTAL_NUM_VARS (18)
#define FRACTAL_NUM_XFORMS (12)
#define FRACTAL_EPS (1e-10)
#define FRACTAL_SUB_BATCH_SIZE (10000)

typedef enum _eFRACTAL_VALIATION
{
	FRACTAL_VALIATION_LINEAR,
	FRACTAL_VALIATION_SINUSOIDAL,
	FRACTAL_VALIATION_SPHERICAL,
	FRACTAL_VALIATION_SWIRL,
	FRACTAL_VALIATION_HORSESHOE,
	FRACTAL_VALIATION_POLAR,
	FRACTAL_VALIATION_HANDKERCHIEF,
	FRACTAL_VALIATION_HEART,
	FRACTAL_VALIATION_DISC,
	FRACTAL_VALIATION_SPIRAL,
	FRACTAL_VALIATION_HYPERBOLIC,
	FRACTAL_VALIATION_SQUARE,
	FRACTAL_VALIATION_EX,
	FRACTAL_VALIATION_JULIA,
	FRACTAL_VALIATION_BENT,
	FRACTAL_VALIATION_WAVES,
	FRACTAL_VALIATION_FISHEYE,
	FRACTAL_VALIATION_POPCORN,
	FRACTAL_VALIATION_RANDOM,
	NUM_FRACTAL_VALIATION
} eFRACTAL_VALIATION;

typedef enum _eFRACTAL_COMPABILITY_TYPE
{
	FRACTAL_COMPABILITY_TYPE_INTEGER,
	FRACTAL_COMPABILITY_TYPE_FLOAT,
	NUM_FRACTAL_COMPABILITY_TYPE
} eFRACTAL_COMPABILITY_TYPE;

typedef struct _FRACTAL_POINT_XFORM
{
	FLOAT_T vars[FRACTAL_NUM_VARS];
	FLOAT_T c[3][2];
	FLOAT_T density;
	FLOAT_T color;
	FLOAT_T symmetry;
	int int_color;
	int int_vars[FRACTAL_NUM_VARS];
	int ci00, ci01, ci10, ci11, ci20, ci21;

	int var_type;
} FRACTAL_POINT_XFORM;

typedef struct _FRACTAL_POINT
{
	int form_var_type;
	FRACTAL_POINT_XFORM xform[FRACTAL_NUM_XFORMS];
	eFRACTAL_VALIATION variation;
	uint8 color_map[256][3];
	int color_map_index;
	FLOAT_T time_value;
	FLOAT_T brightness;
	FLOAT_T contrast;
	FLOAT_T gamma;
	int width;
	int height;
	int spatial_oversample;
	char *name;
	char *nick;
	char *url;
	FLOAT_T center[2];
	FLOAT_T vibrancy;
	FLOAT_T hue_rotation;
	uint8 back_bround[4];
	FLOAT_T zoom;
	FLOAT_T pixels_per_unit;
	FLOAT_T spatial_filter_radius;
	FLOAT_T sample_density;
	int num_batches;
	int white_level;
	int color_map_inter;
	int symmetry;
	FLOAT_T pulse [2][2];
	FLOAT_T wiggle[2][2];

	int *prop_table;
	unsigned int var_flags;
} FRACTAL_POINT;

typedef struct _FRACTAL_CONTROL_POINT
{
	uint32 uint32_array[8193];
	FLOAT_T **points_array;
	int **int_points_array;
} FRACTAL_CONTROL_POINT;

// 関数のプロトタイプ宣言
extern void InitializeFractalPoint(FRACTAL_POINT* point, int width, int height);

extern void ReleaseFractalPoint(FRACTAL_POINT* point);

extern void ResizeFractalPoint(FRACTAL_POINT* point, int width, int height);

extern void CopyFractalPoint(FRACTAL_POINT* dst, const FRACTAL_POINT* src);

extern void FractalPointPreparePropTable(FRACTAL_POINT* point);

extern void FractalPointIterate(
	FRACTAL_POINT* point,
	int num_r_points,
	FLOAT_T (*points)[3]
);

extern void FractalPointIterateFloat(
	FRACTAL_POINT* point,
	int num_r_points,
	FLOAT_T (*points)[3]
);

extern void FractalPointRandomize(
	FRACTAL_POINT* point,
	int minimum,
	int maximum,
	int calculate,
	int compatibility
);

extern void FractalPointClear(FRACTAL_POINT* point);

extern int FractalPointGetNumXForms(FRACTAL_POINT* point);

extern void FractalPointEqualizeWeights(FRACTAL_POINT* point);

extern void FractalPointNormalizeWeights(FRACTAL_POINT* point);

extern void FractalPointComputeWeights(
	FRACTAL_POINT* point,
	FLOAT_T (*triangles)[3][2],
	int t
);

extern void FractalPointSetVariation(FRACTAL_POINT* point, eFRACTAL_VALIATION variation);

extern void FractalXFormTranslate(
	FRACTAL_POINT_XFORM* xform,
	FLOAT_T x,
	FLOAT_T y
);

extern void FractalXFormRotate(FRACTAL_POINT_XFORM* xform, FLOAT_T degrees);

extern void FractalXFormScale(FRACTAL_POINT_XFORM* xform, FLOAT_T scale);

extern void FractalXFormMultiply(
	FRACTAL_POINT_XFORM* xform,
	FLOAT_T a,
	FLOAT_T b,
	FLOAT_T c,
	FLOAT_T d
);

extern int AddSymmetry2FractalPoint(
	FRACTAL_POINT* point,
	int symmetry
);

extern FLOAT_T FractalPointSolve3(
	FLOAT_T x1,
	FLOAT_T x2,
	FLOAT_T x1h,
	FLOAT_T y1,
	FLOAT_T y2,
	FLOAT_T y1h,
	FLOAT_T z1,
	FLOAT_T z2,
	FLOAT_T z1h,
	FLOAT_T* a,
	FLOAT_T* b,
	FLOAT_T* e
);

extern void FractalPointGetXForms(
	FRACTAL_POINT* point,
	FLOAT_T (*triangles)[3][2],
	int t
);

extern void FractalPointInterpolate(
	FRACTAL_POINT* dst,
	FRACTAL_POINT* point1,
	FRACTAL_POINT* point2,
	FLOAT_T tm
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_FRACTAL_POINT_H_
