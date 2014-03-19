#ifndef _INCLUDED_FACE_H_
#define _INCLUDED_FACE_H_

#include "types.h"
#include "animation.h"
#include "ght_hash_table.h"

#define FACE_NAME_SIZE 20
#define FACE_MAX_VERTEX_ID 65536

typedef enum _eFACE_TYPE
{
	FACE_TYPE_BASE,
	FACE_TYPE_EYE_BLOW,
	FACE_TYPE_EYE,
	FACE_TYPE_LIP,
	FACE_TYPE_OTHER
} eFACE_TYPE;

typedef struct _FACE_VERTEX
{
	int id;
	int raw_id;
	float position[3];
} FACE_VERTEX;

typedef struct _FACE
{
	uint8 name[FACE_NAME_SIZE+1];
	uint8 english_name[FACE_NAME_SIZE+1];
	eFACE_TYPE type;
	STRUCT_ARRAY *vertices;
	float weight;
} FACE;

#define FACE_KEYFRAME_NAME_SIZE 15

typedef struct _FACE_KEYFRAME
{
	KEY_FRAME base_data;
	uint8 name[FACE_KEYFRAME_NAME_SIZE+1];
	float weight;
} FACE_KEYFRAME;

typedef struct _FACE_KEYFRAME_LIST
{
	FACE *face;
	STRUCT_ARRAY *keyframes;
	float weight;
	int last_index;
} FACE_KEYFRAME_LIST;

typedef enum _eFACE_ANIMATION_FLAGS
{
	FACE_ANIMATION_FLAG_ENABLE_NULL_FRAME = 0x01
} eFACE_ANIMATION_FLAGS;

typedef struct _FACE_ANIMATION
{
	ght_hash_table_t *name2keyframes;
	struct _PMD_MODEL *model;

	unsigned int flags;
} FACE_ANIMATION;

#endif	// #ifndef _INCLUDED_FACE_H_
