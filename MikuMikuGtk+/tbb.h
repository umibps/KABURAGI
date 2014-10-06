#ifndef _INCLUDED_TBB_H_
#define _INCLUDED_TBB_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

EXTERN void* TbbObjectNew(void);

EXTERN void DeleteTbbObject(void* scheduler);

EXTERN void* SimpleParallelProcessorNew(
	void* buffer,
	size_t data_size,
	int num_data,
	void (*process_func)(void*, int, void*),
	void* user_data
);

EXTERN void DeleteSimpleParallelProcessor(void* processor);

EXTERN void ExecuteSimpleParallelProcessor(void* processor);

typedef enum _ePARALLEL_PROCESSOR_TYPE
{
	PARALLEL_PROCESSOR_TYPE_FOR_LOOP,
	PARALLEL_PROCESSOR_TYPE_REDUCTION
} ePARALLEL_PROCESSOR_TYPE;

EXTERN void* ParallelProcessorNew(
	void* buffer,
	void (*process_func)(void*, int, void*),
	void (*start_func)(void*),
	void (*end_func)(void*),
	void (*initialize_func)(void*, void*),
	void (*join_func)(void*, void*),
	size_t data_size,
	int num_data,
	void* user_data,
	size_t user_data_size,
	ePARALLEL_PROCESSOR_TYPE type
);

EXTERN void DeleteParallelProcessor(void* processor);

EXTERN void ExecuteParallelProcessor(void* processor);

#ifdef __cplusplus
};
#endif

#endif	// #ifndef _INCLUDED_TBB_H_
