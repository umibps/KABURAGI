// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string.h>
#include "memory.h"
#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

static void ReleaseRedoData(HISTORY* history)
{
	int ref;
	int i;

	for(i=0; i<history->rest_redo; i++)
	{
		ref = (history->point+i) % HISTORY_BUFFER_SIZE;
		MEM_FREE_FUNC(history->history[ref].data);
		history->history[ref].data = NULL;
	}
	history->rest_redo = 0;
}

static void AddHistoryData(
	HISTORY_DATA* history,
	const gchar* name,
	const void* data,
	size_t data_size,
	history_func undo,
	history_func redo
)
{
	(void)strcpy(history->name, name);
	history->data_size = data_size;
	history->undo = undo;
	history->redo = redo;
	history->data = MEM_ALLOC_FUNC(data_size);
	(void)memcpy(history->data, data, data_size);
}

void AddHistory(
	HISTORY* history,
	const gchar* name,
	const void* data,
	size_t data_size,
	history_func undo,
	history_func redo
)
{
	ReleaseRedoData(history);

	history->num_step++;

	if(history->rest_undo < HISTORY_BUFFER_SIZE)
	{
		history->rest_undo++;
	}

	if(history->point >= HISTORY_BUFFER_SIZE)
	{
		history->point = 0;
		MEM_FREE_FUNC(history->history->data);
		AddHistoryData(history->history, name, data, data_size, undo, redo);
	}
	else
	{
		MEM_FREE_FUNC(history->history[history->point].data);
		AddHistoryData(&history->history[history->point],
			name, data, data_size, undo, redo);
	}

	history->point++;

	history->flags |= HISTORY_UPDATED;

#ifdef _DEBUG
	g_print("History Add : %s\n", name);
#endif
}

void ExecuteUndo(struct _APPLICATION* app)
{
	DRAW_WINDOW *window = GetActiveDrawWindow(app);

	if(window->history.rest_undo > 0)
	{
		int execute = (window->history.point == 0) ? HISTORY_BUFFER_SIZE - 1
			: window->history.point - 1;
		window->history.history[execute].undo(
			window, window->history.history[execute].data
		);
		window->history.rest_undo--;
		window->history.rest_redo++;

		if(window->history.point > 0)
		{
			window->history.point--;
		}
		else
		{
			window->history.point = HISTORY_BUFFER_SIZE - 1;
		}
	}

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;

	gtk_widget_queue_draw(window->window);
}

void ExecuteRedo(struct _APPLICATION *app)
{
	DRAW_WINDOW *window = GetActiveDrawWindow(app);

	if(window->history.rest_redo > 0)
	{
		int execute = window->history.point;
		if(execute == HISTORY_BUFFER_SIZE)
		{
			execute = 0;
		}
		window->history.history[execute].redo(
			window, window->history.history[execute].data
		);
		window->history.point++;
		window->history.rest_undo++;
		window->history.rest_redo--;

		if(window->history.point == HISTORY_BUFFER_SIZE)
		{
			window->history.point = 0;
		}
	}

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;

	gtk_widget_queue_draw(window->window);
}

#ifdef __cplusplus
}
#endif
