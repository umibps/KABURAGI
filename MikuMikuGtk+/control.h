#ifndef _INCLUDED_CONTROL_H_
#define _INCLUDED_CONTROL_H_

#include "bone.h"
#include "texture.h"
#include "shader_program.h"
#include "texture_draw_helper.h"

typedef enum _eEDIT_MODE
{
	EDIT_MODE_SELECT,
	EDIT_MODE_MOVE,
	EDIT_MODE_ROTATE,
	NUM_EDIT_MODE
} eEDIT_MODE;

typedef struct _HANDLE_STATIC_WORLD
{
	void *world;
	POINTER_ARRAY *bodies;
} HANDLE_STATIC_WORLD;

typedef enum _eHANDLE_FLAGS
{
	HANDLE_FLAG_NONE = 0x0,
	HANDLE_FLAG_ENABLE = 0x1,
	HANDLE_FLAG_DISABLE = 0x2,
	HANDLE_FLAG_MOVE = 0x4,
	HANDLE_FLAG_ROTATE = 0x8,
	HANDLE_FLAG_X = 0x10,
	HANDLE_FLAG_Y = 0x20,
	HANDLE_FLAG_Z = 0x40,
	HANDLE_FLAG_GLOBAL = 0x80,
	HANDLE_FLAG_LOCAL = 0x100,
	HANDLE_FLAG_VIEW = 0x200,
	HANDLE_FLAG_MODEL = 0x400,
	HANDLE_FLAG_OPERATION = HANDLE_FLAG_GLOBAL | HANDLE_FLAG_LOCAL | HANDLE_FLAG_MODEL,
	HANDLE_FLAG_VISIBLE_MOVE = HANDLE_FLAG_MOVE | HANDLE_FLAG_X | HANDLE_FLAG_Y | HANDLE_FLAG_Z | HANDLE_FLAG_MODEL,
	HANDLE_FLAG_VISIBLE_ROTATE = HANDLE_FLAG_ROTATE | HANDLE_FLAG_X | HANDLE_FLAG_Y | HANDLE_FLAG_Z | HANDLE_FLAG_MODEL,
	HANDLE_FLAG_VISIBLE_ALL = HANDLE_FLAG_MOVE | HANDLE_FLAG_ROTATE | HANDLE_FLAG_X | HANDLE_FLAG_Y | HANDLE_FLAG_Z | HANDLE_FLAG_MODEL,
	HANDLE_FLAG_VISIBLE = 0x800,
	HANDLE_FLAG_LOADED = 0x1000
} eHANDLE_FLAGS;

typedef struct _HANDLE_VERTEX
{
	float position[4];
	float normal[3];
} HANDLE_VERTEX;

typedef struct _HANDLE_TEXTURE
{
	TEXTURE_2D *texture;
	int size[2];
	float rect[4];
} HANDLE_TEXTURE;

typedef struct _HANDLE_IMAGE_TEXTURE
{
	HANDLE_TEXTURE enable_move;
	HANDLE_TEXTURE disable_move;
	HANDLE_TEXTURE enable_rotate;
	HANDLE_TEXTURE disable_rotate;
} HANDLE_IMAGE_TEXTURE;

typedef struct _HANDLE_MODEL
{
	struct _CONTROL_HANDLE *handle;
	VERTEX_BUNDLE *bundle;
	VERTEX_BUNDLE_LAYOUT *layout;
	STRUCT_ARRAY *vertices;
	UINT32_ARRAY *indices;
	void *body;
	int num_indices;
} HANDLE_MODEL;

typedef struct _ROTATION_HANDLE
{
	HANDLE_MODEL x, y, z;
} ROTATION_HANDLE;

typedef struct _TRANSLATION_HANDLE
{
	HANDLE_MODEL x, y, z;
	HANDLE_MODEL axis_x, axis_y, axis_z;
} TRANSLATION_HANDLE;

