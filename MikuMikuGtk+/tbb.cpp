
#include <stdio.h>
#include <tbb/parallel_for.h>
#include <tbb/parallel_reduce.h>
#include <tbb/blocked_range.h>
#include <tbb/task_scheduler_init.h>
#include "tbb.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

void* TbbObjectNew(void)
{
#ifdef _DEBUG

# define NUM_OF_THREADS 1
	tbb::task_scheduler_init *tbb = new tbb::task_scheduler_init(NUM_OF_THREADS);
#else
	tbb::task_scheduler_init *tbb = new tbb::task_scheduler_init();
#endif

	return static_cast<void*>(tbb);
}

void DeleteTbbObject(void* scheduler)
{
	tbb::task_scheduler_init *tbb = static_cast<tbb::task_scheduler_init*>(scheduler);

	delete tbb;
}

class SIMPLE_PARALLEL_PROCESSOR
{
public:
	SIMPLE_PARALLEL_PROCESSOR(
		void* buffer,
		void (*process_func)(void*, int, void*),
		size_t data_size,
		int num_data,
		void* user_data
	)
	{
		m_buffer_ref = (unsigned char*)buffer;
		m_process_func = process_func;
		m_data_size = data_size;
		m_num_data = num_data;
		m_user_data = user_data;
	}

	~SIMPLE_PARALLEL_PROCESSOR()
	{
		m_buffer_ref = NULL;
		m_process_func = NULL;
		m_data_size = 0;
		m_num_data = 0;
	}

	void operator()(const tbb::blocked_range<int> &range) const
	{
		for(int i=range.begin(); i != range.end(); ++i)
		{
			void *data = (void*)&m_buffer_ref[m_data_size*i];
			m_process_func(data, i, m_user_data);
		}
	}

	void execute(void)
	{
		static tbb::affinity_partitioner affinity_partitioner;
		tbb::parallel_for(tbb::blocked_range<int>(0, m_num_data), *this, affinity_partitioner);
	}

private:
	unsigned char *m_buffer_ref;
	void (*m_process_func)(void*, int, void*);
	size_t m_data_size;
	int m_num_data;
	void *m_user_data;
};

void* SimpleParallelProcessorNew(
	void* buffer,
	size_t data_size,
	int num_data,
	void (*process_func)(void*, int, void*),
	void* user_data
)
{
	SIMPLE_PARALLEL_PROCESSOR *processor = new SIMPLE_PARALLEL_PROCESSOR(
		buffer, process_func, data_size, num_data, user_data);

	return static_cast<void*>(processor);
}

void DeleteSimpleParallelProcessor(void* processor)
{
	SIMPLE_PARALLEL_PROCESSOR *p = static_cast<SIMPLE_PARALLEL_PROCESSOR*>(processor);

	delete p;
}

void ExecuteSimpleParallelProcessor(void* processor)
{
	SIMPLE_PARALLEL_PROCESSOR *p = static_cast<SIMPLE_PARALLEL_PROCESSOR*>(processor);

	p->execute();
}

class PARALLEL_PROCESSOR
{
public:
	PARALLEL_PROCESSOR(
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
	)
	{
		m_buffer_ref = (unsigned char*)buffer;
		m_process_func = process_func;
		m_start_func = (start_func != NULL) ? start_func : dummy;
		m_end_func = (end_func != NULL) ? end_func : dummy;
		m_initialize_func =
			(initialize_func != NULL) ? initialize_func : (void (*)(void*, void*))dummy;
		m_join_func = (join_func != NULL) ? join_func : (void (*)(void*, void*))dummy;
		m_data_size = data_size;
		m_num_data = num_data;
		m_user_data = user_data;
		m_user_data_size = user_data_size;
		m_type = type;

		switch(type)
		{
		case PARALLEL_PROCESSOR_TYPE_FOR_LOOP:
			PARALLEL_PROCESSOR::m_execute_method = &PARALLEL_PROCESSOR::execute_for_loop;
			PARALLEL_PROCESSOR::m_operator_method = &PARALLEL_PROCESSOR::for_loop_operator;
			break;
		case PARALLEL_PROCESSOR_TYPE_REDUCTION:
			PARALLEL_PROCESSOR::m_execute_method = &PARALLEL_PROCESSOR::execute_reduction;
			PARALLEL_PROCESSOR::m_operator_method = &PARALLEL_PROCESSOR::reduction_operator;
			m_local_user_data = MEM_ALLOC_FUNC(m_user_data_size);
			m_initialize_func(m_user_data, m_local_user_data);
			break;
		}
	}

