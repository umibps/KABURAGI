// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include "memory_stream.h"
#include "plug_in.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

int IsCorretPlugIn(const char* module_path)
{
	GModule *module = g_module_open(module_path, G_MODULE_BIND_LAZY);
	ExecutePlugInFunc func;
	int ret = FALSE;

	if(module == NULL)
	{
		return FALSE;
	}

	if(g_module_symbol(module, "Execute", (gpointer*)&func) != FALSE)
	{
		ret = TRUE;
	}

	(void)g_module_close(module);

	return ret;
}

void ExecutePlugInMenu(GtkWidget* menu, APPLICATION* app)
{
	GModule *module;
	gchar *plug_in_name = (gchar*)g_object_get_data(G_OBJECT(menu), "plug-in-name");
	gchar *module_path = g_build_filename(
		app->current_path, PLUG_IN_DIRECTORY, plug_in_name, NULL);
	ExecutePlugInFunc func;
	gboolean keep = FALSE;

	module = g_module_open(module_path, G_MODULE_BIND_LAZY);

	if(module != NULL)
	{
		gdk_threads_enter();

		if(g_module_symbol(module, "Execute", (gpointer*)&func) != FALSE)
		{
			keep = func(app, plug_in_name, module);
		}

		gdk_threads_leave();

		if(keep == FALSE)
		{
			(void)g_module_close(module);
		}
	}

	g_free(module_path);
}

void ExecutePlugInUndo(DRAW_WINDOW* window, void* data)
{
	MEMORY_STREAM stream = {(uint8*)data, 0, 4096, 1};
	GModule *module;
	PlugInUndoRedoFunc func;
	char plug_in_name[4096];
	guint32 data_length;
	gchar *module_path;

	(void)MemRead(&data_length, sizeof(data_length), 1, &stream);
	(void)MemRead(plug_in_name, 1, data_length, &stream);

	module_path = g_build_filename(
		window->app->current_path, PLUG_IN_DIRECTORY, plug_in_name, NULL);
	module = g_module_open(module_path, G_MODULE_BIND_LAZY);

	if(module != NULL)
	{
		if(g_module_symbol(module, "Undo", (gpointer*)&func) != FALSE)
		{
			(void)MemRead(&data_length, sizeof(data_length), 1, &stream);
			func(window, (void*)&stream.buff_ptr[stream.data_point], data_length);
		}

		(void)g_module_close(module);
	}

	g_free(module_path);
}

void ExecutePlugInRedo(DRAW_WINDOW* window, void* data)
{
	MEMORY_STREAM stream = {(uint8*)data, 0, 4096, 1};
	GModule *module;
	PlugInUndoRedoFunc func;
	char plug_in_name[4096];
	guint32 data_length;
	gchar *module_path;

	(void)MemRead(&data_length, sizeof(data_length), 1, &stream);
	(void)MemRead(plug_in_name, 1, data_length, &stream);

	module_path = g_build_filename(
		window->app->current_path, PLUG_IN_DIRECTORY, plug_in_name, NULL);
	module = g_module_open(module_path, G_MODULE_BIND_LAZY);

	if(module != NULL)
	{
		if(g_module_symbol(module, "Redo", (gpointer*)&func) != FALSE)
		{
			(void)MemRead(&data_length, sizeof(data_length), 1, &stream);
			func(window, (void*)&stream.buff_ptr[stream.data_point], data_length);
		}

		(void)g_module_close(module);
	}

	g_free(module_path);
}

void AddPlugInHistory(DRAW_WINDOW* window, const char* plug_in_name, void* data, size_t data_size)
{
	MEMORY_STREAM *stream = CreateMemoryStream(data_size + 4096);
	guint32 data_length;

	data_length = (guint32)strlen(plug_in_name) + 1;
	(void)MemWrite(&data_length, sizeof(data_length), 1, stream);
	(void)MemWrite(plug_in_name, 1, data_length, stream);

	data_length = (guint32)data_size;
	(void)MemWrite(&data_length, sizeof(data_length), 1, stream);
	(void)MemWrite(data, 1, data_size, stream);

	AddHistory(&window->history, plug_in_name, stream->buff_ptr, stream->data_point,
		ExecutePlugInUndo, ExecutePlugInRedo);

	(void)DeleteMemoryStream(stream);
}

static GtkWidget* LoadPlugInItemLoop(
	const char* directory_path,
	const char* rel_path,
	GtkWidget* parent,
	APPLICATION* app
)
{
	GtkWidget *menu = gtk_menu_new();
	GtkWidget *menu_item;
	GDir *directory = g_dir_open(directory_path, 0, NULL);
	const gchar *file_name;
	char file_path[4096];
	gchar *relative_path = g_strdup(rel_path);

	while((file_name = g_dir_read_name(directory)) != NULL)
	{
		(void)sprintf(file_path, "%s/%s", directory_path, file_name);
		if(g_file_test(file_path, G_FILE_TEST_IS_DIR) != FALSE)
		{
			menu_item = gtk_menu_item_new_with_label(file_name);
			menu = gtk_menu_new();
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);
			gtk_menu_shell_append(GTK_MENU_SHELL(parent), menu_item);
			LoadPlugInItemLoop(file_path, relative_path, menu, app);
		}
		else
		{
			if(IsCorretPlugIn(file_path) != FALSE)
			{
				char *menu_name = MEM_STRDUP_FUNC(file_name);
				char *extention = NULL;
				char *ptr = menu_name;
				while(*ptr != '\0')
				{
					if(*ptr == '.')
					{
						extention = ptr;
					}
					ptr = g_utf8_next_char(ptr);
				}
				if(extention != NULL)
				{
					*extention = '\0';
				}
				menu_item = gtk_menu_item_new_with_label(menu_name);
				gtk_menu_shell_append(GTK_MENU_SHELL(parent), menu_item);
				g_object_set_data(G_OBJECT(menu_item), "plug-in-name",
					g_build_filename(relative_path, file_name, NULL));
				(void)g_signal_connect(G_OBJECT(menu_item), "activate",
					G_CALLBACK(ExecutePlugInMenu), app);
				MEM_FREE_FUNC(menu_name);
			}
		}
	}

	g_free(relative_path);

	return menu;
}

GtkWidget* PlugInMenuItemNew(
	APPLICATION* app
)
{
	GtkWidget *menu_item;
	GtkWidget *menu;
	gchar *plug_in_path;
	char buff[512];

	(void)sprintf(buff, "_%s(_P)", app->labels->menu.plug_in);
	menu_item = gtk_menu_item_new_with_mnemonic(buff);
	menu = gtk_menu_new();
	plug_in_path = g_build_filename(app->current_path, PLUG_IN_DIRECTORY, NULL);

	gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu_item), menu);
	LoadPlugInItemLoop(plug_in_path, "", menu, app);

	g_free(plug_in_path);

	return menu_item;
}

#ifdef __cplusplus
}
#endif
