#ifndef _INCLUDED_MEMORY_H_
#define _INCLUDED_MEMORY_H_

#include "configure.h"

#ifdef _DEBUG
# if defined(CHECK_MEMORY_POOL) && CHECK_MEMORY_POOL == 1
#  define _CRTDBG_MAP_ALLOC
#  include <stdlib.h>
#  include <crtdbg.h>
# else
#  include <stdlib.h>
# endif
// #include "dlmalloc.h"


#ifdef _DEBUG
# if defined(CHECK_MEMORY_POOL) && CHECK_MEMORY_POOL == 1
#  define   malloc(s)             _malloc_dbg(s, _NORMAL_BLOCK, __FILE__, __LINE__)
#  define   calloc(c, s)          _calloc_dbg(c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#  define   realloc(p, s)         _realloc_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#  define   _recalloc(p, c, s)    _recalloc_dbg(p, c, s, _NORMAL_BLOCK, __FILE__, __LINE__)
#  define   _expand(p, s)         _expand_dbg(p, s, _NORMAL_BLOCK, __FILE__, __LINE__)
# endif
#endif

#define MEM_ALLOC_FUNC	malloc
#define MEM_CALLOC_FUNC	calloc
#define MEM_REALLOC_FUNC realloc
#define MEM_STRDUP_FUNC	strdup
#define MEM_FREE_FUNC	free
#else /* _DEBUG */
// #include "dlmalloc.h"
#include <stdlib.h>
/*
#define MEM_ALLOC_FUNC	dlmalloc
#define MEM_CALLOC_FUNC	dlcalloc
#define MEM_REALLOC_FUNC dlrealloc
#define MEM_STRDUP_FUNC	dlstrdup
#define MEM_FREE_FUNC	dlfree
*/
#define MEM_ALLOC_FUNC	malloc
#define MEM_CALLOC_FUNC	calloc
#define MEM_REALLOC_FUNC realloc
#define MEM_STRDUP_FUNC	strdup
#define MEM_FREE_FUNC	free

#endif

#endif /* _INCLUDED_MEMORY_H_ */
