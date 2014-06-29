// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string.h>
#include <gtk/gtk.h>
#include "shader_program.h"
#include "model.h"
#include "application.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

void MakeShaderProgram(SHADER_PROGRAM* program)
{
	if(program->program == 0)
	{
		program->program = glCreateProgram();
	}
}

void ReleaseShaderProgram(SHADER_PROGRAM* program)
{
	glDeleteProgram(program->program);
	program->program = 0;
}

void ShaderProgramBind(SHADER_PROGRAM* program)
{
	glUseProgram(program->program);
}

void ShaderProgramUnbind(SHADER_PROGRAM* program)
{
	glUseProgram(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

int ShaderProgramLink(SHADER_PROGRAM* program)
{
	GLint linked;
	glLinkProgram(program->program);
	glGetProgramiv(program->program, GL_LINK_STATUS, &linked);
	if(linked == 0)
	{
		GLint length;
		glGetProgramiv(program->program, GL_INFO_LOG_LENGTH, &length);
		if(length > 0)
		{
			program->message = (char*)MEM_REALLOC_FUNC(program->message, length);
			glGetProgramInfoLog(program->program, length, &length, program->message);
		}
		glDeleteProgram(program->program);
		return FALSE;
	}

	program->flags |= SHADER_PROGRAM_FLAG_LINKED;
	return TRUE;
}

int ShaderProgramAddShaderSource(SHADER_PROGRAM* program, const char* source, GLenum type)
{
	GLint compiled;
	GLuint shader = glCreateShader(type);
	glShaderSource(shader, 1, &source, 0);
	glCompileShader(shader);

	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
	if(compiled == 0)
	{
		GLint length;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
		if(length > 0)
		{
			program->message = (char*)MEM_REALLOC_FUNC(program->message, length);
			glGetShaderInfoLog(shader, length, &length, program->message);
		}
		glDeleteShader(shader);
		return FALSE;
	}

	glAttachShader(program->program, shader);
	glDeleteShader(shader);

	return TRUE;
}

void BaseShaderProgramBindAttributeLocations(BASE_SHADER_PROGRAM* program)
{
	glBindAttribLocation(program->base_data.program, MODEL_VERTEX_STRIDE, "inPosition");
}

void BaseShaderProgramGetUniformLocations(BASE_SHADER_PROGRAM* program)
{
	program->model_view_projection_uniform_location = glGetUniformLocation(
		program->base_data.program, "modelViewProjectionMatrix");
}

void InitializeBaseShaderProgram(BASE_SHADER_PROGRAM* program)
{
	(void)memset(program, 0, sizeof(*program));

	program->model_view_projection_uniform_location = -1;
	program->position_attribute_location = -1;

	program->bind_attribute_locations = (void (*)(void*))BaseShaderProgramBindAttributeLocations;
	program->get_uniform_locations = (void (*)(void*))BaseShaderProgramGetUniformLocations;
}

int BaseShaderProgramAddShaderSource(
	BASE_SHADER_PROGRAM* program,
	const char* source,
	GLenum type
)
{
	MakeShaderProgram(&program->base_data);
	return ShaderProgramAddShaderSource(&program->base_data, source, type);
}

int BaseShaderProgramLinkProgram(BASE_SHADER_PROGRAM* program)
{
	program->bind_attribute_locations((void*)program);
	if(ShaderProgramLink(&program->base_data) == FALSE)
	{
		return FALSE;
	}
	program->get_uniform_locations((void*)program);
	return TRUE;
}

void BaseShaderProgramSetModelViewProjectionMatrix(BASE_SHADER_PROGRAM* program, float* matrix)
{
	glUniformMatrix4fv(program->model_view_projection_uniform_location, 1, GL_FALSE, matrix);
}

void ObjectProgramBindAttributeLocations(OBJECT_PROGRAM* program)
{
	BaseShaderProgramBindAttributeLocations(&program->program);
	glBindAttribLocation(program->program.base_data.program, MODEL_NORMAL_STRIDE, "inNormal");
	glBindAttribLocation(program->program.base_data.program, MODEL_TEXTURE_COORD_STRIDE, "inTexCoord");
}

void ObjectProgramGetUniformLocations(OBJECT_PROGRAM* program)
{
	BaseShaderProgramGetUniformLocations(&program->program);
	program->normal_matrix_uniform_location = glGetUniformLocation(
		program->program.base_data.program, "normalMatrix");
	program->light_color_uniform_location = glGetUniformLocation(
		program->program.base_data.program, "lightColor");
	program->light_direction_uniform_location = glGetUniformLocation(
		program->program.base_data.program, "lightDirection");
	program->light_view_projection_matrix_uniform_location = glGetUniformLocation(
		program->program.base_data.program, "lightViewProjectionMatrix");
	program->has_main_texture_uniform_location = glGetUniformLocation(
		program->program.base_data.program, "hasMainTexture");
	program->has_depth_texture_uniform_location = glGetUniformLocation(
		program->program.base_data.program, "hasDepthTexture");
	program->main_texture_uniform_location = glGetUniformLocation(
		program->program.base_data.program, "mainTexture");
	program->depth_texture_uniform_location = glGetUniformLocation(
		program->program.base_data.program, "depthTexture");
	program->depth_texture_size_uniform_location = glGetUniformLocation(
		program->program.base_data.program, "depthTextureSize");
	program->enable_soft_shadow_uniform_location = glGetUniformLocation(
		program->program.base_data.program, "useSoftShadow");
	program->opacity_uniform_location = glGetUniformLocation(
		program->program.base_data.program, "opacity");
}

void InitializeObjectProgram(OBJECT_PROGRAM* program)
{
	(void)memset(program, 0, sizeof(*program));
	InitializeBaseShaderProgram(&program->program);

	program->normal_attribute_location = -1;
	program->texture_coord_attribute_location = -1;
	program->normal_matrix_uniform_location = -1;
	program->light_color_uniform_location = -1;
	program->light_direction_uniform_location = -1;
	program->light_view_projection_matrix_uniform_location = -1;
	program->has_main_texture_uniform_location = -1;
	program->has_depth_texture_uniform_location = -1;
	program->main_texture_uniform_location = -1;
	program->depth_texture_uniform_location = -1;
	program->depth_texture_size_uniform_location = -1;
	program->enable_soft_shadow_uniform_location = -1;
	program->opacity_uniform_location = -1;

	program->program.bind_attribute_locations = (void (*)(void*))ObjectProgramBindAttributeLocations;
	program->program.get_uniform_locations = (void (*)(void*))ObjectProgramGetUniformLocations;
}

void ObjectProgramSetLightColor(OBJECT_PROGRAM* program, uint8* color)
{
	float set_value[3] = {color[0] * DIV_PIXEL, color[1] * DIV_PIXEL, color[2] * DIV_PIXEL};
	glUniform3fv(program->light_color_uniform_location, 1, set_value);
}

void ObjectProgramSetLightDirection(OBJECT_PROGRAM* program, float* direction)
{
	glUniform3fv(program->light_direction_uniform_location, 1, direction);
}

void ObjectProgramSetLightViewProjectionMatrix(OBJECT_PROGRAM* program, float* matrix)
{
	glUniformMatrix4fv(program->light_view_projection_matrix_uniform_location, 1, GL_FALSE, matrix);
}

void ObjectProgramSetNormalMatrix(OBJECT_PROGRAM* program, float matrix[16])
{
	float matrix3x3[9] = {
		matrix[0], matrix[1], matrix[2],
		matrix[4], matrix[5], matrix[6],
		matrix[8], matrix[9], matrix[10]
	};
	glUniformMatrix3fv(program->normal_matrix_uniform_location, 1, GL_FALSE, matrix3x3);
}

void ObjectProgramSetDepthTexture(OBJECT_PROGRAM* program, GLuint name)
{
	if(name != 0)
	{
		glActiveTexture(GL_TEXTURE0 + 3);
		glBindTexture(GL_TEXTURE_2D, name);
		glUniform1i(program->depth_texture_uniform_location, 3);
		glUniform1i(program->has_depth_texture_uniform_location, 1);
	}
	else
	{
		glUniform1i(program->has_depth_texture_uniform_location, 0);
	}
}

void ObjectProgramSetDepthTextureSize(OBJECT_PROGRAM* program, float* size)
{
	glUniform2fv(program->depth_texture_size_uniform_location, 1, size);
}

void ObjectProgramSetOpacity(OBJECT_PROGRAM* program, float opacity)
{
	glUniform1f(program->opacity_uniform_location, opacity);
}

void ObjectProgramSetMainTexture(OBJECT_PROGRAM* program, TEXTURE_INTERFACE* texture)
{
	if(texture != NULL)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture->name);
		glUniform1i(program->main_texture_uniform_location, 0);
		glUniform1i(program->has_main_texture_uniform_location, 1);
	}
	else
	{
		glUniform1i(program->has_main_texture_uniform_location, 0);
	}
}

