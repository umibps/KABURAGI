// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <zlib.h>
#include <gtk/gtk.h>
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
#include "script.h"
#include "image_read_write.h"
#include "utils.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _eSCRIPT_HISTORY_TYPE
{
	SCRIPT_HISTORY_NEW_LAYER,
	SCRIPT_HISTORY_PIXEL_CHANGE,
	SCRIPT_HISTORY_VECTOR_CHANGE
} eSCRIPT_HISTORY_TYPE;

typedef struct _CALLBACK_INFO
{
	SCRIPT *script;
	int handler_ref;
	int args_ref;
	int object_ref;
	lua_State *lua;
	GSignalQuery query;
	GtkWidget *object;
} CALLBACK_INFO;

typedef struct _MOUSE_EVENT
{
	gint x, y;
	GdkModifierType state;
} MOUSE_EVENT;

static void ScriptAddHistoryData(SCRIPT* script, uint8* data, size_t data_size, eSCRIPT_HISTORY_TYPE type)
{
	if(data_size > 0)
	{
		guint32 data32 = (guint32)type;
		(void)MemWrite(&data32, sizeof(data32), 1, script->history.data_stream);
		data32 = (guint32)data_size;
		(void)MemWrite(&data32, sizeof(data32), 1, script->history.data_stream);
		(void)MemWrite(data, 1, data_size, script->history.data_stream);
		script->history.num_history++;
	}
}

static void ScriptAddNewLayerUndo(DRAW_WINDOW* window, uint8* data, size_t data_size)
{
	MEMORY_STREAM stream = {data, 0, data_size, 1};
	LAYER *prev_layer = NULL;
	LAYER *target_layer = NULL;
	LAYER *next_bottom;
	eLAYER_TYPE layer_type;
	uint16 data16;

	// レイヤーの種類を取得
	(void)MemRead(&data16, sizeof(data16), 1, &stream);
	layer_type = (eLAYER_TYPE)data16;

	// 削除するレイヤーの名前の長さを取得
	(void)MemRead(&data16, sizeof(data16), 1, &stream);
	if(data16 > 0)
	{
		target_layer = SearchLayer(window->layer, (const char*)&stream.buff_ptr[stream.data_point]);
		MemSeek(&stream, data16, SEEK_CUR);
	}

	// 下のレイヤーの名前の長さを取得
	(void)MemRead(&data16, sizeof(data16), 1, &stream);
	if(data16 > 0)
	{
		prev_layer = SearchLayer(window->layer, (const char*)&stream.buff_ptr[stream.data_point]);
	}

	// レイヤーを削除
	if(prev_layer != NULL)
	{
		next_bottom = window->layer;
	}
	else
	{
		next_bottom = target_layer->next;
	}
	DeleteLayer(&target_layer);
	window->layer = next_bottom;
	window->num_layer--;
}

static void ScriptAddNewLayerRedo(DRAW_WINDOW* window, uint8* data, size_t data_size)
{
	MEMORY_STREAM stream = {data, 0, data_size, 1};
	char layer_name[4096];
	LAYER *prev_layer = NULL;
	LAYER *next_layer = NULL;
	LAYER *new_layer;
	eLAYER_TYPE layer_type;
	uint16 data16;

	// レイヤーの種類を取得
	(void)MemRead(&data16, sizeof(data16), 1, &stream);
	layer_type = (eLAYER_TYPE)data16;

	// 作成するレイヤーの名前の長さを取得
	(void)MemRead(&data16, sizeof(data16), 1, &stream);
	(void)MemRead(layer_name, 1, data16, &stream);

	// 下のレイヤーの名前の長さを取得
	(void)MemRead(&data16, sizeof(data16), 1, &stream);
	if(data16 > 0)
	{
		(void)MemRead(layer_name, 1, data16, &stream);
		prev_layer = SearchLayer(window->layer, layer_name);
	}

	// レイヤーを作成
	new_layer = CreateLayer(0, 0, window->width, window->height, window->channel, layer_type,
		prev_layer, (prev_layer != NULL) ? prev_layer->next : window->layer, layer_name, window);
	window->num_layer++;
}

static void ScriptUpdatePixelsUndo(DRAW_WINDOW* window, uint8* data, size_t data_size)
{
	MEMORY_STREAM stream = {data, 0, data_size, 1};
	MEMORY_STREAM image_stream = {0};
	LAYER *layer;
	gint32 width, height;
	gint32 stride;
	uint8 *pixels;
	char layer_name[4096];
	guint32 data32;
	uint16 data16;

	// ピクセルデータを元に戻すレイヤーを取得
	(void)MemRead(&data16, sizeof(data16), 1, &stream);
	(void)MemRead(layer_name, 1, data16, &stream);
	layer = SearchLayer(window->layer, layer_name);

	// PNGデータを展開
	(void)MemRead(&data32, sizeof(data32), 1, &stream);
	image_stream.block_size = 1;
	image_stream.buff_ptr = &stream.buff_ptr[stream.data_point];
	image_stream.data_size = (size_t)data32;
	pixels = ReadPNGStream((void*)&image_stream, (stream_func_t)MemRead,
		&width, &height, &stride);

	(void)memcpy(layer->pixels, pixels, stride * height);

	MEM_FREE_FUNC(pixels);
}

static void ScriptUpdatePixelsRedo(DRAW_WINDOW* window, uint8* data, size_t data_size)
{
	MEMORY_STREAM stream = {data, 0, data_size, 1};
	MEMORY_STREAM image_stream = {0};
	LAYER *layer;
	gint32 width, height;
	gint32 stride;
	uint8 *pixels;
	char layer_name[4096];
	guint32 data32;
	uint16 data16;

	// ピクセルデータを元に戻すレイヤーを取得
	(void)MemRead(&data16, sizeof(data16), 1, &stream);
	(void)MemRead(layer_name, 1, data16, &stream);
	layer = SearchLayer(window->layer, layer_name);

	// PNGデータを展開
	(void)MemRead(&data32, sizeof(data32), 1, &stream);
	(void)MemSeek(&stream, (long)data32, SEEK_CUR);
	(void)MemRead(&data32, sizeof(data32), 1, &stream);
	image_stream.block_size = 1;
	image_stream.buff_ptr = &stream.buff_ptr[stream.data_point];
	image_stream.data_size = (size_t)data32;
	pixels = ReadPNGStream((void*)&image_stream, (stream_func_t)MemRead,
		&width, &height, &stride);

	(void)memcpy(layer->pixels, pixels, stride * height);

	MEM_FREE_FUNC(pixels);
}

static void ScriptUpdateVectorUndo(DRAW_WINDOW* window, uint8* data, size_t data_size)
{
	MEMORY_STREAM stream = {data, 0, data_size, 1};
	VECTOR_LINE *line;
	VECTOR_LINE *next_line;
	LAYER *layer;
	char layer_name[4096];
	uint16 data16;

	// ベクトルデータを元に戻すレイヤーを取得
	(void)MemRead(&data16, sizeof(data16), 1, &stream);
	(void)MemRead(layer_name, 1, data16, &stream);
	layer = SearchLayer(window->layer, layer_name);

	// 線情報をクリア
	line = layer->layer_data.vector_layer_p->base->next;
	while(line != NULL)
	{
		next_line = line->next;
		DeleteVectorLine(&line);
		line = next_line;
	}

	// ベクトルデータを展開
	ReadVectorLineData(&stream.buff_ptr[stream.data_point], layer);

	// ベクトルデータをラスタライズ
	layer->layer_data.vector_layer_p->flags =
		(VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ALL);
	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

static void ScriptUpdateVectorRedo(DRAW_WINDOW* window, uint8* data, size_t data_size)
{
	MEMORY_STREAM stream = {data, 0, data_size, 1};
	VECTOR_LINE *line;
	VECTOR_LINE *next_line;
	LAYER *layer;
	char layer_name[4096];
	guint32 data32;
	uint16 data16;

	// ベクトルデータを元に戻すレイヤーを取得
	(void)MemRead(&data16, sizeof(data16), 1, &stream);
	(void)MemRead(layer_name, 1, data16, &stream);
	layer = SearchLayer(window->layer, layer_name);

	// 線情報をクリア
	line = layer->layer_data.vector_layer_p->base->next;
	while(line != NULL)
	{
		next_line = line->next;
		DeleteVectorLine(&line);
		line = next_line;
	}

	// ベクトルデータを展開
	(void)MemRead(&data32, sizeof(data32), 1, &stream);
	(void)MemSeek(&stream, data32, SEEK_CUR);
	ReadVectorLineData(&stream.buff_ptr[stream.data_point], layer);

	// ベクトルデータをラスタライズ
	layer->layer_data.vector_layer_p->flags =
		(VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ALL);
	RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
}

static int ScriptPrint(lua_State* lua)
{
	SCRIPT *script;
	GtkTextIter iter;
	char buffer[4096] = {0};
	unsigned int point = 0;
	int stack_size;
	int i;

	stack_size = lua_gettop(lua);
	for(i=1; i<=stack_size; i++)
	{
		switch(lua_type(lua, i))
		{
		case LUA_TNUMBER:
			{
				FLOAT_T num = lua_tonumber(lua, i);
				if(num - (int)num == 0.0)
				{
					point += sprintf(&buffer[point], "%d", (int)num);
				}
				else
				{
					point += sprintf(&buffer[point], "%f", lua_tonumber(lua, i));
				}
			}
			break;
		case LUA_TBOOLEAN:
			if(lua_toboolean(lua, i) != 0)
			{
				point += sprintf(&buffer[point], "true");
			}
			else
			{
				point += sprintf(&buffer[point], "false");
			}
			break;
		case LUA_TSTRING:
			point += sprintf(&buffer[point], "%s", lua_tostring(lua, i));
			break;
		case LUA_TNIL:
			point += sprintf(&buffer[point], "\n warning print a nil value.\n");
			break;
		}
	}

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);
	lua_pop(lua, 1);

	if(script->debug_buffer == NULL)
	{
		return 0;
	}

	gtk_text_buffer_get_end_iter(script->debug_buffer, &iter);
	gtk_text_buffer_insert(script->debug_buffer, &iter, buffer, -1);
	gtk_text_buffer_place_cursor(script->debug_buffer, &iter);

	{
		GtkAdjustment *adjust = gtk_scrolled_window_get_vadjustment(
			GTK_SCROLLED_WINDOW(script->debug_view));
		gtk_adjustment_set_value(adjust, gtk_adjustment_get_upper(adjust));
	}

	(void)printf(buffer);

	return 0;
}

static int ScriptParseRGBA(lua_State* lua)
{
	guint32 pixel_value = luaL_checkunsigned(lua, 1);

#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
	lua_pushinteger(lua,
		(pixel_value & 0x00ff0000) >> 16);
	lua_pushinteger(lua,
		(pixel_value & 0x0000ff00) >> 8);
	lua_pushinteger(lua,
		pixel_value & 0x000000ff);
	lua_pushinteger(lua,
		(pixel_value & 0xff000000) >> 24);
#else
	lua_pushinteger(lua,
		pixel_value & 0x000000ff);
	lua_pushinteger(lua,
		(pixel_value & 0x0000ff00) >> 8);
	lua_pushinteger(lua,
		(pixel_value & 0x00ff0000) >> 16);
	lua_pushinteger(lua,
		(pixel_value & 0xff000000) >> 24);
#endif

	return 4;
}

static int ScriptMergeRGBA(lua_State* lua)
{
	guint32 color_value = 0;
	uint8 *color_p = (uint8*)&color_value;
	int r, g, b, a;

	r = (int)lua_tointeger(lua, 1);
	g = (int)lua_tointeger(lua, 2);
	b = (int)lua_tointeger(lua, 3);
	a = (int)lua_tointeger(lua, 4);

	if(r < 0)
	{
		color_p[0] = 0;
	}
	else if(r > 0xff)
	{
		color_p[0] = 0xff;
	}
	else
	{
		color_p[0] = r;
	}
	if(g < 0)
	{
		color_p[1] = 0;
	}
	else if(g > 0xff)
	{
		color_p[1] = 0xff;
	}
	else
	{
		color_p[1] = g;
	}
	if(b < 0)
	{
		color_p[2] = 0;
	}
	else if(b > 0xff)
	{
		color_p[2] = 0xff;
	}
	else
	{
		color_p[2] = b;
	}
	if(a < 0)
	{
		color_p[3] = 0;
	}
	else if(a > 0xff)
	{
		color_p[3] = 0xff;
	}
	else
	{
		color_p[3] = a;
	}

#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
	r = color_p[0];
	color_p[0] = color_p[2];
	color_p[2] = r;
#endif

	lua_pushunsigned(lua, color_value);

	return 1;
}

static int ScriptGetAverageColor(lua_State* lua)
{
#define DEFAULT_CHECK_SIZE 10
	guint32 color;
	uint8 average[4];
	int num_stack = lua_gettop(lua);
	int check_size;
	int check_x, check_y;
	int width, height;
	int x, y;
	int i, j;

	if(num_stack < 3)
	{
		return 0;
	}
	else if(num_stack >= 4)
	{
		check_size = (int)lua_tointeger(lua, 4);
	}
	else
	{
		check_size = DEFAULT_CHECK_SIZE;
	}

	x = (int)lua_tointeger(lua, 2);
	y = (int)lua_tointeger(lua, 3);

	lua_getfield(lua, 1, "type");
	if(lua_tointeger(lua, 1) != TYPE_NORMAL_LAYER)
	{
		return 0;
	}
	lua_pop(lua, 1);

	for(i=2; i<num_stack; i++)
	{
		lua_remove(lua, i);
	}
	
	lua_getfield(lua, 1, "width");
	width = (int)lua_tointeger(lua, -1);
	lua_pop(lua, 1);
	lua_getfield(lua, 1, "height");
	height = (int)lua_tointeger(lua, -1);
	lua_pop(lua, 1);

	lua_getfield(lua, 1, "pixels");
	if(check_size > 0)
	{
		unsigned int sum_color[4] = {0, 0, 0, 1};
		int count = 1;
		uint8 *rgba = (uint8*)&color;

		for(i=-check_size; i<=check_size; i++)
		{
			check_y = y + i;
			if(check_y > 0 && check_y <= height)
			{
				lua_pushinteger(lua, y+i);
				lua_gettable(lua, -2);
				for(j=-check_size; j<=check_size; j++)
				{
					check_x = x + j;
					if(check_x > 0 && check_x <= width)
					{
						lua_pushinteger(lua, x+j);
						lua_gettable(lua, -2);
						if(lua_type(lua, -1) == LUA_TNUMBER)
						{
							color = lua_tounsigned(lua, -1);
#if 0 // defiend(BIG_ENDIAN)
							sum_color[0] += rgba[0] * rgba[3];
							sum_color[1] += rgba[1] * rgba[3];
							sum_color[2] += rgba[2] * rgba[3];
							sum_color[3] += rgba[3];
#else
							sum_color[0] += rgba[3] * rgba[0];
							sum_color[1] += rgba[2] * rgba[0];
							sum_color[2] += rgba[1] * rgba[0];
							sum_color[3] += rgba[0];
#endif
							count++;
						}
						lua_pop(lua, 1);
					}
				}

				lua_pop(lua, 1);
			}
		}

#if 0 // defiend(BIG_ENDIAN)
		average[0] = sum_color[0] / sum_color[3];
		average[1] = sum_color[1] / sum_color[3];
		average[2] = sum_color[2] / sum_color[3];
		average[3] = sum_color[3] / count;
#else
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
		average[2] = sum_color[0] / sum_color[3];
		average[1] = sum_color[1] / sum_color[3];
		average[0] = sum_color[2] / sum_color[3];
		average[3] = sum_color[3] / count;
# else
		
		average[0] = sum_color[0] / sum_color[3];
		average[1] = sum_color[1] / sum_color[3];
		average[2] = sum_color[2] / sum_color[3];
		average[3] = sum_color[3] / count;
# endif
#endif

		lua_pushunsigned(lua, *((guint32*)average));
	}
	else
	{
		lua_pushinteger(lua, y);
		lua_gettable(lua, 1);
		lua_pushinteger(lua, x);
	}

	return 1;
}

static int ScriptRGB2HSV(lua_State* lua)
{
	uint8 color[3];
	HSV hsv;

	color[0] = (uint8)lua_tointeger(lua, 1);
	color[1] = (uint8)lua_tointeger(lua, 2);
	color[2] = (uint8)lua_tointeger(lua, 3);

	RGB2HSV_Pixel(color, &hsv);

	lua_pushinteger(lua, hsv.h);
	lua_pushinteger(lua, hsv.s);
	lua_pushinteger(lua, hsv.v);

	return 3;
}

