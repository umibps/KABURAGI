#ifndef _INCLUDED_HISTORY_H_
#define _INCLUDED_HISTORY_H_

#define MAX_HISTORY_NUM 128
#define HISTORY_MAX_NAME_LEN 128

typedef void (*history_func)(struct _HISTORY_DATA* data, struct _PROJECT* project);

typedef struct _HISTORY_DATA
{
	uint8 *data;
	size_t data_size;
	char name[HISTORY_MAX_NAME_LEN];
	history_func undo;
	history_func redo;
} HISTORY_DATA;

typedef struct _HISTORY
{
	HISTORY_DATA histories[MAX_HISTORY_NUM];
	int point;
	int num_step;
	int rest_undo;
	int rest_redo;
} HISTORY;

extern void AddControlHistory(
	HISTORY* history,
	const char* name,
	const void* data,
	size_t data_size,
	history_func undo,
	history_func redo
);

#endif	// #ifndef _INCLUDED_HISTORY_H_