typedef struct _BONE_TRANSFORM_STATE
{
	float position[3];
	float rotation[4];
} BONE_TRANSFORM_STATE;

typedef struct _BONE_HANDLE_MOTION_STATE
{
	ght_hash_table_t *bone_transform_states;
	STRUCT_ARRAY *bone_transform_states_data;
	POINTER_ARRAY *selected_bones;
	void *view_transform;
	struct _CONTROL_HANDLE *handle;
} BONE_HANDLE_MOTION_STATE;

typedef struct _CONTROL_HANDLE
{
	HANDLE_STATIC_WORLD world;
	HANDLE_PROGRAM program;
	TEXTURE_DRAW_HELPER helper;
	HANDLE_MODEL *tracked_handle;
	BONE_HANDLE_MOTION_STATE motion_state;
	HANDLE_IMAGE_TEXTURE x, y, z;
	ROTATION_HANDLE rotation;
	TRANSLATION_HANDLE translation;
	HANDLE_TEXTURE global;
	HANDLE_TEXTURE local;
	HANDLE_TEXTURE view;
	BONE_INTERFACE *bone;
	POINTER_ARRAY *set_visibility_bodies;
	float prev_angle;
	float prev_position_3d[3];
	double prev_position_2d[2];
	struct _APPLICATION *application_context;
	struct _PROJECT *project_context;
	unsigned int constraint;
	unsigned int flags;
} CONTROL_HANDLE;

typedef enum _eCONTROL_FLAGS
{
	CONTROL_FLAG_LEFT_BUTTON_MASK = 0x01,
	CONTROL_FLAG_CENTER_BUTTON_MASK = 0x02,
	CONTROL_FLAG_RIGHT_BUTTON_MASK = 0x04,
	CONTROL_FLAG_IMAGE_HANDLE_RECT_INTERSECT = 0x08,
	CONTROL_FLAG_BONE_MOVE = 0x10,
	CONTROL_FLAG_BONE_ROTATE = 0x20
} eCONTROL_FLAG;

#ifdef _CONTROL
# undef _CONTROL
#endif

typedef struct _CONTROL
{
	CONTROL_HANDLE handle;
	BONE_INTERFACE *current_bone;
	eEDIT_MODE edit_mode;
	FLOAT_T click_origin[2];
	FLOAT_T delta[2];
	float last_distance;
	float last_bone_position[3];
	float before_bone_value[4];
	unsigned int handle_flags;
	unsigned int flags;
} CONTROL;

extern int HandleModeFromConstraint(CONTROL_HANDLE* handle);

extern void ResizeHandle(CONTROL_HANDLE* handle, int width, int height);

extern int ControlHandleTestHitModel(
	CONTROL_HANDLE* handle,
	float* ray_from,
	float* ray_to,
	int set_tracked,
	unsigned int* flags,
	float* pick
);

extern int ControlHandleTestHitImage(
	CONTROL_HANDLE* handle,
	float x,
	float y,
	int movable,
	int rotateable,
	int* flags,
	float rect[4]
);

extern void DrawImageHandles(CONTROL_HANDLE* handle, BONE_INTERFACE* bone);

extern void DrawMoveHandle(CONTROL_HANDLE* handle);

extern void DrawRotationHandle(CONTROL_HANDLE* handle);

extern void BoneMotionModelTranslateTo(
	BONE_HANDLE_MOTION_STATE* state,
	float translation[3],
	BONE_INTERFACE* bone,
	unsigned int flags
);

extern void ControlHandleSetVisibilityFlags(CONTROL_HANDLE* handle, unsigned int flags);

extern void ControlHandleSetBone(CONTROL_HANDLE* handle, BONE_INTERFACE* bone);

extern void ControlSetEditMode(CONTROL* control, eEDIT_MODE mode);

extern void ControlRotateBone(CONTROL* control, BONE_INTERFACE* bone, FLOAT_T diff[2]);

#endif	// #ifndef _INCLUDED_CONTROL_H_
