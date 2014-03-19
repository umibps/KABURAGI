// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include "annotation.h"
#include "application.h"
#include "ght_hash_table.h"
#include "parameter.h"
#include "memory.h"

ANNOTATION* FxAnnotationNew(struct _EFFECT* effect, ANNOTATION* annotation, char* name)
{
	ANNOTATION *ret;

	ret = (ANNOTATION*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));
	ret->type = ANNOTATION_TYPE_FX;
	ret->anno.fx.name = MEM_STRDUP_FUNC(name);
	ret->anno.fx.annotation = annotation;
	ret->annotation_table = (ght_hash_table_t*)MEM_ALLOC_FUNC(sizeof(*ret->annotation_table));

	return ret;
}

void AnnotationDestroy(ANNOTATION** annotation)
{
	switch((*annotation)->type)
	{
	case ANNOTATION_TYPE_FX:
		MEM_FREE_FUNC((*annotation)->anno.fx.name);
		break;
	case ANNOTATION_TYPE_NV:
		break;
	}

	MEM_FREE_FUNC((*annotation)->annotation_table);

	MEM_FREE_FUNC(*annotation);
	*annotation = NULL;
}

float FxAnnotationGetFloat(FX_ANNOTATION* annotation, char* name)
{
	float *value;
	if(annotation == NULL)
	{
		return 0.0f;
	}

	value = (float*)ght_get(annotation->float_table, sizeof(float), name);
	if(value != NULL)
	{
		return *value;
	}

	return 0.0f;
}

int FxAnnotationGetInt(FX_ANNOTATION* annotation, char* name)
{
	int *value;
	if(annotation == NULL)
	{
		return 0;
	}

	value = (int*)ght_get(annotation->int_table, sizeof(float), name);
	if(value != NULL)
	{
		return *value;
	}

	return 0;
}

char* FxAnnotationGetString(FX_ANNOTATION* annotation, char* name)
{
	char **value;
	if(annotation == NULL)
	{
		return 0;
	}

	value = (char**)ght_get(annotation->int_table, sizeof(char*), name);
	if(value != NULL)
	{
		return *value;
	}

	return 0;
}

gboolean FxAnnotationGetBooleanValue(FX_ANNOTATION* annotation)
{
	if(annotation->base == PARAMETER_TYPE_BOOLEAN)
	{
		if(strcmp(annotation->value_str, "true") == 0)
		{
			return TRUE;
		}
	}

	return FALSE;
}

ANNOTATION* EffectCacheFxAnnotationRefference(
	struct _EFFECT* effect,
	ANNOTATION* annotation,
	char* name
)
{
	ANNOTATION *ret;
	ANNOTATION *new_annotation;
	ANNOTATION *insert_annotation = annotation;
	ght_hash_table_t *table;
	table = (ght_hash_table_t*)ght_get(annotation->annotation_table, sizeof(table), annotation);
	if(table != NULL)
	{
		ret = (ANNOTATION*)ght_get(table, sizeof(ret), name);
		if(ret != NULL)
		{
			return ret;
		}
	}

	if(FxAnnotationGetFloat(&annotation->anno.fx, name) != 0.0f
		|| FxAnnotationGetInt(&annotation->anno.fx, name) != 0
		|| FxAnnotationGetString(&annotation->anno.fx, name) != NULL
	)
	{
		table = (ght_hash_table_t*)ght_get(annotation->annotation_table, sizeof(table), annotation);
		if(table == NULL)
		{
			annotation = FxAnnotationNew(effect, annotation, name);
			table = annotation->annotation_table;
			(void)ght_insert(table, annotation, sizeof(new_annotation->annotation_table),
				annotation->annotation_table);
		}
		new_annotation = FxAnnotationNew(effect, insert_annotation, name);
		(void)ght_insert(annotation->annotation_table, name, sizeof(new_annotation), new_annotation);
		return new_annotation;
	}

	return NULL;
}