void ZPlotProgramGetUniformLocations(Z_PLOT_PROGRAM* program)
{
	BaseShaderProgramGetUniformLocations(&program->program);
	program->transform_uniform_location = glGetUniformLocation(program->program.base_data.program, "transformMatrix");
}

void InitializeZPlotProgram(Z_PLOT_PROGRAM* program)
{
	(void)memset(program, 0, sizeof(*program));
	InitializeBaseShaderProgram(&program->program);
	program->transform_uniform_location = -1;

	program->program.get_uniform_locations =
		(void (*)(void*))ZPlotProgramGetUniformLocations;
}

void PmxExtendedZPlotProgramBindAttributeLocations(PMX_EXTENDED_Z_PLOT_PROGRAM* program)
{
	BaseShaderProgramBindAttributeLocations(&program->program.program);
	glBindAttribLocation(program->program.program.base_data.program, MODEL_BONE_INDEX_STRIDE, "inBoneIndices");
	glBindAttribLocation(program->program.program.base_data.program, MODEL_BONE_WEIGHT_STRIDE, "inBoneWeights");
}

void PmxExtendedZPlotProgramGetUniformLocations(PMX_EXTENDED_Z_PLOT_PROGRAM* program)
{
	ZPlotProgramGetUniformLocations(&program->program);
	program->bone_matrices_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "boneMatrices");
}

