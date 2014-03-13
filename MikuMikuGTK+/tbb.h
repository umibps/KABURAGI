#ifndef _INCLUDED_TBB_H_
#define _INCLUDED_TBB_H_

#ifdef __cplusplus
extern "C" {
#endif

extern void* TbbObjectNew(void);

extern void DeleteTbbObject(void* scheduler);

extern void* SimpleParallelProcessorNew(
	void* buffer,
	size_t data_size,
	int num_data,
	void (*process_func)(void*, int, void*),
	void* user_data
);

extern void DeleteSimpleParallelProcessor(void* processor);

extern void ExecuteSimpleParallelProcessor(void* processor);

typedef enum _ePARALLEL_PROCESSOR_TYPE
{
	PARALLEL_PROCESSOR_TYPE_FOR_LOOP,
	PARALLEL_PROCESSOR_TYPE_REDUCTION
} ePARALLEL_PROCESSOR_TYPE;

extern void* ParallelProcessorNew(
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

extern void DeleteParallelProcessor(void* processor);

extern void ExecuteParallelProcessor(void* processor);

#ifdef __cplusplus
};
#endif

#endif	// #ifndef _INCLUDED_TBB_H_
