#include "types.h"
#include "technique.h"
#include "configure.h"
#include "application.h"
#include "memory.h"
#include "map_funcs.h"

void* CfTechniqueGetPass(CF_TECHNIQUE* tech, int* num_data)
{
	if(tech->pass_map != NULL)
	{
		int size = (int)STR_PTR_MAP_size(tech->pass_map);
		void **ret = MEM_ALLOC_FUNC(size * sizeof(*ret));
		STR_PTR_MAPIterator iterator;
		int count = 0;

		iterator = STR_PTR_MAP_begin(tech->pass_map);
		while(iterator != NULL)
		{
			ret[count] = iterator->value;
			count++;
			iterator = STR_PTR_MAP_next(iterator);
		}
	}

	return NULL;
}

struct _ANNOTATION* CfTechniqueGetAnnotationRefference(
	TECHNIQUE* tech,
	struct _ANNOTATION* annotation,
	char* name
)
{
	STR_PTR_MAPIterator iterator;
	iterator = STR_PTR_MAP_find(tech->tech.cf.annotation_map, name);
	if(iterator != STR_PTR_MAP_end(tech->tech.cf.annotation_map))
	{
		iterator = STR_PTR_MAP_next(iterator);
		return (ANNOTATION*)iterator->value;
	}

	return NULL;
}

/*
struct _ANNOTATION* NvTechniqueGetAnnotationRefferene(
	TECHNIQUE* tech,
	struct _ANNOTATION* annotation,
	char* name
)
{
	tech->tech.nv.effect
}
*/