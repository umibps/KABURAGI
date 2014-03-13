#ifndef _INCLUDED_APPLICATION_H_
#define _INCLUDED_APPLICATION_H_

#include <stdio.h>
#include <gtk/gtk.h>
#include "system_depends.h"
#include "types.h"
#include "project.h"
#include "ui_label.h"
#include "texture.h"

#define DEFAULT_FPS 30

#define MAX_PROJECT_NUM 64

typedef struct _INITIALIZE_DIRECTORY_PATHS
{
	char *shader_directory_path;
	char *image_directory_path;
	char *icon_directory_path;
	char *model_directory_path;
} INITIALIZE_DIRECTORY_PATHS;

typedef struct _DEFAULT_DATA
{
	DEFAULT_BONE bone;
	DEFAULT_MATERIAL material;

	void *identity_transform;
} DEFAULT_DATA;

typedef struct _APPLICATION
{
	PROJECT *projects[MAX_PROJECT_NUM];
	int num_projects;
	int active_project;

	ght_hash_table_t *texture_chache_map;

	DEFAULT_DATA default_data;

	INITIALIZE_DIRECTORY_PATHS paths;

	TEXT_ENCODE encode;
	void *transform;

	unsigned int flags;

	UI_LABEL label;
} APPLICATION;

extern APPLICATION* MikuMikuGtkNew(
	int widget_width,
	int widget_height,
	int make_first_project,
	const char* initialize_file_path
);

extern char* LoadShaderSource(
	APPLICATION* mikumiku,
	eSHADER_TYPE shader_type,
	MODEL_INTERFACE* model,
	void* user_data
);

extern int UploadTexture(const char* path, TEXTURE_DATA_BRIDGE* bridge, APPLICATION* application);

extern MODEL_INTERFACE* LoadModel(
	APPLICATION* application,
	const char* system_path,
	const char* file_type
);

extern void InitializeDefaultBone(DEFAULT_BONE* bone, APPLICATION* application_context);

extern void InitializeDefaultMaterial(DEFAULT_MATERIAL* material, APPLICATION* application_context);

extern char* LoadSubstituteShaderSource(
	const char* file_path
);
extern uint8* LoadSubstituteModel(const char* path, size_t* data_size);

extern void LoadTextureDrawHelper(
	TEXTURE_DRAW_HELPER* helper,
	const float base_texture_coord[8],
	APPLICATION* application
);

extern void InitializeControlHandle(
	CONTROL_HANDLE* handle,
	int width,
	int height,
	PROJECT* project,
	APPLICATION* application
);
extern void LoadControlHandle(CONTROL_HANDLE* handle, APPLICATION* application);
extern void InitializeControl(
	CONTROL* control,
	int width,
	int height,
	PROJECT* project
);

#endif	// #ifndef _INCLUDED_APPLICATION_H_

