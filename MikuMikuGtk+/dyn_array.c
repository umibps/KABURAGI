#include <string.h>
#include "dyn_array.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

void InitializeDynPointerArray(DYN_POINTER_ARRAY* dyn_array, int block_size)
{
	(void)memset(dyn_array, 0, sizeof(*dyn_array));
	dyn_array->block_size = block_size;
	dyn_array->memory = (void**)MEM_ALLOC_FUNC(sizeof(void*)*block_size);
	dyn_array->allocated = block_size;
}

void ReleaseDynPointerArray(DYN_POINTER_ARRAY* dyn_array, void (*destroy_func)(void*))
{
	if(destroy_func != NULL)
	{
		int i;
		for(i=0; i<dyn_array->size; i++)
		{
			destroy_func(dyn_array->memory[i]);
		}
	}

	MEM_FREE_FUNC(dyn_array->memory);
}

void DynPointerArrayClear(DYN_POINTER_ARRAY* dyn_array)
{
	dyn_array->size = 0;
}

static void EnsureCapacity(DYN_POINTER_ARRAY* dyn_array, int capacity)
{
	if(capacity > dyn_array->allocated)
	{
		int new_capacity = capacity * 2;
		dyn_array->memory = (void**)MEM_REALLOC_FUNC(
			dyn_array->memory, sizeof(void*)*new_capacity);
		dyn_array->allocated = new_capacity;
	}
}

void DynPointerArrayPush(DYN_POINTER_ARRAY* dyn_array, const void* data)
{
	EnsureCapacity(dyn_array, dyn_array->size + 1);
	dyn_array->memory[dyn_array->size++] = (void*)data;
}

void** DynPointerArrayPushArray(DYN_POINTER_ARRAY* dyn_array, int count)
{
	void *ret;
	EnsureCapacity(dyn_array, dyn_array->size + count);
	ret = &dyn_array->memory[dyn_array->size];
	dyn_array->size += count;
	return ret;
}

void* DynPointerArrayPop(DYN_POINTER_ARRAY* dyn_array)
{
	return dyn_array->memory[--dyn_array->size];
}

void DynPoniterArrayPopArray(DYN_POINTER_ARRAY* dyn_array, int count)
{
	dyn_array->size -= count;
}

int DynPointerArrayIsEmpty(DYN_POINTER_ARRAY* dyn_array)
{
	return dyn_array->size == 0;
}

void* DynPointerArrayPeekTop(DYN_POINTER_ARRAY* dyn_array)
{
	return dyn_array->memory[dyn_array->size-1];
}

void InitializeDynCharacterArray(DYN_CHARACTER_ARRAY* dyn_array, int block_size)
{
	(void)memset(dyn_array, 0, sizeof(*dyn_array));
	dyn_array->block_size = block_size;
	dyn_array->buffer = (char*)MEM_ALLOC_FUNC(sizeof(*dyn_array->buffer)*block_size);
	dyn_array->allocated = block_size;
}

void ReleaseDynCharacterArray(DYN_CHARACTER_ARRAY* dyn_array)
{
	MEM_FREE_FUNC(dyn_array->buffer);
}

void DynCharacterArrayClear(DYN_CHARACTER_ARRAY* dyn_array)
{
	dyn_array->size = 0;
}

static void EnsureCapacityCharacterArray(DYN_CHARACTER_ARRAY* dyn_array, int capacity)
{
	if(capacity > dyn_array->allocated)
	{
		int new_capacity = capacity * 2;
		dyn_array->buffer = (char*)MEM_REALLOC_FUNC(
			dyn_array->buffer, sizeof(*dyn_array->buffer)*new_capacity);
		dyn_array->allocated = new_capacity;
	}
}

void DynCharacterArrayPush(DYN_CHARACTER_ARRAY* dyn_array, char data)
{
	EnsureCapacityCharacterArray(dyn_array, dyn_array->size + 1);
	dyn_array->buffer[dyn_array->size++] = data;
}

char* DynCharacterArrayPushArray(DYN_CHARACTER_ARRAY* dyn_array, int count)
{
	char *ret;
	EnsureCapacityCharacterArray(dyn_array, dyn_array->size + count);
	ret = &dyn_array->buffer[dyn_array->size];
	dyn_array->size += count;
	return ret;
}

char DynCharacterArrayPop(DYN_CHARACTER_ARRAY* dyn_array)
{
	return dyn_array->buffer[--dyn_array->size];
}

void DynCharacterArrayPopArray(DYN_CHARACTER_ARRAY* dyn_array, int count)
{
	dyn_array->size -= count;
}

int DynCharacterArrayIsEmpty(DYN_CHARACTER_ARRAY* dyn_array)
{
	return dyn_array->size == 0;
}

char DynCharacterArrayPeekTop(DYN_CHARACTER_ARRAY* dyn_array)
{
	return dyn_array->buffer[dyn_array->size-1];
}

#ifdef __cplusplus
}
#endif
