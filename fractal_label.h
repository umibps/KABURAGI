#ifndef _INCLUDED_FRACTAL_LABEL_H_
#define _INCLUDED_FRACTAL_LABEL_H_

typedef struct _FRACTAL_LABEL
{
	char *triangle, *transform, *variations,
		*colors, *random, *weight, *symmetry;
	char *linear, *sinusoidal, *spherical,
		*swirl, *horseshoe, *polar, *handkerchief,
		*heart, *disc, *spiral, *hyperbolic, *diamond,
		*ex, *julia, *bent, *waves, *fisheye, *popcorn;
	char *preserve_weight, *update, *update_immediately;
	char *adjust, *rendering, *gamma, *brightness,
		*vibrancy, *camera, *zoom, *option, *oversample,
		*filter_radius, *forced_symmetry, *type, *order,
		*none, *bilateral, *rotational, *dihedral,
		*mutation, *directions, *controls, *speed,
		*trend, *auto_zoom, *add, *del, *flip_horizontally,
		*flip_vertically, *flip_horizon_all,
		*flip_vertical_all, *new_seed, *reset, *seed;
} FRACTAL_LABEL;

#endif	// #ifndef _INCLUDED_FRACTAL_LABEL_H_