static int ScriptHSV2RGB(lua_State* lua)
{
	HSV hsv;
	int h, s, v;
	uint8 color[3];

	h = (int)lua_tointeger(lua, 1);
	s = (int)lua_tointeger(lua, 2);
	v = (int)lua_tointeger(lua, 3);

	while(h < 0)
	{
		h += 360;
	}
	while(h >= 360)
	{
		h -= 360;
	}

	if(s < 0)
	{
		s = 0;
	}
	else if(s > 0xff)
	{
		s = 0xff;
	}

	if(v < 0)
	{
		v = 0;
	}
	else if(v > 0xff)
	{
		v = 0xff;
	}

	hsv.h = (int16)h, hsv.s = (uint8)s, hsv.v = (uint8)v;
	HSV2RGB_Pixel(&hsv, color);

	lua_pushinteger(lua, color[0]);
	lua_pushinteger(lua, color[1]);
	lua_pushinteger(lua, color[2]);

	return 3;
}

static void ScriptReturnLayer(lua_State* lua, LAYER* layer)
{
	guint32 pixel_value;
	uint8 *pixel;
	int data_num;
	int i, j;

	switch(layer->layer_type)
	{
	case TYPE_VECTOR_LAYER:
		data_num = 9;
	default:
		data_num = 8;
	}

	lua_createtable(lua, 0, data_num);
	lua_pushinteger(lua, layer->width);
	lua_setfield(lua, -2, "width");
	lua_pushinteger(lua, layer->height);
	lua_setfield(lua, -2, "height");
	lua_pushstring(lua, layer->name);
	lua_setfield(lua, -2, "name");
	lua_pushinteger(lua, layer->alpha);
	lua_setfield(lua, -2, "alpha");
	lua_pushunsigned(lua, layer->flags);
	lua_setfield(lua, -2, "flags");
	lua_pushinteger(lua, layer->layer_type);
	lua_setfield(lua, -2, "type");
	lua_pushlightuserdata(lua, layer->surface_p);
	lua_setfield(lua, -2, "surface");

	switch(layer->layer_type)
	{
	case TYPE_NORMAL_LAYER:
		lua_createtable(lua, 0, layer->height);
		for(i=0; i<layer->height; i++)
		{
			lua_pushinteger(lua, i+1);
			lua_createtable(lua, 0, layer->width);
			pixel = &layer->pixels[i*layer->stride];
			for(j=0; j<layer->width; j++, pixel+=4)
			{
				lua_pushinteger(lua, j+1);
				pixel_value = pixel[0] << 24 | pixel[1] << 16 | pixel[2] << 8 | pixel[3];
				lua_pushunsigned(lua, pixel_value);
				lua_settable(lua, -3);
			}

			lua_settable(lua, -3);
		}
		lua_setfield(lua, -2, "pixels");
		break;
	case TYPE_VECTOR_LAYER:
		{
			VECTOR_LINE *line = layer->layer_data.vector_layer_p->base->next;
			VECTOR_LINE *for_count = line;
			unsigned int num_lines = 0;

			while(for_count != NULL)
			{
				num_lines++;
				for_count = for_count->next;
			}

			lua_pushunsigned(lua, num_lines);
			lua_setfield(lua, -2, "num_lines");
			lua_createtable(lua, 0, layer->layer_data.vector_layer_p->num_lines-1);
			for(i=0; i<(int)num_lines; i++)
			{
				lua_pushunsigned(lua, i+1);
				lua_createtable(lua, 0, 6);
				lua_pushinteger(lua, line->vector_type);
				lua_setfield(lua, -2, "vector_type");
				lua_pushinteger(lua, line->flags);
				lua_setfield(lua, -2, "flags");
				lua_pushinteger(lua, line->num_points);
				lua_setfield(lua, -2, "num_points");
				lua_pushnumber(lua, line->blur);
				lua_setfield(lua, -2, "blur");
				lua_pushnumber(lua, line->outline_hardness);
				lua_setfield(lua, -2, "outline_hardness");
				lua_createtable(lua, 0, line->num_points);
				for(j=0; j<line->num_points; j++)
				{
					lua_pushinteger(lua, j+1);
					lua_createtable(lua, 0, 7);
					lua_pushinteger(lua, line->points[j].vector_type);
					lua_setfield(lua, -2, "vector_type");
					lua_pushinteger(lua, line->points[j].flags);
					lua_setfield(lua, -2, "flags");
					pixel_value = line->points[j].color[0] << 24 |
						line->points[j].color[1] << 16 | line->points[j].color[2] << 8 | line->points[j].color[3];
					lua_pushunsigned(lua, pixel_value);
					lua_setfield(lua, -2, "color");
					lua_pushnumber(lua, line->points[j].pressure);
					lua_setfield(lua, -2, "pressure");
					lua_pushnumber(lua,line->points[j].size*2);
					lua_setfield(lua, -2, "size");
					lua_pushnumber(lua, line->points[j].x);
					lua_setfield(lua, -2, "x");
					lua_pushnumber(lua, line->points[j].y);
					lua_setfield(lua, -2, "y");
					lua_settable(lua, -3);
				}
				lua_setfield(lua, -2, "points");
				lua_settable(lua, -3);

				line = line->next;
			}
			lua_setfield(lua, -2, "lines");
		}
		break;
	}
}

static void ScriptReturnLayerInfo(lua_State* lua, LAYER* layer)
{
	lua_createtable(lua, 0, 6);
	lua_pushinteger(lua, layer->width);
	lua_setfield(lua, -2, "width");
	lua_pushinteger(lua, layer->height);
	lua_setfield(lua, -2, "height");
	lua_pushstring(lua, layer->name);
	lua_setfield(lua, -2, "name");
	lua_pushinteger(lua, layer->alpha);
	lua_setfield(lua, -2, "alpha");
	lua_pushunsigned(lua, layer->flags);
	lua_setfield(lua, -2, "flags");
	lua_pushinteger(lua, layer->layer_type);
	lua_setfield(lua, -2, "type");
}

static int ScriptUpdateLayerPixels(lua_State* lua)
{
	SCRIPT *script;
	LAYER *layer;
	const char *layer_name;
	uint16 data16;
	guint32 data32;
	guint32 *pixels;
	guint32 pixel_value;
	MEMORY_STREAM *history_data;
	MEMORY_STREAM *image_data;
	int i, j;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);
	lua_pop(lua, 1);

	if(script->app->window_num == 0)
	{
		return 0;
	}

	lua_getfield(lua, -1, "name");
	layer_name = luaL_checkstring(lua, -1);
	layer = script->app->draw_window[script->app->active_window]->layer;
	while(layer != NULL && strcmp(layer_name, layer->name) != 0)
	{
		layer = layer->next;
	}
	lua_pop(lua, 1);

	if(layer->layer_type != TYPE_NORMAL_LAYER)
	{
		return 0;
	}

	history_data = CreateMemoryStream(layer->stride * layer->height * 2);
	image_data = CreateMemoryStream(layer->stride * layer->height);

	// ピクセルデータを更新するレイヤーの名前を記憶
	data16 = (uint16)strlen(layer_name) + 1;
	(void)MemWrite(&data16, sizeof(data16), 1, history_data);
	(void)MemWrite(layer_name, 1, data16, history_data);

	// 更新前のピクセルデータを記憶
		// PNG圧縮しておく
	(void)WritePNGStream((void*)image_data, (stream_func_t)MemWrite, NULL,
		layer->pixels, layer->width, layer->height, layer->stride,
			layer->stride / layer->width, 0, Z_DEFAULT_COMPRESSION);
	data32 = (guint32)image_data->data_point;
	(void)MemWrite(&data32, sizeof(data32), 1, history_data);
	(void)MemWrite(image_data->buff_ptr, 1, image_data->data_point, history_data);

	lua_getfield(lua, -1, "pixels");
	for(i=0; i<layer->height; i++)
	{
		lua_pushinteger(lua, i+1);
		lua_gettable(lua, -2);
		pixels = (uint32*)&layer->pixels[i*layer->stride];
		for(j=0; j<layer->width; j++, pixels++)
		{
			lua_pushinteger(lua, j+1);
			lua_gettable(lua, -2);
			pixel_value = luaL_checkunsigned(lua, -1);
			*pixels = pixel_value;
			lua_pop(lua, 1);
		}

		lua_pop(lua, 1);
	}

	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	gtk_widget_queue_draw(layer->window->window);

	// 更新後のピクセルデータを記憶
		// PNG圧縮しておく
	(void)MemSeek(image_data, 0, SEEK_SET);
	(void)WritePNGStream((void*)image_data, (stream_func_t)MemWrite, NULL,
		layer->pixels, layer->width, layer->height, layer->stride,
			layer->stride / layer->width, 0, Z_DEFAULT_COMPRESSION);
	data32 = (guint32)image_data->data_point;
	(void)MemWrite(&data32, sizeof(data32), 1, history_data);
	(void)MemWrite(image_data->buff_ptr, 1, image_data->data_point, history_data);

	ScriptAddHistoryData(script, history_data->buff_ptr, history_data->data_point,
		SCRIPT_HISTORY_PIXEL_CHANGE);

	(void)DeleteMemoryStream(image_data);
	(void)DeleteMemoryStream(history_data);

	return 0;
}

static int ScriptUpdateVectorLayer(lua_State* lua)
{
	SCRIPT *script;
	LAYER *layer;
	const char *layer_name;
	guint32 color_value;
	uint16 data16;
	uint8* color = (uint8*)&color_value;
	VECTOR_LINE *line;
	MEMORY_STREAM *history_data;
	MEMORY_STREAM *vector_data;
	MEMORY_STREAM *compress_data;
	unsigned int num_lines;
	unsigned int i, j;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);
	lua_pop(lua, 1);

	if(script->app->window_num == 0)
	{
		return 0;
	}

	lua_getfield(lua, -1, "name");
	layer_name = luaL_checkstring(lua, -1);
	layer = script->app->draw_window[script->app->active_window]->layer;
	while(layer != NULL && strcmp(layer_name, layer->name) != 0)
	{
		layer = layer->next;
	}
	lua_pop(lua, 1);

	if(layer->layer_type != TYPE_VECTOR_LAYER)
	{
		return 0;
	}

	history_data = CreateMemoryStream(layer->stride * layer->height * 2);
	vector_data = CreateMemoryStream(layer->stride * layer->height);
	compress_data = CreateMemoryStream(layer->stride * layer->height);

	// ベクトルデータを更新するレイヤーの名前を記憶
	data16 = (uint16)strlen(layer_name) + 1;
	(void)MemWrite(&data16, sizeof(data16), 1, history_data);
	(void)MemWrite(layer_name, 1, data16, history_data);

	// 更新前のベクトルデータを記憶
	WriteVectorLineData(layer, (void*)history_data, (stream_func_t)MemWrite,
		vector_data, compress_data, Z_DEFAULT_COMPRESSION);

	lua_getfield(lua, -1, "num_lines");
	num_lines = luaL_checkunsigned(lua, -1);
	lua_pop(lua, 1);

	lua_getfield(lua, -1, "lines");
	line = layer->layer_data.vector_layer_p->base->next;

	for(i=0; i<num_lines; i++)
	{
		lua_pushunsigned(lua, i+1);
		lua_gettable(lua, -2);

		if(line == NULL)
		{
			line = layer->layer_data.vector_layer_p->top_line =
				CreateVectorLine(layer->layer_data.vector_layer_p->top_line, NULL);
			layer->layer_data.vector_layer_p->num_lines++;
		}

		lua_getfield(lua, -1, "vector_type");
		line->vector_type = (uint8)luaL_checkinteger(lua, -1);
		lua_pop(lua, 1);
		lua_getfield(lua, -1, "flags");
		line->flags = (uint8)luaL_checkinteger(lua, -1);
		lua_pop(lua, 1);
		lua_getfield(lua, -1, "num_points");
		line->num_points = (uint16)luaL_checkinteger(lua, -1);
		lua_pop(lua, 1);
		lua_getfield(lua, -1, "blur");
		line->blur = luaL_checknumber(lua, -1);
		lua_pop(lua, 1);
		lua_getfield(lua, -1, "outline_hardness");
		line->outline_hardness = luaL_checknumber(lua, -1);
		lua_pop(lua, 1);

		if(line->points == NULL)
		{
			line->buff_size = (line->num_points/VECTOR_LINE_BUFFER_SIZE+1)*VECTOR_LINE_BUFFER_SIZE;
			line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(
				sizeof(*line->points)*line->buff_size);
			(void)memset(line->points, 0, sizeof(*line->points)*VECTOR_LINE_BUFFER_SIZE);
		}

		lua_getfield(lua, -1, "points");
		for(j=0; j<line->num_points; j++)
		{
			lua_pushinteger(lua, j+1);
			lua_gettable(lua, -2);

			lua_getfield(lua, -1, "vector_type");
			line->points[j].vector_type = (uint8)luaL_checkinteger(lua, -1);
			lua_pop(lua, 1);
			lua_getfield(lua, -1, "flags");
			line->points[j].flags = (uint8)luaL_checkinteger(lua, -1);
			lua_pop(lua, 1);
			lua_getfield(lua, -1, "color");
			color_value = luaL_checkunsigned(lua, -1);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			line->points[j].color[0]  = color[2];
			line->points[j].color[1]  = color[1];
			line->points[j].color[2]  = color[0];
			line->points[j].color[3]  = color[3];
#else
			line->points[j].color[0]  = color[0];
			line->points[j].color[1]  = color[1];
			line->points[j].color[2]  = color[2];
			line->points[j].color[3]  = color[3];
#endif
			lua_pop(lua, 1);
			lua_getfield(lua, -1, "pressure");
			line->points[j].pressure = luaL_checknumber(lua, -1);
			lua_pop(lua, 1);
			lua_getfield(lua, -1, "size");
			line->points[j].size = luaL_checknumber(lua, -1) * 0.5;
			lua_pop(lua, 1);
			lua_getfield(lua, -1, "x");
			line->points[j].x = luaL_checknumber(lua, -1);
			lua_pop(lua, 1);
			lua_getfield(lua, -1, "y");
			line->points[j].y = luaL_checknumber(lua, -1);
			lua_pop(lua, 2);
		}
		lua_pop(lua, 2);

		line = line->next;
	}

	layer->layer_data.vector_layer_p->flags |=
		VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ALL;
	RasterizeVectorLayer(layer->window, layer, layer->layer_data.vector_layer_p);
	
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	gtk_widget_queue_draw(layer->window->window);

	// 更新後のベクトルデータを記憶
	WriteVectorLineData(layer, (void*)history_data, (stream_func_t)MemWrite,
		vector_data, compress_data, Z_DEFAULT_COMPRESSION);

	// 履歴データを追加
	ScriptAddHistoryData(script, history_data->buff_ptr, history_data->data_point,
		SCRIPT_HISTORY_VECTOR_CHANGE);

	(void)DeleteMemoryStream(compress_data);
	(void)DeleteMemoryStream(vector_data);
	(void)DeleteMemoryStream(history_data);

	return 0;
}

static int ScriptAddVectorLine(lua_State* lua)
{
	SCRIPT *script;
	int layer_type;
	int vector_type;
	int vector_flags;
	VECTOR_POINT *points;
	FLOAT_T outline_hardness;
	FLOAT_T blur;
	guint32 color_value;
	uint8 *color = (uint8*)&color_value;
	unsigned int num_lines;
	unsigned int num_points;
	unsigned int i;

	num_points = lua_gettop(lua) - 5;
	if(num_points < 2)
	{
		return 0;
	}
	points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*points)*num_points);
	(void)memset(points, 0, sizeof(*points)*num_points);

	vector_type = (int)lua_tointeger(lua, 2);
	outline_hardness = lua_tonumber(lua, 3);
	blur = lua_tonumber(lua, 4);
	vector_flags = (int)lua_tointeger(lua, 5);

	for(i=0; i<num_points; i++)
	{
		lua_getfield(lua, i+6, "vector_type");
		points[i].vector_type = (uint8)luaL_checkinteger(lua, -1);
		lua_pop(lua, 1);
		lua_getfield(lua, i+6, "flags");
		points[i].flags = (uint8)luaL_checkinteger(lua, -1);
		lua_pop(lua, 1);
		lua_getfield(lua, i+6, "color");
		color_value = luaL_checkunsigned(lua, -1);
		lua_pop(lua, 1);
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
		points[i].color[0]  = color[2];
		points[i].color[1]  = color[1];
		points[i].color[2]  = color[0];
		points[i].color[3]  = color[3];
#else
		points[i].color[0]  = color[0];
		points[i].color[1]  = color[1];
		points[i].color[2]  = color[2];
		points[i].color[3]  = color[3];
#endif
		lua_getfield(lua, i+6, "pressure");
		points[i].pressure = luaL_checknumber(lua, -1);
		lua_pop(lua, 1);
		lua_getfield(lua, i+6, "size");
		points[i].size = luaL_checknumber(lua, -1);
		lua_pop(lua, 1);
		lua_getfield(lua, i+6, "x");
		points[i].x = luaL_checknumber(lua, -1);
		lua_pop(lua, 1);
		lua_getfield(lua, i+6, "y");
		points[i].y = luaL_checknumber(lua, -1);
		lua_pop(lua, 1);
	}

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);
	lua_pop(lua, lua_gettop(lua)-1);

	if(script->app->window_num == 0)
	{
		return 0;
	}

	lua_getfield(lua, 1, "type");
	layer_type = (int)luaL_checkinteger(lua, -1);
	lua_pop(lua, 1);
	if(layer_type != TYPE_VECTOR_LAYER)
	{
		return 0;
	}

	lua_getfield(lua, 1, "num_lines");
	num_lines = luaL_checkunsigned(lua, -1);
	lua_pop(lua, 1);
	lua_pushunsigned(lua, num_lines+1);
	lua_setfield(lua, -2, "num_lines");

	lua_getfield(lua, 1, "lines");
	lua_pushinteger(lua, num_lines+1);
	lua_createtable(lua, 0, 6);
	lua_pushinteger(lua, vector_type);
	lua_setfield(lua, -2, "vector_type");
	lua_pushinteger(lua, vector_flags);
	lua_setfield(lua, -2, "flags");
	lua_pushnumber(lua, blur);
	lua_setfield(lua, -2, "blur");
	lua_pushnumber(lua, outline_hardness);
	lua_setfield(lua, -2, "outline_hardness");
	lua_pushinteger(lua, num_points);
	lua_setfield(lua, -2, "num_points");
	lua_createtable(lua, 0, num_points);
	for(i=0; i<num_points; i++)
	{
		lua_pushinteger(lua, i+1);
		lua_createtable(lua, 0, 7);
		lua_pushinteger(lua, vector_type);
		lua_setfield(lua, -2, "vector_type");
		lua_pushinteger(lua, 0);
		lua_setfield(lua, -2, "flags");
		color_value = *((guint32*)points[i].color);
		lua_pushinteger(lua, color_value);
		lua_setfield(lua, -2, "color");
		lua_pushnumber(lua, points[i].pressure);
		lua_setfield(lua, -2, "pressure");
		lua_pushnumber(lua, points[i].size);
		lua_setfield(lua, -2, "size");
		lua_pushnumber(lua, points[i].x);
		lua_setfield(lua, -2, "x");
		lua_pushnumber(lua, points[i].y);
		lua_setfield(lua, -2, "y");
		lua_settable(lua, -3);
	}
	lua_setfield(lua, -2, "points");
	lua_settable(lua, -3);

	return 0;
}