void InitializePmxExtendedZPlotProgram(PMX_EXTENDED_Z_PLOT_PROGRAM* program)
{
	InitializeZPlotProgram(&program->program);
	program->bone_matrices_uniform_location = -1;

	program->program.program.bind_attribute_locations =
		(void (*)(void*))PmxExtendedZPlotProgramBindAttributeLocations;
	program->program.program.get_uniform_locations =
		(void (*)(void*))PmxExtendedZPlotProgramGetUniformLocations;
}

void PmxExtendedZPlotProgramSetBoneMatrices(PMX_EXTENDED_Z_PLOT_PROGRAM* program, float* value, size_t size)
{
	glUniformMatrix4fv(program->bone_matrices_uniform_location, (GLsizei)size, GL_FALSE, value);
}

void PmxEdgeProgramBindAttributeLocations(PMX_EDGE_PROGRAM* program)
{
	BaseShaderProgramBindAttributeLocations(&program->program);
	glBindAttribLocation(program->program.base_data.program, MODEL_NORMAL_STRIDE, "inNormal");
	glBindAttribLocation(program->program.base_data.program, MODEL_BONE_INDEX_STRIDE, "inBoneIndices");
	glBindAttribLocation(program->program.base_data.program, MODEL_BONE_WEIGHT_STRIDE, "inBoneWeights");
}

