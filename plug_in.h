#ifndef _INCLUDED_PLUG_IN_H_
#define _INCLUDED_PLUG_IN_H_

#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*ExecutePlugInFunc)(APPLICATION* app, const char* plug_in_name, GModule* module);
typedef void (*PlugInUndoRedoFunc)(DRAW_WINDOW* canvas, void* data, size_t data_length);

typedef GtkWidget* (*SettingWidgetNewFunc)(APPLICATION* application);
typedef brush_core_func BrushInputCallbackFunc;
typedef brush_update_func BrushUpdateCallbackFunc;
typedef void (*BrushDrawCursorFunc)(
	DRAW_WINDOW* canvas, gdouble x, gdouble y, void* data);
typedef void (*BrushChangeColorFunc)(const uint8 color[3], void* data);

EXTERN int IsCorretPlugIn(const char* module_path);

EXTERN void ExecutePlugInMenu(GtkWidget* menu, APPLICATION* app);

EXTERN GtkWidget* PlugInMenuItemNew(
	APPLICATION* app
);

EXTERN void AddPlugInHistory(DRAW_WINDOW* window, const char* plug_in_name, void* data, size_t data_size);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_PLUG_IN_H_
