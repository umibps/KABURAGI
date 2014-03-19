// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string.h>
#include <float.h>
#include "scene.h"
#include "memory.h"
#include "render_engine.h"
#include "project.h"
#include "application.h"
#include "texture.h"
#include "memory.h"

PMX_RENDER_ENGINE* PmxRenderEngineNew(
	PROJECT* project_context,
	SCENE* scene,
	MODEL_INTERFACE* model,
	int is_vertex_skinning
)
{
	PMX_RENDER_ENGINE *ret;
	int num_materials;
	int i;

	ret = (PMX_RENDER_ENGINE*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	MEM_FREE_FUNC(model->get_materials(model, &num_materials));
	ret->project_context = project_context;
	ret->scene = scene;
	ret->interface_data.model = model;
	ret->buffer = VertexBundleNew();
	ret->aa_bb_min[0] = FLT_MAX;
	ret->aa_bb_min[1] = FLT_MAX;
	ret->aa_bb_min[2] = FLT_MAX;
	ret->aa_bb_max[0] = - FLT_MAX;
	ret->aa_bb_max[1] = - FLT_MAX;
	ret->aa_bb_max[2] = - FLT_MAX;
	ret->allocated_textures = ght_create(DEFAULT_BUFFER_SIZE);
	ret->material_textures = StructArrayNew(sizeof(PMX_RENDER_ENGINE_MATERIAL_TEXTURE), num_materials + 1);
	ret->flags |= PMX_RENDER_ENGINE_FLAG_CULL_FACE_STATE | PMX_RENDER_ENGINE_FLAG_UPDATE_EVEN;

	model->get_index_buffer((void*)model, (void**)&ret->index_buffer);
	model->get_static_vertex_buffer((void*)model, (void**)&ret->static_buffer);
	model->get_dynanic_vertex_buffer((void*)model, (void**)&ret->dynamic_buffer, ret->index_buffer);
	if(is_vertex_skinning != FALSE)
	{
		model->get_matrix_buffer((void*)model, (void**)&ret->matrix_buffer, ret->dynamic_buffer, ret->index_buffer);
	}

	switch(ret->index_buffer->get_type(ret->index_buffer))
	{
	case MODEL_INDEX8:
		ret->index_type = GL_UNSIGNED_BYTE;
		break;
	case MODEL_INDEX16:
		ret->index_type = GL_UNSIGNED_SHORT;
		break;
	case MODEL_INDEX32:
		ret->index_type = GL_UNSIGNED_INT;
		break;
	default:
		ret->index_type = GL_UNSIGNED_INT;
		break;
	}

	for(i=0; i<MAX_PMX_VERTEX_ARRAY_OBJECT_TYPE; i++)
	{
		ret->bundles[i] = VertexBundleLayoutNew();
	}

	ret->interface_data.type = RENDER_ENGINE_PMX;
	ret->interface_data.render_model = (void (*)(void*))PmxRenderEngineRenderModel;
	ret->interface_data.render_shadow = (void (*)(void*))PmxRenderEngineRenderShadow;
	ret->interface_data.render_edge = (void (*)(void*))PmxRenderEngineRenderEdge;
	ret->interface_data.render_z_plot = (void (*)(void*))PmxRenderEngineRenderZPlot;
	ret->interface_data.update = (void (*)(void*))PmxRenderEngineUpdate;
	ret->interface_data.get_effect = (EFFECT_INTERFACE* (*)(void*, eEFFECT_SCRIPT_ORDER_TYPE))PmxRenderEngineGetEffect;
	ret->interface_data.prepare_post_process = (void (*)(void*))DummyFuncNoReturn;
	ret->interface_data.perform_pre_process = (void (*)(void*))DummyFuncNoReturn;
	ret->interface_data.delete_func = (void (*)(void*))DeletePmxRenderEngine;

	return ret;
}

void DeletePmxRenderEngine(PMX_RENDER_ENGINE* engine)
{
	int i;
	engine->index_buffer->base.delete_func(engine->index_buffer);
	engine->static_buffer->base.delete_func(engine->static_buffer);
	engine->dynamic_buffer->base.delete_func(engine->dynamic_buffer);
	if(engine->matrix_buffer != NULL)
	{
		engine->matrix_buffer->base.delete_func(engine->matrix_buffer);
	}
	ReleaseShaderProgram((SHADER_PROGRAM*)&engine->edge_program);
	ReleaseShaderProgram((SHADER_PROGRAM*)&engine->model_program);
	ReleaseShaderProgram((SHADER_PROGRAM*)&engine->shadow_program);
	ReleaseShaderProgram((SHADER_PROGRAM*)&engine->z_plot_program);
	DeleteVertexBundle(&engine->buffer);
	for(i=0; i<MAX_PMX_VERTEX_ARRAY_OBJECT_TYPE; i++)
	{
		DeleteVertexBundleLayout(&engine->bundles[i]);
	}
	ght_finalize(engine->allocated_textures);
	StructArrayDestroy(&engine->material_textures, NULL);
	MEM_FREE_FUNC(engine);
}

int PmxRenderEngineCreateProgram(
	PMX_RENDER_ENGINE* engine,
	BASE_SHADER_PROGRAM* program,
	eSHADER_TYPE vertex_shader_type,
	eSHADER_TYPE vertex_skinning_shader_type,
	eSHADER_TYPE fragment_shader_type,
	void* user_data
)
{
	char *vertex_shader_source;
	char *fragment_shader_source;
	int ret;

	if((engine->flags & PMX_RENDER_ENGINE_FLAG_IS_VERTEX_SHADER_SKINNING) != 0)
	{
		vertex_shader_source = LoadShaderSource(engine->project_context->application_context,
			vertex_skinning_shader_type, engine->interface_data.model, user_data);
	}
	else
	{
		vertex_shader_source = LoadShaderSource(engine->project_context->application_context,
			vertex_shader_type, engine->interface_data.model, user_data);
	}

	fragment_shader_source = LoadShaderSource(engine->project_context->application_context,
		fragment_shader_type, engine->interface_data.model, user_data);

	BaseShaderProgramAddShaderSource(program, vertex_shader_source, GL_VERTEX_SHADER);
	BaseShaderProgramAddShaderSource(program, fragment_shader_source, GL_FRAGMENT_SHADER);
	ret = BaseShaderProgramLinkProgram(program);
	MEM_FREE_FUNC(vertex_shader_source);
	MEM_FREE_FUNC(fragment_shader_source);

	return ret;
}

int PmxRenderEngineUploadMaterials(PMX_RENDER_ENGINE* engine, void* user_data)
{
	TEXTURE_INTERFACE *t;
	MATERIAL_INTERFACE **materials;
	PMX_RENDER_ENGINE_MATERIAL_TEXTURE *textures =
		(PMX_RENDER_ENGINE_MATERIAL_TEXTURE*)engine->material_textures->buffer;
	TEXTURE_DATA_BRIDGE bridge = {NULL, TEXTURE_FLAG_TEXTURE_2D};
	char file_path[8192];
	int num_materials;
	int i;

	materials = (MATERIAL_INTERFACE**)engine->interface_data.model->get_materials(
		engine->interface_data.model, &num_materials);
	for(i=0; i<num_materials; i++)
	{
		PMX_RENDER_ENGINE_MATERIAL_TEXTURE *texture = &textures[i];
		MATERIAL_INTERFACE *material = materials[i];
		const int material_index = material->index;
		char *texture_path;

		bridge.flags = TEXTURE_FLAG_TEXTURE_2D | TEXTURE_FLAG_ASYNC_LOADING_TEXTURE;
		if((texture_path = material->main_texture) != NULL)
		{
			(void)strcpy(file_path, ((MODEL_INTERFACE*)engine->interface_data.model)->model_path);
			(void)strcat(file_path, "/");
			(void)strcat(file_path, texture_path);
			if(UploadTexture(file_path, &bridge, engine->project_context->application_context) != FALSE)
			{
				t = bridge.texture;
				texture->main_texture = t;
				(void)ght_insert(engine->allocated_textures, t, sizeof(t), &t);
			}
			else
			{
				return FALSE;
			}
		}
		if((texture_path = material->sphere_texture) != NULL)
		{
			(void)strcpy(file_path, ((MODEL_INTERFACE*)engine->interface_data.model)->model_path);
			(void)strcat(file_path, "/");
			(void)strcat(file_path, texture_path);

			if(UploadTexture(file_path, &bridge, engine->project_context->application_context) != FALSE)
			{
				t = bridge.texture;
				texture->sphere_texture = t;
				(void)ght_insert(engine->allocated_textures, t, sizeof(t), &t);
			}
			else
			{
				return FALSE;
			}
		}
		bridge.flags |= TEXTURE_FLAG_TOON_TEXTURE;
		if((material->flags & MATERIAL_FLAG_USE_SHARED_TOON_TEXTURE) != 0)
		{
			char buf[4096];
			int suceeded;
			(void)sprintf(buf, "%stoon%02d.bmp",
				engine->project_context->application_context->paths.image_directory_path,
				material->get_toon_texture_index((void*)material) + 1);
			suceeded = UploadTexture(buf, &bridge, engine->project_context->application_context);
			if(suceeded != FALSE)
			{
				t = bridge.texture;
				texture->toon_texture = t;
				(void)ght_insert(engine->allocated_textures, t, sizeof(t), &t);
			}
			else
			{
				return FALSE;
			}
		}
		else if((texture_path = material->toon_texture) != NULL)
		{
			(void)strcpy(file_path, ((MODEL_INTERFACE*)engine->interface_data.model)->model_path);
			(void)strcat(file_path, "/");
			(void)strcat(file_path, texture_path);

			if(UploadTexture(file_path, &bridge, engine->project_context->application_context) != FALSE)
			{
				t = bridge.texture;
				texture->toon_texture = t;
				(void)ght_insert(engine->allocated_textures, t, sizeof(t), &t);
			}
			else
			{
				return FALSE;
			}
		}
	}

	return TRUE;
}

void PmxRenderEngineGetVertexBundleType(
	PMX_RENDER_ENGINE* engine,
	ePMX_VERTEX_ARRAY_OBJECT_TYPE* vao,
	ePMX_VERTEX_BUFFER_OBJECT_TYPE* vbo
)
{
	if((engine->flags & PMX_RENDER_ENGINE_FLAG_UPDATE_EVEN) != 0)
	{
		*vao = PMX_VERTEX_ARRAY_OBJECT_TYPE_VERTEX_ARRAY_ODD;
		*vbo = PMX_VERTEX_BUFFER_OBJECT_TYPE_DYNAMIC_VERTEX_BUFFER_ODD;
	}
	else
	{
		*vao = PMX_VERTEX_ARRAY_OBJECT_TYPE_VERTEX_ARRAY_EVEN;
		*vbo = PMX_VERTEX_BUFFER_OBJECT_TYPE_DYNAMIC_VERTEX_BUFFER_EVEN;
	}
}

void PmxRenderEngineGetEdgeBundleType(
	PMX_RENDER_ENGINE* engine,
	ePMX_VERTEX_ARRAY_OBJECT_TYPE* vao,
	ePMX_VERTEX_BUFFER_OBJECT_TYPE* vbo
)
{
	if((engine->flags & PMX_RENDER_ENGINE_FLAG_UPDATE_EVEN) != 0)
	{
		*vao = PMX_VERTEX_ARRAY_OBJECT_TYPE_EDGE_VERTEX_ARRAY_ODD;
		*vbo = PMX_VERTEX_BUFFER_OBJECT_TYPE_DYNAMIC_VERTEX_BUFFER_ODD;
	}
	else
	{
		*vao = PMX_VERTEX_ARRAY_OBJECT_TYPE_EDGE_VERTEX_ARRAY_EVEN;
		*vbo = PMX_VERTEX_BUFFER_OBJECT_TYPE_DYNAMIC_VERTEX_BUFFER_EVEN;
	}
}

void PmxRenderEngineUnbindVertexBundle(PMX_RENDER_ENGINE* engine)
{
	ePMX_VERTEX_ARRAY_OBJECT_TYPE vao;
	ePMX_VERTEX_BUFFER_OBJECT_TYPE vbo;
	PmxRenderEngineGetEdgeBundleType(engine, &vao, &vbo);
	if(VertexBundleLayoutUnbind(engine->bundles[vao]) == FALSE)
	{
		VertexBundleUnbind(VERTEX_BUNDLE_VERTEX_BUFFER);
		VertexBundleUnbind(VERTEX_BUNDLE_INDEX_BUFFER);
	}
}

void PmxRenderEngineBindStaticVertexAttributePointers(PMX_RENDER_ENGINE* engine)
{
	size_t offset;
	size_t size;
	size = engine->static_buffer->base.get_stride_size(engine->static_buffer);
	offset = engine->static_buffer->base.get_stride_offset(engine->static_buffer, MODEL_TEXTURE_COORD_STRIDE);
	glVertexAttribPointer(MODEL_TEXTURE_COORD_STRIDE, 2, GL_FLOAT, GL_FALSE, (GLsizei)size, (void*)offset);
	glEnableVertexAttribArray(MODEL_TEXTURE_COORD_STRIDE);
	if((engine->flags & PMX_RENDER_ENGINE_FLAG_IS_VERTEX_SHADER_SKINNING) != 0)
	{
		offset = engine->static_buffer->base.get_stride_offset(
			engine->static_buffer, MODEL_BONE_INDEX_STRIDE);
		glVertexAttribPointer(MODEL_BONE_INDEX_STRIDE, 4, GL_FLOAT, GL_FALSE, (GLsizei)size, (void*)offset);
		glEnableVertexAttribArray(MODEL_BONE_INDEX_STRIDE);
		offset = engine->static_buffer->base.get_stride_offset(
			engine->static_buffer, MODEL_BONE_WEIGHT_STRIDE);
		glVertexAttribPointer(MODEL_BONE_WEIGHT_STRIDE, 4, GL_FLOAT, GL_FALSE, (GLsizei)size, (void*)offset);
		glEnableVertexAttribArray(MODEL_BONE_WEIGHT_STRIDE);
	}
}

void PmxRenderEngineBindDynamicVertexAtrritbutePointers(PMX_RENDER_ENGINE* engine)
{
	size_t offset;
	size_t size;
	offset = engine->dynamic_buffer->base.get_stride_offset(engine->dynamic_buffer, MODEL_VERTEX_STRIDE);
	size = engine->dynamic_buffer->base.get_stride_size(engine->dynamic_buffer);
	glVertexAttribPointer(MODEL_VERTEX_STRIDE, ((engine->flags & PMX_RENDER_ENGINE_FLAG_IS_VERTEX_SHADER_SKINNING) != 0) ? 4 : 3,
		GL_FLOAT, GL_FALSE, (GLsizei)size, (void*)offset);
	offset = engine->dynamic_buffer->base.get_stride_offset(engine->dynamic_buffer, MODEL_NORMAL_STRIDE);
	glVertexAttribPointer(MODEL_NORMAL_STRIDE, 3, GL_FLOAT, GL_FALSE, (GLsizei)size, (void*)offset);
	offset = engine->dynamic_buffer->base.get_stride_offset(engine->dynamic_buffer, MODEL_UVA1_STRIDE);
	glVertexAttribPointer(MODEL_UVA1_STRIDE, 4, GL_FLOAT, GL_FALSE, (GLsizei)size, (void*)offset);
	glEnableVertexAttribArray(MODEL_VERTEX_STRIDE);
	glEnableVertexAttribArray(MODEL_NORMAL_STRIDE);
	glEnableVertexAttribArray(MODEL_UVA1_STRIDE);
}

void PmxRenderEngineBindEdgeVertexAttributePointers(PMX_RENDER_ENGINE* engine)
{
	size_t size;
	size_t offset;

	size = engine->dynamic_buffer->base.get_stride_size(engine->dynamic_buffer);
	offset = engine->dynamic_buffer->base.get_stride_offset(
		engine->dynamic_buffer, ((engine->flags & PMX_RENDER_ENGINE_FLAG_IS_VERTEX_SHADER_SKINNING) != 0) ? MODEL_VERTEX_STRIDE : MODEL_EDGE_VERTEX_STRIDE);
	glVertexAttribPointer(MODEL_VERTEX_STRIDE, ((engine->flags & PMX_RENDER_ENGINE_FLAG_IS_VERTEX_SHADER_SKINNING) != 0) ? 4 : 3,
		GL_FLOAT, GL_FALSE, (GLsizei)size, (void*)offset);
	glEnableVertexAttribArray(MODEL_VERTEX_STRIDE);
	if((engine->flags & PMX_RENDER_ENGINE_FLAG_IS_VERTEX_SHADER_SKINNING) != 0)
	{
		offset = engine->dynamic_buffer->base.get_stride_offset(
			engine->dynamic_buffer, MODEL_NORMAL_STRIDE);
		glVertexAttribPointer(MODEL_NORMAL_STRIDE, 4, GL_FLOAT, GL_FALSE,
			(GLsizei)size, (void*)offset);
		glEnableVertexAttribArray(MODEL_NORMAL_STRIDE);
	}
}

void PmxRenderEngineBindVertexBundle(PMX_RENDER_ENGINE* engine)
{
	ePMX_VERTEX_ARRAY_OBJECT_TYPE vao;
	ePMX_VERTEX_BUFFER_OBJECT_TYPE vbo;
	PmxRenderEngineGetVertexBundleType(engine, &vao, &vbo);
	if(VertexBundleLayoutBind(engine->bundles[vao]) == FALSE)
	{
		VertexBundleBind(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, vbo);
		PmxRenderEngineBindDynamicVertexAtrritbutePointers(engine);
		VertexBundleBind(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, PMX_VERTEX_BUFFER_OBJECT_TYPE_STATIC_VERTEX_BUFFER);
		PmxRenderEngineBindStaticVertexAttributePointers(engine);
		VertexBundleBind(engine->buffer, VERTEX_BUNDLE_INDEX_BUFFER, PMX_VERTEX_BUFFER_OBJECT_TYPE_INDEX_BUFFER);
	}
}

void PmxRenderEngineBindEdgeBundle(PMX_RENDER_ENGINE* engine)
{
	ePMX_VERTEX_ARRAY_OBJECT_TYPE vao;
	ePMX_VERTEX_BUFFER_OBJECT_TYPE vbo;
	PmxRenderEngineGetEdgeBundleType(engine, &vao, &vbo);
	if(VertexBundleLayoutBind(engine->bundles[vao]) == FALSE)
	{
		VertexBundleBind(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, vbo);
		PmxRenderEngineBindEdgeVertexAttributePointers(engine);
		VertexBundleBind(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, PMX_VERTEX_BUFFER_OBJECT_TYPE_STATIC_VERTEX_BUFFER);
		PmxRenderEngineBindStaticVertexAttributePointers(engine);
		VertexBundleBind(engine->buffer, VERTEX_BUNDLE_INDEX_BUFFER, PMX_VERTEX_BUFFER_OBJECT_TYPE_INDEX_BUFFER);
	}
}

void PmxRenderEngineMakeVertexBundle(PMX_RENDER_ENGINE* engine, GLuint vbo)
{
	VertexBundleBind(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, vbo);
	PmxRenderEngineBindDynamicVertexAtrritbutePointers(engine);
	VertexBundleBind(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, PMX_VERTEX_BUFFER_OBJECT_TYPE_STATIC_VERTEX_BUFFER);
	PmxRenderEngineBindStaticVertexAttributePointers(engine);
	VertexBundleBind(engine->buffer, VERTEX_BUNDLE_INDEX_BUFFER, PMX_VERTEX_BUFFER_OBJECT_TYPE_INDEX_BUFFER);
	PmxRenderEngineUnbindVertexBundle(engine);
}

void PmxRenderEngineMakeEdgeBundle(PMX_RENDER_ENGINE* engine, GLuint vbo)
{
	VertexBundleBind(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, vbo);
	PmxRenderEngineBindEdgeVertexAttributePointers(engine);
	VertexBundleBind(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, PMX_VERTEX_BUFFER_OBJECT_TYPE_STATIC_VERTEX_BUFFER);
	PmxRenderEngineBindStaticVertexAttributePointers(engine);
	VertexBundleBind(engine->buffer, VERTEX_BUNDLE_INDEX_BUFFER, PMX_VERTEX_BUFFER_OBJECT_TYPE_INDEX_BUFFER);
	PmxRenderEngineUnbindVertexBundle(engine);
}

void PmxRenderEngineUpdate(PMX_RENDER_ENGINE* engine)
{
	ePMX_VERTEX_BUFFER_OBJECT_TYPE vbo =
		((engine->flags & PMX_RENDER_ENGINE_FLAG_UPDATE_EVEN) == 0) ? PMX_VERTEX_BUFFER_OBJECT_TYPE_DYNAMIC_VERTEX_BUFFER_EVEN : PMX_VERTEX_BUFFER_OBJECT_TYPE_DYNAMIC_VERTEX_BUFFER_ODD;
	void *address;

	if(((MODEL_INTERFACE*)engine->interface_data.model)->is_visible(engine->interface_data.model) == FALSE)
	{
		return;
	}

	VertexBundleBind(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, vbo);
	if((address = VertexBundleMap(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, 0,
		engine->dynamic_buffer->base.get_size(engine->dynamic_buffer))) != NULL)
	{
		CAMERA *camera = &engine->scene->camera;
		engine->dynamic_buffer->perform_transform(engine->dynamic_buffer, address,
			camera->position, engine->aa_bb_min, engine->aa_bb_max);
		if((engine->flags & PMX_RENDER_ENGINE_FLAG_IS_VERTEX_SHADER_SKINNING) != 0)
		{
			engine->matrix_buffer->update(engine->matrix_buffer, address);
		}
		VertexBundleUnmap(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, address);
	}
	VertexBundleUnbind(VERTEX_BUNDLE_VERTEX_BUFFER);

	((MODEL_INTERFACE*)engine->interface_data.model)->set_aa_bb(engine->interface_data.model, engine->aa_bb_min, engine->aa_bb_max);
	engine->flags ^= PMX_RENDER_ENGINE_FLAG_UPDATE_EVEN;
}

int PmxRenderEngineUpload(PMX_RENDER_ENGINE* engine, void* user_data)
{
	VERTEX_BUNDLE_LAYOUT *layout;
	void *address;
	int ret = TRUE;
	int vss = FALSE;

	vss = engine->flags & PMX_RENDER_ENGINE_FLAG_IS_VERTEX_SHADER_SKINNING;

	InitializePmxEdgeProgram(&engine->edge_program);
	InitializePmxModelProgram(&engine->model_program);
	InitializePmxShadowProgram(&engine->shadow_program);
	InitializePmxExtendedZPlotProgram(&engine->z_plot_program);

	if(PmxRenderEngineCreateProgram(engine, &engine->edge_program.program,
		SHADER_TYPE_EDGE_VERTEX, SHADER_TYPE_EDGE_WITH_SKINNING_VERTEX, SHADER_TYPE_EDGE_FRAGMENT, user_data) == FALSE)
	{
		return FALSE;
	}
	if(PmxRenderEngineCreateProgram(engine, &engine->model_program.program.program,
		SHADER_TYPE_MODEL_VERTEX, SHADER_TYPE_MODEL_WITH_SKINNING_VERTEX, SHADER_TYPE_MODEL_FRAGMENT, user_data) == FALSE)
	{
		return FALSE;
	}
	if(PmxRenderEngineCreateProgram(engine, &engine->shadow_program.program.program,
		SHADER_TYPE_SHADOW_VERTEX, SHADER_TYPE_SHADOW_WITH_SKINNING_VERTEX, SHADER_TYPE_SHADOW_FRAGMENT, user_data) == FALSE)
	{
		return FALSE;
	}
	if(PmxRenderEngineCreateProgram(engine, &engine->z_plot_program.program.program,
		SHADER_TYPE_Z_PLOT_VERTEX, SHADER_TYPE_Z_PLOT_WITH_SKINNING_VERTEX, SHADER_TYPE_Z_PLOT_FRAGMENT, user_data) == FALSE)
	{
		return FALSE;
	}
	if(PmxRenderEngineUploadMaterials(engine, user_data) == FALSE)
	{
		return FALSE;
	}

	MakeVertexBundle(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, PMX_VERTEX_BUFFER_OBJECT_TYPE_DYNAMIC_VERTEX_BUFFER_EVEN,
		GL_DYNAMIC_DRAW, 0, engine->dynamic_buffer->base.get_size(engine->dynamic_buffer));
	MakeVertexBundle(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, PMX_VERTEX_BUFFER_OBJECT_TYPE_DYNAMIC_VERTEX_BUFFER_ODD,
		GL_DYNAMIC_DRAW, 0, engine->dynamic_buffer->base.get_size(engine->dynamic_buffer));
	MakeVertexBundle(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, PMX_VERTEX_BUFFER_OBJECT_TYPE_STATIC_VERTEX_BUFFER,
		GL_STATIC_DRAW, 0, engine->static_buffer->base.get_size(engine->static_buffer));
	VertexBundleBind(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, PMX_VERTEX_BUFFER_OBJECT_TYPE_STATIC_VERTEX_BUFFER);
	address = VertexBundleMap(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, 0, engine->static_buffer->base.get_size(engine->static_buffer));
	engine->static_buffer->update(engine->static_buffer, address);
	VertexBundleUnmap(engine->buffer, VERTEX_BUNDLE_VERTEX_BUFFER, address);
	VertexBundleUnbind(VERTEX_BUNDLE_VERTEX_BUFFER);
	MakeVertexBundle(engine->buffer, VERTEX_BUNDLE_INDEX_BUFFER, PMX_VERTEX_BUFFER_OBJECT_TYPE_INDEX_BUFFER, GL_STATIC_DRAW,
		engine->index_buffer->bytes(engine->index_buffer), engine->index_buffer->base.get_size(engine->index_buffer));

	layout = engine->bundles[PMX_VERTEX_ARRAY_OBJECT_TYPE_VERTEX_ARRAY_EVEN];
	if(MakeVertexBundleLayout(layout) != FALSE && VertexBundleLayoutBind(layout) != FALSE)
	{
		PmxRenderEngineMakeVertexBundle(engine, PMX_VERTEX_BUFFER_OBJECT_TYPE_DYNAMIC_VERTEX_BUFFER_EVEN);
	}
	VertexBundleLayoutUnbind(layout);

	layout = engine->bundles[PMX_VERTEX_ARRAY_OBJECT_TYPE_VERTEX_ARRAY_ODD];
	if(MakeVertexBundleLayout(layout) != FALSE && VertexBundleLayoutBind(layout) != FALSE)
	{
		PmxRenderEngineMakeVertexBundle(engine, PMX_VERTEX_BUFFER_OBJECT_TYPE_DYNAMIC_VERTEX_BUFFER_ODD);
	}
	VertexBundleLayoutUnbind(layout);

	layout = engine->bundles[PMX_VERTEX_ARRAY_OBJECT_TYPE_EDGE_VERTEX_ARRAY_EVEN];
	if(MakeVertexBundleLayout(layout) != FALSE && VertexBundleLayoutBind(layout) != FALSE)
	{
		PmxRenderEngineMakeEdgeBundle(engine, PMX_VERTEX_BUFFER_OBJECT_TYPE_DYNAMIC_VERTEX_BUFFER_EVEN);
	}
	VertexBundleLayoutUnbind(layout);

	layout = engine->bundles[PMX_VERTEX_ARRAY_OBJECT_TYPE_EDGE_VERTEX_ARRAY_ODD];
	if(MakeVertexBundleLayout(layout) != FALSE && VertexBundleLayoutBind(layout) != FALSE)
	{
		PmxRenderEngineMakeEdgeBundle(engine, PMX_VERTEX_BUFFER_OBJECT_TYPE_DYNAMIC_VERTEX_BUFFER_ODD);
	}
	VertexBundleLayoutUnbind(layout);

	VertexBundleUnbind(VERTEX_BUNDLE_VERTEX_BUFFER);
	VertexBundleUnbind(VERTEX_BUNDLE_INDEX_BUFFER);

	((MODEL_INTERFACE*)engine->interface_data.model)->set_visible(engine->interface_data.model, TRUE);
	PmxRenderEngineUpdate(engine);
	PmxRenderEngineUpdate(engine);

	return ret;
}

void PmxRenderEngineRenderModel(PMX_RENDER_ENGINE* engine)
{
	PMX_RENDER_ENGINE_MATERIAL_TEXTURE *textures = (PMX_RENDER_ENGINE_MATERIAL_TEXTURE*)engine->material_textures->buffer;
	MATERIAL_INTERFACE **materials;
	float matrix[16];
	float diffuse[4];
	float specular[4];
	float material_ambient[4];
	float material_diffuse[4];
	float material_specular[4];
	float blend[4];
	float light_color[3];
	float opacity;
	GLuint texture_id = 0;
	size_t offset;
	size_t size;
	int num_materials;
	int num_indices;
	int has_model_transparent;
	int is_vertex_shader_skinning = engine->flags & PMX_RENDER_ENGINE_FLAG_IS_VERTEX_SHADER_SKINNING;
	int cull_face_state = engine->flags & PMX_RENDER_ENGINE_FLAG_CULL_FACE_STATE;
	int i;

	ShaderProgramBind(&engine->model_program.program.program.base_data);
	GetContextMatrix(matrix, engine->interface_data.model,
		MATRIX_FLAG_WORLD_MATRIX | MATRIX_FLAG_VIEW_MATRIX | MATRIX_FLAG_PROJECTION_MATRIX | MATRIX_FLAG_CAMERA_MATRIX,
		engine->project_context);
	BaseShaderProgramSetModelViewProjectionMatrix(
		&engine->model_program.program.program, matrix);
	GetContextMatrix(matrix, engine->interface_data.model,
		MATRIX_FLAG_WORLD_MATRIX | MATRIX_FLAG_VIEW_MATRIX | MATRIX_FLAG_CAMERA_MATRIX,
		engine->project_context);
	ObjectProgramSetNormalMatrix(&engine->model_program.program, matrix);
	GetContextMatrix(matrix, engine->interface_data.model,
		MATRIX_FLAG_WORLD_MATRIX | MATRIX_FLAG_VIEW_MATRIX | MATRIX_FLAG_PROJECTION_MATRIX | MATRIX_FLAG_LIGHT_MATRIX,
		engine->project_context);
	ObjectProgramSetLightViewProjectionMatrix(&engine->model_program.program, matrix);

	if(engine->scene->shadow_map != NULL)
	{
		float size[3];
		void *texture = engine->scene->shadow_map->get_texture(engine->scene->shadow_map);
		texture_id = (GLuint)texture;
		engine->scene->shadow_map->get_size(engine->scene->shadow_map, size);
		ObjectProgramSetDepthTextureSize(&engine->model_program.program, size);
	}

	ObjectProgramSetLightColor(&engine->model_program.program, engine->scene->light.vertex.color);
	ObjectProgramSetLightDirection(&engine->model_program.program, engine->scene->light.vertex.position);
	PmxModelProgramSetToonEnable(&engine->model_program, engine->scene->light.flags & LIGHT_FLAG_TOON_ENABLE);
	PmxModelProgramSetCameraPosition(&engine->model_program, engine->scene->camera.look_at);
	opacity = ((MODEL_INTERFACE*)engine->interface_data.model)->opacity;
	ObjectProgramSetOpacity(&engine->model_program.program, opacity);

	materials = (MATERIAL_INTERFACE**)((MODEL_INTERFACE*)engine->interface_data.model)->get_materials(engine->interface_data.model, &num_materials);
	has_model_transparent = !FuzzyZero(opacity - 1.0f);
	PmxRenderEngineBindVertexBundle(engine);
	light_color[0] = engine->scene->light.vertex.color[0] * DIV_PIXEL;
	light_color[1] = engine->scene->light.vertex.color[1] * DIV_PIXEL;
	light_color[2] = engine->scene->light.vertex.color[2] * DIV_PIXEL;
	size = engine->index_buffer->base.get_stride_size(engine->index_buffer);
	//offset = engine->dynamic_buffer->base.get_stride_offset(engine->dynamic_buffer, MODEL_VERTEX_STRIDE);
	offset = 0;

	for(i=0; i<num_materials; i++)
	{
		MATERIAL_INTERFACE *material = materials[i];
		PMX_RENDER_ENGINE_MATERIAL_TEXTURE *material_texture = &textures[i];

		material->get_ambient(material, material_ambient);
		material->get_diffuse(material, material_diffuse);
		material->get_specular(material, material_specular);
		diffuse[0] = material_ambient[0] + material_diffuse[0] * light_color[0];
		diffuse[1] = material_ambient[1] + material_diffuse[1] * light_color[1];
		diffuse[2] = material_ambient[2] + material_diffuse[2] * light_color[2];
		diffuse[3] = material_diffuse[3];
		specular[0] = material_specular[0] * light_color[0];
		specular[1] = material_specular[1] * light_color[1];
		specular[2] = material_specular[2] * light_color[2];
		specular[3] = 1;
		PmxModelProgramSetMaterialColor(&engine->model_program, diffuse);
		PmxModelProgramSetMaterialSpecular(&engine->model_program, specular);
		PmxModelProgramSetMaterialShininess(&engine->model_program, material->get_shininess(material));
		material->get_main_texture_blend(material, blend);
		PmxModelProgramSetMainTextureBlend(&engine->model_program, blend);
		material->get_sphere_texture_blend(material, blend);
		PmxModelProgramSetSphereTextureBlend(&engine->model_program, blend);
		material->get_toon_texture_blend(material, blend);
		PmxModelProgramSetToonTextureBlend(&engine->model_program, blend);
		ObjectProgramSetMainTexture(&engine->model_program.program, material_texture->main_texture);
		PmxModelProgramSetSphereTexture(&engine->model_program, material->sphere_texture_render_mode,
			material_texture->sphere_texture);
		PmxModelProgramSetToonTexture(&engine->model_program, material_texture->toon_texture);
		if(texture_id != 0 && material->is_self_shadow_enabled(material) != 0)
		{
			ObjectProgramSetDepthTexture(&engine->model_program.program, texture_id);
		}
		else
		{
			ObjectProgramSetDepthTexture(&engine->model_program.program, 0);
		}
		if(is_vertex_shader_skinning != FALSE)
		{
			PmxModelProgramSetBoneMatrices(&engine->model_program,
				engine->matrix_buffer->bytes(engine->matrix_buffer, i), engine->matrix_buffer->get_size(engine->matrix_buffer, i));
		}
		if(has_model_transparent == FALSE && cull_face_state != FALSE && (material->flags & MATERIAL_FLAG_DISABLE_CULLING) != 0)
		{
			//glDisable(GL_CULL_FACE);
			cull_face_state = FALSE;
		}
		else if(cull_face_state == FALSE && (material->flags & MATERIAL_FLAG_DISABLE_CULLING) == 0)
		{
			//glEnable(GL_CULL_FACE);
			cull_face_state = TRUE;
		}

		num_indices = material->get_index_range(material).count;
		glDrawElements(GL_TRIANGLES, num_indices, engine->index_type, (GLvoid*)offset);

		offset += num_indices * size;
	}

	PmxRenderEngineUnbindVertexBundle(engine);
	ShaderProgramUnbind(&engine->model_program.program.program.base_data);

	if(cull_face_state == FALSE)
	{
		glEnable(GL_CULL_FACE);
		cull_face_state = TRUE;
	}

	MEM_FREE_FUNC(materials);
}

void PmxRenderEngineRenderShadow(PMX_RENDER_ENGINE* engine)
{
	MATERIAL_INTERFACE **materials;
	float matrix[16];
	size_t offset = 0;
	size_t size;
	int is_vertex_shader_skinning = engine->flags & PMX_RENDER_ENGINE_FLAG_IS_VERTEX_SHADER_SKINNING;
	int num_materials;
	int i;

	ShaderProgramBind(&engine->shadow_program.program.program.base_data);
	GetContextMatrix(matrix, engine->interface_data.model,
		MATRIX_FLAG_WORLD_MATRIX | MATRIX_FLAG_VIEW_MATRIX | MATRIX_FLAG_PROJECTION_MATRIX | MATRIX_FLAG_SHADOW_MATRIX,
		engine->project_context);
	BaseShaderProgramSetModelViewProjectionMatrix(&engine->shadow_program.program.program, matrix);
	ObjectProgramSetLightColor(&engine->shadow_program.program, engine->scene->light.vertex.color);
	ObjectProgramSetLightDirection(&engine->shadow_program.program, engine->scene->light.vertex.position);

	materials = (MATERIAL_INTERFACE**)((MODEL_INTERFACE*)engine->interface_data.model)->get_materials(engine->interface_data.model, &num_materials);

	PmxRenderEngineBindEdgeBundle(engine);
	size = engine->index_buffer->base.get_stride_size(engine->index_buffer);
	glDisable(GL_CULL_FACE);
	for(i=0; i<num_materials; i++)
	{
		MATERIAL_INTERFACE *material = materials[i];
		const int num_indices = material->get_index_range(material).count;
		if(material->has_shadow(material))
		{
			if(is_vertex_shader_skinning != FALSE)
			{
				PmxShadowProgramSetBoneMatrices(&engine->shadow_program,
					engine->matrix_buffer->bytes(engine->matrix_buffer, i), engine->matrix_buffer->get_size(engine->matrix_buffer, i));
			}
			glDrawElements(GL_TRIANGLES, num_indices, engine->index_type, (GLvoid*)offset);
		}
		offset += num_indices * size;
	}

	PmxRenderEngineUnbindVertexBundle(engine);
	glEnable(GL_CULL_FACE);
	ShaderProgramUnbind(&engine->shadow_program.program.program.base_data);

	MEM_FREE_FUNC(materials);
}

void PmxRenderEngineRenderEdge(PMX_RENDER_ENGINE* engine)
{
	MATERIAL_INTERFACE **materials;
	FLOAT_T edge_scale_factor = 0;
	float matrix[16];
	float value[4];
	const float opacity = ((MODEL_INTERFACE*)engine->interface_data.model)->opacity;
	size_t offset = 0;
	size_t size;
	int is_vertex_shader_skinning = engine->flags & PMX_RENDER_ENGINE_FLAG_IS_VERTEX_SHADER_SKINNING;
	int num_materials;
	int is_opaque;
	int i;

	ShaderProgramBind(&engine->edge_program.program.base_data);
	GetContextMatrix(matrix, engine->interface_data.model,
		MATRIX_FLAG_WORLD_MATRIX | MATRIX_FLAG_VIEW_MATRIX | MATRIX_FLAG_PROJECTION_MATRIX | MATRIX_FLAG_CAMERA_MATRIX,
		engine->project_context);
	BaseShaderProgramSetModelViewProjectionMatrix(&engine->edge_program.program, matrix);
	PmxEdgeProgramSetOpacity(&engine->edge_program, opacity);

	materials = (MATERIAL_INTERFACE**)((MODEL_INTERFACE*)engine->interface_data.model)->get_materials(engine->interface_data.model, &num_materials);

	if(is_vertex_shader_skinning != FALSE)
	{
		edge_scale_factor =
			((MODEL_INTERFACE*)engine->interface_data.model)->get_edge_scale_factor(engine->interface_data.model, engine->scene->camera.position);
	}
	size = engine->index_buffer->base.get_stride_size(engine->index_buffer);
	is_opaque = FuzzyZero(opacity - 1.0f);
	if(is_opaque != FALSE)
	{
		glDisable(GL_BLEND);
	}
	glCullFace(GL_FRONT);
	PmxRenderEngineBindEdgeBundle(engine);
	for(i=0; i<num_materials; i++)
	{
		MATERIAL_INTERFACE *material = materials[i];
		const int num_indices = material->get_index_range(material).count;
		material->get_edge_color(material, value);
		PmxEdgeProgramSetColor(&engine->edge_program, value);
		if(material->is_edge_enabled(material))
		{
			if(is_vertex_shader_skinning != FALSE)
			{
				PmxEdgeProgramSetBoneMatrices(&engine->edge_program,
					engine->matrix_buffer->bytes(engine->matrix_buffer, i), engine->matrix_buffer->get_size(engine->matrix_buffer, i));
				PmxEdgeProgramSetSize(&engine->edge_program, (float)(material->get_edge_size(material) * edge_scale_factor));
			}
			glDrawElements(GL_TRIANGLES, num_indices, engine->index_type, (GLvoid*)offset);
		}
		offset += num_indices * size;
	}
	PmxRenderEngineUnbindVertexBundle(engine);
	glCullFace(GL_BACK);
	if(is_opaque != FALSE)
	{
		glEnable(GL_BLEND);
	}
	ShaderProgramUnbind(&engine->edge_program.program.base_data);

	MEM_FREE_FUNC(materials);
}

void PmxRenderEngineRenderZPlot(PMX_RENDER_ENGINE* engine)
{
	MATERIAL_INTERFACE **materials;
	float matrix[16];
	size_t offset = 0;
	size_t size;
	int is_vertex_shader_skinning = engine->flags & PMX_RENDER_ENGINE_FLAG_IS_VERTEX_SHADER_SKINNING;
	int num_materials;
	int i;

	ShaderProgramBind(&engine->z_plot_program.program.program.base_data);
	GetContextMatrix(matrix, engine->interface_data.model,
		MATRIX_FLAG_WORLD_MATRIX | MATRIX_FLAG_VIEW_MATRIX | MATRIX_FLAG_PROJECTION_MATRIX | MATRIX_FLAG_LIGHT_MATRIX,
		engine->project_context);
	BaseShaderProgramSetModelViewProjectionMatrix(&engine->z_plot_program.program.program, matrix);
	
	materials = (MATERIAL_INTERFACE**)((MODEL_INTERFACE*)engine->interface_data.model)->get_materials(engine->interface_data.model, &num_materials);
	size = engine->index_buffer->base.get_stride_size(engine->index_buffer);
	PmxRenderEngineBindVertexBundle(engine);
	glDisable(GL_CULL_FACE);
	for(i=0; i<num_materials; i++)
	{
		MATERIAL_INTERFACE *material = materials[i];
		const int num_indices = material->get_index_range(material).count;
		if(material->has_shadow_map(material))
		{
			if(is_vertex_shader_skinning != FALSE)
			{
				PmxExtendedZPlotProgramSetBoneMatrices(&engine->z_plot_program,
					engine->matrix_buffer->bytes(engine->matrix_buffer, i), engine->matrix_buffer->get_size(engine->matrix_buffer, i));
			}
			glDrawElements(GL_TRIANGLES, num_indices, engine->index_type, (GLvoid*)offset);
		}
		offset += num_indices * size;
	}
	PmxRenderEngineUnbindVertexBundle(engine);
	glEnable(GL_CULL_FACE);
	ShaderProgramUnbind(&engine->z_plot_program.program.program.base_data);

	MEM_FREE_FUNC(materials);
}

EFFECT_INTERFACE* PmxRenderEngineGetEffect(PMX_RENDER_ENGINE* engine, eEFFECT_SCRIPT_ORDER_TYPE type)
{
	return NULL;
}

ASSET_RENDER_ENGINE* AssetRenderEngineNew(
	PROJECT* project_context,
	SCENE* scene,
	MODEL_INTERFACE* model
)
{
	ASSET_RENDER_ENGINE *ret = (ASSET_RENDER_ENGINE*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));
	ret->project = project_context;
	ret->scene = scene;
	ret->layout = VertexBundleLayoutNew();
	ret->cull_face_state = TRUE;
	ret->textures = ght_create(ASSET_MODEL_BUFFER_SIZE);
	ght_set_hash(ret->textures, (ght_fn_hash_t)GetStringHash);
	ret->vao = ght_create(ASSET_MODEL_BUFFER_SIZE);
	ret->vbo = ght_create(ASSET_MODEL_BUFFER_SIZE);
	ret->indices = ght_create(ASSET_MODEL_BUFFER_SIZE);
	ret->asset_programs = ght_create(ASSET_MODEL_BUFFER_SIZE);
	ret->z_plot_programs = ght_create(ASSET_MODEL_BUFFER_SIZE);
	ret->interface_data.type = RENDER_ENGINE_ASSET;
	ret->interface_data.model = model;
	ret->interface_data.render_model =
		(void (*)(void*))AssetRenderEngineRenderModel;
	ret->interface_data.render_edge =
		(void (*)(void*))DummyFuncNoReturn;
	ret->interface_data.update =
		(void (*)(void*))DummyFuncNoReturn;
	ret->interface_data.get_effect =
		(EFFECT_INTERFACE* (*)(void*, eEFFECT_SCRIPT_ORDER_TYPE))AssetRenderEngineGetEffect;

	return ret;
}