void PmxEdgeProgramGetUniformLocations(PMX_EDGE_PROGRAM* program)
{
	BaseShaderProgramGetUniformLocations(&program->program);
	program->color_uniform_location = glGetUniformLocation(program->program.base_data.program, "color");
	program->edge_size_uniform_location = glGetUniformLocation(program->program.base_data.program, "edgeSize");
	program->opacity_uniform_location = glGetUniformLocation(program->program.base_data.program, "opacity");
	program->bone_matrices_uniform_location = glGetUniformLocation(program->program.base_data.program, "boneMatrices");
}

void InitializePmxEdgeProgram(PMX_EDGE_PROGRAM* program)
{
	(void)memset(program, 0, sizeof(*program));
	InitializeBaseShaderProgram(&program->program);
	program->color_uniform_location = -1;
	program->edge_size_uniform_location = -1;
	program->opacity_uniform_location = -1;
	program->bone_matrices_uniform_location = -1;

	program->program.bind_attribute_locations =
		(void (*)(void*))PmxEdgeProgramBindAttributeLocations;
	program->program.get_uniform_locations =
		(void (*)(void*))PmxEdgeProgramGetUniformLocations;
}

void PmxEdgeProgramSetColor(PMX_EDGE_PROGRAM* program, float* color)
{
	glUniform4fv(program->color_uniform_location, 1, color);
}

void PmxEdgeProgramSetSize(PMX_EDGE_PROGRAM* program, float size)
{
	glUniform1f(program->edge_size_uniform_location, size);
}

void PmxEdgeProgramSetOpacity(PMX_EDGE_PROGRAM* program, float opacity)
{
	glUniform1f(program->opacity_uniform_location, opacity);
}

void PmxEdgeProgramSetBoneMatrices(PMX_EDGE_PROGRAM* program, float* value, size_t size)
{
	glUniformMatrix4fv(program->bone_matrices_uniform_location, (GLsizei)size, GL_FALSE, value);
}

void PmxShadowProgramBindAttributeLocations(PMX_SHADOW_PROGRAM* program)
{
	glBindAttribLocation(program->program.program.base_data.program, MODEL_BONE_INDEX_STRIDE, "inBoneIndices");
	glBindAttribLocation(program->program.program.base_data.program, MODEL_BONE_WEIGHT_STRIDE, "inBoneWeights");
}

void PmxShadowProgramGetUniformLocations(PMX_SHADOW_PROGRAM* program)
{
	ObjectProgramGetUniformLocations(&program->program);
	program->shadow_matrix_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "shadowMatrix");
	program->bone_matrices_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "boneMatrix");
}

void InitializePmxShadowProgram(PMX_SHADOW_PROGRAM* program)
{
	(void)memset(program, 0, sizeof(*program));
	InitializeObjectProgram(&program->program);
	program->shadow_matrix_uniform_location = -1;
	program->bone_matrices_uniform_location = -1;

	program->program.program.bind_attribute_locations =
		(void (*)(void*))PmxShadowProgramBindAttributeLocations;
	program->program.program.get_uniform_locations =
		(void (*)(void*))PmxShadowProgramGetUniformLocations;
}

void PmxShadowProgramSetBoneMatrices(PMX_SHADOW_PROGRAM* program, float* value, size_t size)
{
	glUniformMatrix4fv(program->bone_matrices_uniform_location, (GLsizei)size, GL_FALSE, value);
}

void PmxModelProgramBindAttributeLocations(PMX_MODEL_PROGRAM* program)
{
	ObjectProgramBindAttributeLocations(&program->program);
	glBindAttribLocation(program->program.program.base_data.program, MODEL_UVA1_STRIDE, "inUVA1");
	glBindAttribLocation(program->program.program.base_data.program, MODEL_BONE_INDEX_STRIDE, "inBoneIndices");
	glBindAttribLocation(program->program.program.base_data.program, MODEL_BONE_WEIGHT_STRIDE, "inBoneWeights");
}

