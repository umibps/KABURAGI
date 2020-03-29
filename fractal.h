#ifndef _INCLUDED_FRACTAL_H_
#define _INCLUDED_FRACTAL_H_

#include "fractal_point.h"

#ifdef __cplusplus
extern "C" {
#endif

#define FRACTAL_RAND rand
#define FRACTAL_FLOAT_RAND() (rand() / (double)RAND_MAX)
#define FRACTAL_SRAND srand

typedef enum _eFRACTAL_FLAGS
{
	FRACTAL_FLAG_STOP = 0x01,
	FRACTAL_FLAG_USE_ACCULATION_BUFFER = 0x02,
	FRACTAL_FLAG_FIXED_REFERENCE = 0x04,
	FRACTAL_FLAG_KEEP_BACK_GROUND = 0x08,
	FRACTAL_FLAG_CHECK_FLAME_BACK = 0x10,
	FRACTAL_FLAG_USE_XFORM_COLOR = 0x20,
	FRACTAL_FLAG_CHECK_PRESERVE = 0x40,
	FRACTAL_FLAG_SELECT_LOCK = 0x80,
	FRACTAL_FLAG_UI_DISABLED = 0x100,
	FRACTAL_FLAG_USE_TIME_SEED = 0x200
} eFRACTAL_FLAGS;

typedef enum _eFRACTAL_SYMMETRY_TYPE
{
	FRACTAL_SYMMETRY_TYPE_BILATERAL = 1,
	FRACTAL_SYMMETRY_TYPE_ROTATIONAL,
	FRACTAL_SYMMETRY_TYPE_ROTATIONAL_AND_REFLECTIVE
} eFRACTAL_SYMMETRY_TYPE;

typedef struct _FRACTAL
{
	FRACTAL_POINT control_point;

	int oversample;
	int filter_width;
	FLOAT_T **filter;

	uint8 (*pixels)[4];
	int image_width;
	int image_height;
	int bucket_width;
	int bucket_height;
	int bucket_size;
	int gutter_width;

	FLOAT_T sample_density;

	int (*buckets)[4];
	int (*accumulate)[4];

	uint8 color_map[256][4];

	FLOAT_T back_ground[4];
	int vib_gam_n;
	FLOAT_T vibrancy;
	FLOAT_T gamma;

	FLOAT_T bounds[4];
	FLOAT_T size[2];
	FLOAT_T pp_ux, pp_uy;

	int nr_slices;
	int slice;
	int compatiblity;
	int random_start;
	int random_seed;
	int transforms;
	int max_transforms;
	int min_transforms;
	eFRACTAL_VALIATION variation;
	eFRACTAL_SYMMETRY_TYPE symmetry_type;
	int symmetry_order;
	int color_map_index;
	int random_gradient;

	unsigned int flags;
} FRACTAL;

typedef enum _eFRACTAL_MUTATION_FLAGS
{
	FRACTAL_MUTATION_FLAG_MAINTAIN_SYMMETRY = 0x01,
	FRACTAL_MUTATION_FLAG_SAME_NUM_TRANSFORM = 0x02
} eFRACTAL_MUTATION_FLAG;

typedef struct _FRACTAL_MUTAION
{
	FRACTAL *mutants;
	FRACTAL_POINT *control_points;
	int num_mutants;
	int max_transforms;
	int min_transforms;
	eFRACTAL_VALIATION trend;
	FLOAT_T time_value;
	int random_seed;
	unsigned int flags;
} FRACTAL_MUTATION;

// 関数のプロトタイプ宣言
extern void InitializeFractal(FRACTAL* fractal, int width, int height);

extern void ReleaseFractal(FRACTAL* fractal);

extern void ResizeFractal(FRACTAL* fractal, int width, int height);

extern void CopyFractal(FRACTAL* dst, FRACTAL* src);

extern void FractalRender(FRACTAL* fractal, int random_seed);

extern void FractalRandomizeControlPoint(
	FRACTAL* fractal,
	FRACTAL_POINT* point,
	int minimum,
	int maximum,
	int algorithm
);

extern void FractalSetVariation(FRACTAL* fractal, FRACTAL_POINT* point);

extern int FractalTrianglesFromControlPoint(
	FRACTAL* fractal,
	FRACTAL_POINT* point,
	FLOAT_T (*triangles)[3][2]
);

extern int FractalTransformAffine(
	FLOAT_T (*triangle)[2],
	FLOAT_T (*triangles)[3][2]
);

extern FLOAT_T FractalDistance(
	FLOAT_T x1,
	FLOAT_T y1,
	FLOAT_T x2,
	FLOAT_T y2
);

extern void InitializeFractalMutation(
	FRACTAL_MUTATION* mutation,
	FRACTAL* fractal,
	int num_mutants,
	int random_seed
);

extern void ReleaseFractalMutation(FRACTAL_MUTATION* mutation);

extern void FractalMutationInterpolate(FRACTAL_MUTATION* mutation);

extern void FractalMutationRandomSet(FRACTAL_MUTATION* mutation);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_FRACTAL_H_
