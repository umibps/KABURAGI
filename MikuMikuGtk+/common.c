// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <ctype.h>
#include <string.h>
#include "memory.h"
#include "utils.h"

/**************************************
* StringCompareIgnoreCaseŠÖ”         *
* ‘å•¶Žš/¬•¶Žš‚ð–³Ž‹‚µ‚Ä•¶Žš—ñ‚ð”äŠr *
* ˆø”                                *
* str1	: ”äŠr•¶Žš—ñ1                 *
* str2	: ”äŠr•¶Žš—ñ2                 *
* •Ô‚è’l                              *
*	•¶Žš—ñ‚Ì·(“¯•¶Žš—ñ‚È‚ç0)         *
**************************************/
int StringCompareIgnoreCase(const char* str1, const char* str2)
{
	int ret;

	while((ret = toupper(*str1) - toupper(*str2)) == 0)
	{
		str1++, str2++;
		if(*str1 == '\0')
		{
			return 0;
		}
	}

	return ret;
}

int StringNumCompareIgnoreCase(const char* str1, const char* str2, int num)
{
	int ret;
	int count = 0;
	while((ret = toupper(*str1) - toupper(*str2)) == 0)
	{
		str1++,	str2++;
		if(*str1 == '\0')
		{
			return 0;
		}
		count++;
		if(count >= num)
		{
			return ret;
		}
	}
	return ret;
}

const char* StringStringIgnoreCase(const char* str, const char* compare)
{
	char *upper = MEM_STRDUP_FUNC(str);
	char *find = MEM_STRDUP_FUNC(compare);
	char *ret = NULL;
	const char *str_point;
	char *p;
	int position;

	p = upper;
	while(*p != '\0')
	{
		*p = toupper(*p);
		p++;
	}
	p = find;
	while(*p != '\0')
	{
		*p = toupper(*p);
		p++;
	}

	str_point = strstr(upper, find);
	if(str_point != NULL)
	{
		position = (int)(upper - str_point);
		ret = (char*)&str[position];
	}
	MEM_FREE_FUNC(upper);
	MEM_FREE_FUNC(find);

	return ret;
}

