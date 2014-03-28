#ifndef _INCLUDED_PMD_MODEL_H_
#define _INCLUDED_PMD_MODEL_H_

#include "types.h"
#include "utils.h"
#include "ik.h"
#include "face.h"
#include "rigid_body.h"
#include "constraint.h"
#include "ght_hash_table.h"

#define PMD_MODEL_BUFFER_SIZE 32

#define MAX_CUSTOM_TEXTURE 11
#define MAX_CUSTOM_TEXTURE_NAME 100
#define PMD_MODEL_NAME_SIZE 20
#define PMD_MODEL_COMMENT_SIZE 256
#define BONE_CATEGORY_NAME_SIZE 50

typedef struct _SKIN_VERTEX
{
	float position[3];
	float normal[3];
	float texture_coord[4];
	float bone[3];
	float edge[3];
} SKIN_VERTEX;

typedef enum _ePMD_MODEL_FLAGS
{
	PMD_MODEL_FLAG_ENABLE_SIMULATION = 0x01,
	PMD_MODEL_FLAG_ENABLE_SOFTWARE_SKINNING = 0x02,
	PMD_MODEL_FLAG_ENABLE_TOON = 0x04,
	PMD_MODEL_FLAG_VISIBLE = 0x08
} ePMD_MODEL_FLAGS;

typedef struct _PMD_MODEL
{
	uint8 name[PMD_MODEL_NAME_SIZE+1];
	uint8 comment[PMD_MODEL_COMMENT_SIZE+1];
	uint8 english_name[PMD_MODEL_NAME_SIZE+1];
	uint8 english_comment[PMD_MODEL_COMMENT_SIZE+1];
	uint8 textures[MAX_CUSTOM_TEXTURE][MAX_CUSTOM_TEXTURE_NAME+1];
	STRUCT_ARRAY *vertices;
	POINTER_ARRAY *indices;
	STRUCT_ARRAY *materials;
	STRUCT_ARRAY *bones;
	STRUCT_ARRAY *iks;
	STRUCT_ARRAY *faces;
	STRUCT_ARRAY *faces_for_ui;
	STRUCT_ARRAY *rigid_bodies;
	STRUCT_ARRAY *constraints;
	BONE root_bone;
	BONE *base_bone;
	FACE *base_face;
	ght_hash_table_t *name2bone;
	ght_hash_table_t *name2face;
	STRUCT_ARRAY *motions;
	POINTER_ARRAY *bone_category_names;
	POINTER_ARRAY *bone_category_english_names;
	POINTER_ARRAY *skinning_transform;
	POINTER_ARRAY *bones_for_ui;
	WORD_ARRAY *fades_for_ui_indices;
	POINTER_ARRAY *rotate_bones;
	BONE **ordered_bones;
	BYTE_ARRAY *is_ik_simulated;
	STRUCT_ARRAY *skinned_vertices;
	void *world;
	void *user_data;
	uint16 *indices_pointer;
	uint16 *edge_indices_pointer;
	QUATERNION rotation_offset;
	uint8 edge_color[3];
	float edge_offset;
	float position_offset[3];
	float light_position[3];

	unsigned int flags;
} PMD_MODEL;

#endif	// #ifndef _INCLUDED_PMD_MODEL_H_