static int ScriptGetBottomLayer(lua_State* lua)
{
	SCRIPT *script;
	DRAW_WINDOW *window;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(script->app->window_num == 0)
	{
		return 0;
	}

	window = script->window;

	ScriptReturnLayer(lua, window->layer);

	return 1;
}

static int ScriptGetBottomLayerInfo(lua_State* lua)
{
	SCRIPT *script;
	DRAW_WINDOW *window;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(script->app->window_num == 0)
	{
		return 0;
	}

	window = script->window;

	ScriptReturnLayerInfo(lua, window->layer);

	return 1;
}

static int ScriptGetPreviousLayer(lua_State* lua)
{
	SCRIPT *script;
	DRAW_WINDOW *window;
	LAYER *layer;
	const char *name;

	lua_getfield(lua, -1, "name");
	name = luaL_checkstring(lua, -1);

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(script->app->window_num == 0)
	{
		return 0;
	}

	window = script->window;
	layer = window->layer;

	while(strcmp(name, layer->name) != 0 && layer != NULL)
	{
		layer = layer->next;
	}

	if(layer == NULL || layer->prev == NULL)
	{
		return 0;
	}

	ScriptReturnLayer(lua, layer->prev);

	return 1;
}

static int ScriptGetPreviousLayerInfo(lua_State* lua)
{
	SCRIPT *script;
	DRAW_WINDOW *window;
	LAYER *layer;
	const char *name;

	lua_getfield(lua, -1, "name");
	name = luaL_checkstring(lua, -1);

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(script->app->window_num == 0)
	{
		return 0;
	}

	window = script->window;
	layer = window->layer;

	while(strcmp(name, layer->name) != 0 && layer != NULL)
	{
		layer = layer->next;
	}

	if(layer == NULL || layer->prev == NULL)
	{
		return 0;
	}

	ScriptReturnLayerInfo(lua, layer->prev);

	return 1;
}

static int ScriptGetNextLayer(lua_State* lua)
{
	SCRIPT *script;
	DRAW_WINDOW *window;
	LAYER *layer;
	const char *name;

	lua_getfield(lua, -1, "name");
	name = luaL_checkstring(lua, -1);

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(script->app->window_num == 0)
	{
		return 0;
	}

	window = script->window;
	layer = window->layer;

	while(strcmp(name, layer->name) != 0 && layer != NULL)
	{
		layer = layer->next;
	}

	if(layer == NULL || layer->next == NULL)
	{
		return 0;
	}

	ScriptReturnLayer(lua, layer->next);

	return 1;
}

static int ScriptGetNextLayerInfo(lua_State* lua)
{
	SCRIPT *script;
	DRAW_WINDOW *window;
	LAYER *layer;
	const char *name;

	lua_getfield(lua, -1, "name");
	name = luaL_checkstring(lua, -1);

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(script->app->window_num == 0)
	{
		return 0;
	}

	window = script->window;
	layer = window->layer;

	while(strcmp(name, layer->name) != 0 && layer != NULL)
	{
		layer = layer->next;
	}

	if(layer == NULL || layer->next == NULL)
	{
		return 0;
	}

	ScriptReturnLayerInfo(lua, layer->next);

	return 1;
}

static int ScriptGetActiveLayer(lua_State* lua)
{
	SCRIPT *script;
	DRAW_WINDOW *window;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(script->app->window_num == 0)
	{
		return 0;
	}

	window = script->window;

	ScriptReturnLayer(lua, window->active_layer);

	return 1;
}

static int ScriptGetActiveLayerInfo(lua_State* lua)
{
	SCRIPT *script;
	DRAW_WINDOW *window;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(script->app->window_num == 0)
	{
		return 0;
	}

	window = script->window;

	ScriptReturnLayerInfo(lua, window->active_layer);

	return 1;
}

static int ScriptGetCanvasLayer(lua_State* lua)
{
	SCRIPT *script;
	DRAW_WINDOW *window;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(script->app->window_num == 0)
	{
		return 0;
	}

	window = script->window;

	ScriptReturnLayer(lua, window->mixed_layer);

	return 1;
}

static int ScriptNewLayer(lua_State* lua)
{
	SCRIPT *script;
	DRAW_WINDOW *window;
	MEMORY_STREAM *history_data;
	LAYER *new_layer, *prev_layer = NULL, *next_layer = NULL;
	char set_name[MAX_LAYER_NAME_LENGTH];
	const char *layer_name = NULL;
	const char *prev_name = NULL;
	uint16 data16 = 0;
	int layer_type = TYPE_NORMAL_LAYER;
	int type;
	int num_stack = lua_gettop(lua);
	int i;

	type = lua_type(lua, 1);
	if(type == LUA_TSTRING)
	{
		layer_name = lua_tostring(lua, 1);
	}
	else if(type == LUA_TTABLE)
	{
		lua_getfield(lua, -1, "name");
		layer_name = luaL_checkstring(lua, 1);
		lua_pop(lua, 1);
	}
	else
	{
		return 0;
	}

	if(num_stack >= 2)
	{
		type = lua_type(lua, 2);
		if(type == LUA_TSTRING)
		{
			prev_name = lua_tostring(lua, 2);
		}
		else if(type == LUA_TTABLE)
		{
			lua_getfield(lua, 2, "name");
			prev_name = luaL_checkstring(lua, -1);
			lua_pop(lua, 1);
		}
		else if(type == LUA_TNUMBER)
		{
			layer_type = (uint8)lua_tointeger(lua, 2);
		}
	}

	if(num_stack >= 3)
	{
		type = lua_type(lua, 3);
		if(type == LUA_TSTRING)
		{
			prev_name = lua_tostring(lua, 3);
		}
		else if(type == LUA_TTABLE)
		{
			lua_getfield(lua, 3, "name");
			prev_name = luaL_checkstring(lua, -1);
			lua_pop(lua, 1);
		}
		else if(type == LUA_TNUMBER)
		{
			layer_type = (uint8)lua_tointeger(lua, 3);
		}
	}

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);
	window = script->window;

	history_data = CreateMemoryStream(4096);
	data16 = (uint16)layer_type;
	(void)MemWrite(&data16, sizeof(data16), 1, history_data);

	i = 1;
	(void)strcpy(set_name, layer_name);
	while(CorrectLayerName(window->layer, set_name) == 0)
	{
		(void)sprintf(set_name, "%s (%d)", layer_name, i);
		i++;
	}

	data16 = (uint16)strlen(set_name) + 1;
	(void)MemWrite(&data16, sizeof(data16), 1, history_data);
	(void)MemWrite(set_name, 1, data16, history_data);

	if(prev_name != NULL)
	{
		prev_layer = SearchLayer(window->layer, prev_name);
		data16 = (uint16)strlen(prev_name)+1;
		(void)MemWrite(&data16, sizeof(data16), 1, history_data);
		(void)MemWrite(prev_name, 1, data16, history_data);
	}
	else
	{
		data16 = 0;
		(void)MemWrite(&data16, sizeof(data16), 1, history_data);
	}

	if(prev_layer == NULL)
	{
		next_layer = window->layer;
	}
	else
	{
		next_layer = prev_layer->next;
	}

	new_layer = CreateLayer(0, 0, window->width, window->height, window->channel,
		layer_type, prev_layer, next_layer, set_name, window);
	if(prev_layer == NULL)
	{
		window->layer = new_layer;
	}
	window->num_layer++;

	LayerViewAddLayer(new_layer, window->layer, window->app->layer_window.view,
		window->num_layer);

	ScriptReturnLayer(lua, new_layer);

	ScriptAddHistoryData(script, history_data->buff_ptr, history_data->data_point,
		SCRIPT_HISTORY_NEW_LAYER);
	(void)DeleteMemoryStream(history_data);

	return 1;
}

static int ScriptGetLayer(lua_State* lua)
{
	SCRIPT *script;
	LAYER *layer;
	const char *name = lua_tostring(lua, 1);

	if(name == NULL || name[0] == '\0')
	{
		return 0;
	}

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(script->app->window_num == 0)
	{
		return 0;
	}

	layer = script->app->draw_window[script->app->active_window]->layer;

	while(layer != NULL && strcmp(name, layer->name) != 0)
	{
		layer = layer->next;
	}

	if(layer == NULL)
	{
		return 0;
	}

	ScriptReturnLayer(lua, layer);

	return 1;
}

static int ScriptGetBlendedUnderLayer(lua_State* lua)
{
	SCRIPT *script;
	DRAW_WINDOW *window;
	LAYER *layer;
	LAYER *src;
	LAYER *blended;
	const char *layer_name;
	int use_background = 0;
	int stack_top = lua_gettop(lua);

	if(stack_top > 1)
	{
		use_background = lua_toboolean(lua, -1);
		lua_pop(lua, 1);
	}

	if(lua_type(lua, -1) == LUA_TNIL)
	{
		return 0;
	}

	lua_getfield(lua, -1, "name");
	layer_name = luaL_checkstring(lua, -1);

	use_background = lua_toboolean(lua, -1);

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);
	lua_pop(lua, 1);

	window = script->app->draw_window[script->app->active_window];

	layer = window->layer;
	while(layer != NULL && strcmp(layer_name, layer->name) != 0)
	{
		layer = layer->next;
	}
	lua_pop(lua, 1);

	blended = CreateLayer(0, 0, window->width, window->height,
		window->channel, TYPE_NORMAL_LAYER, NULL, NULL, NULL, window);
	script->work[script->num_work] = blended;
	script->num_work++;

	if(use_background != 0)
	{
		(void)memcpy(blended->pixels, window->back_ground, window->pixel_buf_size);
	}

	src = window->layer;
	while(src != NULL && src != layer)
	{
		if((src->flags & LAYER_FLAG_INVISIBLE) == 0)
		{
			window->layer_blend_functions[src->layer_mode](src, blended);
		}
		src = src->next;
	}

	ScriptReturnLayer(lua, blended);

	return 1;
}

static void OnDestroyMainWindow(GtkWidget* window, SCRIPT* script)
{
	if(script->debug_window != NULL)
	{
		gtk_widget_destroy(script->debug_window);
	}

	script->main_window = NULL;
}

static int ScriptCreateWindow(lua_State* lua)
{
	SCRIPT *script;
	const char *title = luaL_checkstring(lua, -1);

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(script->main_window != NULL)
	{
		return 0;
	}

	script->main_window = gtk_dialog_new();
	gtk_window_set_position(GTK_WINDOW(script->main_window), GTK_WIN_POS_MOUSE);
	gtk_window_set_title(GTK_WINDOW(script->main_window), title);
	gtk_window_set_transient_for(GTK_WINDOW(script->main_window), GTK_WINDOW(script->app->window));
	lua_pushlightuserdata(lua, script->main_window);
	gtk_window_set_modal(GTK_WINDOW(script->main_window), TRUE);
	g_signal_connect(G_OBJECT(script->main_window), "destroy", G_CALLBACK(OnDestroyMainWindow), script);

	return 1;
}

static void OnDestroyDebugWindow(GtkWidget* window, SCRIPT* script)
{
	script->debug_window = NULL;
}

static int ScriptCreateDebugWindow(lua_State* lua)
{
	SCRIPT *script;
	GtkWidget *text_field;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(script->main_window == NULL)
	{
		return 0;
	}

	script->debug_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(script->debug_window), "Debug");
	text_field = gtk_text_view_new();
	script->debug_view = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(script->debug_view, 300, 300);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(script->debug_view), text_field);
	script->debug_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_field));
	gtk_container_add(GTK_CONTAINER(script->debug_window), script->debug_view);
	gtk_widget_set_sensitive(text_field, FALSE);
	g_signal_connect(G_OBJECT(script->debug_window), "destroy",
		G_CALLBACK(OnDestroyDebugWindow), script);

	return 0;
}

static int ScriptRunDialog(lua_State* lua)
{
	GtkWidget *dialog = GTK_WIDGET(lua_topointer(lua, 1));

	lua_pushinteger(lua, gtk_dialog_run(GTK_DIALOG(dialog)));

	return 1;
}

static int ScriptMainLoop(lua_State* lua)
{
	SCRIPT *script;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(script->main_window != NULL)
	{
		if(script->debug_window != NULL)
		{
			gtk_widget_show_all(script->debug_window);
		}

		gtk_widget_show_all(script->main_window);
		(void)gtk_dialog_run(GTK_DIALOG(script->main_window));
	}

	return 0;
}

static int ScriptEnd(lua_State* lua)
{
	SCRIPT *script;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(script->main_window != NULL)
	{
		gtk_widget_destroy(script->main_window);
	}

	return 0;
}

