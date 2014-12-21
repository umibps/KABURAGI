#ifndef _INCLUDED_DYN_ARRAY_H_
#define _INCLUDED_DYN_ARRAY_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
# define EXTERN extern __declspec(dllexport)
#else
# define EXTERN extern
#endif

typedef struct _DYN_POINTER_ARRAY
{
	void **memory;
	int allocated;
	int size;
	int block_size;
} DYN_POINTER_ARRAY;

typedef struct _DYN_CHARACTER_ARRAY
{
	char *buffer;
	int allocated;
	int size;
	int block_size;
} DYN_CHARACTER_ARRAY;

EXTERN void InitializeDynPointerArray(DYN_POINTER_ARRAY* dyn_array, int block_size);

EXTERN void ReleaseDynPointerArray(DYN_POINTER_ARRAY* dyn_array, void (*destroy_func)(void*));

EXTERN void DynPointerArrayClear(DYN_POINTER_ARRAY* dyn_array);

EXTERN void DynPointerArrayPush(DYN_POINTER_ARRAY* dyn_array, const void* data);

EXTERN void** DynPointerArrayPushArray(DYN_POINTER_ARRAY* dyn_array, int count);

EXTERN void* DynPointerArrayPop(DYN_POINTER_ARRAY* dyn_array);

EXTERN void DynPoniterArrayPopArray(DYN_POINTER_ARRAY* dyn_array, int count);

EXTERN int DynPointerArrayIsEmpty(DYN_POINTER_ARRAY* dyn_array);

EXTERN void* DynPointerArrayPeekTop(DYN_POINTER_ARRAY* dyn_array);

EXTERN void InitializeDynCharacterArray(DYN_CHARACTER_ARRAY* dyn_array, int block_size);

EXTERN void ReleaseDynCharacterArray(DYN_CHARACTER_ARRAY* dyn_array);

EXTERN void DynCharacterArrayClear(DYN_CHARACTER_ARRAY* dyn_array);

EXTERN void DynCharacterArrayPush(DYN_CHARACTER_ARRAY* dyn_array, char data);

EXTERN char* DynCharacterArrayPushArray(DYN_CHARACTER_ARRAY* dyn_array, int count);

EXTERN char DynCharacterArrayPop(DYN_CHARACTER_ARRAY* dyn_array);

EXTERN void DynCharacterArrayPopArray(DYN_CHARACTER_ARRAY* dyn_array, int count);

EXTERN char DynCharacterArrayPeekTop(DYN_CHARACTER_ARRAY* dyn_array);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_DYN_ARRAY_H_
