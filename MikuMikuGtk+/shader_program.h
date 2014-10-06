#ifndef _INCLUDED_SHADER_PROGRAM_H_
#define _INCLUDED_SHADER_PROGRAM_H_

#include <GL/glew.h>
#include "types.h"
#include "texture.h"
#include "material.h"

typedef enum _eSHADER_PROGRAM_FLAGS
{
	SHADER_PROGRAM_FLAG_LINKED = 0x01
} eSHADER_PROGRAM_FLAGS;

typedef struct _SHADER_PROGRAM
{
	void *delegate;
	GLuint program;
	GLuint model_view_projection_uniform_location;
	GLuint position_attribute_location;
	unsigned int flags;
	char *message;
} SHADER_PROGRAM;

typedef struct _BASE_SHADER_PROGRAM
{
	SHADER_PROGRAM base_data;
	void (*bind_attribute_locations)(void*);
	void (*get_uniform_locations)(void*);
	GLint model_view_projection_uniform_location;
	GLint position_attribute_location;
} BASE_SHADER_PROGRAM;

typedef struct _OBJECT_PROGRAM
{
	BASE_SHADER_PROGRAM program;
	GLint normal_attribute_location;
	GLint texture_coord_attribute_location;
	GLint normal_matrix_uniform_location;
	GLint light_color_uniform_location;
	GLint light_direction_uniform_location;
	GLint light_view_projection_matrix_uniform_location;
	GLint has_main_texture_uniform_location;
	GLint has_depth_texture_uniform_location;
	GLint main_texture_uniform_location;
	GLint depth_texture_uniform_location;
	GLint depth_texture_size_uniform_location;
	GLint enable_soft_shadow_uniform_location;
	GLint opacity_uniform_location;
} OBJECT_PROGRAM;

typedef struct _Z_PLOT_PROGRAM
{
	BASE_SHADER_PROGRAM program;
	GLint transform_uniform_location;
} Z_PLOT_PROGRAM;

typedef struct _PMX_EXTENDED_Z_PLOT_PROGRAM
{
	Z_PLOT_PROGRAM program;
	GLint bone_matrices_uniform_location;
} PMX_EXTENDED_Z_PLOT_PROGRAM;

typedef struct _PMX_EDGE_PROGRAM
{
	BASE_SHADER_PROGRAM program;
	GLint color_uniform_location;
	GLint edge_size_uniform_location;
	GLint opacity_uniform_location;
	GLint bone_matrices_uniform_location;
} PMX_EDGE_PROGRAM;

typedef struct _PMX_SHADOW_PROGRAM
{
	OBJECT_PROGRAM program;
	GLint shadow_matrix_uniform_location;
	GLint bone_matrices_uniform_location;
} PMX_SHADOW_PROGRAM;

typedef struct _PMX_MODEL_PROGRAM
{
	OBJECT_PROGRAM program;
	GLint camera_position_uniform_location;
	GLint material_color_uniform_location;
	GLint material_specular_uniform_location;
	GLint material_shininess_uniform_location;
	GLint main_texture_blend_uniform_location;
	GLint sphere_texture_blend_uniform_location;
	GLint toon_texture_blend_uniform_locaion;
	GLint sphere_texture_uniform_location;
	GLint has_sphere_texture_uniform_location;
	GLint is_sph_texture_uniform_location;
	GLint is_spa_texture_uniform_location;
	GLint is_sub_texture_uniform_location;
	GLint toon_texture_uniform_location;
	GLint has_toon_texture_uniform_location;
	GLint use_toon_uniform_location;
	GLint bone_matrices_uniform_location;
} PMX_MODEL_PROGRAM;

typedef struct _ASSET_MODEL_PROGRAM
{
	OBJECT_PROGRAM program;
	GLint model_matrix_uniform_location;
	GLint view_projection_matrix_uniform_location;
	GLint camera_position_uniform_location;
	GLint material_color_uniform_location;
	GLint material_diffuse_uniform_location;
	GLint material_specular_uniform_location;
	GLint material_shininess_uniform_location;
	GLint has_sub_texture_uniform_location;
	GLint is_main_sphere_map_uniform_location;
	GLint is_sub_sphere_map_uniform_location;
	GLint is_main_additive_uniform_location;
	GLint is_sub_additive_uniform_location;
	GLint sub_texture_uniform_location;
} ASSET_MODEL_PROGRAM;