BYTE_ARRAY* ByteArrayNew(size_t block_size)
{
	BYTE_ARRAY *ret;

	if(block_size == 0)
	{
		return NULL;
	}

	ret = (BYTE_ARRAY*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->block_size = ret->buffer_size = block_size;
	ret->buffer = (uint8*)MEM_ALLOC_FUNC(block_size);
	(void)memset(ret->buffer, 0, block_size);

	return ret;
}

void ByteArrayAppend(BYTE_ARRAY* barray, uint8 data)
{
	barray->buffer[barray->num_data] = data;
	barray->num_data++;

	if(barray->num_data >= barray->buffer_size)
	{
		size_t before_size = barray->buffer_size;
		barray->buffer_size += barray->block_size;
		barray->buffer = (uint8*)MEM_REALLOC_FUNC(barray->buffer, barray->buffer_size);
		(void)memset(&barray->buffer[before_size], 0, barray->block_size);
	}
}

void ByteArrayDestroy(BYTE_ARRAY** barray)
{
	MEM_FREE_FUNC((*barray)->buffer);
	MEM_FREE_FUNC(*barray);

	*barray = NULL;
}

void ByteArrayResize(BYTE_ARRAY* barray, size_t new_size)
{
	size_t alloc_size;

	if(barray->buffer_size == new_size)
	{
		return;
	}
	if(barray->block_size > 1)
	{
		alloc_size = ((new_size + barray->block_size - 1)
			/ barray->block_size) * barray->block_size;
	}
	else
	{
		alloc_size = new_size;
	}
	barray->buffer = (uint8*)MEM_REALLOC_FUNC(
		barray->buffer, sizeof(uint8) * alloc_size);
	if(alloc_size < barray->num_data)
	{
		barray->num_data = alloc_size-1;
	}
	barray->buffer_size = alloc_size;
}

WORD_ARRAY* WordArrayNew(size_t block_size)
{
	WORD_ARRAY *ret;

	if(block_size == 0)
	{
		return NULL;
	}

	ret = (WORD_ARRAY*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->block_size = ret->buffer_size = block_size;
	ret->buffer = (uint16*)MEM_ALLOC_FUNC(block_size * sizeof(uint16));
	(void)memset(ret->buffer, 0, sizeof(uint16)*block_size);

	return ret;
}

void WordArrayAppend(WORD_ARRAY* warray, uint16 data)
{
	warray->buffer[warray->num_data] = data;
	warray->num_data++;

	if(warray->num_data >= warray->buffer_size)
	{
		size_t before_size = warray->buffer_size;
		warray->buffer_size += warray->block_size;
		warray->buffer = (uint16*)MEM_REALLOC_FUNC(
			warray->buffer, warray->buffer_size * sizeof(uint16));
		(void)memset(&warray->buffer[before_size], 0, warray->block_size * sizeof(uint16));
	}
}

void WordArrayDestroy(WORD_ARRAY** warray)
{
	MEM_FREE_FUNC((*warray)->buffer);
	MEM_FREE_FUNC(*warray);
	*warray = NULL;
}

void WordArrayResize(WORD_ARRAY* warray, size_t new_size)
{
	size_t alloc_size;

	if(warray->buffer_size == new_size)
	{
		return;
	}
	if(warray->block_size > 1)
	{
		alloc_size = ((new_size + warray->block_size - 1)
			/ warray->block_size) * warray->block_size;
	}
	else
	{
		alloc_size = new_size;
	}
	warray->buffer = (uint16*)MEM_REALLOC_FUNC(
		warray->buffer, sizeof(uint16) * alloc_size);
	if(alloc_size < warray->num_data)
	{
		warray->num_data = alloc_size-1;
	}
	warray->buffer_size = alloc_size;
}

UINT32_ARRAY* Uint32ArrayNew(size_t block_size)
{
	UINT32_ARRAY *ret;

	if(block_size == 0)
	{
		return NULL;
	}

	ret = (UINT32_ARRAY*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->block_size = ret->buffer_size = block_size;
	ret->buffer = (uint32*)MEM_ALLOC_FUNC(block_size * sizeof(uint32));

	return ret;
}

void Uint32ArrayAppend(UINT32_ARRAY* uarray, uint32 data)
{
	uarray->buffer[uarray->num_data] = data;
	uarray->num_data++;

	if(uarray->num_data >= uarray->buffer_size)
	{
		size_t before_size = uarray->buffer_size;
		uarray->buffer_size += uarray->block_size;
		uarray->buffer = (uint32*)MEM_REALLOC_FUNC(
			uarray->buffer, uarray->buffer_size * sizeof(uint32));
		(void)memset(&uarray->buffer[before_size], 0, uarray->block_size * sizeof(uint32));
	}
}

void Uint32ArrayDestroy(UINT32_ARRAY** uarray)
{
	MEM_FREE_FUNC((*uarray)->buffer);
	MEM_FREE_FUNC(*uarray);
	*uarray = NULL;
}

void Uint32ArrayResize(UINT32_ARRAY* uint32_array, size_t new_size)
{
	size_t alloc_size;

	if(uint32_array->buffer_size == new_size)
	{
		return;
	}
	if(uint32_array->block_size > 1)
	{
		alloc_size = ((new_size + uint32_array->block_size - 1)
			/ uint32_array->block_size) * uint32_array->block_size;
	}
	else
	{
		alloc_size = new_size;
	}
	uint32_array->buffer = (uint32*)MEM_REALLOC_FUNC(
		uint32_array->buffer, sizeof(uint32) * alloc_size);
	if(alloc_size < uint32_array->num_data)
	{
		uint32_array->num_data = alloc_size-1;
	}
	uint32_array->buffer_size = alloc_size;
}

POINTER_ARRAY* PointerArrayNew(size_t block_size)
{
	POINTER_ARRAY *ret;

	if(block_size == 0)
	{
		return NULL;
	}

	ret = (POINTER_ARRAY*)MEM_ALLOC_FUNC(sizeof(*ret));
	ret->num_data = 0;
	ret->block_size = ret->buffer_size = block_size;
	ret->buffer = (void**)MEM_ALLOC_FUNC(sizeof(*ret)*block_size);
	(void)memset(ret->buffer, 0, sizeof(void*)*block_size);

	return ret;
}

void PointerArrayRelease(
	POINTER_ARRAY* pointer_array,
	void (*destroy_func)(void*)
)
{
	size_t i;
	for(i=0; i<pointer_array->num_data; i++)
	{
		destroy_func(pointer_array->buffer[i]);
	}
	pointer_array->num_data = 0;
}

void PointerArrayDestroy(
	POINTER_ARRAY** pointer_array,
	void (*destroy_func)(void*)
)
{
	if(pointer_array == NULL || *pointer_array == NULL)
	{
		return;
	}

	if(destroy_func != NULL)
	{
		PointerArrayRelease(*pointer_array, destroy_func);
	}

	MEM_FREE_FUNC((*pointer_array)->buffer);
	MEM_FREE_FUNC(*pointer_array);

	*pointer_array = NULL;
}

void PointerArrayAppend(POINTER_ARRAY* pointer_array, void* data)
{
	pointer_array->buffer[pointer_array->num_data] = data;
	pointer_array->num_data++;

	if(pointer_array->num_data == pointer_array->buffer_size)
	{
		size_t before_size = pointer_array->buffer_size;
		pointer_array->buffer_size += pointer_array->block_size;
		pointer_array->buffer = (void**)MEM_REALLOC_FUNC(
			pointer_array->buffer, pointer_array->buffer_size * sizeof(void*));
		(void)memset(&pointer_array->buffer[before_size], 0,
			pointer_array->block_size * sizeof(void*));
	}
}

void PointerArrayRemoveByIndex(
	POINTER_ARRAY* pointer_array,
	size_t index,
	void (*destroy_func)(void*)
)
{
	if(index < pointer_array->num_data)
	{
		size_t i;

		if(destroy_func != NULL)
		{
			destroy_func(pointer_array->buffer[index]);
		}

		pointer_array->num_data--;
		for(i=index; i<pointer_array->num_data; i++)
		{
			pointer_array->buffer[i] = pointer_array->buffer[i+1];
		}
	}
}

void PointerArrayRemoveByData(
	POINTER_ARRAY* pointer_array,
	void* data,
	void (*destroy_func)(void*)
)
{
	size_t i, j;
	for(i=0; i<pointer_array->num_data; i++)
	{
		if(pointer_array->buffer[i] == data)
		{
			if(destroy_func != NULL)
			{
				destroy_func(data);
			}

			pointer_array->num_data--;
			for(j=i; j<pointer_array->num_data; j++)
			{
				pointer_array->buffer[j] = pointer_array->buffer[j+1];
			}

			return;
		}
	}
}

int PointerArrayDoesCointainData(POINTER_ARRAY* pointer_array, void* data)
{
	size_t i;
	for(i=0; i<pointer_array->num_data; i++)
	{
		if(pointer_array->buffer[i] == data)
		{
			return TRUE;
		}
	}

	return FALSE;
}

STRUCT_ARRAY* StructArrayNew(size_t data_size, size_t block_size)
{
	STRUCT_ARRAY *ret;

	if(block_size == 0)
	{
		return NULL;
	}

	ret = (STRUCT_ARRAY*)MEM_ALLOC_FUNC(sizeof(*ret));
	ret->num_data = 0;
	ret->data_size = data_size;
	ret->block_size = ret->buffer_size = block_size;
	ret->buffer = (uint8*)MEM_ALLOC_FUNC(data_size*block_size);
	(void)memset(ret->buffer, 0, data_size*block_size);

	return ret;
}

void StructArrayDestroy(
	STRUCT_ARRAY** struct_array,
	void (*destroy_func)(void*)
)
{
	if(destroy_func != NULL)
	{
		size_t i;
		uint8 *buffer = (uint8*)(*struct_array)->buffer;

		for(i=0; i<(*struct_array)->num_data; i++)
		{
			destroy_func(&buffer[i*(*struct_array)->data_size]);
		}
	}

	MEM_FREE_FUNC((*struct_array)->buffer);
	MEM_FREE_FUNC(*struct_array);

	*struct_array = NULL;
}

void StructArrayAppend(STRUCT_ARRAY* struct_array, void* data)
{
	uint8 *dst = struct_array->buffer;
	(void)memcpy(&dst[struct_array->num_data*struct_array->data_size],
		data, struct_array->data_size);
	struct_array->num_data++;

	if(struct_array->num_data == struct_array->buffer_size)
	{
		size_t before_size = struct_array->buffer_size;
		struct_array->buffer_size += struct_array->block_size;
		dst = struct_array->buffer = (uint8*)MEM_REALLOC_FUNC(
			struct_array->buffer, struct_array->buffer_size * struct_array->data_size);
		(void)memset(&dst[before_size*struct_array->data_size], 0,
			struct_array->block_size * struct_array->data_size);
	}
}

void* StructArrayReserve(STRUCT_ARRAY* struct_array)
{
	void *ret = (void*)&struct_array->buffer[struct_array->data_size*struct_array->num_data];
	struct_array->num_data++;

	if(struct_array->num_data == struct_array->buffer_size)
	{
		uint8 *dst;
		size_t before_size = struct_array->buffer_size;
		struct_array->buffer_size += struct_array->block_size;
		dst = struct_array->buffer = (uint8*)MEM_REALLOC_FUNC(
			struct_array->buffer, struct_array->buffer_size * struct_array->data_size);
		(void)memset(&dst[before_size*struct_array->data_size], 0,
			struct_array->block_size * struct_array->data_size);
		ret = (void*)&dst[(before_size-1)*struct_array->data_size];
	}

	return ret;
}

void StructArrayResize(STRUCT_ARRAY* struct_array, size_t new_size)
{
	size_t alloc_size;

	if(struct_array->buffer_size == new_size)
	{
		return;
	}
	if(struct_array->block_size > 1)
	{
		alloc_size = ((new_size + struct_array->block_size - 1)
			/ struct_array->block_size) * struct_array->block_size;
	}
	else
	{
		alloc_size = new_size;
	}
	struct_array->buffer = (uint8*)MEM_REALLOC_FUNC(
		struct_array->buffer, struct_array->data_size * alloc_size);
	if(alloc_size < struct_array->num_data)
	{
		struct_array->num_data = alloc_size-1;
	}
	struct_array->buffer_size = alloc_size;
}

void StructArrayRemoveByIndex(
	STRUCT_ARRAY* struct_array,
	size_t index,
	void (*destroy_func)(void*)
)
{
	if(index < struct_array->num_data)
	{
		size_t i;
		uint8 *buffer = struct_array->buffer;

		if(destroy_func != NULL)
		{
			destroy_func(&buffer[index*struct_array->data_size]);
		}

		struct_array->num_data--;
		for(i=index; i<struct_array->num_data; i++)
		{
			(void)memcpy(&buffer[i*struct_array->data_size],
				&buffer[(i+1)*struct_array->data_size], struct_array->data_size);
		}
	}
}
