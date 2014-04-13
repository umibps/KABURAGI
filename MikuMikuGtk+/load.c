// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include "application.h"
#include "pmx_model.h"
#include "pmd_model.h"
#include "memory.h"

MODEL_INTERFACE* LoadModel(
	APPLICATION* application,
	const char* system_path,
	const char* file_type
)
{
	PROJECT *project = application->projects[application->active_project];
	SCENE *scene = project->scene;
	FILE *fp;
	uint8 *data;
	long file_size;

	if(StringCompareIgnoreCase(file_type, ".pmx") == 0)
	{
		PMX_MODEL *model = (PMX_MODEL*)MEM_ALLOC_FUNC(sizeof(*model));
		char *utf8_path = Locale2UTF8(system_path);
		char *model_path = GetDirectoryName(utf8_path);

		fp = fopen(system_path, "rb");

		if(fp == NULL)
		{
			MEM_FREE_FUNC(utf8_path);
			MEM_FREE_FUNC(model_path);
			MEM_FREE_FUNC(model);
			return FALSE;
		}

		(void)fseek(fp, 0, SEEK_END);
		file_size = ftell(fp);
		rewind(fp);
		data = (uint8*)MEM_ALLOC_FUNC(file_size);
		(void)fread(data, 1, file_size, fp);
		(void)fclose(fp);

		InitializePmxModel(model, scene, &application->encode, model_path);
		if(LoadPmxModel(model, data, file_size) == FALSE)
		{
			ReleasePmxModel(model);
			MEM_FREE_FUNC(model);
			MEM_FREE_FUNC(utf8_path);
			MEM_FREE_FUNC(model_path);
			return NULL;
		}

		PointerArrayAppend(scene->models, model);
		PointerArrayAppend(scene->engines, SceneCreateRenderEngine(
			project->scene, RENDER_ENGINE_PMX, (MODEL_INTERFACE*)model, 0, project));
		if((project->flags & PROJECT_FLAG_ALWAYS_PHYSICS) == 0)
		{
			model->flags &= PMX_FLAG_ENABLE_PHYSICS;
		}

		MEM_FREE_FUNC(utf8_path);
		MEM_FREE_FUNC(model_path);

		PmxModelJoinWorld(model, project->world.world);
		scene->selected_model = (MODEL_INTERFACE*)model;
		SceneSetEmptyMotion(scene, (MODEL_INTERFACE*)model);
		if((project->flags & PROJECT_FLAG_ALWAYS_PHYSICS) != 0)
		{
			model->interface_data.set_enable_physics(model, TRUE);
		}

		return (MODEL_INTERFACE*)model;
	}
	else if(StringCompareIgnoreCase(file_type, ".pmd") == 0)
	{
		PMD2_MODEL *model = (PMD2_MODEL*)MEM_ALLOC_FUNC(sizeof(*model));
		char *utf8_path = Locale2UTF8(system_path);
		char *model_path = GetDirectoryName(utf8_path);

		fp = fopen(system_path, "rb");

		if(fp == NULL)
		{
			MEM_FREE_FUNC(utf8_path);
			MEM_FREE_FUNC(model_path);
			MEM_FREE_FUNC(model);
			return FALSE;
		}

		(void)fseek(fp, 0, SEEK_END);
		file_size = ftell(fp);
		rewind(fp);
		data = (uint8*)MEM_ALLOC_FUNC(file_size);
		(void)fread(data, 1, file_size, fp);
		(void)fclose(fp);

		InitializePmd2Model(model, scene, model_path);
		if(LoadPmd2Model(model, data, file_size) == FALSE)
		{
			return NULL;
		}

		PointerArrayAppend(scene->models, model);
		PointerArrayAppend(scene->engines, SceneCreateRenderEngine(
			project->scene, RENDER_ENGINE_PMX, (MODEL_INTERFACE*)model, 0, project));

		Pmd2ModelJoinWorld(model, project->world.world);
		scene->selected_model = (MODEL_INTERFACE*)model;
		SceneSetEmptyMotion(scene, (MODEL_INTERFACE*)model);

		return (MODEL_INTERFACE*)model;
	}
	else
	{
		ASSET_MODEL *model = (ASSET_MODEL*)MEM_ALLOC_FUNC(sizeof(*model));
		char *utf8_path = Locale2UTF8(system_path);
		char *model_path = GetDirectoryName(utf8_path);
		char *file_name;
		char *str;

		fp = fopen(system_path, "rb");

		if(fp == NULL)
		{
			MEM_FREE_FUNC(utf8_path);
			MEM_FREE_FUNC(model_path);
			MEM_FREE_FUNC(model);
			return NULL;
		}

		file_name = str = utf8_path;
		while(*str != '\0')
		{
			if(*str == '\\' || *str == '/')
			{
				file_name = str + 1;
			}
			str = NextCharUTF8(str);
		}

		(void)fseek(fp, 0, SEEK_END);
		file_size = ftell(fp);
		rewind(fp);
		data = (uint8*)MEM_ALLOC_FUNC(file_size);
		(void)fread(data, 1, file_size, fp);
		(void)fclose(fp);

		InitializeAssetModel(model, scene, application);
		if(LoadAssetModel(model, data, file_size, file_name, file_type, model_path) == FALSE)
		{
			return NULL;
		}

		PointerArrayAppend(scene->models, model);
		PointerArrayAppend(scene->engines, SceneCreateRenderEngine(
			project->scene, RENDER_ENGINE_ASSET, (MODEL_INTERFACE*)model, 0, project));

		MEM_FREE_FUNC(utf8_path);
		MEM_FREE_FUNC(model_path);

		return (MODEL_INTERFACE*)model;
	}

	return NULL;
}
