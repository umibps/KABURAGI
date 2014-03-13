#ifndef _INCLUDED_COMMON_TOOLS_H_
#define _INCLUDED_COMMON_TOOLS_H_

#include <gtk/gtk.h>
#include "draw_window.h"
#include "brush_core.h"
#include "ini_file.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*common_tool_func)(DRAW_WINDOW* window, gdouble x, gdouble y,
	struct _COMMON_TOOL_CORE* core, void* state);

typedef struct _COMMON_TOOL_CORE
{
	gdouble min_x, min_y, max_x, max_y;
	gchar* name;
	char* image_file_path;

	void* tool_data;
	common_tool_func press_func, motion_func, release_func;
	void (*display_func)(DRAW_WINDOW *window, struct _COMMON_TOOL_CORE* core);
	brush_update_func button_update, motion_update;
	GtkWidget* (*create_detail_ui)(struct _APPLICATION* app, void* data);
	GtkWidget* button;

	uint16 tool_type;
	uint8 flags;
	gchar hot_key;
} COMMON_TOOL_CORE;

typedef enum _eCOMMON_TOOL_FLAGS
{
	COMMON_TOOL_DO_RELEASE_FUNC_AT_CHANGI_TOOL = 0x01
} eCOMMON_TOOL_FLAGS;

typedef enum _eCOLOR_PICKER_SOURCE
{
	COLOR_PICKER_SOURCE_ACTIVE_LAYER,
	COLOR_PICKER_SOURCE_CANVAS
} eCOLOR_PICKER_SOURCE;

typedef struct _COLOR_PICKER
{
	struct _APPLICATION* app;
	uint8 mode;
} COLOR_PICKER;

typedef enum _eSELECT_RECTANGLE_FLAGS
{
	SELECT_RECTANGLE_STARTED = 0x01,
	SELECT_RECTANGLE_SHIFT_MASK = 0x02,
	SELECT_RECTANGLE_HAS_BEFORE_AREA = 0x04,
	SELECT_RECTANGLE_HAS_BEFORE_DATA = 0x08
} eSELECT_RECTANGLE_FLAGS;

typedef struct _SELECT_RECTANGLE
{
	struct
	{
		int32 min_x, min_y, max_x, max_y;
	} before_point;
	int32 start_x, start_y;
	gdouble x, y;
	int32 select_start;
	uint16 flags;
} SELECT_RECTANGLE;

#define SELECT_ECLIPSE_MOVE_DISTANCE 24

typedef enum _eSELECT_ECLIPSE_FLAGS
{
	SELECT_ECLIPSE_STARTED = 0x01,
	SELECT_ECLIPSE_SHIFT_MASK = 0x02,
	SELECT_ECLIPSE_HAS_EDITABLE_AREA = 0x04
} eSELECT_ECLISPSE_FLAGS;

typedef enum _eSELECT_ECLIPSE_MOVE_POINT
{
	SELECT_ECLIPSE_MOVE_NONE,
	SELECT_ECLIPSE_MOVE,
	SELECT_ECLIPSE_MOVE_LEFT_UP,
	SELECT_ECLIPSE_MOVE_LEFT_DOWN,
	SELECT_ECLIPSE_MOVE_RIGHT_UP,
	SELECT_ECLIPSE_MOVE_RIGHT_DOWN
} eSELECT_ECLIPSE_MOVE_POINT;

typedef struct _SELECT_ECLIPSE
{
	struct
	{
		FLOAT_T min_x, min_y, max_x, max_y;
	} selected_area;
	FLOAT_T start_x, start_y;
	FLOAT_T before_x, before_y;
	int16 select_start;
	int8 move_point;
	uint32 flags;
} SELECT_ECLIPSE;

typedef enum _eFREE_SELECT_FLAGS
{
	FREE_SELECT_STARTED = 0x01,
	FREE_SELECT_SHIFT_MASK = 0x02,
	FREE_SELECT_HAS_BEFORE_AREA = 0x04,
	FREE_SELECT_HAS_BEFORE_DATA = 0x08
} eFREE_SELECT_FLAGS;

typedef struct _FREE_SELECT
{
	struct
	{
		int32 min_x, min_y, max_x, max_y;
	} before_point, select_size;
	gdouble before_x, before_y;
	cairo_t *cairo_p;
	cairo_surface_t *surface_p;
	uint16 flags;
} FREE_SELECT;

typedef enum _eFUZZY_SELECT_MODE
{
	FUZZY_SELECT_RGB,
	FUZZY_SELECT_RGBA,
	FUZZY_SELECT_ALPHA
} eFUZZY_SELECT_MODE;

typedef enum _eFUZZY_SELECT_TARGET
{
	FUZZY_SELECT_ACTIVE_LAYER,
	FUZZY_SELECT_CANVAS
} eFUZZY_SELECT_TARGET;

typedef enum _eFUZZY_SELECT_FLAGS
{
	FUZZY_SELECT_HAS_BEFORE_AREA = 0x01,
	FUZZY_SELECT_ANTI_ALIAS = 0x02
} eFUZZY_SELECT_FLAGS;

typedef struct _FUZZY_SELECT
{
	uint16 threshold;
	uint8 select_mode;
	uint8 select_target;
	uint8 select_direction;
	uint8 flags;
	int16 extend;
} FUZZY_SELECT;

typedef enum _eSELECT_BY_COLOR_MODE
{
	SELECT_BY_RGB,
	SELECT_BY_RGBA
} eSELECT_BY_COLOR_MODE;

typedef enum _eSELECT_BY_COLOR_TARGET
{
	SELECT_BY_COLOR_ACTIVE_LAYER,
	SELECT_BY_COLOR_CANVAS
} eSELECT_BY_COLOR_TARGET;

typedef struct _SELECT_BY_COLOR
{
	uint16 threshold;
	uint8 select_mode;
	uint8 select_target;
} SELECT_BY_COLOR;

typedef struct _HAND_TOOL
{
	gdouble before_x, before_y;
} HAND_TOOL;

typedef enum _eCOMMON_TOOL_TYPE
{
	TYPE_COLOR_PICKER,
	TYPE_SELECT_RECTANGLE,
	TYPE_SELECT_ECLIPSE,
	TYPE_FREE_SELECT,
	TYPE_FUZZY_SELECT,
	TYPE_SELECT_BY_COLOR,
	TYPE_HAND_TOOL
} eCOMMON_TOOL_TYPE;

// 関数のプロトタイプ宣言
extern void LoadCommonToolDetailData(
	COMMON_TOOL_CORE* core,
	INI_FILE_PTR file,
	const char* section_name,
	const char* tool_type,
	struct _APPLICATION* app
);

/*****************************************
* SetColorPicker関数                     *
* スポイトツールのデータセット           *
* 引数                                   *
* core		: 共通ツールの基本情報       *
* picker	: スポイトツールの詳細データ *
*****************************************/
extern void SetColorPicker(
	COMMON_TOOL_CORE* core,
	COLOR_PICKER* picker
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_COMMON_TOOLS_H_