static int CallBack(CALLBACK_INFO* info)
{
	cairo_t *cairo_p = NULL;
	int stack_top = lua_gettop(info->lua);
	int arg_num = 1;

	lua_rawgeti(info->lua, LUA_REGISTRYINDEX, info->handler_ref);
	if(lua_isnil(info->lua, -1) != 0)
	{
		lua_pop(info->lua, 1);
		return luaL_error(info->lua, "Callback handler not found");
	}

	lua_pushlightuserdata(info->lua, info->object);

	if(strcmp(info->query.signal_name, "button-press-event") == 0
		|| strcmp(info->query.signal_name, "motion-notify-event") == 0
		|| strcmp(info->query.signal_name, "button-release-event") == 0)
	{
		MOUSE_EVENT mouse;
#if GTK_MAJOR_VERSION <= 2
		gdk_window_get_pointer(info->object->window, &mouse.x, &mouse.y, &mouse.state);
#else
		gdk_window_get_pointer(gtk_widget_get_window(info->object), &mouse.x, &mouse.y, &mouse.state);
#endif
		arg_num++;
		
		lua_createtable(info->lua, 0, 3);
		lua_pushinteger(info->lua, mouse.x);
		lua_setfield(info->lua, -2, "x");
		lua_pushinteger(info->lua, mouse.y);
		lua_setfield(info->lua, -2, "y");
		lua_pushunsigned(info->lua, mouse.state);
		lua_setfield(info->lua, -2, "state");
	}
#if GTK_MAJOR_VERSION <= 2
	else if(strcmp(info->query.signal_name, "expose-event") == 0)
	{
		cairo_p = gdk_cairo_create(info->object->window);
#else
	else if(strcmp(info->query.signal_name, "draw") == 0)
	{
		cairo_p = gdk_cairo_create(gtk_widget_get_window(info->object));
#endif
		arg_num++;

		lua_pushlightuserdata(info->lua, cairo_p);
	}

	if(info->args_ref != 0)
	{
		lua_rawgeti(info->lua, LUA_REGISTRYINDEX, info->args_ref);
		arg_num++;
	}

	lua_call(info->lua, arg_num, 0);

	if(cairo_p != NULL)
	{
		cairo_destroy(cairo_p);
	}

	return 0;
}

static void FreeCallBackInfo(void* data, GClosure* closure)
{
	CALLBACK_INFO *info = (CALLBACK_INFO*)data;
	//luaL_unref(info->lua, LUA_REGISTRYINDEX, info->handler_ref);
	//luaL_unref(info->lua, LUA_REGISTRYINDEX, info->object_ref);

	MEM_FREE_FUNC(info);
}

static int ScriptSignalConnect(lua_State* lua)
{
	CALLBACK_INFO *info;
	SCRIPT *script;
	GtkWidget *widget;
	const char *signal;
	gulong handler_id;
	guint signal_id;
	int stack_top;

	widget = (GtkWidget*)lua_topointer(lua, 1);
	signal = lua_tostring(lua, 2);

	info = (CALLBACK_INFO*)MEM_ALLOC_FUNC(sizeof(*info));
	(void)memset(info, 0, sizeof(*info));
	info->lua = lua;
	info->object = widget;

#if GTK_MAJOR_VERSION >= 3
	if(strcmp(signal, "expose_event") == 0
		|| strcmp(signal, "expose-event") == 0)
	{
		signal = "draw";
	}
#endif

	signal_id = g_signal_lookup(signal, G_OBJECT_TYPE(widget));
	if(signal_id == 0)
	{
		luaL_error(lua, "Couldn't find signal %s", signal);
	}

	g_signal_query(signal_id, &info->query);

	stack_top = lua_gettop(lua);
	lua_pushvalue(lua, 3);
	info->handler_ref = luaL_ref(lua, LUA_REGISTRYINDEX);

	if(stack_top > 3 && lua_type(lua, 4) != LUA_TNIL)
	{
		lua_pushvalue(lua, 4);
		lua_rawseti(lua, -2, 1);
		info->args_ref = luaL_ref(lua, LUA_REGISTRYINDEX);
	}

	lua_getglobal(lua, "SCRIPT_DATA");
	info->script = script = (SCRIPT*)lua_topointer(lua, -1);
	info->object = widget;

	lua_pushvalue(lua, 1);
	info->object_ref = luaL_ref(lua, LUA_REGISTRYINDEX);

	handler_id = g_signal_connect_data(G_OBJECT(widget),
		signal, G_CALLBACK(CallBack), info, FreeCallBackInfo, G_CONNECT_SWAPPED);

	lua_pushnumber(lua, handler_id);

	return 1;
}

static int ScriptWidgetDestroy(lua_State* lua)
{
	gtk_widget_destroy(GTK_WIDGET(lua_topointer(lua, 1)));

	return 0;
}

static int ScriptObjectSetData(lua_State* lua)
{
	GObject *object;
	const char *key;
	int args_ref;

	object = G_OBJECT(lua_topointer(lua, 1));
	key = lua_tostring(lua, 2);

	lua_pushvalue(lua, 3);
	args_ref = luaL_ref(lua, LUA_REGISTRYINDEX);

	g_object_set_data(object, key, GINT_TO_POINTER(args_ref));

	return 0;
}

static int ScriptObjectGetData(lua_State* lua)
{
	GObject *object;
	const char *key;
	int args_ref;

	object = G_OBJECT(lua_topointer(lua, 1));
	key = lua_tostring(lua, 2);

	args_ref = GPOINTER_TO_INT(g_object_get_data(object, key));
	lua_rawgeti(lua, LUA_REGISTRYINDEX, args_ref);

	return 1;
}

static int ScriptSetSizeRequest(lua_State* lua)
{
	GtkWidget *widget = GTK_WIDGET(lua_topointer(lua, -3));
	int width = (int)luaL_checkinteger(lua, -2);
	int height = (int)luaL_checkinteger(lua, -1);

	if(GTK_IS_WIDGET(widget) == FALSE)
	{
		return 0;
	}

	gtk_widget_set_size_request(widget, width, height);

	return 0;
}

static int ScriptGetWidgetSize(lua_State* lua)
{
	GtkWidget *widget = GTK_WIDGET(lua_topointer(lua, -1));
	GtkAllocation allocation;

	if(GTK_IS_WIDGET(widget) == FALSE)
	{
		return 0;
	}

	gtk_widget_get_allocation(widget, &allocation);

	lua_pushinteger(lua, allocation.width);
	lua_pushinteger(lua, allocation.height);

	return 2;
}

static int ScriptShowWidget(lua_State* lua)
{
	GtkWidget *widget = GTK_WIDGET(lua_topointer(lua, -1));
	gtk_widget_show_all(widget);

	return 0;
}

static int ScriptSetEvents(lua_State* lua)
{
	GtkWidget *widget = GTK_WIDGET(lua_topointer(lua, -2));
	int events = luaL_checkunsigned(lua, -1);

	if(GTK_IS_WIDGET(widget) != FALSE)
	{
		gtk_widget_set_events(widget, events);
	}

	return 0;
}

static int ScriptAddEvents(lua_State* lua)
{
	GtkWidget *widget = GTK_WIDGET(lua_topointer(lua, -2));
	int events = luaL_checkunsigned(lua, -1);

	if(GTK_IS_WIDGET(widget) != FALSE)
	{
		gtk_widget_add_events(widget, events);
	}

	return 0;
}

static int ScriptQueueDraw(lua_State* lua)
{
	GtkWidget *widget = GTK_WIDGET(lua_topointer(lua, -1));

	if(GTK_IS_WIDGET(widget) != FALSE)
	{
		gtk_widget_queue_draw(widget);
	}

	return 0;
}

static int ScriptAdjustmentNew(lua_State* lua)
{
	GtkAdjustment *adjust;
	gdouble start, lower, upper, step, page, page_size;
	start = lua_tonumber(lua, 1);
	lower = lua_tonumber(lua, 2);
	upper = lua_tonumber(lua, 3);
	step = lua_tonumber(lua, 4);
	page = lua_tonumber(lua, 5);
	page_size = lua_tonumber(lua, 6);

	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(
		start, lower, upper, step, page, page_size));

	lua_pushlightuserdata(lua, adjust);

	return 1;
}

static int ScriptAdjustmentGetValue(lua_State* lua)
{
	GtkAdjustment *adjust = GTK_ADJUSTMENT(lua_topointer(lua, 1));

	if(GTK_IS_ADJUSTMENT(adjust) == FALSE)
	{
		return 0;
	}

	lua_pushnumber(lua,
		gtk_adjustment_get_value(adjust));

	return 1;
}

static int ScriptSeparatorNew(lua_State* lua)
{
	if(lua_toboolean(lua, 1) == 0)
	{
		lua_pushlightuserdata(lua,
			gtk_hseparator_new());
	}
	else
	{
		lua_pushlightuserdata(lua,
			gtk_vseparator_new());
	}

	return 1;
}

static int ScriptLabelNew(lua_State* lua)
{
	const char *label = luaL_checkstring(lua, -1);

	lua_pushlightuserdata(lua,
		gtk_label_new(label));

	return 1;
}

static int ScriptLabelSetText(lua_State* lua)
{
	GtkLabel *label = GTK_LABEL(lua_topointer(lua, -2));
	const char *text = luaL_checkstring(lua, -1);

	if(GTK_IS_LABEL(label) == FALSE)
	{
		return 0;
	}

	gtk_label_set_text(label, text);

	return 0;
}

static int ScriptButtonNew(lua_State* lua)
{
	GtkWidget *button;
	const char *label = NULL;

	if(lua_gettop(lua) > 0 && lua_type(lua, 1) == LUA_TSTRING)
	{
		label = luaL_checkstring(lua, -1);
		button = gtk_button_new_with_label(label);
	}
	else
	{
		button = gtk_button_new();
	}

	lua_pushlightuserdata(lua, button);

	return 1;
}

static int ScriptToggleButtonNew(lua_State* lua)
{
	const char *label;

	label = lua_tostring(lua, -1);

	lua_pushlightuserdata(lua,
		gtk_toggle_button_new_with_label(label));

	return 1;
}

static int ScriptCheckButtonNew(lua_State* lua)
{
	const char* label;

	label = lua_tostring(lua, -1);

	lua_pushlightuserdata(lua,
		gtk_check_button_new_with_label(label));

	return 1;
}

static int ScriptRadioButtonNew(lua_State* lua)
{
	const char *label;
	GSList *group;

	group = (GSList*)lua_topointer(lua, 1);
	label = lua_tostring(lua, 2);

	lua_pushlightuserdata(lua,
		gtk_radio_button_new_with_label(group, label));

	return 1;
}

static int ScriptGetRadioButtonGroup(lua_State* lua)
{
	GtkRadioButton *button = GTK_RADIO_BUTTON(
		lua_topointer(lua, 1));

	if(button == NULL)
	{
		return 0;
	}

	lua_pushlightuserdata(lua,
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(button)));

	return 1;
}

static int ScriptGetToggleButtonActive(lua_State* lua)
{
	GtkToggleButton *button = GTK_TOGGLE_BUTTON(
		lua_topointer(lua, -1));

	if(GTK_IS_TOGGLE_BUTTON(button) == FALSE)
	{
		return 0;
	}

	lua_pushboolean(lua,
		gtk_toggle_button_get_active(button));

	return 1;
}

static int ScriptHBoxNew(lua_State* lua)
{
	gboolean homogeneous = lua_toboolean(lua, -2);
	int spacing = (int)lua_tointeger(lua, -1);

	lua_pushlightuserdata(lua, gtk_hbox_new(homogeneous, spacing));

	return 1;
}

static int ScriptVBoxNew(lua_State* lua)
{
	gboolean homogeneous = lua_toboolean(lua, -2);
	int spacing = (int)lua_tointeger(lua, -1);

	lua_pushlightuserdata(lua, gtk_vbox_new(homogeneous, spacing));

	return 1;
}

static int ScriptBoxPackStart(lua_State* lua)
{
	GtkBox *box = GTK_BOX(lua_topointer(lua, -5));
	GtkWidget *widget = GTK_WIDGET(lua_topointer(lua, -4));
	int expand = lua_toboolean(lua, -3);
	int fill = lua_toboolean(lua, -2);
	int padding = (int)luaL_checkinteger(lua, -1);

	if(GTK_IS_BOX(box) != FALSE && GTK_IS_WIDGET(widget) != FALSE)
	{
		gtk_box_pack_start(box, widget, expand, fill, padding);
	}

	return 0;
}

static int ScriptBoxPackEnd(lua_State* lua)
{
	GtkBox *box = GTK_BOX(lua_topointer(lua, -5));
	GtkWidget *widget = GTK_WIDGET(lua_topointer(lua, -4));
	int expand = lua_toboolean(lua, -3);
	int fill = lua_toboolean(lua, -2);
	int padding = (int)luaL_checkinteger(lua, -1);

	if(GTK_IS_BOX(box) != FALSE && GTK_IS_WIDGET(widget) != FALSE)
	{
		gtk_box_pack_end(box, widget, expand, fill, padding);
	}

	return 0;
}

static int ScriptHScaleNew(lua_State* lua)
{
	GtkAdjustment *adjust = GTK_ADJUSTMENT(lua_topointer(lua, 1));

	lua_pushlightuserdata(lua, gtk_hscale_new(adjust));

	return 1;
}

static int ScriptVScaleNew(lua_State* lua)
{
	GtkAdjustment *adjust = GTK_ADJUSTMENT(lua_topointer(lua, 1));

	lua_pushlightuserdata(lua, gtk_vscale_new(adjust));

	return 1;
}

static int ScriptSpinScaleNew(lua_State* lua)
{
	GtkAdjustment *adjust = GTK_ADJUSTMENT(lua_topointer(lua, 1));
	const char *label = lua_tostring(lua, 2);
	int digit = (int)lua_tointeger(lua, 3);

	lua_pushlightuserdata(lua, SpinScaleNew(adjust, label, digit));

	return 1;
}

static int ScriptSpinScaleSetLimits(lua_State* lua)
{
	if(lua_gettop(lua) >= 3)
	{
		GtkWidget *widget = GTK_WIDGET(lua_topointer(lua, 1));
		FLOAT_T min_value = lua_tonumber(lua, 2);
		FLOAT_T max_value = lua_tonumber(lua, 3);

		SpinScaleSetScaleLimits((SPIN_SCALE*)widget, min_value, max_value);
	}

	return 0;
}

static int ScriptSpinButtonNew(lua_State* lua)
{
	GtkAdjustment *adjust = GTK_ADJUSTMENT(lua_topointer(lua, 1));

	lua_pushlightuserdata(lua, gtk_spin_button_new(adjust, 1, 0));

	return 1;
}

static int ScriptComboBoxNew(lua_State* lua)
{
#if GTK_MAJOR_VERSION <= 2
	lua_pushlightuserdata(lua,
		gtk_combo_box_new_text());
#else
	lua_pushlightuserdata(lua,
		gtk_combo_box_text_new());
#endif

	return 1;
}

static int ScriptComboBoxAppendText(lua_State* lua)
{
	GtkComboBox *combo = GTK_COMBO_BOX(lua_topointer(lua, -2));
	const char *text = luaL_checkstring(lua, -1);

	if(GTK_IS_COMBO_BOX(combo) == FALSE)
	{
		return 0;
	}

#if GTK_MAJOR_VERSION <= 2
	gtk_combo_box_append_text(combo, text);
#else
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), text);
#endif

	return 0;
}

static int ScriptComboBoxPrependText(lua_State* lua)
{
	GtkComboBox *combo = GTK_COMBO_BOX(lua_topointer(lua, 1));
	const char *text = lua_tostring(lua, 2);

	if(GTK_IS_COMBO_BOX(combo) == FALSE)
	{
		return 0;
	}

#if GTK_MAJOR_VERSION <= 2
	gtk_combo_box_prepend_text(combo, text);
#else
	gtk_combo_box_text_prepend_text(GTK_COMBO_BOX_TEXT(combo), text);
#endif

	return 0;
}

static int ScriptComboBoxSetActive(lua_State* lua)
{
	GtkComboBox *combo = GTK_COMBO_BOX(lua_topointer(lua, 1));
	gint active = (int)lua_tointeger(lua, 2);

	gtk_combo_box_set_active(combo, active);

	return 0;
}

static int ScriptComboBoxGetActive(lua_State* lua)
{
	GtkComboBox *combo = GTK_COMBO_BOX(lua_topointer(lua, 1));
	
	lua_pushinteger(lua, gtk_combo_box_get_active(combo));

	return 1;
}

static int ScriptComboBoxGetValue(lua_State* lua)
{
	GtkComboBox *combo = GTK_COMBO_BOX(lua_topointer(lua, -1));
	const gchar *value;

	if(GTK_IS_COMBO_BOX(combo) == FALSE)
	{
		return 0;
	}

#if GTK_MAJOR_VERSION <= 2
	value = gtk_combo_box_get_active_text(combo);
#else
	value = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo));
#endif

	lua_pushstring(lua, value);

	return 1;
}

static int ScriptComboSelectBlendModeBoxNew(lua_State* lua)
{
#if GTK_MAJOR_VERSION <= 2
	GtkWidget *combo = gtk_combo_box_new_text();
#else
	GtkWidget *combo = gtk_combo_box_text_new();
#endif
	SCRIPT *script;
	int i;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);
	for(i=0; i<LAYER_BLEND_SLELECTABLE_NUM; i++)
	{
#if GTK_MAJOR_VERSION <= 2
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo),
			script->app->labels->layer_window.blend_modes[i]);
#else
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo),
			script->app->labels->layer_window.blend_modes[i]);
#endif
	}

	lua_pushlightuserdata(lua, (void*)combo);

	return 1;
}

static int ScriptColorSelectionGetCurrentColor(lua_State* lua)
{
	GtkWidget *selection = GTK_WIDGET(lua_topointer(lua, 1));
	GdkColor color;
	
	gtk_color_selection_get_current_color(
		GTK_COLOR_SELECTION(selection), &color);

	lua_pushinteger(lua, color.red & 0xff);
	lua_pushinteger(lua, color.green & 0xff);
	lua_pushinteger(lua, color.blue & 0xff);

	return 3;
}

static int ScriptColorSelectionSetCurrentColor(lua_State* lua)
{
	GtkWidget *selection = GTK_WIDGET(lua_topointer(lua, 1));
	GdkColor color = {0};
	int r, g, b;

	r = (int)lua_tointeger(lua, 2);
	color.red = (uint16)(r | (r << 8));
	g = (int)lua_tointeger(lua, 3);
	color.green = (uint16)(g | (g << 8));
	b = (int)lua_tointeger(lua, 4);
	color.blue = (uint16)(b | (b << 8));

	gtk_color_selection_set_current_color(
		GTK_COLOR_SELECTION(selection), &color);

	return 0;
}

static int ScriptColorSelectionDialogNew(lua_State*lua)
{
	SCRIPT *script;
	const char *title = lua_tostring(lua, 1);
	GtkWidget *dialog = gtk_color_selection_dialog_new(title);

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(script->app->window));
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);

	lua_pushlightuserdata(lua, dialog);

	return 1;
}

static int ScriptColorSelectionDialogGetColorSelection(lua_State* lua)
{
	GtkWidget *dialog = GTK_WIDGET(lua_topointer(lua, 1));

	lua_pushlightuserdata(lua, gtk_color_selection_dialog_get_color_selection(
		GTK_COLOR_SELECTION_DIALOG(dialog)));

	return 1;
}