static int SplitTexturePath(
	const char* path,
	char** main_texture,
	char** sub_texture
)
{
	const char *first_asterisk;
	if(path == NULL)
	{
		return FALSE;
	}
	first_asterisk = strstr(path, "*");
	if(first_asterisk != NULL)
	{
		int position = (int)(first_asterisk - path);
		*main_texture = (char*)MEM_ALLOC_FUNC(position+1);
		(void)memcpy(main_texture, path, position-1);
		main_texture[position] = '\0';
		*sub_texture = MEM_STRDUP_FUNC(first_asterisk);
		return TRUE;
	}
	else
	{
		*main_texture = MEM_STRDUP_FUNC(path);
		*sub_texture = MEM_ALLOC_FUNC(1);
		(*sub_texture)[0] = '\0';
	}
	return FALSE;
}

int AssetRenderEngineCreateProgram(
	ASSET_RENDER_ENGINE* engine,
	BASE_SHADER_PROGRAM* program,
	eSHADER_TYPE vertex_shader_type,
	eSHADER_TYPE fragment_shader_type,
	void* user_data
)
{
	char *source;
	source = LoadShaderSource(engine->project->application_context, vertex_shader_type,
		engine->interface_data.model, user_data);
	BaseShaderProgramAddShaderSource(program, source, GL_VERTEX_SHADER);
	MEM_FREE_FUNC(source);
	source = LoadShaderSource(engine->project->application_context, fragment_shader_type,
		engine->interface_data.model, user_data);
	BaseShaderProgramAddShaderSource(program, source, GL_FRAGMENT_SHADER);
	MEM_FREE_FUNC(source);
	return BaseShaderProgramLinkProgram(program);
}

