// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <ctype.h>
#include "annotation.h"
#include "application.h"
#include "ght_hash_table.h"
#include "parameter.h"
#include "utils.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif
/*
void InitializeFxAnnotation(FX_ANNOTATION* annotation, char* name)
{
	(void)memset(annotation, 0, sizeof(*annotation));
	annotation->name = MEM_STRDUP_FUNC(name);
}

int FxAnnotationGetBooleanValue(FX_ANNOTATION* annotation)
{
	if(annotation->base == EFFECT_PARAMETER_TYPE_BOOLEAN)
	{
		return strcmp(annotation->value, "true") == 0;
	}

	return FALSE;
}

int FxAnnotationGetIntegerValue(FX_ANNOTATION* annotation)
{
	if(annotation->base == EFFECT_PARAMETER_TYPE_INTEGER)
	{
		return atoi(annotation->value);
	}

	return 0;
}

int* FxAnnotationGetIntegerValues(FX_ANNOTATION* annotation, size_t* size)
{
#define BLOCK_SIZE 32
	INTEGER_ARRAY *values = IntegerArrayNew(BLOCK_SIZE);
	char *copy = MEM_STRDUP_FUNC(annotation->value);
	char buff[32];
	int *ret;
	char **lines;
	int num_lines;
	int i;

	if(annotation->base != PARAMETER_TYPE_INTEGER)
	{
		return NULL;
	}

	lines = SplitString(copy, "\n", &num_lines);

	for(i=0; i<num_lines; i++)
	{
		char *str = lines[i];
		int num_char;
		while(*str != '\0')
		{
			if(isdigit(*str) != 0)
			{
				num_char = 0;
				do
				{
					buff[num_char] = *str;
					num_char++;
					str++;
				} while(isdigit(*str) != 0);

				IntegerArrayAppend(values, atoi(buff));
			}
			else
			{
				str++;
			}
		}
	}

	MEM_FREE_FUNC(lines);
	MEM_FREE_FUNC(copy);

	ret = values->buffer;
	MEM_FREE_FUNC(values);

	return ret;
#undef BLOCK_SIZE
}

float FxAnnotationGetFloatValue(FX_ANNOTATION* annotation)
{
	if(annotation->base == PARAMETER_TYPE_FLOAT)
	{
		return (float)atof(annotation->value);
	}

	return 0;
}

float* FxAnnotationGetFloatValues(FX_ANNOTATION* annotation, size_t* size)
{
#define BLOCK_SIZE 32
	FLOAT_ARRAY *values = FloatArrayNew(BLOCK_SIZE);
	char *copy = MEM_STRDUP_FUNC(annotation->value);
	char buff[32];
	float *ret;
	char **lines;
	int num_lines;
	int i;

	if(annotation->base != PARAMETER_TYPE_FLOAT)
	{
		return NULL;
	}

	lines = SplitString(copy, "\n", &num_lines);

	for(i=0; i<num_lines; i++)
	{
		char *str = lines[i];
		int num_char;
		while(*str != '\0')
		{
			if(isdigit(*str) != 0 || *str == '.')
			{
				num_char = 0;
				do
				{
					buff[num_char] = *str;
					num_char++;
					str++;
				} while(isdigit(*str) != 0 || *str == '.');

				FloatArrayAppend(values, (float)atof(buff));
			}
			else
			{
				str++;
			}
		}
	}

	MEM_FREE_FUNC(lines);
	MEM_FREE_FUNC(copy);

	ret = values->buffer;
	MEM_FREE_FUNC(values);

	return ret;
#undef BLOCK_SIZE
}
*/
#ifdef __cplusplus
}
#endif