static int ScriptContainerAdd(lua_State* lua)
{
	GtkContainer *container = GTK_CONTAINER(
		lua_topointer(lua, -2));
	GtkWidget *widget = GTK_WIDGET(
		lua_topointer(lua, -1));

	if(GTK_IS_DIALOG(container) != FALSE)
	{
		gtk_container_add(GTK_CONTAINER(
			gtk_dialog_get_content_area(GTK_DIALOG(container))), widget);
	}
	else
	{
		gtk_container_add(container, widget);
	}

	return 0;
}

static int ScriptCreateButton(lua_State* lua)
{
	lua_pushlightuserdata(lua,
		gtk_button_new_with_label(luaL_checkstring(lua, -1)));

	return 1;
}

static int ScriptCreateDrawingArea(lua_State* lua)
{
	lua_pushlightuserdata(lua, gtk_drawing_area_new());

	return 1;
}

static int ScriptProgressBarNew(lua_State* lua)
{
	lua_pushlightuserdata(lua, gtk_progress_bar_new());

	return 1;
}

static int ScriptProgressBarSetFraction(lua_State* lua)
{
	GtkWidget *widget = GTK_WIDGET(lua_topointer(lua, 1));
	double value = lua_tonumber(lua, 2);

	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(widget), value);

	return 0;
}

static int ScriptProgressBarSetText(lua_State* lua)
{
	GtkWidget *widget = GTK_WIDGET(lua_topointer(lua, 1));
	const char *text = lua_tostring(lua, 2);

	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(widget), text);

	return 0;
}

static void OnCairoWidgetDestroy(GtkWidget* widget)
{
	cairo_t *cairo_p = (cairo_t*)g_object_get_data(G_OBJECT(widget), "cairo_pointer");
	if(cairo_p != NULL)
	{
		cairo_destroy(cairo_p);
	}
}

static void OnCairoDestroy(void *data)
{
	if(data != NULL)
	{
		g_object_set_data(G_OBJECT(data), "cairo_pointer", NULL);
	}
}

static int ScriptGetWidgetCairo(lua_State* lua)
{
	GtkWidget *widget = GTK_WIDGET(lua_topointer(lua, -1));
#if GTK_MAJOR_VERSION <= 2
	cairo_t *cairo_p = gdk_cairo_create(widget->window);
#else
	cairo_t * cairo_p = gdk_cairo_create(gtk_widget_get_window(widget));
#endif
	const cairo_user_data_key_t widget_key = {0};

	if(cairo_p == NULL)
	{
		return 0;
	}

	cairo_set_user_data(cairo_p, &widget_key, (void*)widget,
		(cairo_destroy_func_t)OnCairoDestroy);
	(void)g_signal_connect(G_OBJECT(widget), "destroy",
		G_CALLBACK(OnCairoWidgetDestroy), NULL);

	lua_pushlightuserdata(lua, cairo_p);

	return 1;
}

static int ScriptCairoCreate(lua_State* lua)
{
	cairo_surface_t *surface_p =
		(cairo_surface_t*)lua_topointer(lua, -1);
	cairo_t *cairo_p = cairo_create(surface_p);

	lua_pushlightuserdata(lua, cairo_p);

	return 1;
}

static int ScriptCairoSurfaceCreate(lua_State* lua)
{
	SCRIPT *script;
	cairo_surface_t *surface_p;
	int width, height;
	int i;

	width = (int)lua_tointeger(lua, 1);
	height = (int)lua_tointeger(lua, 2);
	surface_p = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	for(i=0; i<sizeof(script->surfaces)/sizeof(script->surfaces[0]); i++)
	{
		if(script->surfaces[i] == NULL)
		{
			script->surfaces[i] = surface_p;
		}
	}
	
	lua_pushlightuserdata(lua, surface_p);

	return 1;
}

static int ScriptCairoDestroy(lua_State* lua)
{
	if(lua_gettop(lua) > 0)
	{
		cairo_destroy((cairo_t*)lua_topointer(lua, -1));
	}

	return 0;
}

static int ScriptSurfaceDestroy(lua_State* lua)
{
	if(lua_gettop(lua) > 0)
	{
		SCRIPT *script;
		int i;

		cairo_surface_t *surface_p = (cairo_surface_t*)lua_topointer(lua, -1);
		cairo_surface_destroy(surface_p);

		lua_getglobal(lua, "SCRIPT_DATA");
		script = (SCRIPT*)lua_topointer(lua, -1);

		for(i=0; i<sizeof(script->surfaces)/sizeof(script->surfaces[0]); i++)
		{
			if(script->surfaces[i] == surface_p)
			{
				script->surfaces[i] = NULL;
			}
		}
	}

	return 0;
}

static int ScriptGetSurfacePixels(lua_State* lua)
{
	cairo_surface_t *surface_p =
		(cairo_surface_t*)lua_topointer(lua, -1);
	guint32 pixel_value;
	uint8 *pixels;
	uint8 *pixel;
	int width, height;
	int stride;
	int i, j;

	pixels = cairo_image_surface_get_data(surface_p);
	width = cairo_image_surface_get_width(surface_p);
	height = cairo_image_surface_get_height(surface_p);
	stride = cairo_image_surface_get_stride(surface_p);

	lua_createtable(lua, 0, height);
	for(i=0; i<height; i++)
	{
		lua_pushinteger(lua, i+1);
		lua_createtable(lua, 0, width);
		pixel = &pixels[i*stride];
		for(j=0; j<width; j++, pixel+=4)
		{
			lua_pushinteger(lua, j+1);
			pixel_value = pixel[0] << 24 | pixel[1] << 16 | pixel[2] << 8 | pixel[3];
			lua_pushunsigned(lua, pixel_value);
			lua_settable(lua, -3);
		}

		lua_settable(lua, -3);
	}

	return 1;
}

static int ScriptCairoSetSourceRGBA(lua_State* lua)
{
	FLOAT_T r, g, b, a;
	cairo_t *cairo_p;
	int stack_size = lua_gettop(lua);

	if(stack_size < 4)
	{
		return 0;
	}
	else if(stack_size > 5)
	{
		stack_size = 5;
	}

	cairo_p = (cairo_t*)lua_topointer(lua, -stack_size);
	if(cairo_p == NULL)
	{
		return 0;
	}

	r = lua_tonumber(lua, -stack_size+1);
	g = lua_tonumber(lua, -stack_size+2);
	b = lua_tonumber(lua, -stack_size+3);

	if(stack_size >= 5)
	{
		a = lua_tonumber(lua, -stack_size+4);
	}
	else
	{
		a = 1;
	}

#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
	cairo_set_source_rgba(cairo_p, b, g, r, a);
#else
	cairo_set_source_rgba(cairo_p, r, g, b, a);
#endif

	return 0;
}

static int ScriptCairoSetSourceSurface(lua_State* lua)
{
	cairo_t *cairo_p;
	cairo_surface_t *surface_p;
	FLOAT_T x = 0, y = 0;
	int stack_size = lua_gettop(lua);

	if(stack_size < 2)
	{
		return 0;
	}
	
	cairo_p = (cairo_t*)lua_topointer(lua, 1);
	surface_p = (cairo_surface_t*)lua_topointer(lua, 2);

	if(stack_size >= 4)
	{
		x = lua_tonumber(lua, 3);
		y = lua_tonumber(lua, 4);
	}

	cairo_set_source_surface(cairo_p, surface_p, x, y);

	return 0;
}

static int ScriptCairoStroke(lua_State* lua)
{
	int stack_size = lua_gettop(lua);

	if(stack_size > 0)
	{
		cairo_stroke((cairo_t*)lua_topointer(lua, -1));
	}

	return 0;
}

static int ScriptCairoFill(lua_State* lua)
{
	int stack_size = lua_gettop(lua);

	if(stack_size > 0)
	{
		cairo_fill((cairo_t*)lua_topointer(lua, -1));
	}

	return 0;
}

static int ScriptCairoPaint(lua_State* lua)
{
	cairo_t *cairo_p;
	FLOAT_T alpha = 1;
	int stack_size = lua_gettop(lua);

	if(stack_size <= 0)
	{
		return 0;
	}

	cairo_p = (cairo_t*)lua_topointer(lua, 1);
	if(stack_size >= 2)
	{
		alpha = lua_tonumber(lua, 2);
	}

	cairo_paint_with_alpha(cairo_p, alpha);

	return 0;
}

static int ScriptCairoSetOperator(lua_State* lua)
{
	if(lua_gettop(lua) >= 2)
	{
		cairo_set_operator((cairo_t*)lua_topointer(lua, -2),
			lua_tointeger(lua, -1));
	}

	return 0;
}

static int ScriptCairoMoveTo(lua_State* lua)
{
	if(lua_gettop(lua) >= 3)
	{
		cairo_move_to((cairo_t*)lua_topointer(lua, -3),
			lua_tonumber(lua, -2), lua_tonumber(lua, -1));
	}

	return 0;
}

static int ScriptCairoLineTo(lua_State* lua)
{
	if(lua_gettop(lua) >= 3)
	{
		cairo_line_to((cairo_t*)lua_topointer(lua, -3),
			lua_tonumber(lua, -2), lua_tonumber(lua, -1));
	}

	return 0;
}

static int ScriptCairoArc(lua_State* lua)
{
	if(lua_gettop(lua) > 0)
	{
		cairo_arc((cairo_t*)lua_topointer(lua, -6),
			lua_tonumber(lua, -5), lua_tonumber(lua, -4),
				lua_tonumber(lua, -3), lua_tonumber(lua, -2),
					lua_tonumber(lua, -1));
	}

	return 0;
}

static int ScriptCairoTranslate(lua_State* lua)
{
	if(lua_gettop(lua) >= 3)
	{
		cairo_translate((cairo_t*)lua_topointer(lua, 1),
			lua_tonumber(lua, 2), lua_tonumber(lua, 3));
	}

	return 0;
}

static int ScriptCairoPaintPixels(lua_State* lua)
{
	guint32 *pixels;
	uint8 *image;
	guint32 pixel_value;
	cairo_t *cairo_p;
	cairo_surface_t *surface_p;
	double x1, x2, y1, y2;
	int width, height;
	int stride;
	int i, j;

	cairo_p = (cairo_t*)lua_topointer(lua, 1);
	cairo_clip_extents(cairo_p, &x1, &y1, &x2, &y2);
	width = (int)(x2 - x1),	height = (int)(y2 - y1);
	stride = width * 4;

	image = (uint8*)MEM_ALLOC_FUNC(stride * height);
	surface_p = cairo_image_surface_create_for_data(image,
		CAIRO_FORMAT_ARGB32, width, height, stride);

	for(i=0; i<height; i++)
	{
		lua_pushinteger(lua, i+1);
		lua_gettable(lua, -2);
		pixels = (uint32*)&image[i*stride];
		for(j=0; j<width; j++, pixels++)
		{
			lua_pushinteger(lua, j+1);
			lua_gettable(lua, -2);
			pixel_value = lua_tounsigned(lua, -1);
			*pixels = pixel_value;
			lua_pop(lua, 1);
		}

		lua_pop(lua, 1);
	}

	cairo_set_source_surface(cairo_p, surface_p, 0, 0);
	cairo_paint(cairo_p);

	cairo_surface_destroy(surface_p);
	MEM_FREE_FUNC(image);

	return 0;
}

static int ScriptCairoScale(lua_State* lua)
{
	if(lua_gettop(lua) >= 3)
	{
		cairo_scale((cairo_t*)lua_topointer(lua, 1),
			lua_tonumber(lua, 2), lua_tonumber(lua, 3));
	}

	return 0;
}

static int ScriptCairoRotate(lua_State* lua)
{
	if(lua_gettop(lua) >= 2)
	{
		cairo_rotate((cairo_t*)lua_topointer(lua, 1),
			lua_tonumber(lua, 2));
	}

	return 0;
}

static int ScriptCairoSave(lua_State* lua)
{
	if(lua_gettop(lua) > 0)
	{
		cairo_save((cairo_t*)lua_topointer(lua, 1));
	}

	return 0;
}

static int ScriptCairoRestore(lua_State* lua)
{
	if(lua_gettop(lua) > 0)
	{
		cairo_restore((cairo_t*)lua_topointer(lua, 1));
	}

	return 0;
}

/*************************************************
* ScriptBrushPatternNew関数                      *
* スクリプトブラシのパターンを作成する           *
* 引数                                           *
* script	: スクリプト実行用の構造体のアドレス *
*************************************************/
void ScriptBrushPatternNew(SCRIPT* script)
{
	SCRIPT_BRUSH *brush = script->brush_data;

	if(brush->brush_surface != NULL)
	{
		cairo_surface_destroy(brush->brush_surface);
	}

	if(brush->channel == 4)
	{
		brush->brush_surface = cairo_image_surface_create_for_data(
			brush->pixels, CAIRO_FORMAT_ARGB32, brush->width, brush->height, brush->stride);
		brush->core->brush_pattern = cairo_pattern_create_for_surface(brush->brush_surface);
	}
	else if(brush->channel == 1)
	{
		cairo_t *for_paint;
		int size = (int)brush->r * 2 + 1;
		MEM_FREE_FUNC(brush->pixels);
		brush->pixels = (uint8*)MEM_ALLOC_FUNC(size*size*4);
		(void)memset(brush->pixels, 0, size*size*4);
		brush->brush_surface = cairo_image_surface_create_for_data(
			brush->pixels, CAIRO_FORMAT_ARGB32, brush->width, brush->height, brush->width * 4);
		for_paint = cairo_create(brush->brush_surface);
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
		cairo_set_source_rgb(for_paint, script->app->tool_window.color_chooser->rgb[2]*DIV_PIXEL,
			script->app->tool_window.color_chooser->rgb[1]*DIV_PIXEL, script->app->tool_window.color_chooser->rgb[0]*DIV_PIXEL);
#else
		cairo_set_source_rgb(for_paint, script->app->tool_window.color_chooser->rgb[0]*DIV_PIXEL,
			script->app->tool_window.color_chooser->rgb[1]*DIV_PIXEL, script->app->tool_window.color_chooser->rgb[2]*DIV_PIXEL);
#endif
		cairo_rectangle(for_paint, 0, 0, brush->width, brush->height);
		cairo_mask_surface(for_paint, brush->alpha_surface, 0, 0);
		cairo_destroy(for_paint);

		brush->core->brush_pattern = cairo_pattern_create_for_surface(brush->brush_surface);
	}
	else if(brush->channel == 2)
	{
		cairo_t *for_paint;
		HSV hsv;
		uint8 color[3];
		uint8 *mask_pixels;
		cairo_surface_t *mask;
		uint8 *paint_pixels;
		cairo_surface_t *paint_surface;
		int stride;
		int i, j;

#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
		color[0] = script->app->tool_window.color_chooser->rgb[2];
		color[1] = script->app->tool_window.color_chooser->rgb[1];
		color[2] = script->app->tool_window.color_chooser->rgb[0];
#else
		color[0] = script->app->tool_window.color_chooser->rgb[0];
		color[1] = script->app->tool_window.color_chooser->rgb[1];
		color[2] = script->app->tool_window.color_chooser->rgb[2];
#endif
		RGB2HSV_Pixel(color, &hsv);

		stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, brush->width);
		mask_pixels = (uint8*)MEM_ALLOC_FUNC(stride*brush->height);
		mask = cairo_image_surface_create_for_data(mask_pixels,
			CAIRO_FORMAT_A8, brush->width, brush->height, stride);
		(void)memset(mask_pixels, 0, stride*brush->height);

		for(i=0; i<brush->height; i++)
		{
			for(j=0; j<brush->width; j++)
			{
				mask_pixels[i*stride+j] =
					brush->alpha[i*brush->stride+j*2+1];
			}
		}

		paint_pixels = (uint8*)MEM_ALLOC_FUNC(brush->width*4*brush->height);
		paint_surface = cairo_image_surface_create_for_data(
			paint_pixels, CAIRO_FORMAT_ARGB32, brush->width, brush->height, brush->width*4);

		switch(brush->colorize_mode)
		{
		case PATTERN_MODE_SATURATION:
			for(i=0; i<brush->height; i++)
			{
				for(j=0; j<brush->width; j++)
				{
					hsv.s = brush->alpha[i*brush->stride+j*2];
					HSV2RGB_Pixel(&hsv, color);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
					paint_pixels[i*brush->width*4+j*4+0] = color[2];
					paint_pixels[i*brush->width*4+j*4+1] = color[1];
					paint_pixels[i*brush->width*4+j*4+2] = color[0];
#else
					paint_pixels[i*brush->width*4+j*4+0] = color[0];
					paint_pixels[i*brush->width*4+j*4+1] = color[1];
					paint_pixels[i*brush->width*4+j*4+2] = color[2];
#endif
					paint_pixels[i*brush->width*4+j*4+3] = 0xff;
				}
			}
			break;
		case PATTERN_MODE_BRIGHTNESS:
			for(i=0; i<brush->height; i++)
			{
				for(j=0; j<brush->width; j++)
				{
					hsv.v = brush->alpha[i*brush->stride+j*2];
					HSV2RGB_Pixel(&hsv, color);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
					paint_pixels[i*brush->width*4+j*4+0] = color[2];
					paint_pixels[i*brush->width*4+j*4+1] = color[1];
					paint_pixels[i*brush->width*4+j*4+2] = color[0];
#else
					paint_pixels[i*brush->width*4+j*4+0] = color[0];
					paint_pixels[i*brush->width*4+j*4+1] = color[1];
					paint_pixels[i*brush->width*4+j*4+2] = color[2];
#endif
					paint_pixels[i*brush->width*4+j*4+3] = 0xff;
				}
			}
			break;
		case PATTERN_MODE_FORE_TO_BACK:
			for(i=0; i<brush->height; i++)
			{
				for(j=0; j<brush->width; j++)
				{
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
					paint_pixels[i*brush->width*4+j*4+0] = ((brush->alpha[i*brush->stride+j*2]+1)*color[2]
					+ (0xff - brush->alpha[i*brush->stride+j*2]+1) * script->app->tool_window.color_chooser->back_rgb[2]) >> 8;
					paint_pixels[i*brush->width*4+j*4+1] = ((brush->alpha[i*brush->stride+j*2]+1)*color[1]
					+ (0xff - brush->alpha[i*brush->stride+j*2]+1) * script->app->tool_window.color_chooser->back_rgb[1]) >> 8;
					paint_pixels[i*brush->width*4+j*4+2] = ((brush->alpha[i*brush->stride+j*2]+1)*color[0]
					+ (0xff - brush->alpha[i*brush->stride+j*2]+1) * script->app->tool_window.color_chooser->back_rgb[0]) >> 8;
#else
					paint_pixels[i*brush->width*4+j*4+0] = ((brush->alpha[i*brush->stride+j*2]+1)*color[0]
					+ (0xff - brush->alpha[i*brush->stride+j*2]+1) * script->app->tool_window.color_chooser->back_rgb[0]) >> 8;
					paint_pixels[i*brush->width*4+j*4+1] = ((brush->alpha[i*brush->stride+j*2]+1)*color[1]
					+ (0xff - brush->alpha[i*brush->stride+j*2]+1) * script->app->tool_window.color_chooser->back_rgb[1]) >> 8;
					paint_pixels[i*brush->width*4+j*4+2] = ((brush->alpha[i*brush->stride+j*2]+1)*color[2]
					+ (0xff - brush->alpha[i*brush->stride+j*2]+1) * script->app->tool_window.color_chooser->back_rgb[2]) >> 8;
#endif
					paint_pixels[i*brush->width*4+j*4+3] = 0xff;
				}
			}
			break;
		}

		brush->pixels = (uint8*)MEM_REALLOC_FUNC(brush->pixels, brush->width*4*brush->height);
		(void)memset(brush->pixels, 0, brush->width*4*brush->height);
		brush->brush_surface = cairo_image_surface_create_for_data(brush->pixels,
			CAIRO_FORMAT_ARGB32, brush->width, brush->height, brush->width*4);
		for_paint = cairo_create(brush->brush_surface);
		cairo_set_source_surface(for_paint, paint_surface, 0, 0);
		cairo_mask_surface(for_paint, mask, 0, 0);
		cairo_surface_destroy(mask);
		MEM_FREE_FUNC(mask_pixels);
		cairo_surface_destroy(paint_surface);
		MEM_FREE_FUNC(paint_pixels);
		cairo_destroy(for_paint);

		brush->core->brush_pattern = cairo_pattern_create_for_surface(brush->brush_surface);
		cairo_pattern_set_extend(brush->core->brush_pattern, CAIRO_EXTEND_NONE);
	}
}

