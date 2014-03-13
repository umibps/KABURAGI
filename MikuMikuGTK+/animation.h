#ifndef _INCLUDED_ANIMATION_H_
#define _INCLUDED_ANIMATION_H_

#include "types.h"
#include "utils.h"

typedef struct _KEY_FRAME
{
	float frame_index;
} KEY_FRAME;

typedef enum _eANIMATION_FLAGS
{
	ANIMATION_FLAGS_ENABLE_AUTOMATIC_REFRESH = 0x01
} eANIMATION_FLAGS;

typedef struct _ANIMATION
{
	STRUCT_ARRAY *frames;
	int last_index;
	float max_frame;
	float current_frame;
	float previous_frame;

	unsigned int flags;
} ANIMATION;

#endif	// #ifndef _INCLUDED_ANIMATION_H_