void AssetRenderEngineBindStaticVertexAttributePointers(ASSET_RENDER_ENGINE* engine)
{
	glVertexAttribPointer(MODEL_VERTEX_STRIDE, 3, GL_FLOAT, GL_FALSE, sizeof(ASSET_RENDER_ENGINE_VERTEX), NULL);
	glVertexAttribPointer(MODEL_NORMAL_STRIDE, 3, GL_FLOAT, GL_FALSE, sizeof(ASSET_RENDER_ENGINE_VERTEX),
		(GLvoid*)offsetof(ASSET_RENDER_ENGINE_VERTEX, normal));
	glVertexAttribPointer(MODEL_TEXTURE_COORD_STRIDE, 2, GL_FLOAT, GL_FALSE, sizeof(ASSET_RENDER_ENGINE_VERTEX),
		(GLvoid*)offsetof(ASSET_RENDER_ENGINE_VERTEX, tex_coord));
	glEnableVertexAttribArray(MODEL_VERTEX_STRIDE);
	glEnableVertexAttribArray(MODEL_NORMAL_STRIDE);
	glEnableVertexAttribArray(MODEL_TEXTURE_COORD_STRIDE);
}

void AssetRenderEngineBindVertexBundle(ASSET_RENDER_ENGINE* engine, const struct aiMesh* mesh)
{
	VERTEX_BUNDLE_LAYOUT *layout = (VERTEX_BUNDLE_LAYOUT*)ght_get(engine->vao, sizeof(mesh), mesh);
	if(VertexBundleLayoutBind(layout) == FALSE)
	{
		VERTEX_BUNDLE *bundle = (VERTEX_BUNDLE*)ght_get(engine->vbo, sizeof(mesh), mesh);
		VertexBundleBind(bundle, VERTEX_BUNDLE_VERTEX_BUFFER, 0);
		AssetRenderEngineBindStaticVertexAttributePointers(engine);
		VertexBundleBind(bundle, VERTEX_BUNDLE_INDEX_BUFFER, 0);
	}
}