static int ScriptBrushCirclePatternNew(lua_State* lua)
{
	SCRIPT *script;
	cairo_t *for_paint;
	cairo_pattern_t *pattern;
	FLOAT_T alpha;
	FLOAT_T blur;
	FLOAT_T outline_hardness;
	int r;

	{
		FLOAT_T float_r = lua_tonumber(lua, 1);
		alpha = lua_tonumber(lua, 2);
		blur = lua_tonumber(lua, 3);
		outline_hardness = lua_tonumber(lua, 4);

		lua_getglobal(lua, "SCRIPT_DATA");
		script = (SCRIPT*)lua_topointer(lua, -1);
		script->brush_data->r = float_r;
		script->brush_data->width =
			script->brush_data->height = (int)script->brush_data->r * 2 + 1;
		script->brush_data->channel = 1;
	}

	if(script->brush_data->image_path != NULL)
	{
		g_free(script->brush_data->image_path);
		script->brush_data->image_path = NULL;
		MEM_FREE_FUNC(script->brush_data->cursor_pixels);
		cairo_surface_destroy(script->brush_data->cursor_surface);
		script->brush_data->cursor_pixels = NULL;
	}

	if(script->brush_data->alpha_surface != NULL)
	{
		cairo_surface_destroy(script->brush_data->alpha_surface);
	}

	r = (int)script->brush_data->r * 2 + 1;
	script->brush_data->channel = 1;
	script->brush_data->stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, r);
	script->brush_data->alpha = (uint8*)MEM_ALLOC_FUNC(r*script->brush_data->stride);
	(void)memset(script->brush_data->alpha, 0, r*script->brush_data->stride);
	script->brush_data->alpha_surface = cairo_image_surface_create_for_data(
		script->brush_data->alpha, CAIRO_FORMAT_A8, r, r, script->brush_data->stride);
	for_paint = cairo_create(script->brush_data->alpha_surface);
	pattern = cairo_pattern_create_radial(script->brush_data->r, script->brush_data->r, 0,
		script->brush_data->r, script->brush_data->r, script->brush_data->r);
	cairo_pattern_add_color_stop_rgba(pattern, 0, 0, 0, 0, alpha);
	cairo_pattern_add_color_stop_rgba(pattern, 1-blur, 0, 0, 0, alpha);
	cairo_pattern_add_color_stop_rgba(pattern, 1, 0, 0, 0, alpha*outline_hardness);
	cairo_pattern_set_extend(pattern, CAIRO_EXTEND_NONE);
	cairo_set_source(for_paint, pattern);
	cairo_paint(for_paint);

	ScriptBrushPatternNew(script);

	cairo_pattern_destroy(pattern);
	cairo_destroy(for_paint);

	return 0;
}

static int ScriptBrushImagePatternNew(lua_State* lua)
{
	SCRIPT *script;
	GFile *fp;
	GFileInputStream *stream;
	uint8 *pixels;
	const char *file_name = lua_tostring(lua, 1);
	guint32 width, height, stride;
	int cursor_stride;
	int i, j;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(script->brush_data->image_path != NULL)
	{
		g_free(script->brush_data->image_path);
	}
	script->brush_data->image_path =
		g_build_filename(script->app->current_path, "brushes", file_name, NULL);

	if((fp = g_file_new_for_path(script->brush_data->image_path)) == NULL)
	{
		return 0;
	}
	if((stream = g_file_read(fp, NULL, NULL)) == NULL)
	{
		g_object_unref(fp);
		return 0;
	}

	pixels = ReadPNGStream((void*)stream, (stream_func_t)FileRead,
		&width, &height, &stride);
	script->brush_data->width = (int)width,	script->brush_data->height = (int)height;
	script->brush_data->stride = stride;
	script->brush_data->channel = (int)(stride / width);
	cursor_stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, (int)stride);
	if(script->brush_data->cursor_pixels != NULL)
	{
		cairo_surface_destroy(script->brush_data->cursor_surface);
		MEM_FREE_FUNC(script->brush_data->cursor_pixels);
	}
	script->brush_data->cursor_pixels = (uint8*)MEM_ALLOC_FUNC(
		cursor_stride * height);
	(void)memset(script->brush_data->cursor_pixels, 0, cursor_stride * height);

	if(width > height)
	{
		script->brush_data->r = width / 2;
	}
	else
	{
		script->brush_data->r = height / 2;
	}

	if(script->brush_data->channel >= 3)
	{
		MEM_FREE_FUNC(script->brush_data->pixels);
		script->brush_data->pixels = pixels;
	}

	switch(script->brush_data->channel)
	{
	case 1:
		{
			uint8 *temp_pixels;
			script->brush_data->stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, width);
			temp_pixels = (uint8*)MEM_ALLOC_FUNC(script->brush_data->stride * height);
			if(script->brush_data->alpha_surface != NULL)
			{
				cairo_surface_destroy(script->brush_data->alpha_surface);
				MEM_FREE_FUNC(script->brush_data->alpha);
			}
			for(i=0; i<(int)height; i++)
			{
				for(j=0; j<(int)width; j++)
				{
					temp_pixels[i*script->brush_data->stride+j] = 0xff - pixels[i*stride+j];
				}
			}
			script->brush_data->alpha = temp_pixels;
			script->brush_data->alpha_surface = cairo_image_surface_create_for_data(
				temp_pixels, CAIRO_FORMAT_A8, width, height, script->brush_data->stride);
			MEM_FREE_FUNC(pixels);

			LaplacianFilter(temp_pixels, cursor_stride, height,
				script->brush_data->stride, script->brush_data->cursor_pixels);
		}
		break;
	case 2:
		{
			uint8 *cursor_temp;
			script->brush_data->alpha = pixels;

			cursor_temp = (uint8*)MEM_ALLOC_FUNC(cursor_stride*height);
			(void)memset(cursor_temp, 0, cursor_stride*height);
			for(i=0; i<(int)height; i++)
			{
				for(j=0; j<(int)width; j++)
				{
					cursor_temp[i*cursor_stride+j] = pixels[i*stride+j*2+1];
				}
			}
			script->brush_data->cursor_pixels = (uint8*)MEM_ALLOC_FUNC(cursor_stride*height);
			(void)memset(script->brush_data->cursor_pixels, 0, cursor_stride*height);
			LaplacianFilter(cursor_temp, cursor_stride, height,
				cursor_stride, script->brush_data->cursor_pixels);
			MEM_FREE_FUNC(cursor_temp);
		}
		break;
	case 3:
		{
			uint8 *temp_pixels = (uint8*)MEM_ALLOC_FUNC(width*4*height);
			int i, j;

			for(i=0; i<(int)height; i++)
			{
				for(j=0; j<(int)width; j++)
				{
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
					temp_pixels[i*width+j*4+0] = pixels[i*stride+j*3+2];
					temp_pixels[i*width+j*4+1] = pixels[i*stride+j*3+1];
					temp_pixels[i*width+j*4+2] = pixels[i*stride+j*3+0];
#else
					temp_pixels[i*width+j*4+0] = pixels[i*stride+j*3+2];
					temp_pixels[i*width+j*4+1] = pixels[i*stride+j*3+1];
					temp_pixels[i*width+j*4+2] = pixels[i*stride+j*3+0];
#endif
					temp_pixels[i*width*j*4+3] = 0xff;
				}
			}

			MEM_FREE_FUNC(pixels);
			script->brush_data->pixels = temp_pixels;
			script->brush_data->channel = 4;
		}
		// break忘れでは無い
	case 4:
		{
			uint8 *cursor_temp = (uint8*)MEM_ALLOC_FUNC(cursor_stride*height);
			(void)memset(cursor_temp, 0, cursor_stride*height);
			for(i=0; i<(int)height; i++)
			{
				for(j=0; j<(int)width; j++)
				{
					cursor_temp[i*cursor_stride+j] = script->brush_data->pixels[i*width*4+j*4+3];
				}
			}
			LaplacianFilter(cursor_temp, cursor_stride, height,
				cursor_stride, script->brush_data->cursor_pixels);
			MEM_FREE_FUNC(cursor_temp);
		}
		break;
	}
	script->brush_data->cursor_surface = cairo_image_surface_create_for_data(
		script->brush_data->cursor_pixels, CAIRO_FORMAT_A8, width, height, cursor_stride);

	ScriptBrushPatternNew(script);

	lua_pushinteger(lua, (int)width);
	lua_pushinteger(lua, (int)height);

	return 2;
}

static int SriptDrawBrush(lua_State* lua)
{
	SCRIPT *script;
	SCRIPT_BRUSH *brush_data;
	DRAW_WINDOW *window;
	cairo_matrix_t matrix;
	cairo_surface_t *update_surface;
	cairo_t *update;
	cairo_surface_t *temp_surface;
	cairo_t *temp_update;
	uint8 *mask;
	uint8 *ref_pix;
	uint8 *paint_pix;
	FLOAT_T r;
	FLOAT_T x, y;
	FLOAT_T trans_x, trans_y;
	FLOAT_T sin_value, cos_value;
	FLOAT_T half_width, half_height;
	FLOAT_T alpha;
	FLOAT_T real_x, real_y;
	int start_x, start_y;
	int width, height;
	int stride;
	int i, j;

	x = lua_tonumber(lua, 1);
	y = lua_tonumber(lua, 2);
	if(lua_gettop(lua) >= 3)
	{
		alpha = lua_tonumber(lua, 3);
	}
	else
	{
		alpha = 1;
	}

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);
	window = script->app->draw_window[script->app->active_window];
	brush_data = script->brush_data;

	half_width = brush_data->width * brush_data->zoom * 0.5;
	half_height = brush_data->height * brush_data->zoom * 0.5;
	r = brush_data->r * brush_data->zoom;

	start_x = (int)(x - r) - 1;
	width = ((int)r + 1) * 2;
	if(start_x < 0)
	{
		start_x = 0;
	}
	else if(start_x >= window->width)
	{
		return 0;
	}
	if(start_x + width > window->width)
	{
		width = window->width - start_x;
	}
	stride = width * 4;

	start_y = (int)(y - r) - 1;
	height = ((int)r + 1) * 2;
	if(start_y < 0)
	{
		start_y = 0;
	}
	else if(start_y >= window->height)
	{
		return 0;
	}
	if(start_y + height > window->height)
	{
		height = window->height - start_y;
	}

	if(width <= 0 || height <= 0)
	{
		return 0;
	}

	if((brush_data->flags & SCRIPT_BRUSH_STARTED) == 0)
	{
		brush_data->core->min_x = start_x;
		brush_data->core->max_x = start_x + width;
		brush_data->core->min_y = start_y;
		brush_data->core->max_y = start_y + height;
		brush_data->flags |= SCRIPT_BRUSH_STARTED;
	}
	else
	{
		if(brush_data->core->min_x > start_x)
		{
			brush_data->core->min_x = start_x;
		}
		if(brush_data->core->max_x < start_x + width)
		{
			brush_data->core->max_x = start_x + width;
		}
		if(brush_data->core->min_y > start_y)
		{
			brush_data->core->min_y = start_y;
		}
		if(brush_data->core->max_y < start_y + height)
		{
			brush_data->core->max_y = start_y + height;
		}
	}

	window->update.x = (int)brush_data->core->min_x;
	window->update.y = (int)brush_data->core->min_y;
	window->update.width = (int)(brush_data->core->max_x - brush_data->core->min_x);
	window->update.height = (int)(brush_data->core->max_y - brush_data->core->min_y);
	window->flags |= DRAW_WINDOW_UPDATE_PART;

	update_surface = cairo_surface_create_for_rectangle(
		window->mask_temp->surface_p, x - r, y - r, r*2+1, r*2+1);
	update = cairo_create(update_surface);
	temp_surface = cairo_surface_create_for_rectangle(
		window->temp_layer->surface_p, x - r, y - r, r*2+1, r*2+1);
	temp_update = cairo_create(temp_surface);

	for(i=0; i<height; i++)
	{
		(void)memset(&window->mask_temp->pixels[(start_y+i)*window->mask_temp->stride+start_x*4],
			0, stride);
	}

	sin_value = sin(brush_data->arg);
	cos_value = cos(brush_data->arg);
	trans_x = r - (half_width * cos_value + half_height * sin_value);
	trans_y = r + (half_width * sin_value - half_height * cos_value);

	cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
	cairo_matrix_init_scale(&matrix, 1/brush_data->zoom, 1/brush_data->zoom);
	cairo_matrix_rotate(&matrix, brush_data->arg);
	cairo_matrix_translate(&matrix, - trans_x, - trans_y);

	mask = window->mask_temp->pixels;
	cairo_pattern_set_matrix(brush_data->core->brush_pattern, &matrix);
	if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) == 0)
	{
		if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
		{
			cairo_set_source(update, brush_data->core->brush_pattern);
			cairo_paint_with_alpha(update, alpha);
		}
		else
		{
			for(i=0; i<height; i++)
			{
				(void)memset(&window->temp_layer->pixels[(start_y+i)*window->temp_layer->stride+start_x*4],
					0, stride);
			}
			cairo_set_source(temp_update, brush_data->core->brush_pattern);
			cairo_paint_with_alpha(temp_update, alpha);
			cairo_set_source_surface(update, temp_surface, 0, 0);
			cairo_mask_surface(update, window->active_layer->surface_p, - x + r, - y + r);
		}
	}
	else
	{
		if((window->active_layer->flags & LAYER_LOCK_OPACITY) == 0)
		{
			for(i=0; i<height; i++)
			{
				(void)memset(&window->temp_layer->pixels[(start_y+i)*window->temp_layer->stride+start_x*4],
					0, stride);
			}
			cairo_set_source(temp_update, brush_data->core->brush_pattern);
			cairo_paint_with_alpha(temp_update, alpha);
			cairo_set_source_surface(update, temp_surface, 0, 0);
			cairo_mask_surface(update, window->selection->surface_p, - x + r, - y + r);
		}
		else
		{
			cairo_set_source(update, brush_data->core->brush_pattern);
			cairo_paint_with_alpha(update, alpha);
			for(i=0; i<height; i++)
			{
				(void)memset(&window->temp_layer->pixels[(start_y+i)*window->temp_layer->stride+start_x*4],
					0, stride);
			}
			cairo_set_source_surface(temp_update, update_surface, 0, 0);
			cairo_mask_surface(temp_update, window->selection->surface_p, - x + r, - y + r);
			for(i=0; i<height; i++)
			{
				(void)memset(&window->mask_temp->pixels[(start_y+i)*window->temp_layer->stride+start_x*4],
					0, stride);
			}
			cairo_set_source_surface(update, temp_surface, 0, 0);
			cairo_mask_surface(update, window->active_layer->surface_p, - x + r, - y + r);
		}
	}

	if(window->app->textures.active_texture != 0)
	{
		mask = window->temp_layer->pixels;
		for(i=0; i<height; i++)
		{
			(void)memset(&window->temp_layer->pixels[(start_y+i)*window->temp_layer->stride+start_x*4],
				0, stride);
		}
		cairo_set_source_surface(temp_update, update_surface, 0, 0);
		cairo_mask_surface(temp_update, window->texture->surface_p, - x + r, - y + r);
	}

	for(i=0; i<height; i++)
	{
		ref_pix = &window->work_layer->pixels[
			(start_y+i)*window->work_layer->stride+start_x*4];
		paint_pix = &mask[(start_y+i)*window->work_layer->stride+start_x*4];
		for(j=0; j<width; j++, ref_pix += 4, paint_pix += 4)
		{
			if(ref_pix[3] < paint_pix[3])
			{
				ref_pix[0] = (uint8)((uint32)(((int)paint_pix[0]-(int)ref_pix[0])
					* paint_pix[3] >> 8) + ref_pix[0]);
				ref_pix[1] = (uint8)((uint32)(((int)paint_pix[1]-(int)ref_pix[1])
					* paint_pix[3] >> 8) + ref_pix[1]);
				ref_pix[2] = (uint8)((uint32)(((int)paint_pix[2]-(int)ref_pix[2])
					* paint_pix[3] >> 8) + ref_pix[2]);
				ref_pix[3] = (uint8)((uint32)(((int)paint_pix[3]-(int)ref_pix[3])
					* paint_pix[3] >> 8) + ref_pix[3]);
			}
		}
	}

	cairo_surface_destroy(temp_surface);
	cairo_destroy(temp_update);
	cairo_surface_destroy(update_surface);
	cairo_destroy(update);

	real_x = ((x-window->width/2)*window->cos_value + (y-window->height/2)*window->sin_value) * window->zoom_rate
		+ window->rev_add_cursor_x;
	real_y = (- (y-window->width/2)*window->sin_value + (y-window->height/2)*window->cos_value) * window->zoom_rate
		+ window->rev_add_cursor_y;
	gtk_widget_queue_draw_area(window->window, (int)(real_x - r * window->zoom_rate) - 1,
		(int)(real_y - r * window->zoom_rate) - 1, (int)r*2+1, (int)r*2+1);

	return 0;
}

