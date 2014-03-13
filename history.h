#ifndef _INCLUDED_HISTORY_H_
#define _INCLUDED_HISTORY_H_

#include <stdio.h>
#include <gtk/gtk.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define HISTORY_BUFFER_SIZE 256
#define HISTORY_MAX_NAME_LEN 128

typedef enum _eHISTORY_FLAGS
{
	HISTORY_UPDATED = 0x01
} eHISTORY_FLAGS;

typedef void (*history_func)(struct _DRAW_WINDOW* window, void* data);

typedef struct _HISTORY_DATA
{
	gchar name[HISTORY_MAX_NAME_LEN];
	size_t data_size;
	void* data;
	history_func undo, redo;
} HISTORY_DATA;

typedef struct _HISTORY
{
	uint16 point;
	uint16 num_step;
	uint16 rest_undo;
	uint16 rest_redo;

	uint32 flags;

	HISTORY_DATA history[HISTORY_BUFFER_SIZE];
} HISTORY;

extern void AddHistory(
	HISTORY* history,
	const gchar* name,
	const void* data,
	size_t data_size,
	history_func undo,
	history_func redo
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_HISTORY_H_