	PARALLEL_PROCESSOR(const PARALLEL_PROCESSOR& self, tbb::split)
	{
		m_type = self.m_type;
		m_buffer_ref = self.m_buffer_ref;
		m_process_func = self.m_process_func;
		m_data_size = self.m_data_size;
		m_num_data = self.m_num_data;
		m_user_data = self.m_user_data;
		m_user_data_size = self.m_user_data_size;
		m_initialize_func = self.m_initialize_func;
		m_join_func = self.m_join_func;
		m_start_func = self.m_start_func;
		m_end_func = self.m_end_func;
		PARALLEL_PROCESSOR::m_execute_method = self.m_execute_method;
		m_operator_method = self.m_operator_method;
		m_local_user_data = MEM_ALLOC_FUNC(m_user_data_size);
		m_initialize_func(self.m_local_user_data, m_local_user_data);
	}

	~PARALLEL_PROCESSOR()
	{
		m_buffer_ref = NULL;
		m_process_func = NULL;
		m_data_size = 0;
		m_num_data = 0;

		switch(m_type)
		{
		case PARALLEL_PROCESSOR_TYPE_FOR_LOOP:
			break;
		case PARALLEL_PROCESSOR_TYPE_REDUCTION:
			MEM_FREE_FUNC(m_local_user_data);
			break;
		}
	}

	void operator()(const tbb::blocked_range<int> &range) const
	{
		(this->*m_operator_method)(range);
	}

	void execute(void)
	{
		(this->*m_execute_method)();
	}

	void join(const PARALLEL_PROCESSOR& self)
	{
		m_join_func(m_user_data, self.m_local_user_data);
	}

private:
	void execute_for_loop(void)
	{
		static tbb::affinity_partitioner affinity_partitioner;
		tbb::parallel_for(tbb::blocked_range<int>(0, m_num_data), *this, affinity_partitioner);
	}

	void execute_reduction(void)
	{
		tbb::parallel_reduce(tbb::blocked_range<int>(0, m_num_data), *this);
	}

	void for_loop_operator(const tbb::blocked_range<int> &range) const
	{
		m_start_func(m_user_data);
		for(int i=range.begin(); i != range.end(); ++i)
		{
			void *data = (void*)&m_buffer_ref[m_data_size*i];
			m_process_func(data, i, m_user_data);
		}
		m_end_func(m_user_data);
	}

	void reduction_operator(const tbb::blocked_range<int> &range) const
	{
		m_start_func(m_local_user_data);
		for(int i=range.begin(); i != range.end(); ++i)
		{
			void *data = (void*)&m_buffer_ref[m_data_size*i];
			m_process_func(data, i, m_user_data);
		}
		m_end_func(m_local_user_data);
	}

	static void dummy(void* user_data)
	{
	}

	ePARALLEL_PROCESSOR_TYPE m_type;
	unsigned char *m_buffer_ref;
	void (*m_process_func)(void*, int, void*);
	size_t m_data_size;
	int m_num_data;
	void *m_user_data;
	size_t m_user_data_size;
	void (*m_initialize_func)(void*, void*);
	void (*m_start_func)(void*);
	void (*m_end_func)(void*);
	void (*m_join_func)(void*, void*);
	void (PARALLEL_PROCESSOR::*m_execute_method)(void);
	void (PARALLEL_PROCESSOR::*m_operator_method)(const tbb::blocked_range<int> &range) const;
	void* m_local_user_data;
};

void* ParallelProcessorNew(
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
)
{
	PARALLEL_PROCESSOR *processor = new PARALLEL_PROCESSOR(
		buffer, process_func, start_func, end_func, initialize_func, join_func, data_size,
			num_data, user_data, user_data_size, type);

	return static_cast<void*>(processor);
}

void DeleteParallelProcessor(void* processor)
{
	PARALLEL_PROCESSOR *p = static_cast<PARALLEL_PROCESSOR*>(processor);

	delete p;
}

void ExecuteParallelProcessor(void* processor)
{
	PARALLEL_PROCESSOR *p = static_cast<PARALLEL_PROCESSOR*>(processor);

	p->execute();
}

#ifdef __cplusplus
};
#endif