void AssetRenderEngineUnbindVertexBundle(ASSET_RENDER_ENGINE* engine, const struct aiMesh* mesh)
{
	VERTEX_BUNDLE_LAYOUT *layout = (VERTEX_BUNDLE_LAYOUT*)ght_get(engine->vao, sizeof(mesh), mesh);
	if(VertexBundleLayoutUnbind(layout) == FALSE)
	{
		VertexBundleUnbind(VERTEX_BUNDLE_VERTEX_BUFFER);
		VertexBundleUnbind(VERTEX_BUNDLE_INDEX_BUFFER);
	}
}

void AssetRenderEngineMakeVertexBundle(
	ASSET_RENDER_ENGINE* engine,
	const struct aiMesh* mesh,
	STRUCT_ARRAY* vertices,
	UINT32_ARRAY* indices
)
{
	VERTEX_BUNDLE_LAYOUT *layout;
	VERTEX_BUNDLE *bundle;
	layout = VertexBundleLayoutNew();
	(void)ght_insert(engine->vao, layout, sizeof(mesh), mesh);
	bundle = VertexBundleNew();
	(void)ght_insert(engine->vbo, bundle, sizeof(mesh), mesh);
	MakeVertexBundle(bundle, VERTEX_BUNDLE_INDEX_BUFFER, 0, GL_STATIC_DRAW,
		(const void*)indices->buffer, sizeof(*indices->buffer) * indices->num_data);
	MakeVertexBundle(bundle, VERTEX_BUNDLE_VERTEX_BUFFER, 0, GL_STATIC_DRAW,
		(const void*)vertices->buffer, vertices->data_size * vertices->num_data);
	(void)MakeVertexBundleLayout(layout);
	(void)VertexBundleLayoutBind(layout);
	VertexBundleBind(bundle, VERTEX_BUNDLE_VERTEX_BUFFER, 0);
	AssetRenderEngineBindStaticVertexAttributePointers(engine);
	VertexBundleBind(bundle, VERTEX_BUNDLE_INDEX_BUFFER, 0);
	AssetRenderEngineUnbindVertexBundle(engine, mesh);
	(void)ght_insert(engine->indices,
		(void*)((unsigned int)indices->num_data), sizeof(mesh), mesh);
}

