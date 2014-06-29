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

	APPLICATION_WIDGETS widgets;

	DEFAULT_DATA default_data;

	INITIALIZE_DIRECTORY_PATHS paths;

	TEXT_ENCODE encode;
	void *transform;

	unsigned int flags;

	UI_LABEL label;

	char *application_path;
} APPLICATION;

#ifdef __cplusplus
extern "C" {
#endif

extern APPLICATION* MikuMikuGtkNew(
	int widget_width,
	int widget_height,
	int make_first_project,
	const char* initialize_file_path,
	const char* application_path
);

extern char* LoadShaderSource(
	APPLICATION* mikumiku,
	eSHADER_TYPE shader_type,
	MODEL_INTERFACE* model,
	void* user_data
);

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
	char* utf8_vertex_shader_file_path,
	char* utf8_fragment_shader_file_path,
	APPLICATION* application
);

extern void InitializeControlHandle(
	CONTROL_HANDLE* handle,
	int width,
	int height,
	PROJECT* project,
	APPLICATION* application
);

extern void ExecuteControlUndo(APPLICATION* application);
extern void ExecuteControlRedo(APPLICATION* application);

extern void ModelJoinWorld(MODEL_INTERFACE* model, WORLD* world);
extern void ModelLeaveWorld(MODEL_INTERFACE* model, WORLD* world);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_APPLICATION_H_