void PmxModelProgramGetUniformLocations(PMX_MODEL_PROGRAM* program)
{
	ObjectProgramGetUniformLocations(&program->program);
	program->camera_position_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "cameraPosition");
	program->material_color_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "materialColor");
	program->material_specular_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "materialSpecular");
	program->material_shininess_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "materialShininess");
	program->main_texture_blend_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "mainTextureBlend");
	program->sphere_texture_blend_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "sphereTextureBlend");
	program->toon_texture_blend_uniform_locaion = glGetUniformLocation(
		program->program.program.base_data.program, "toonTextureBlend");
	program->sphere_texture_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "sphereTexture");
	program->has_sphere_texture_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "hasSphereTexture");
	program->is_sph_texture_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "isSPHTexture");
	program->is_spa_texture_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "isSPATexture");
	program->is_sub_texture_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "isSubTexture");
	program->toon_texture_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "toonTexture");
	program->has_toon_texture_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "hasToonTexture");
	program->use_toon_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "useToon");
	program->bone_matrices_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "boneMatrices");
}

void InitializePmxModelProgram(PMX_MODEL_PROGRAM* program)
{
	(void)memset(program, 0, sizeof(*program));
	InitializeObjectProgram(&program->program);
	program->camera_position_uniform_location = -1;
	program->material_color_uniform_location = -1;
	program->material_specular_uniform_location = -1;
	program->material_shininess_uniform_location = -1;
	program->main_texture_blend_uniform_location = -1;
	program->sphere_texture_blend_uniform_location = -1;
	program->toon_texture_blend_uniform_locaion = -1;
	program->has_sphere_texture_uniform_location = -1;
	program->is_sph_texture_uniform_location = -1;
	program->is_spa_texture_uniform_location = -1;
	program->is_sub_texture_uniform_location = -1;
	program->toon_texture_uniform_location = -1;
	program->has_toon_texture_uniform_location = -1;
	program->use_toon_uniform_location = -1;
	program->bone_matrices_uniform_location = -1;

	program->program.program.bind_attribute_locations =
		(void (*)(void*))PmxModelProgramBindAttributeLocations;
	program->program.program.get_uniform_locations =
		(void (*)(void*))PmxModelProgramGetUniformLocations;
}

void PmxModelProgramSetCameraPosition(PMX_MODEL_PROGRAM* program, float* position)
{
	glUniform3fv(program->camera_position_uniform_location, 1, position);
}

void PmxModelProgramSetToonEnable(PMX_MODEL_PROGRAM* program, int value)
{
	glUniform1i(program->use_toon_uniform_location, (value != FALSE) ? 1 : 0);
}

void PmxModelProgramSetMaterialColor(PMX_MODEL_PROGRAM* program, float* color)
{
	glUniform4fv(program->material_color_uniform_location, 1, color);
}

void PmxModelProgramSetMaterialSpecular(PMX_MODEL_PROGRAM* program, float* specular)
{
	glUniform3fv(program->material_specular_uniform_location, 1, specular);
}

void PmxModelProgramSetMaterialShininess(PMX_MODEL_PROGRAM* program, float shininess)
{
	glUniform1f(program->material_shininess_uniform_location, shininess);
}

void PmxModelProgramSetMainTextureBlend(PMX_MODEL_PROGRAM* program, float* color)
{
	glUniform4fv(program->main_texture_blend_uniform_location, 1, color);
}

void PmxModelProgramSetSphereTextureBlend(PMX_MODEL_PROGRAM* program, float* color)
{
	glUniform4fv(program->sphere_texture_blend_uniform_location, 1, color);
}

void PmxModelProgramSetToonTextureBlend(PMX_MODEL_PROGRAM* program, float* color)
{
	glUniform4fv(program->toon_texture_blend_uniform_locaion, 1, color);
}