int AssetRenderEngineUploadRecurse(
	ASSET_RENDER_ENGINE* engine,
	const struct aiScene* scene,
	const struct aiNode* node,
	void* user_data
)
{
	int ret = TRUE;
	const unsigned int num_meshes = node->mNumMeshes;
	ASSET_MODEL_PROGRAM *asset_program;
	Z_PLOT_PROGRAM *z_plot_program;
	STRUCT_ARRAY *vertices;
	ASSET_RENDER_ENGINE_VERTEX vertex = {0};
	UINT32_ARRAY *indices;
	unsigned int num_child_nodes;
	unsigned int i, j, k;

	asset_program = (ASSET_MODEL_PROGRAM*)MEM_ALLOC_FUNC(sizeof(*asset_program));
	InitializeAssetModelProgram(asset_program);
	(void)ght_insert(engine->asset_programs, asset_program, sizeof(node), node);
	if(AssetRenderEngineCreateProgram(engine, &asset_program->program.program,
		SHADER_TYPE_MODEL_VERTEX, SHADER_TYPE_MODEL_FRAGMENT, user_data) == FALSE)
	{
		return ret;
	}
	z_plot_program = (Z_PLOT_PROGRAM*)MEM_ALLOC_FUNC(sizeof(*z_plot_program));
	InitializeZPlotProgram(z_plot_program);
	(void)ght_insert(engine->z_plot_programs, z_plot_program, sizeof(node), node);
	if(AssetRenderEngineCreateProgram(engine, &z_plot_program->program,
		SHADER_TYPE_Z_PLOT_VERTEX, SHADER_TYPE_Z_PLOT_FRAGMENT, user_data) == FALSE)
	{
		return ret;
	}

	vertices = StructArrayNew(sizeof(ASSET_RENDER_ENGINE_VERTEX), DEFAULT_BUFFER_SIZE);
	indices = Uint32ArrayNew(DEFAULT_BUFFER_SIZE);
	for(i=0; i<num_meshes; i++)
	{
		const struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		const unsigned int num_faces = mesh->mNumFaces;
		struct aiFace *face;
		unsigned int num_indices;
		unsigned int num_vertices;
		int has_normals;
		int has_tex_coords;
		struct aiVector3D *v;
		struct aiVector3D *n;
		struct aiVector3D *t;
#if 1
		for(j=0; j<num_faces; j++)
		{
			face = &mesh->mFaces[j];
			num_indices = face->mNumIndices;
			for(k=0; k<num_indices; k++)
			{
				Uint32ArrayAppend(indices, (uint32)face->mIndices[k]);
			}
		}
		has_normals = (mesh->mNormals != NULL && mesh->mNumVertices > 0);
		has_tex_coords = aiMeshHasTextureCoords(mesh, 0);
		v = mesh->mVertices;
		n = (has_normals != FALSE) ? mesh->mNormals : NULL;
		t = (has_tex_coords != FALSE) ? mesh->mTextureCoords[0] : NULL;
		num_vertices = mesh->mNumVertices;
		for(j=0; j<num_vertices; j++)
		{
			vertex.position[0] = v[j].x;
			vertex.position[1] = v[j].y;
			vertex.position[2] = v[j].z;
			vertex.position[3] = 1;
			if(n != NULL)
			{
				vertex.normal[0] = n[j].x;
				vertex.normal[1] = n[j].y;
				vertex.normal[2] = n[j].z;
			}
			else
				{
					vertex.normal[0] = vertex.normal[1] = vertex.normal[2] = 1.0f;
				}
			if(t != NULL)
			{
				vertex.tex_coord[0] = t[j].x;
				vertex.tex_coord[1] = t[j].y;
				vertex.tex_coord[2] = t[j].z;
			}
			StructArrayAppend(vertices, &vertex);
		}
#else
		has_normals = (mesh->mNormals != NULL && mesh->mNumVertices > 0);
		has_tex_coords = aiMeshHasTextureCoords(mesh, 0);
		v = mesh->mVertices;
		n = (has_normals != FALSE) ? mesh->mNormals : NULL;
		t = (has_tex_coords != FALSE) ? mesh->mTextureCoords[0] : NULL;
		for(j=0; j<num_faces; j++)
		{
			face = &mesh->mFaces[j];
			num_indices = face->mNumIndices;
			for(k=0; k<num_indices; k++)
			{
				vertex.position[0] = v[face->mIndices[k]].x;
				vertex.position[1] = v[face->mIndices[k]].y;
				vertex.position[2] = v[face->mIndices[k]].z;
				vertex.position[3] = 1;
				if(has_normals != FALSE)
				{
					vertex.normal[0] = n[face->mIndices[k]].x;
					vertex.normal[1] = n[face->mIndices[k]].y;
					vertex.normal[2] = n[face->mIndices[k]].z;
				}
				else
				{
					vertex.normal[0] = vertex.normal[1] = vertex.normal[2] = 1.0f;
				}
				if(has_tex_coords != FALSE)
				{
					vertex.tex_coord[0] = t[face->mIndices[k]].x;
					vertex.tex_coord[1] = t[face->mIndices[k]].y;
					vertex.tex_coord[2] = t[face->mIndices[k]].z;
				}
				StructArrayAppend(vertices, &vertex);
			}
			Uint32ArrayAppend(indices, (uint32)j);
		}
#endif
		AssetRenderEngineMakeVertexBundle(engine, mesh, vertices, indices);
		vertices->num_data = 0;
		indices->num_data = 0;
		AssetRenderEngineUnbindVertexBundle(engine, mesh);
	}

	num_child_nodes = node->mNumChildren;
	for(i=0; i<num_child_nodes; i++)
	{
		ret = AssetRenderEngineUploadRecurse(engine, scene,
			node->mChildren[i], user_data);
		if(ret == FALSE)
		{
			break;;
		}
	}
	StructArrayDestroy(&vertices, NULL);
	Uint32ArrayDestroy(&indices);
	return ret;
}

