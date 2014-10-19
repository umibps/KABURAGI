#ifndef _INCLUDED_FRACTAL_COLOR_MAP_H_
#define _INCLUDED_FRACTAL_COLOR_MAP_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

//#define FRACTAL_NUM_COLOR_MAP 1
#define FRACTAL_NUM_COLOR_MAP 84

extern void FractalGetColorMap(int map_index, uint8 color[256][3]);

extern void FractalColorMapGetHueRotationMap(
	int map_index,
	FLOAT_T hue_rotation,
	uint8 color[256][3]
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_FRACTAL_COLOR_MAP_H_
