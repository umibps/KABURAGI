#ifndef _INCLUDED_MAP_FUNCS_H_
#define _INCLUDED_MAP_FUNCS_H_

#include "types.h"

extern STR_STR_MAP* STR_STR_MAP_new(void);
extern STR_STR_MAPIterator STR_STR_MAP_begin(STR_STR_MAP* self);
extern STR_STR_MAPIterator STR_STR_MAP_next(STR_STR_MAPIterator pos);
extern STR_STR_MAPIterator STR_STR_MAP_end(STR_STR_MAP* self);
extern STR_STR_MAPIterator STR_STR_MAP_insert(STR_STR_MAP* self, char* key, char* value);
extern char* const *STR_STR_MAP_key(STR_STR_MAPIterator pos);
extern STR_STR_MAPIterator STR_STR_MAP_find(STR_STR_MAP* self, char* key);
extern char** STR_STR_MAP_value(STR_STR_MAPIterator pos);
extern int STR_STR_MAP_size(STR_STR_MAP* self);

extern STR_PTR_MAP* STR_PTR_MAP_new(void);
extern STR_PTR_MAPIterator STR_PTR_MAP_begin(STR_PTR_MAP* self);
extern STR_PTR_MAPIterator STR_PTR_MAP_next(STR_PTR_MAPIterator pos);
extern STR_PTR_MAPIterator STR_PTR_MAP_end(STR_PTR_MAP* self);
extern STR_PTR_MAPIterator STR_PTR_MAP_insert(STR_PTR_MAP* self, char* key, void* value);
extern void* const *STR_PTR_MAP_key(STR_PTR_MAPIterator pos);
extern STR_PTR_MAPIterator STR_PTR_MAP_find(STR_PTR_MAP* self, char* key);
extern void** STR_PTR_MAP_value(STR_PTR_MAPIterator pos);
extern int STR_PTR_MAP_size(STR_PTR_MAP* self);

extern PTR_PTR_MAP* PTR_PTR_MAP_new(void);
extern PTR_PTR_MAPIterator PTR_PTR_MAP_begin(PTR_PTR_MAP* self);
extern PTR_PTR_MAPIterator PTR_PTR_MAP_next(PTR_PTR_MAPIterator pos);
extern PTR_PTR_MAPIterator PTR_PTR_MAP_end(PTR_PTR_MAP* self);
extern PTR_PTR_MAPIterator PTR_PTR_MAP_insert(PTR_PTR_MAP* self, void* key, void* value);
extern void* const *PTR_PTR_MAP_key(PTR_PTR_MAPIterator pos);
extern PTR_PTR_MAPIterator PTR_PTR_MAP_find(PTR_PTR_MAP* self, void* key);
extern void** PTR_PTR_MAP_value(PTR_PTR_MAPIterator pos);
extern int PTR_PTR_MAP_size(PTR_PTR_MAP* self);

#endif	// #ifndef _INCLUDED_MAP_FUNCS_H_