int AssetRenderEngineUpload(ASSET_RENDER_ENGINE* engine, const char* directory)
{
	APPLICATION *application = engine->project->application_context;
	ASSET_MODEL *model = (ASSET_MODEL*)engine->interface_data.model;
	TEXTURE_DATA_BRIDGE bridge = {NULL, TEXTURE_FLAG_TEXTURE_2D};
	struct aiScene *scene;
	struct aiString texture_path;
	char *path;
	char *main_texture;
	char *sub_texture;
	int ret = TRUE;
	void *user_data = NULL;
	unsigned int num_items;
	unsigned int i;

	if(model == NULL || model->scene == NULL)
	{
		return FALSE;
	}

	scene = model->scene;
	num_items = scene->mNumMaterials;
	for(i=0; i<num_items; i++)
	{
		struct aiMaterial *material = scene->mMaterials[i];
		enum aiReturn found = AI_SUCCESS;
		int texture_index = 0;
		do
		{
			if((found = aiGetMaterialTexture(material, aiTextureType_DIFFUSE, texture_index, &texture_path,
				NULL, NULL, NULL, NULL, NULL, NULL)) != aiReturn_SUCCESS)
			{
				break;
			}
			path = texture_path.data;
			if(SplitTexturePath(path, &main_texture, &sub_texture) != FALSE)
			{
				if(ght_get(engine->textures, (unsigned int)strlen(main_texture), main_texture) == NULL)
				{
					ret = UploadTexture(main_texture, &bridge, application);
					if(ret != FALSE)
					{
						(void)ght_insert(engine->textures, bridge.texture,
							(unsigned int)strlen(main_texture), main_texture);
					}
					else
					{
						return ret;
					}
				}
				if(ght_get(engine->textures, (unsigned int)strlen(sub_texture), sub_texture) == NULL)
				{
					ret = UploadTexture(sub_texture, &bridge, application);
					if(ret != FALSE)
					{
						(void)ght_insert(engine->textures, bridge.texture,
							(unsigned int)strlen(sub_texture), sub_texture);
					}
					else
					{
						return ret;
					}
				}
			}
			else if(ght_get(engine->textures, (unsigned int)strlen(main_texture), main_texture) == NULL)
			{
				ret = UploadTexture(main_texture, &bridge, application);
				if(ret != FALSE)
				{
					(void)ght_insert(engine->textures, bridge.texture,
						(unsigned int)strlen(main_texture), main_texture);
				}
				else
				{
					return ret;
				}
			}
			texture_index++;
		} while(found == AI_SUCCESS);
	}
	AssetRenderEngineUploadRecurse(engine, scene, scene->mRootNode, user_data);
	engine->interface_data.model->set_visible(engine->interface_data.model, ret);
	return ret;
}