typedef enum _eHANDLE_PROGRAM_VERTEX_TYPE
{
	HANDLE_PROGRAM_VERTEX_TYPE_POSITION
} eHANDLE_PROGRAM_VERTEX_TYPE;

typedef struct _HANDLE_PROGRAM
{
	SHADER_PROGRAM base_data;
	GLint color;
	GLint model_view_projection_matrix;
} HANDLE_PROGRAM;

typedef enum _eTEXTURE_DRAW_HELPER_PROGRAM_VERTEX_TYPE
{
	TEXTURE_DRAW_HELPER_PROGRAM_VERTEX_TYPE_POSITION,
	TEXTURE_DRAW_HELPER_PROGRAM_VERTEX_TYPE_TEXTURE_COORD
} eTEXTURE_DRAW_HELPER_PROGRAM_VERTEX_TYPE;

typedef struct _TEXTURE_DRAW_HELPER_PROGRAM
{
	SHADER_PROGRAM base_data;
	GLint model_view_projection_matrix;
	GLint main_texture;
} TEXTURE_DRAW_HELPER_PROGRAM;

#ifdef __cplusplus
extern "C" {
#endif

extern void MakeShaderProgram(SHADER_PROGRAM* program);

extern void ReleaseShaderProgram(SHADER_PROGRAM* program);

extern void FreeShaderProgram(SHADER_PROGRAM* program);

extern void ShaderProgramBind(SHADER_PROGRAM* program);

extern void ShaderProgramUnbind(SHADER_PROGRAM* program);

extern int ShaderProgramLink(SHADER_PROGRAM* program);

extern int ShaderProgramAddShaderSource(SHADER_PROGRAM* program, const char* source, GLenum type);

extern void InitializeBaseShaderProgram(BASE_SHADER_PROGRAM* program);
extern int BaseShaderProgramAddShaderSource(
	BASE_SHADER_PROGRAM* program,
	const char* source,
	GLenum type
);
extern int BaseShaderProgramLinkProgram(BASE_SHADER_PROGRAM* program);
extern void BaseShaderProgramSetModelViewProjectionMatrix(BASE_SHADER_PROGRAM* program, float* matrix);

extern void InitializeObjectProgram(OBJECT_PROGRAM* program);
extern void ObjectProgramSetNormalMatrix(OBJECT_PROGRAM* program, float matrix[16]);
extern void ObjectProgramSetLightColor(OBJECT_PROGRAM* program, uint8* color);
extern void ObjectProgramSetLightDirection(OBJECT_PROGRAM* program, float* direction);
extern void ObjectProgramSetLightViewProjectionMatrix(OBJECT_PROGRAM* program, float* matrix);
extern void ObjectProgramSetDepthTexture(OBJECT_PROGRAM* program, GLuint name);
extern void ObjectProgramSetDepthTextureSize(OBJECT_PROGRAM* program, float* size);
extern void ObjectProgramSetOpacity(OBJECT_PROGRAM* program, float opacity);
extern void ObjectProgramSetMainTexture(OBJECT_PROGRAM* program, TEXTURE_INTERFACE* texture);

extern void InitializeZPlotProgram(Z_PLOT_PROGRAM* program);

extern void InitializePmxExtendedZPlotProgram(PMX_EXTENDED_Z_PLOT_PROGRAM* program);
extern void PmxExtendedZPlotProgramSetBoneMatrices(PMX_EXTENDED_Z_PLOT_PROGRAM* program, float* value, size_t size);

extern void InitializePmxEdgeProgram(PMX_EDGE_PROGRAM* program);
extern void PmxEdgeProgramSetColor(PMX_EDGE_PROGRAM* program, float* color);
extern void PmxEdgeProgramSetSize(PMX_EDGE_PROGRAM* program, float size);
extern void PmxEdgeProgramSetOpacity(PMX_EDGE_PROGRAM* program, float opacity);
extern void PmxEdgeProgramSetBoneMatrices(PMX_EDGE_PROGRAM* program, float* value, size_t size);

extern void InitializePmxShadowProgram(PMX_SHADOW_PROGRAM* program);
extern void PmxShadowProgramSetBoneMatrices(PMX_SHADOW_PROGRAM* program, float* value, size_t size);

extern void InitializePmxModelProgram(PMX_MODEL_PROGRAM* program);
extern void PmxModelProgramSetCameraPosition(PMX_MODEL_PROGRAM* program, float* position);
extern void PmxModelProgramSetToonEnable(PMX_MODEL_PROGRAM* program, int value);
extern void PmxModelProgramSetMaterialColor(PMX_MODEL_PROGRAM* program, float* color);
extern void PmxModelProgramSetMaterialSpecular(PMX_MODEL_PROGRAM* program, float* specular);
extern void PmxModelProgramSetMaterialShininess(PMX_MODEL_PROGRAM* program, float shininess);
extern void PmxModelProgramSetMainTextureBlend(PMX_MODEL_PROGRAM* program, float* color);
extern void PmxModelProgramSetSphereTextureBlend(PMX_MODEL_PROGRAM* program, float* color);
extern void PmxModelProgramSetToonTextureBlend(PMX_MODEL_PROGRAM* program, float* color);
extern void PmxModelProgramSetSphereTexture(
	PMX_MODEL_PROGRAM* program,
	eMATERIAL_SPHERE_TEXTURE_RENDER_MODE mode,
	TEXTURE_INTERFACE* texture
);
extern void PmxModelProgramSetToonTexture(PMX_MODEL_PROGRAM* program, TEXTURE_INTERFACE* texture);
extern void PmxModelProgramSetBoneMatrices(PMX_MODEL_PROGRAM* program, float* value, size_t size);

extern void InitializeAssetModelProgram(ASSET_MODEL_PROGRAM* program);
extern void AssetModelProgramSetCameraPosition(ASSET_MODEL_PROGRAM* program, const float* position);
extern void AssetModelProgramSetModelMatrix(ASSET_MODEL_PROGRAM* program, const float* matrix);
extern void AssetModelProgramSetViewProjectionMatrix(ASSET_MODEL_PROGRAM* program, const float* matrix);
extern void AssetModelProgramSetMaterialColor(ASSET_MODEL_PROGRAM* program, const float* color);
extern void AssetModelProgramSetMaterialDiffuse(ASSET_MODEL_PROGRAM* program, const float* diffuse);
extern void AssetModelProgramSetMaterialSpecular(ASSET_MODEL_PROGRAM* program, const float* specular);
extern void AssetModelProgramSetMaterialShininess(ASSET_MODEL_PROGRAM* program, float shininess);
extern void AssetModelProgramSetIsMainSphereMap(ASSET_MODEL_PROGRAM* program, int value);
extern void AssetModelProgramSetIsSubSphereMap(ASSET_MODEL_PROGRAM* program, int value);
extern void AssetModelProgramSetIsMainAdditive(ASSET_MODEL_PROGRAM* program, int value);
extern void AssetModelProgramSetIsSubAdditive(ASSET_MODEL_PROGRAM* program, int value);
extern void AssetModelProgramSetSubTexture(ASSET_MODEL_PROGRAM* program, TEXTURE_INTERFACE* texture);
extern void AssetModelProgramGetUniformLocations(ASSET_MODEL_PROGRAM* program);

extern void InitializeHandleProgram(HANDLE_PROGRAM* program);
extern int HandleProgramLink(HANDLE_PROGRAM* program);
extern void HandleProgramAddShader(HANDLE_PROGRAM* program, const char* system_path, GLuint type);
extern void HandleProgramEnableAttributes(void);
extern void HandleProgramSetColor(
	HANDLE_PROGRAM* program,
	float red,
	float green,
	float blue,
	float alpha
);
extern void HandleProgramSetMatrix(HANDLE_PROGRAM* program, float matrix[16]);

extern void InitializeTextureDrawHelperProgram(TEXTURE_DRAW_HELPER_PROGRAM* program);
extern void LoadTextureDrawHelperProgram(
	TEXTURE_DRAW_HELPER_PROGRAM* program,
	const char* path,
	GLenum type
);
extern int TextureDrawHelperProgramLink(TEXTURE_DRAW_HELPER_PROGRAM* program);
extern void TextureDrawHelperProgramSetUniformValues(
	TEXTURE_DRAW_HELPER_PROGRAM* program,
	const float matrix[16],
	GLuint texture_id
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_SHADER_PROGRAM_H_