static int ScriptBrushSetZoom(lua_State* lua)
{
	FLOAT_T zoom = lua_tonumber(lua, 1);
	SCRIPT *script;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	script->brush_data->zoom = zoom;

	return 0;
}

static int ScriptBrushSetRotate(lua_State* lua)
{
	FLOAT_T rotate = lua_tonumber(lua, 1);
	SCRIPT *script;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	script->brush_data->arg = rotate;

	return 0;
}

static int ScriptBrushSetBlendMode(lua_State* lua)
{
	int blend_mode = (int)lua_tointeger(lua, 1);
	SCRIPT *script;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	if(blend_mode < 0 || blend_mode >= LAYER_BLEND_SLELECTABLE_NUM)
	{
		return 0;
	}

	script->brush_data->blend_mode = (uint16)blend_mode;

	return 0;
}

static int ScriptBrushSetColorizeMode(lua_State* lua)
{
	SCRIPT *script;
	int mode;

	mode = (int)lua_tointeger(lua, 1);

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	script->brush_data->colorize_mode = (uint8)mode;

	return 0;
}

static int ScriptBrushGetDistance(lua_State* lua)
{
	SCRIPT *script;

	lua_getglobal(lua, "SCRIPT_DATA");
	script = (SCRIPT*)lua_topointer(lua, -1);

	lua_pushnumber(lua, script->brush_data->distance);

	return 1;
}

static const struct luaL_Reg g_script_functions[] =
{
	{"print", ScriptPrint},
	{"ParseRGBA", ScriptParseRGBA},
	{"MergeRGBA", ScriptMergeRGBA},
	{"GetAverageColor", ScriptGetAverageColor},
	{"RGB2HSV", ScriptRGB2HSV},
	{"HSV2RGB", ScriptHSV2RGB},
	{"WindowNew", ScriptCreateWindow},
	{"ShowDebug", ScriptCreateDebugWindow},
	{"DialogRun", ScriptRunDialog},
	{"MainLoop", ScriptMainLoop},
	{"ScriptEnd", ScriptEnd},
	{"SignalConnect", ScriptSignalConnect},
	{"WidgetDestroy", ScriptWidgetDestroy},
	{"ObjectSetData", ScriptObjectSetData},
	{"ObjectGetData", ScriptObjectGetData},
	{"SetSizeRequest", ScriptSetSizeRequest},
	{"GetWidgetSize", ScriptGetWidgetSize},
	{"ShowWidget", ScriptShowWidget},
	{"SetEvents", ScriptSetEvents},
	{"AddEvents", ScriptAddEvents},
	{"QueueDraw", ScriptQueueDraw},
	{"AdjustmentNew", ScriptAdjustmentNew},
	{"AdjustmentGetValue", ScriptAdjustmentGetValue},
	{"SeparatorNew", ScriptSeparatorNew},
	{"LabelNew", ScriptLabelNew},
	{"LabelSetText", ScriptLabelSetText},
	{"ButtonNew", ScriptButtonNew},
	{"ToggleButtonNew", ScriptToggleButtonNew},
	{"CheckButtonNew", ScriptCheckButtonNew},
	{"RadioButtonNew", ScriptRadioButtonNew},
	{"ToggleButtonGetActive", ScriptGetToggleButtonActive},
	{"RadioButtonGetGroup", ScriptGetRadioButtonGroup},
	{"HBoxNew", ScriptHBoxNew},
	{"VBoxNew", ScriptVBoxNew},
	{"SpinButtonNew", ScriptSpinButtonNew},
	{"BoxPackStart", ScriptBoxPackStart},
	{"BoxPackEnd", ScriptBoxPackEnd},
	{"HScaleNew", ScriptHScaleNew},
	{"VScaleNew", ScriptVScaleNew},
	{"SpinScaleNew", ScriptSpinScaleNew},
	{"SpinScaleSetLimits", ScriptSpinScaleSetLimits},
	{"ComboBoxNew", ScriptComboBoxNew},
	{"ComboBoxAppendText", ScriptComboBoxAppendText},
	{"ComboBoxPrependText", ScriptComboBoxPrependText},
	{"ComboBoxSetActive", ScriptComboBoxSetActive},
	{"ComboBoxGetActive", ScriptComboBoxGetActive},
	{"ComboBoxGetValue", ScriptComboBoxGetValue},
	{"ComboBoxSelectBlendModeNew", ScriptComboSelectBlendModeBoxNew},
	{"ColorSelectionGetCurrentColor", ScriptColorSelectionGetCurrentColor},
	{"ColorSelectionSetCurrentColor", ScriptColorSelectionSetCurrentColor},
	{"ColorSelectionDialogNew", ScriptColorSelectionDialogNew},
	{"ColorSelectionDialogGetColorSelection", ScriptColorSelectionDialogGetColorSelection},
	{"ContainerAdd", ScriptContainerAdd},
	{"DrawingAreaNew", ScriptCreateDrawingArea},
	{"ProgressBarNew", ScriptProgressBarNew},
	{"ProgressBarSetFraction", ScriptProgressBarSetFraction},
	{"ProgressBarSetText", ScriptProgressBarSetText},
	{"GetWidgetCairo", ScriptGetWidgetCairo},
	{"CairoCreate", ScriptCairoCreate},
	{"CairoSurfaceCreate", ScriptCairoSurfaceCreate},
	{"GetSurfacePixels", ScriptGetSurfacePixels},
	{"CairoDestroy", ScriptCairoDestroy},
	{"SurfaceDestroy", ScriptSurfaceDestroy},
	{"CairoSetSourceRGB", ScriptCairoSetSourceRGBA},
	{"CairoSetSourceSurface", ScriptCairoSetSourceSurface},
	{"CairoStroke", ScriptCairoStroke},
	{"CairoFill", ScriptCairoFill},
	{"CairoPaint", ScriptCairoPaint},
	{"CairoPaintPixels", ScriptCairoPaintPixels},
	{"CairoSetOperator", ScriptCairoSetOperator},
	{"CairoMoveTo", ScriptCairoMoveTo},
	{"CairoLineTo", ScriptCairoLineTo},
	{"CairoArc", ScriptCairoArc},
	{"CairoTranslate", ScriptCairoTranslate},
	{"CairoScale", ScriptCairoScale},
	{"CairoRotate", ScriptCairoRotate},
	{"CairoSave", ScriptCairoSave},
	{"CairoRestore", ScriptCairoRestore},
	{"UpdatePixels", ScriptUpdateLayerPixels},
	{"UpdateVectorLayer", ScriptUpdateVectorLayer},
	{"AddVectorLine", ScriptAddVectorLine},
	{"NewLayer", ScriptNewLayer},
	{"GetLayer", ScriptGetLayer},
	{"GetBottomLayer", ScriptGetBottomLayer},
	{"GetBottomLayerInfo", ScriptGetBottomLayerInfo},
	{"GetPreviousLayer", ScriptGetPreviousLayer},
	{"GetPreviousLayerInfo", ScriptGetPreviousLayerInfo},
	{"GetNextLayer", ScriptGetNextLayer},
	{"GetNextLayerInfo", ScriptGetNextLayerInfo},
	{"GetActiveLayer", ScriptGetActiveLayer},
	{"GetActiveLayerInfo", ScriptGetActiveLayerInfo},
	{"GetCanvasLayer", ScriptGetCanvasLayer},
	{"GetBlendedUnderLayer", ScriptGetBlendedUnderLayer},
	{"CirclePatternNew", ScriptBrushCirclePatternNew},
	{"ImagePatternNew", ScriptBrushImagePatternNew},
	{"DrawBrush", SriptDrawBrush},
	{"SetBrushZoom", ScriptBrushSetZoom},
	{"SetBrushRotate", ScriptBrushSetRotate},
	{"SetBrushBlendMode", ScriptBrushSetBlendMode},
	{"SetBrushColorizeMode", ScriptBrushSetColorizeMode},
	{"GetDragDistance", ScriptBrushGetDistance}
};

/*********************************************************
* CreateScript関数                                       *
* Luaスクリプト実行用のデータ作成                        *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
* file_path	: スクリプトファイルのパス                   *
* 返り値                                                 *
*	初期化された構造体のアドレス                         *
*********************************************************/
SCRIPT* CreateScript(APPLICATION* app, const char* file_path)
{
	SCRIPT *ret = (SCRIPT*)MEM_ALLOC_FUNC(sizeof(*ret));
	GFile *fp = g_file_new_for_path(file_path);
	GFileInputStream *stream = g_file_read(fp, NULL, NULL);
	size_t data_size;
	char *script_data;
	int i;
	(void)memset(ret, 0, sizeof(*ret));

	if(stream == NULL)
	{
		MEM_FREE_FUNC(ret);
		return NULL;
	}

	(void)FileSeek((void*)stream, 0, SEEK_END);
	data_size = FileSeekTell((void*)stream);
	script_data = (char*)MEM_ALLOC_FUNC(data_size+1);
	(void)FileSeek((void*)stream, 0, SEEK_SET);
	(void)FileRead(script_data, 1, data_size, stream);
	script_data[data_size] = '\0';
	g_object_unref(stream);
	g_object_unref(fp);

	ret->app = app;
	if(app->window_num > 0)
	{
		guint32 dummy = 0;
		ret->window = GetActiveDrawWindow(app);
		ret->history.data_stream = CreateMemoryStream(1024 * 1024 * 8);
		(void)MemWrite(&dummy, sizeof(dummy), 1, ret->history.data_stream);
		(void)MemWrite(&dummy, sizeof(dummy), 1, ret->history.data_stream);
	}
	ret->state = luaL_newstate();
	luaL_openlibs(ret->state);

	for(i=0; i<sizeof(g_script_functions)/sizeof(g_script_functions[0]); i++)
	{
		lua_register(ret->state, g_script_functions[i].name, g_script_functions[i].func);
	}

	if(luaL_dostring(ret->state, script_data) != 0)
	{
		GtkWidget *dialog;
		char message[8192];
		(void)sprintf(message, "Failed to open %s.\n%s", file_path, lua_tostring(ret->state, -1));
		dialog = gtk_message_dialog_new(
			GTK_WINDOW(app->window), GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, message);
		gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
		(void)gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);

		MEM_FREE_FUNC(ret);
		return NULL;
	}

	// アプリケーションの情報を登録しておく
	lua_pushlightuserdata(ret->state, (void*)ret);
	lua_setglobal(ret->state, "SCRIPT_DATA");

	// 定数の登録
	lua_pushinteger(ret->state, TYPE_NORMAL_LAYER);
	lua_setglobal(ret->state, "TYPE_NORMAL_LAYER");
	lua_pushinteger(ret->state, TYPE_VECTOR_LAYER);
	lua_setglobal(ret->state, "TYPE_VECTOR_LAYER");
	lua_pushinteger(ret->state, TYPE_TEXT_LAYER);
	lua_setglobal(ret->state, "TYPE_TEXT_LAYER");
	lua_pushinteger(ret->state, TYPE_LAYER_SET);
	lua_setglobal(ret->state, "TYPE_LAYER_SET");
	lua_pushinteger(ret->state, VECTOR_LINE_STRAIGHT);
	lua_setglobal(ret->state, "VECTOR_LINE_STRAIGHT");
	lua_pushinteger(ret->state, VECTOR_LINE_BEZIER_OPEN);
	lua_setglobal(ret->state, "VECTOR_LINE_BEZIER_OPEN");
	lua_pushinteger(ret->state, VECTOR_LINE_STRAIGHT_CLOSE);
	lua_setglobal(ret->state, "VECTOR_LINE_STRAIGHT_CLOSE");
	lua_pushinteger(ret->state, VECTOR_LINE_BEZIER_CLOSE);
	lua_setglobal(ret->state, "VECTOR_LINE_BEZIER_CLOSE");
	lua_pushinteger(ret->state, VECTOR_LINE_ANTI_ALIAS);
	lua_setglobal(ret->state, "VECTOR_LINE_ANTI_ALIAS");
	lua_pushinteger(ret->state, GDK_EXPOSURE_MASK);
	lua_setglobal(ret->state, "EXPOSURE_MASK");
	lua_pushinteger(ret->state, GDK_BUTTON_PRESS_MASK);
	lua_setglobal(ret->state, "BUTTON_PRESS_MASK");
	lua_pushinteger(ret->state, GDK_BUTTON_RELEASE_MASK);
	lua_setglobal(ret->state, "BUTTON_RELEASE_MASK");
	lua_pushinteger(ret->state, GDK_POINTER_MOTION_MASK);
	lua_setglobal(ret->state, "POINTER_MOTION_NOTIFY_MASK");
	lua_pushinteger(ret->state, GDK_POINTER_MOTION_HINT_MASK);
	lua_setglobal(ret->state, "POINTER_MOTION_HINT_MASK");
	lua_pushinteger(ret->state, GDK_BUTTON1_MASK);
	lua_setglobal(ret->state, "BUTTON1_MASK");
	lua_pushinteger(ret->state, GDK_BUTTON2_MASK);
	lua_setglobal(ret->state, "BUTTON2_MASK");
	lua_pushinteger(ret->state, GDK_BUTTON3_MASK);
	lua_setglobal(ret->state, "BUTTON3_MASK");
	lua_pushinteger(ret->state, GTK_RESPONSE_ACCEPT);
	lua_setglobal(ret->state, "RESPONSE_ACCEPT");
	lua_pushinteger(ret->state, GTK_RESPONSE_OK);
	lua_setglobal(ret->state, "RESPONSE_OK");
	lua_pushinteger(ret->state, GTK_RESPONSE_CANCEL);
	lua_setglobal(ret->state, "RESPONSE_CANCEL");
	lua_pushinteger(ret->state, CAIRO_OPERATOR_SOURCE);
	lua_setglobal(ret->state, "CAIRO_OPERATOR_SOURCE");
	lua_pushinteger(ret->state, CAIRO_OPERATOR_OVER);
	lua_setglobal(ret->state, "CAIRO_OPERATOR_OVER");
	lua_pushinteger(ret->state, CAIRO_OPERATOR_ATOP);
	lua_setglobal(ret->state, "CAIRO_OPERATOR_ATOP");
	lua_pushinteger(ret->state, CAIRO_OPERATOR_DEST_OUT);
	lua_setglobal(ret->state, "CAIRO_OPERATOR_DEST_OUT");
	lua_pushinteger(ret->state, CAIRO_OPERATOR_ADD);
	lua_setglobal(ret->state, "CAIRO_OPERATOR_ADD");
	lua_pushinteger(ret->state, CAIRO_OPERATOR_MULTIPLY);
	lua_setglobal(ret->state, "CAIRO_OPERATOR_MULTIPLY");
	lua_pushinteger(ret->state, LAYER_BLEND_NORMAL);
	lua_setglobal(ret->state, "BLEND_NORMAL");
	lua_pushinteger(ret->state, LAYER_BLEND_ADD);
	lua_setglobal(ret->state, "BLEND_ADD");
	lua_pushinteger(ret->state, LAYER_BLEND_MULTIPLY);
	lua_setglobal(ret->state, "BLEND_MULTIPLY");
	lua_pushinteger(ret->state, LAYER_BLEND_SCREEN);
	lua_setglobal(ret->state, "BLEND_SCREEN");
	lua_pushinteger(ret->state, LAYER_BLEND_OVERLAY);
	lua_setglobal(ret->state, "BLEND_OVERLAY");
	lua_pushinteger(ret->state, LAYER_BLEND_LIGHTEN);
	lua_setglobal(ret->state, "BLEND_LIGHTEN");
	lua_pushinteger(ret->state, LAYER_BLEND_DARKEN);
	lua_setglobal(ret->state, "BLEND_DARKEN");
	lua_pushinteger(ret->state, LAYER_BLEND_DODGE);
	lua_setglobal(ret->state, "BLEND_COLOR_DODGE");
	lua_pushinteger(ret->state, LAYER_BLEND_BURN);
	lua_setglobal(ret->state, "BLEND_COLOR_BURN");
	lua_pushinteger(ret->state, LAYER_BLEND_HARD_LIGHT);
	lua_setglobal(ret->state, "BLEND_HARD_LIGHT");
	lua_pushinteger(ret->state, LAYER_BLEND_SOFT_LIGHT);
	lua_setglobal(ret->state, "BLEND_SOFT_LIGHT");
	lua_pushinteger(ret->state, LAYER_BLEND_DIFFERENCE);
	lua_setglobal(ret->state, "BLEND_DIFFERENCE");
	lua_pushinteger(ret->state, LAYER_BLEND_EXCLUSION);
	lua_setglobal(ret->state, "BLEND_EXCLUSION");
	lua_pushinteger(ret->state, LAYER_BLEND_HSL_HUE);
	lua_setglobal(ret->state, "BLEND_HSL_HUE");
	lua_pushinteger(ret->state, LAYER_BLEND_HSL_SATURATION);
	lua_setglobal(ret->state, "BLEND_HSL_SATURATION");
	lua_pushinteger(ret->state, LAYER_BLEND_HSL_LUMINOSITY);
	lua_setglobal(ret->state, "BLEND_HSL_LUMINOSITY");
	lua_pushinteger(ret->state, PATTERN_MODE_SATURATION);
	lua_setglobal(ret->state, "PATTERN_MODE_SATURATION");
	lua_pushinteger(ret->state, PATTERN_MODE_BRIGHTNESS);
	lua_setglobal(ret->state, "PATTERN_MODE_BRIGHTNESS");
	lua_pushinteger(ret->state, PATTERN_MODE_FORE_TO_BACK);
	lua_setglobal(ret->state, "PATTERN_MODE_FORE_TO_BACK");

	MEM_FREE_FUNC(script_data);

	// ファイル名を記憶
	{
		const char *file_name = file_path;
		const char *str = file_path;
		while(*str != '\0')
		{
			if(*str == '/' || *str == 0xc2
				|| *str == '\\')
			{
				file_name = str+1;
			}
			str = g_utf8_next_char(str);
		}

		ret->file_name = MEM_STRDUP_FUNC(file_name);
	}

	return ret;
}

