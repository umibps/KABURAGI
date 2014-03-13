// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_WARNINGS
#endif

#include "effect_engine.h"
#include "application.h"
#include "map_funcs.h"

int EffectEngineContainsSubset(
	EFFECT_ENGINE* engine,
	ANNOTATION* annotation,
	int subset,
	int num_materials
)
{
	if(annotation != NULL)
	{
		char *str = annotation->anno.fx.value_str;
		char *end;
		char *token;
		char *find;
		if(str == NULL || *str == '\0')
		{
			return 1;
		}

		end = str + strlen(str);
		token = strtok(str, ",");
		while(token != NULL)
		{
			if(strtol(token, NULL, 10) == subset)
			{
				return 1;
			}
			find = strstr(token, "-");
			if(find != NULL && find < end)
			{
				int from;
				int to;
				int get_first = 0, get_second = 0;
				char *s1 = find, *s2 = find + 1;
				char str1[128] = {0}, str2[128] = {0};
				int counter;
				char *p;

				p = token;
				counter = 0;
				while(p < find)
				{
					str1[counter] = *p;
					counter++;
					p++;
				}

				p = s2;
				counter = 0;
				while(*p != '\0' && *p != ',' && *p != '-')
				{
					str2[counter] = *p;
					counter++;
					p++;
				}

				from = strtol(str1, NULL, 10);
				to = strtol(str2, NULL, 10);
				if(to == 0)
				{
					to = num_materials;
				}
				if(from > to)
				{
					int temp = from;
					from = to;
					to = temp;
				}
				if(from <= subset && subset <= 10)
				{
					return 1;
				}
			}
		}

		return 0;
	}

	return 1;
}

int EffectEngineTestTechnique(
	EFFECT_ENGINE* engine,
	struct _TECHNIQUE* tech,
	char *pass,
	int offset,
	int num_materials,
	int has_texture,
	int has_sphere_map,
	int use_toon
)
{
	if(tech != NULL && tech->type == TECHNIQUE_TYPE_CF)
	{
		ANNOTATION *anno_mmd_pass;
		ANNOTATION *anno_subset;
		ANNOTATION *annotation_ref;
		STR_PTR_MAPIterator iterator;
		int ok = 1;

		iterator = STR_PTR_MAP_find(tech->tech.cf.annotation_map, "MMDPass");
		anno_mmd_pass = (ANNOTATION*)(*STR_PTR_MAP_value(iterator));
		if(anno_mmd_pass != NULL)
		{
			ok &= (strcmp(anno_mmd_pass->anno.fx.value_str, pass) == 0) ? 1 : 0;
		}
		iterator = STR_PTR_MAP_find(tech->tech.cf.annotation_map, "Subset");
		anno_subset = (ANNOTATION*)(*STR_PTR_MAP_value(iterator));
		if(anno_subset != NULL)
		{
			ok &=
				(EffectEngineContainsSubset(engine, anno_subset, offset, num_materials) != 0) ? 1 : 0;
		}
		iterator = STR_PTR_MAP_find(tech->tech.cf.annotation_map, "UseTexture");
		annotation_ref = (ANNOTATION*)(*STR_PTR_MAP_value(iterator));
		if(annotation_ref != NULL)
		{
			ok &= (annotation_ref->anno.fx.values->ivalue != FALSE
				&& has_texture != FALSE) ? 1: 0;
		}
		iterator = STR_PTR_MAP_find(tech->tech.cf.annotation_map, "UseSphereMap");
		annotation_ref = (ANNOTATION*)(*STR_PTR_MAP_value(iterator));
		if(annotation_ref != NULL)
		{
			ok &= (annotation_ref->anno.fx.values->ivalue != FALSE
				&& has_sphere_map != FALSE) ? 1: 0;
		}
		iterator = STR_PTR_MAP_find(tech->tech.cf.annotation_map, "UseToon");
		annotation_ref = (ANNOTATION*)(*STR_PTR_MAP_value(iterator));
		if(annotation_ref != NULL)
		{
			ok &= (annotation_ref->anno.fx.values->ivalue != FALSE
				&& use_toon != FALSE) ? 1: 0;
		}

		return ok;
	}

	return 0;
}

struct _TECHNIQUE* EffectEngineFindTechniqueIn(
	EFFECT_ENGINE* engine,
	struct _TECHNIQUE* tech,
	char* pass,
	int offset,
	int num_materials,
	int has_texture,
	int has_sphere_map,
	int use_toon
)
{
	TECHNIQUE *technique;
	int i;
	for(i=0; i<engine->techniques.num_tech; i++)
	{
		technique = &engine->techniques.techs[i];
		if(EffectEngineTestTechnique(engine, technique, pass,
			offset, num_materials, has_texture, has_sphere_map, use_toon) != 0)
		{
			return technique;
		}
	}

	return NULL;
}