static void PmxModelProgramEnableSphereTexture(PMX_MODEL_PROGRAM* program, TEXTURE_INTERFACE* texture)
{
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, texture->name);
	glUniform1i(program->sphere_texture_uniform_location, 1);
	glUniform1i(program->has_sphere_texture_uniform_location, 1);
}

void PmxModelProgramSetSphereTexture(
	PMX_MODEL_PROGRAM* program,
	eMATERIAL_SPHERE_TEXTURE_RENDER_MODE mode,
	TEXTURE_INTERFACE* texture
)
{
	if(texture != NULL)
	{
		switch(mode)
		{
		case MATERIAL_RENDER_NONE:
		default:
			glUniform1i(program->has_sphere_texture_uniform_location, 0);
			glUniform1i(program->is_sph_texture_uniform_location, 0);
			glUniform1i(program->is_spa_texture_uniform_location, 0);
			glUniform1i(program->is_sub_texture_uniform_location, 0);
			break;
		case MATERIAL_MULTI_TEXTURE:
			PmxModelProgramEnableSphereTexture(program, texture);
			glUniform1i(program->is_sph_texture_uniform_location, 1);
			glUniform1i(program->is_spa_texture_uniform_location, 0);
			glUniform1i(program->is_sub_texture_uniform_location, 0);
			break;
		case MATERIAL_ADD_TEXTURE:
			PmxModelProgramEnableSphereTexture(program, texture);
			glUniform1i(program->is_sph_texture_uniform_location, 0);
			glUniform1i(program->is_spa_texture_uniform_location, 1);
			glUniform1i(program->is_sub_texture_uniform_location, 0);
			break;
		case MATERIAL_SUB_TEXTURE:
			PmxModelProgramEnableSphereTexture(program, texture);
			glUniform1i(program->is_sph_texture_uniform_location, 0);
			glUniform1i(program->is_spa_texture_uniform_location, 0);
			glUniform1i(program->is_sub_texture_uniform_location, 1);
			break;
		}
	}
	else
	{
		glUniform1i(program->has_sphere_texture_uniform_location, 0);
	}
}

void PmxModelProgramSetToonTexture(PMX_MODEL_PROGRAM* program, TEXTURE_INTERFACE* texture)
{
	if(texture != NULL)
	{
		glActiveTexture(GL_TEXTURE0 + 2);
		glBindTexture(GL_TEXTURE_2D, texture->name);
		glUniform1i(program->toon_texture_uniform_location, 2);
		glUniform1i(program->has_toon_texture_uniform_location, 1);
	}
	else
	{
		glUniform1i(program->has_toon_texture_uniform_location, 0);
	}
}

void PmxModelProgramSetBoneMatrices(PMX_MODEL_PROGRAM* program, float* value, size_t size)
{
	glUniformMatrix4fv(program->bone_matrices_uniform_location, (GLsizei)size, GL_FALSE, value);
}

void InitializeAssetModelProgram(ASSET_MODEL_PROGRAM* program)
{
	(void)memset(program, 0, sizeof(*program));
	InitializeObjectProgram(&program->program);
	program->camera_position_uniform_location = -1;
	program->model_matrix_uniform_location = -1;
	program->view_projection_matrix_uniform_location = -1;
	program->material_color_uniform_location = -1;
	program->material_diffuse_uniform_location = -1;
	program->material_specular_uniform_location = -1;
	program->material_shininess_uniform_location = -1;
	program->has_sub_texture_uniform_location = -1;
	program->is_main_sphere_map_uniform_location = -1;
	program->is_sub_sphere_map_uniform_location = -1;
	program->is_main_additive_uniform_location = -1;
	program->is_sub_additive_uniform_location = -1;
	program->sub_texture_uniform_location = -1;
	program->program.program.get_uniform_locations =
		(void (*)(void*))AssetModelProgramGetUniformLocations;
}