/***********************************************
* DeleteScript関数                             *
* Luaスクリプト実行用のデータを削除            *
* 引数                                         *
* script	: 削除するデータポインタのアドレス *
***********************************************/
void DeleteScript(SCRIPT** script)
{
	int i, j;

	if(*script == NULL)
	{
		return;
	}

	for(i=0; i<sizeof((*script)->work)/sizeof((*script)->work[0]); i++)
	{
		if((*script)->work[i] == NULL)
		{
			break;
		}
		else
		{
			DeleteLayer(&((*script)->work[i]));
		}
	}

	for(i=0; i<sizeof((*script)->surfaces)/sizeof((*script)->surfaces[0]); i++)
	{
		if((*script)->surfaces[i] != NULL)
		{
			cairo_surface_t *surface_p = (*script)->surfaces[i];
			cairo_surface_destroy(surface_p);
			for(j=i+1; j<sizeof((*script)->surfaces)/sizeof((*script)->surfaces[0]); j++)
			{
				if((*script)->surfaces[j] == surface_p)
				{
					(*script)->surfaces[j] = NULL;
				}
			}
		}
	}

	if((*script)->main_window != NULL)
	{
		gtk_widget_destroy((*script)->main_window);
	}

	(void)DeleteMemoryStream((*script)->history.data_stream);
	lua_close((*script)->state);
	MEM_FREE_FUNC((*script)->file_name);
	MEM_FREE_FUNC(*script);
	*script = NULL;
}

/*************************************************
* RunScript関数                                  *
* スクリプトを実行する                           *
* 引数                                           *
* script	: スクリプト実行用の構造体のアドレス *
*************************************************/
void RunScript(SCRIPT* script)
{
	if(script == NULL)
	{
		return;
	}

	// main関数をスタックに積む
	lua_getglobal(script->state, "main");

	// main関数を呼び出す(引数0個)
	if(lua_pcall(script->state, 0, 0, 0) != 0)
	{
		GtkWidget *dialog = gtk_message_dialog_new(
			GTK_WINDOW(script->app->window), GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					lua_tostring(script->state, -1));
		gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
		(void)gtk_dialog_run(GTK_DIALOG(dialog));
		gtk_widget_destroy(dialog);
	}
}

#define SCRIPT_BLOCK_SIZE 32

void SearchScriptLoop(SCRIPTS* scripts, const char* directly, int* buff_size)
{
	// ディレクトリオープン
	GDir *dir = g_dir_open(directly, 0, NULL);
	// 下層ディレクトリ
	GDir *child_dir;
	// 下層ディレクトリのパス
	char child_dir_path[4096];
	// ファイル名
	gchar *file_name;

	// ディレクトリオープン失敗
	if(dir == NULL)
	{
		return;
	}

	// ディレクトリのファイルを読み込む
	while((file_name = (gchar*)g_dir_read_name(dir)) != NULL)
	{
		(void)sprintf(child_dir_path, "%s/%s", directly, file_name);
		child_dir = g_dir_open(child_dir_path, 0, NULL);
		if(child_dir != NULL)
		{
			g_dir_close(child_dir);
			SearchScriptLoop(scripts, child_dir_path, buff_size);
		}
		else
		{
			// Luaファイル判定
			size_t name_length = strlen(file_name);
			if(StringCompareIgnoreCase(&file_name[name_length-4], ".lua") == 0)
			{
				char *name = file_name;
				size_t i;

				for(i=0; i<name_length; i++)
				{
					if(file_name[i] == '/' || file_name[i] == '\\')
					{
						name = &file_name[i+1];
					}
				}

				scripts->file_paths[scripts->num_script] = MEM_STRDUP_FUNC(child_dir_path);
				file_name[name_length-4] = '\0';
				scripts->file_names[scripts->num_script] = MEM_STRDUP_FUNC(name);
				scripts->num_script++;

				if(scripts->num_script >= (*buff_size) + 1)
				{
					*buff_size += SCRIPT_BLOCK_SIZE;
					scripts->file_paths = (char**)MEM_REALLOC_FUNC(scripts->file_paths,
						sizeof(*scripts->file_paths)*(*buff_size));
					scripts->file_names = (char**)MEM_REALLOC_FUNC(scripts->file_names,
						sizeof(*scripts->file_names)*(*buff_size));
				}
			}
		}
	}	// ディレクトリのファイルを読み込む
			// while((file_name = (gchar*)g_dir_read_name(dir)) != NULL)
}

/***************************************************************
* InitializeScripts関数                                        *
* スクリプトディレクトリにあるファイル一覧を作成               *
* 引数                                                         *
* scripts		: スクリプトファイルを管理する構造体のアドレス *
* scripts_path	: スクリプトディレクトリのパス                 *
***************************************************************/
void InitializeScripts(SCRIPTS* scripts, const char* scripts_path)
{
	int buff_size = SCRIPT_BLOCK_SIZE;

	scripts->num_script = 0;
	scripts->file_paths = (char**)MEM_ALLOC_FUNC(
		sizeof(*scripts->file_paths)*SCRIPT_BLOCK_SIZE);
	scripts->file_names = (char**)MEM_ALLOC_FUNC(
		sizeof(*scripts->file_names)*SCRIPT_BLOCK_SIZE);

	SearchScriptLoop(scripts, scripts_path, &buff_size);
}

/*************************************
* ScriptUndo関数                     *
* スクリプト実行に対する元に戻す操作 *
* 引数                               *
* window	: 元に戻す描画領域の情報 *
* p			: 履歴データ             *
*************************************/
void ScriptUndo(DRAW_WINDOW* window, void* p)
{
	// 履歴データのストリーム
	MEMORY_STREAM stream = {(uint8*)p, 0, sizeof(guint32)*2, 1};
	// データのバイト数
	guint32 history_data_size;
	// 履歴データの数
	guint32 num_history;
	// 個々の履歴データのデータ位置、サイズ、タイプ
	uint8 **data_pointers;
	guint32 *data_size;
	guint32 *data_types;
	// for文用のカウンタ
	int i;

	// 履歴データの総バイト数を読み込む
	(void)MemRead(&history_data_size, sizeof(history_data_size), 1, &stream);
	stream.data_size = history_data_size;
	// 履歴データの個数を読み込む
	(void)MemRead(&num_history, sizeof(num_history), 1, &stream);

	// 個々の履歴データの位置を設定
	data_pointers = (uint8**)MEM_ALLOC_FUNC(sizeof(*data_pointers)*num_history);
	data_size = (guint32*)MEM_ALLOC_FUNC(sizeof(*data_size)*num_history);
	data_types = (guint32*)MEM_ALLOC_FUNC(sizeof(*data_types)*num_history);
	for(i=0; i<(int)num_history; i++)
	{
		// 履歴データのタイプ
		(void)MemRead(&data_types[i], sizeof(*data_types), 1, &stream);
		// 履歴データのサイズ
		(void)MemRead(&data_size[i], sizeof(*data_size), 1, &stream);
		// 履歴データの位置
		data_pointers[i] = &stream.buff_ptr[stream.data_point];
		(void)MemSeek(&stream, data_size[i], SEEK_CUR);
	}

	for(i=(int)num_history-1; i>=0; i--)
	{
		switch(data_types[i])
		{
		case SCRIPT_HISTORY_NEW_LAYER:
			ScriptAddNewLayerUndo(window, data_pointers[i], data_size[i]);
			break;
		case SCRIPT_HISTORY_PIXEL_CHANGE:
			ScriptUpdatePixelsUndo(window, data_pointers[i], data_size[i]);
			break;
		case SCRIPT_HISTORY_VECTOR_CHANGE:
			ScriptUpdateVectorUndo(window, data_pointers[i], data_size[i]);
			break;
		}
	}

	ClearLayerView(&window->app->layer_window);
	LayerViewSetDrawWindow(&window->app->layer_window, window);

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	gtk_widget_queue_draw(window->window);

	MEM_FREE_FUNC(data_types);
	MEM_FREE_FUNC(data_size);
	MEM_FREE_FUNC(data_pointers);
}

/*************************************
* ScriptRedo関数                     *
* スクリプト実行に対するやり直し操作 *
* 引数                               *
* window	: やり直す描画領域の情報 *
* p			: 履歴データ             *
*************************************/
void ScriptRedo(DRAW_WINDOW* window, void* p)
{
	// 履歴データのストリーム
	MEMORY_STREAM stream = {(uint8*)p, 0, sizeof(guint32)*2, 1};
	// データのバイト数
	guint32 history_data_size;
	// 履歴データの数
	guint32 num_history;
	// 個々の履歴データのデータ位置、サイズ、タイプ
	uint8 **data_pointers;
	guint32 *data_size;
	guint32 *data_types;
	// for文用のカウンタ
	int i;

	// 履歴データの総バイト数を読み込む
	(void)MemRead(&history_data_size, sizeof(history_data_size), 1, &stream);
	stream.data_size = history_data_size;
	// 履歴データの個数を読み込む
	(void)MemRead(&num_history, sizeof(num_history), 1, &stream);

	// 個々の履歴データの位置を設定
	data_pointers = (uint8**)MEM_ALLOC_FUNC(sizeof(*data_pointers)*num_history);
	data_size = (guint32*)MEM_ALLOC_FUNC(sizeof(*data_size)*num_history);
	data_types = (guint32*)MEM_ALLOC_FUNC(sizeof(*data_types)*num_history);
	for(i=0; i<(int)num_history; i++)
	{
		// 履歴データのタイプ
		(void)MemRead(&data_types[i], sizeof(*data_types), 1, &stream);
		// 履歴データのサイズ
		(void)MemRead(&data_size[i], sizeof(*data_size), 1, &stream);
		// 履歴データの位置
		data_pointers[i] = &stream.buff_ptr[stream.data_point];
		(void)MemSeek(&stream, data_size[i], SEEK_CUR);
	}

	for(i=0; i<(int)num_history; i++)
	{
		switch(data_types[i])
		{
		case SCRIPT_HISTORY_NEW_LAYER:
			ScriptAddNewLayerRedo(window, data_pointers[i], data_size[i]);
			break;
		case SCRIPT_HISTORY_PIXEL_CHANGE:
			ScriptUpdatePixelsRedo(window, data_pointers[i], data_size[i]);
			break;
		case SCRIPT_HISTORY_VECTOR_CHANGE:
			ScriptUpdateVectorRedo(window, data_pointers[i], data_size[i]);
			break;
		}
	}

	ClearLayerView(&window->app->layer_window);
	LayerViewSetDrawWindow(&window->app->layer_window, window);

	window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	gtk_widget_queue_draw(window->window);

	MEM_FREE_FUNC(data_types);
	MEM_FREE_FUNC(data_size);
	MEM_FREE_FUNC(data_pointers);
}

/*****************************************
* AddScriptHistroy関数                   *
* スクリプト実行に対する履歴データの追加 *
* script	: スクリプト実行用の情報     *
*****************************************/
void AddScriptHistory(SCRIPT* script)
{
	// 操作された描画領域
	DRAW_WINDOW *window = script->app->draw_window[script->app->active_window];
	// 履歴データのサイズ
	guint32 data_size;
	// 4バイト書き出し用
	guint32 data32;

	// 有効な描画領域が無ければ終了
	if(script->app->window_num == 0)
	{
		return;
	}

	// 履歴データを作成
	data_size = (guint32)script->history.data_stream->data_size;
	(void)MemSeek(script->history.data_stream, 0, SEEK_SET);
	(void)MemWrite(&data_size, sizeof(data_size), 1, script->history.data_stream);
	data32 = script->history.num_history;
	(void)MemWrite(&data32, sizeof(data32), 1, script->history.data_stream);

	// 履歴データを追加
	AddHistory(&window->history, script->app->labels->menu.script, script->history.data_stream->buff_ptr,
		(uint32)data_size, ScriptUndo, ScriptRedo);
}

/*********************************************************
* ExecuteScript関数                                      *
* メニューからスクリプトが選択された時のコールバック関数 *
* 引数                                                   *
* menu_item	: メニューアイテムウィジェット               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
void ExecuteScript(GtkWidget* menu_item, APPLICATION* app)
{
	// 実行されたメニューのID
	int id = GPOINTER_TO_INT(g_object_get_data(
		G_OBJECT(menu_item), "script_id"));
	// IDからスクリプトファイルを決定してロード
	SCRIPT* script = CreateScript(app, app->scripts.file_paths[id]);

	if(script != NULL)
	{
		// スクリプト実行
		RunScript(script);

		// 履歴データを作成
		AddScriptHistory(script);

		// スクリプト用のデータを削除
		DeleteScript(&script);

		GetActiveDrawWindow(app)->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
	}
}

#ifdef __cplusplus
}
#endif