void AssetRenderEngineSetMaterial(
	ASSET_RENDER_ENGINE* engine,
	const struct aiMaterial* material,
	ASSET_MODEL_PROGRAM* program
)
{
	TEXTURE_INTERFACE *texture = NULL;
	int texture_index = 0;
	char *main_texture;
	char *sub_texture;
	struct aiString texture_path;
	struct aiColor4D asset_color;
	VECTOR4 ambient;
	VECTOR4 diffuse;
	VECTOR4 color;
	GLuint texture_name = 0;
	float float_value;
	float strength;
	int two_side;
	uint8 *light_color;

	if(aiGetMaterialTexture(material, aiTextureType_DIFFUSE, 0, &texture_path,
		NULL, NULL, NULL, NULL, NULL, NULL) == aiReturn_SUCCESS)
	{
		int is_additive = FALSE;
		if(SplitTexturePath(texture_path.data, &main_texture, &sub_texture) != FALSE)
		{
			texture = (TEXTURE_INTERFACE*)ght_get(engine->textures, (unsigned int)strlen(sub_texture), sub_texture);
			is_additive = StringStringIgnoreCase(sub_texture, ".spa") != NULL ? TRUE : FALSE;
			AssetModelProgramSetSubTexture(program, texture);
			AssetModelProgramSetIsSubAdditive(program, is_additive);
			AssetModelProgramSetIsSubSphereMap(program, is_additive || StringStringIgnoreCase(sub_texture, ".sph") != NULL);
		}
		texture = (TEXTURE_INTERFACE*)ght_get(engine->textures, (unsigned int)strlen(main_texture), main_texture);
		is_additive = StringStringIgnoreCase(main_texture, ".spa") != NULL;
		AssetModelProgramSetIsMainAdditive(program, is_additive);
		AssetModelProgramSetIsMainSphereMap(program, is_additive || StringStringIgnoreCase(main_texture, ".sph") != NULL);
		ObjectProgramSetMainTexture(&program->program, texture);
	}
	else
	{
		ObjectProgramSetMainTexture(&program->program, NULL);
		AssetModelProgramSetSubTexture(program, NULL);
	}
	aiGetMaterialColor(material, AI_MATKEY_COLOR_AMBIENT, &asset_color);
	ambient[0] = asset_color.r,	ambient[1] = asset_color.g,	ambient[2] = asset_color.b, ambient[3] = asset_color.a;
	aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &asset_color);
	diffuse[0] = asset_color.r, diffuse[1] = asset_color.g, diffuse[2] = asset_color.b, diffuse[3] = asset_color.a;
	light_color = engine->project->scene->light.vertex.color;
	color[0] = diffuse[0] * (0.7f - light_color[0] * DIV_PIXEL);
	color[1] = diffuse[0] * (0.7f - light_color[1] * DIV_PIXEL);
	color[2] = diffuse[0] * (0.7f - light_color[2] * DIV_PIXEL);
	color[3] = 1;
	AssetModelProgramSetMaterialColor(program, color);
	AssetModelProgramSetMaterialDiffuse(program, diffuse);
	aiGetMaterialColor(material, AI_MATKEY_COLOR_SPECULAR, &asset_color);
	color[0] = asset_color.r * (light_color[0] * DIV_PIXEL);
	color[1] = asset_color.g * (light_color[1] * DIV_PIXEL);
	color[2] = asset_color.b * (light_color[2] * DIV_PIXEL);
	color[3] = asset_color.a;
	AssetModelProgramSetMaterialSpecular(program, color);
	if(aiGetMaterialFloatArray(material, AI_MATKEY_SHININESS, &float_value, NULL) == aiReturn_SUCCESS)
	{
		if(aiGetMaterialFloatArray(material, AI_MATKEY_SHININESS_STRENGTH, &strength, NULL) == aiReturn_SUCCESS)
		{
			AssetModelProgramSetMaterialShininess(program, float_value * strength);
		}
		else
		{
			AssetModelProgramSetMaterialShininess(program, float_value);
		}
	}
	else
	{
		AssetModelProgramSetMaterialShininess(program, 15.0f);
	}
	if(aiGetMaterialFloatArray(material, AI_MATKEY_OPACITY, &float_value, NULL) == aiReturn_SUCCESS)
	{
		ObjectProgramSetOpacity(&program->program, float_value * engine->interface_data.model->opacity);
	}
	else
	{
		ObjectProgramSetOpacity(&program->program, engine->interface_data.model->opacity);
	}
	/*
	if(engine->project->scene->shadow_map != NULL)
	{
		texture_name = (engine->project->scene->shadow_map->g
	*/
	if(texture_name != 0 && FuzzyZero(float_value - 0.98f) != 0)
	{
		ObjectProgramSetDepthTexture(&program->program, texture_name);
	}
	else
	{
		ObjectProgramSetDepthTexture(&program->program, 0);
	}

	if(aiGetMaterialIntegerArray(material, AI_MATKEY_TWOSIDED, &two_side, NULL) == aiReturn_SUCCESS
		&& two_side != 0 && engine->cull_face_state == FALSE)
	{
		glEnable(GL_CULL_FACE);
		engine->cull_face_state = TRUE;
	}
	else if(engine->cull_face_state != FALSE)
	{
		glDisable(GL_CULL_FACE);
		engine->cull_face_state = FALSE;
	}
}