void AssetModelProgramSetCameraPosition(ASSET_MODEL_PROGRAM* program, const float* position)
{
	glUniform3fv(program->camera_position_uniform_location, 1, position);
}

void AssetModelProgramSetModelMatrix(ASSET_MODEL_PROGRAM* program, const float* matrix)
{
	glUniformMatrix4fv(program->model_matrix_uniform_location, 1, GL_FALSE, matrix);
}

void AssetModelProgramSetViewProjectionMatrix(ASSET_MODEL_PROGRAM* program, const float* matrix)
{
	glUniformMatrix4fv(program->view_projection_matrix_uniform_location, 1, GL_FALSE, matrix);
}

void AssetModelProgramSetMaterialColor(ASSET_MODEL_PROGRAM* program, const float* color)
{
	glUniform3fv(program->material_color_uniform_location, 1, color);
}

void AssetModelProgramSetMaterialDiffuse(ASSET_MODEL_PROGRAM* program, const float* diffuse)
{
	glUniform4fv(program->material_diffuse_uniform_location, 1, diffuse);
}

void AssetModelProgramSetMaterialSpecular(ASSET_MODEL_PROGRAM* program, const float* specular)
{
	glUniform4fv(program->material_specular_uniform_location, 1, specular);
}

void AssetModelProgramSetMaterialShininess(ASSET_MODEL_PROGRAM* program, float shininess)
{
	glUniform1f(program->material_shininess_uniform_location, shininess);
}

void AssetModelProgramSetIsMainSphereMap(ASSET_MODEL_PROGRAM* program, int value)
{
	glUniform1i(program->is_main_sphere_map_uniform_location, value);
}

void AssetModelProgramSetIsSubSphereMap(ASSET_MODEL_PROGRAM* program, int value)
{
	glUniform1i(program->is_sub_sphere_map_uniform_location, value);
}

void AssetModelProgramSetIsMainAdditive(ASSET_MODEL_PROGRAM* program, int value)
{
	glUniform1i(program->is_main_additive_uniform_location, value);
}

void AssetModelProgramSetIsSubAdditive(ASSET_MODEL_PROGRAM* program, int value)
{
	glUniform1i(program->is_sub_additive_uniform_location, value);
}

void AssetModelProgramSetSubTexture(ASSET_MODEL_PROGRAM* program, TEXTURE_INTERFACE* texture)
{
	if(texture != NULL)
	{
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, texture->name);
		glUniform1i(program->sub_texture_uniform_location, 1);
		glUniform1i(program->has_sub_texture_uniform_location, 1);
	}
	else
	{
		glUniform1i(program->has_sub_texture_uniform_location, 0);
	}
}

void AssetModelProgramGetUniformLocations(ASSET_MODEL_PROGRAM* program)
{
	ObjectProgramGetUniformLocations(&program->program);
	program->camera_position_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "cameraPosition");
	program->model_matrix_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "modelMatrix");
	program->view_projection_matrix_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "viewProjectionMatrix");
	program->material_color_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "materialColor");
	program->material_diffuse_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "materialDiffuse");
	program->material_specular_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "materialSpecular");
	program->material_shininess_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "materialShininess");
	program->has_sub_texture_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "hasSubTexture");
	program->is_main_sphere_map_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "isMainSphereMap");
	program->is_sub_sphere_map_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "isSubSphereMap");
	program->is_main_additive_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "isMainAdditive");
	program->is_sub_additive_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "isSubAdditive");
	program->sub_texture_uniform_location = glGetUniformLocation(
		program->program.program.base_data.program, "subTexture");
}

void InitializeHandleProgram(HANDLE_PROGRAM* program)
{
	(void)memset(program, 0, sizeof(*program));

	program->base_data.model_view_projection_uniform_location = -1;
	program->color = -1;
	program->model_view_projection_matrix = -1;
}

int HandleProgramLink(HANDLE_PROGRAM* program)
{
	int ok;
	glBindAttribLocation(program->base_data.program, HANDLE_PROGRAM_VERTEX_TYPE_POSITION, "inPosition");
	ok = ShaderProgramLink(&program->base_data);
	if(ok != FALSE)
	{
		program->color = glGetUniformLocation(program->base_data.program, "color");
		program->model_view_projection_matrix = glGetUniformLocation(
			program->base_data.program, "modelViewProjectionMatrix");
	}
	return ok;
}

void HandleProgramAddShader(HANDLE_PROGRAM* program, const char* system_path, GLuint type)
{
	FILE *fp = fopen(system_path, "rb");
	char *source;

	MakeShaderProgram(&program->base_data);

	if(fp != NULL)
	{
		long length;

		(void)fseek(fp, 0, SEEK_END);
		length = ftell(fp);
		rewind(fp);
		source = (char*)MEM_ALLOC_FUNC(length);
		(void)fread(source, 1, length, fp);
		(void)fclose(fp);
	}
	else
	{
		source = LoadSubstituteShaderSource(system_path);
	}

	(void)ShaderProgramAddShaderSource(&program->base_data, source, type);

	MEM_FREE_FUNC(source);
}

void HandleProgramEnableAttributes(void)
{
	glEnableVertexAttribArray(HANDLE_PROGRAM_VERTEX_TYPE_POSITION);
}

void HandleProgramSetColor(
	HANDLE_PROGRAM* program,
	float red,
	float green,
	float blue,
	float alpha
)
{
	glUniform4f(program->color, red, green, blue, alpha);
}

void HandleProgramSetMatrix(HANDLE_PROGRAM* program, float matrix[16])
{
	glUniformMatrix4fv(program->model_view_projection_matrix, 1, GL_FALSE, matrix);
}

void InitializeTextureDrawHelperProgram(TEXTURE_DRAW_HELPER_PROGRAM* program)
{
	(void)memset(program, 0, sizeof(*program));

	program->model_view_projection_matrix = -1;
	program->main_texture = -1;
}

void LoadTextureDrawHelperProgram(
	TEXTURE_DRAW_HELPER_PROGRAM* program,
	const char* path,
	GLenum type
)
{
	char *source;
	FILE *fp = fopen(path, "rb");

	if(fp != NULL)
	{
		long size;
		(void)fseek(fp, 0, SEEK_END);
		size = ftell(fp);
		rewind(fp);
		source = (char*)MEM_ALLOC_FUNC(size);
		(void)fread(source, 1, size, fp);
		(void)fclose(fp);
	}
	else
	{
		source = LoadSubstituteShaderSource(path);
	}

	ShaderProgramAddShaderSource(&program->base_data, source, type);
	MEM_FREE_FUNC(source);
}

int TextureDrawHelperProgramLink(TEXTURE_DRAW_HELPER_PROGRAM* program)
{
	int ok;
	glBindAttribLocation(program->base_data.program, TEXTURE_DRAW_HELPER_PROGRAM_VERTEX_TYPE_POSITION, "inPosition");
	glBindAttribLocation(program->base_data.program, TEXTURE_DRAW_HELPER_PROGRAM_VERTEX_TYPE_TEXTURE_COORD, "inTexCoord");
	ok = ShaderProgramLink(&program->base_data);
	if(ok != FALSE)
	{
		program->model_view_projection_matrix = glGetUniformLocation(program->base_data.program,
			"modelViewProjectionMatrix");
		program->main_texture = glGetUniformLocation(program->base_data.program, "mainTexture");
	}

	return ok;
}

void TextureDrawHelperProgramSetUniformValues(
	TEXTURE_DRAW_HELPER_PROGRAM* program,
	const float matrix[16],
	GLuint texture_id
)
{
	glUniformMatrix4fv(program->model_view_projection_matrix, 4, GL_FALSE, matrix);
	glUniform1ui(program->main_texture, texture_id);
}

#ifdef __cplusplus
}
#endif