void AssetRenderEngineRenderModelRecurse(
	ASSET_RENDER_ENGINE* engine,
	const struct aiScene* scene,
	const struct aiNode* node
)
{
	ASSET_MODEL_PROGRAM *program;
	const unsigned int num_meshes = node->mNumMeshes;
	float matrix[16];
	int num_indices;
	unsigned int i;

	program = (ASSET_MODEL_PROGRAM*)ght_get(engine->asset_programs,
		sizeof(node), node);
	ShaderProgramBind((SHADER_PROGRAM*)program);
	GetContextMatrix(matrix, engine->interface_data.model,
		MATRIX_FLAG_VIEW_MATRIX | MATRIX_FLAG_PROJECTION_MATRIX | MATRIX_FLAG_CAMERA_MATRIX,
			engine->project);
	AssetModelProgramSetViewProjectionMatrix(program, matrix);
	GetContextMatrix(matrix, engine->interface_data.model,
		MATRIX_FLAG_WORLD_MATRIX | MATRIX_FLAG_VIEW_MATRIX | MATRIX_FLAG_PROJECTION_MATRIX | MATRIX_FLAG_LIGHT_MATRIX,
			engine->project);
	ObjectProgramSetLightViewProjectionMatrix(&program->program, matrix);
	GetContextMatrix(matrix, engine->interface_data.model,
		MATRIX_FLAG_WORLD_MATRIX | MATRIX_FLAG_CAMERA_MATRIX,
			engine->project);
	AssetModelProgramSetModelMatrix(program, matrix);
	ObjectProgramSetLightColor(&program->program,
		engine->project->scene->light.vertex.color);
	ObjectProgramSetLightDirection(&program->program,
		engine->project->scene->light.vertex.position);
	ObjectProgramSetOpacity(&program->program, engine->interface_data.model->opacity);
	AssetModelProgramSetCameraPosition(program, engine->project->scene->camera.position);
	for(i=0; i<num_meshes; i++)
	{
		const struct aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		AssetRenderEngineSetMaterial(engine, scene->mMaterials[mesh->mMaterialIndex], program);
		AssetRenderEngineBindVertexBundle(engine, mesh);
		num_indices = (int)ght_get(engine->indices, sizeof(mesh), mesh);
		glDrawElements(GL_TRIANGLES, (GLsizei)num_indices, GL_UNSIGNED_INT, NULL);
		AssetRenderEngineUnbindVertexBundle(engine, mesh);
	}
	ShaderProgramUnbind(&program->program.program.base_data);
	for(i=0; i<node->mNumChildren; i++)
	{
		AssetRenderEngineRenderModelRecurse(engine, scene, node->mChildren[i]);
	}
}

void AssetRenderEngineRenderModel(ASSET_RENDER_ENGINE* engine)
{
	ASSET_MODEL *model = (ASSET_MODEL*)engine->interface_data.model;
	if(model == NULL || engine->interface_data.model->is_visible(model) == FALSE)
	{
		return;
	}
	AssetRenderEngineRenderModelRecurse(engine, model->scene, model->scene->mRootNode);
	if(engine->cull_face_state == FALSE)
	{
		glEnable(GL_CULL_FACE);
		engine->cull_face_state = TRUE;
	}
}

EFFECT_INTERFACE* AssetRenderEngineGetEffect(ASSET_RENDER_ENGINE* engine, eEFFECT_SCRIPT_ORDER_TYPE type)
{
	return NULL;
}
