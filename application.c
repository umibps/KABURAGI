/*
* application.c
* アプリケーションの初期化
* メニューの動作を定義
*/

// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "application.h"
#include "menu.h"
#include "labels.h"
#include "tool_box.h"
#include "input.h"
#include "display.h"
#include "save.h"
#include "utils.h"
#include "memory.h"
#include "image_read_write.h"
#include "ini_file.h"
#include "brushes.h"
#include "tlg.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _SPLASH_WINDOW
{
	APPLICATION *app;
	GtkWidget *window;
	GtkWidget *image;
	guint timer_id;
} SPLASH_WINDOW;

typedef struct _INITIALIZE_DATA
{
	uint8 fg_color[3], bg_color[3];
} INITIALIZE_DATA;

typedef struct _REALIZE_DATA
{
	guint signal_id;
	APPLICATION *app;
} REALIZE_DATA;

/*********************************************************
* GetActiveDrawWindow関数                                *
* アクティブな描画領域を取得する                         *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	アクティブな描画領域                                 *
*********************************************************/
DRAW_WINDOW* GetActiveDrawWindow(APPLICATION* app)
{
	DRAW_WINDOW *ret = app->draw_window[app->active_window];
	if(ret == NULL)
	{
		return NULL;
	}

	if(ret->focal_window != NULL)
	{
		return ret->focal_window;
	}
	return ret;
}

/*********************************************************
* SplashWindowTimeOut関数                                *
* 一定時間スプラッシュウィンドウを表示したら呼び出される *
* 引数                                                   *
* window	: スプラッシュウィンドウウィジェット         *
* 返り値                                                 *
*	FALSE(関数呼び出し終了)                              *
*********************************************************/
static int SplashWindowTimeOut(GtkWidget* window)
{
	gtk_widget_destroy(window);

	return FALSE;
}

/*********************************************************
* InitializeSplashWindow関数                             *
* スプラッシュウィンドウの初期化&表示                    *
* 引数                                                   *
* window		: スプラッシュウィンドウのデータアドレス *
*********************************************************/
void InitializeSplashWindow(SPLASH_WINDOW* window)
{
	// イメージ画像のパス(Mac対策)
	gchar *file_path;
	// ウィンドウに設定するタイトル
	char title[256];

	// ウィンドウ作成
	window->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	// ウィンドウタイトルを設定
	(void)sprintf(title, "Paint Soft %s", (GetHas3DLayer(window->app) == FALSE) ? "KABURAGI" : "MIKADO");
	gtk_window_set_title(GTK_WINDOW(window->window), title);
	// スプラッシュウィンドウへ
	gtk_window_set_type_hint(GTK_WINDOW(window->window), GDK_WINDOW_TYPE_HINT_SPLASHSCREEN);
	// イメージをロード
	file_path = g_build_filename(window->app->current_path, "image/splash.png", NULL);
	window->image = gtk_image_new_from_file(file_path);
	g_free(file_path);

	gtk_container_add(GTK_CONTAINER(window->window), window->image);

	// ウィンドウを表示
	gtk_widget_show_all(window->window);

	// 待機中のイベントを処理する
	while(gtk_events_pending() != FALSE)
	{
		gtk_main_iteration();
	}

	// 時間で消去
	window->timer_id = g_timeout_add(2000, (GSourceFunc)SplashWindowTimeOut, window->window);
}

typedef enum _eDROP_LIST
{
	DROP_URI = 10,
	NUM_DROP_LIST = 1
} eDROP_LIST;

/*********************************************************
* ReadInitializeFile関数                                 *
* 初期化ファイルを読み込む                               *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
* file_path	: 初期化ファイルのパス                       *
* 返り値                                                 *
*	正常終了:0	失敗:負の値                              *
*********************************************************/
int ReadInitializeFile(APPLICATION* app, const char* file_path, INITIALIZE_DATA* init_data)
{
	// 初期化ファイル解析用
	INI_FILE_PTR file;
	// ファイル読み込みストリーム
	GFile* fp = g_file_new_for_path(file_path);
	GFileInputStream* stream = g_file_read(fp, NULL, NULL);
	// ファイルサイズ取得用
	GFileInfo *file_info;
	// ファイルサイズ
	size_t data_size;
	// 文字列の一時保存
	char str[4096];
	// GLibのメモリの一時保存
	char *glib_str;
	// 色データ
	char color_string[128];
	uint32 color;

	if(stream == NULL)
	{
		return -1;
	}
	// ファイルサイズを取得
	file_info = g_file_input_stream_query_info(stream,
		G_FILE_ATTRIBUTE_STANDARD_SIZE, NULL, NULL);
	data_size = (size_t)g_file_info_get_size(file_info);
	// ファイル読み込み用
	file = CreateIniFile(stream,
		(size_t (*)(void*, size_t, size_t, void*))FileRead, data_size, INI_READ);

	// システムコードの読み込み
	app->system_code = IniFileStrdup(file, "SYSTEM_CODE", "CODE");
	// データファイル・ディレクトリパスの読み込み
		// 通常レイヤーのブラシ
	app->brush_file_path = IniFileStrdup(file, "BRUSH_DATA", "PATH");
	glib_str = g_convert(app->brush_file_path, -1, "UTF-8", app->system_code, NULL, NULL, NULL);
	MEM_FREE_FUNC(app->brush_file_path);
	app->brush_file_path = glib_str;
	// ベクトルレイヤーのブラシ
	app->vector_brush_file_path = IniFileStrdup(file, "VECTOR_BRUSH_DATA", "PATH");
	glib_str = g_convert(app->vector_brush_file_path, -1, "UTF-8", app->system_code, NULL, NULL, NULL);
	MEM_FREE_FUNC(app->vector_brush_file_path);
	app->vector_brush_file_path = glib_str;
	// 全種類のレイヤー共通で使えるツール
	app->common_tool_file_path = IniFileStrdup(file, "COMMON_TOOL_DATA", "PATH");
	glib_str = g_convert(app->common_tool_file_path, -1, "UTF-8", app->system_code, NULL, NULL, NULL);
	MEM_FREE_FUNC(app->common_tool_file_path);
	app->common_tool_file_path = glib_str;
	// 表示テキストのデータ
	app->language_file_path = IniFileStrdup(file, "LANGUAGE_FILE", "PATH");
	// 各データディレクトリ
	app->pattern_path = IniFileStrdup(file, "PATTERN_DIRECTORY", "PATH");
	app->stamp_path = IniFileStrdup(file, "STAMP_DIRECTORY", "PATH");
	app->texture_path = IniFileStrdup(file, "TEXTURE_DIRECTORY", "PATH");
	app->script_path = IniFileStrdup(file, "SCRIPT_DIRECTORY", "PATH");

	// ウィンドウの位置とサイズを読み込む
	app->window_x = IniFileGetInteger(file, "WINDOW", "X");
	app->window_y = IniFileGetInteger(file, "WINDOW", "Y");
	app->window_width = IniFileGetInteger(file, "WINDOW", "WIDTH");
	app->window_height = IniFileGetInteger(file, "WINDOW", "HEIGHT");
	app->left_pane_position = IniFileGetInteger(file, "WINDOW", "LEFT_PANE");
	app->right_pane_position = IniFileGetInteger(file, "WINDOW", "RIGHT_PANE");
	if(IniFileGetInteger(file, "WINDOW", "FULLSCREEN") != 0)
	{
		app->flags |= APPLICATION_FULL_SCREEN;
	}
	if(IniFileGetInteger(file, "WINDOW", "MAXIMIZE") != 0)
	{
		app->flags |= APPLICATION_WINDOW_MAXIMIZE;
	}

	// RCファイルのパスを読み込む
	app->preference.theme = IniFileStrdup(file, "WINDOW", "RC_FILE");
	if(app->preference.theme != NULL)
	{
		gchar* file_path;
		if(app->preference.theme[0] == '.'
			&& app->preference.theme[1] == '/')
		{
			file_path = g_build_filename(app->current_path,
				&app->preference.theme[2], NULL);
			gtk_rc_parse(file_path);
		}
		else
		{
			gtk_rc_parse(app->preference.theme);
		}
	}

	// ツールボックスウィンドウの位置とサイズを読み込む
	app->tool_window.window_x = IniFileGetInteger(file, "TOOL_BOX", "X");
	app->tool_window.window_y = IniFileGetInteger(file, "TOOL_BOX", "Y");
	app->tool_window.window_width = IniFileGetInteger(file, "TOOL_BOX", "WIDTH");
	app->tool_window.window_height = IniFileGetInteger(file, "TOOL_BOX", "HEIGHT");
	app->tool_window.place = IniFileGetInteger(file, "TOOL_BOX", "PLACE");
	app->tool_window.pane_position = IniFileGetInteger(file, "TOOL_BOX", "PANE");
	// 色データを読み込む
	{
		uint8 *color_buff = (uint8*)&color;

		if(IniFileGetString(file, "TOOL_BOX", "FORE_GROUND_COLOR", color_string, 128) != 0)
		{
			(void)sscanf(color_string, "%x", &color);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			init_data->fg_color[0] = color_buff[2];
			init_data->fg_color[1] = color_buff[1];
			init_data->fg_color[2] = color_buff[0];
#else
			init_data->fg_color[0] = color_buff[0];
			init_data->fg_color[1] = color_buff[1];
			init_data->fg_color[2] = color_buff[2];
#endif
		}
		else
		{
			init_data->fg_color[0] = init_data->fg_color[1]
				= init_data->fg_color[2] = 0xff;
		}

		if(IniFileGetString(file, "TOOL_BOX", "BACK_GROUND_COLOR", color_string, 128) != 0)
		{
			(void)sscanf(color_string, "%x", &color);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			init_data->bg_color[0] = color_buff[2];
			init_data->bg_color[1] = color_buff[1];
			init_data->bg_color[2] = color_buff[0];
#else
			init_data->bg_color[0] = color_buff[0];
			init_data->bg_color[1] = color_buff[1];
			init_data->bg_color[2] = color_buff[2];
#endif
		}
		else
		{
			init_data->bg_color[0] = init_data->bg_color[1]
				= init_data->bg_color[2] = 0xff;
		}
	}

	if(IniFileGetInteger(file, "TOOL_BOX", "SHOW_COLOR_CIRCLE") != 0)
	{
		app->tool_window.flags |= TOOL_SHOW_COLOR_CIRCLE;
	}
	if(IniFileGetInteger(file, "TOOL_BOX", "SHOW_COLOR_PALLETE") != 0)
	{
		app->tool_window.flags |= TOOL_SHOW_COLOR_PALLETE;
	}

	switch(app->tool_window.place)
	{
	case UTILITY_PLACE_WINDOW:
		break;
	case UTILITY_PLACE_LEFT:
		app->tool_window.flags |= TOOL_DOCKED;
		break;
	case UTILITY_PLACE_RIGHT:
		app->tool_window.flags |= TOOL_PLACE_RIGHT | TOOL_DOCKED;
		break;
	}

	// レイヤービューウィンドウの位置とサイズを読み込む
	app->layer_window.window_x = IniFileGetInteger(file, "LAYER_VIEW", "X");
	app->layer_window.window_y = IniFileGetInteger(file, "LAYER_VIEW", "Y");
	app->layer_window.window_width = IniFileGetInteger(file, "LAYER_VIEW", "WIDTH");
	app->layer_window.window_height = IniFileGetInteger(file, "LAYER_VIEW", "HEIGHT");
	app->layer_window.place = IniFileGetInteger(file, "LAYER_VIEW", "PLACE");
	app->layer_window.pane_position = IniFileGetInteger(file, "LAYER_VIEW", "POSITION");

	switch(app->layer_window.place)
	{
	case UTILITY_PLACE_WINDOW:
		break;
	case UTILITY_PLACE_LEFT:
		app->layer_window.flags |= LAYER_WINDOW_DOCKED;
		break;
	case UTILITY_PLACE_RIGHT:
		app->layer_window.flags |= LAYER_WINDOW_DOCKED | LAYER_WINDOW_PLACE_RIGHT;
		break;
	}

	// ナビゲーションウィンドウの位置とサイズを読み込む
	app->navigation_window.window_x = IniFileGetInteger(file, "NAVIGATION", "X");
	app->navigation_window.window_y = IniFileGetInteger(file, "NAVIGATION", "Y");
	app->navigation_window.window_width = IniFileGetInteger(file, "NAVIGATION", "WIDTH");
	app->navigation_window.window_height = IniFileGetInteger(file, "NAVIGATION", "HEIGHT");

	// プレビューウィンドウの位置とサイズを読み込む
	app->preview_window.window_x = IniFileGetInteger(file, "PREVIEW", "X");
	app->preview_window.window_y = IniFileGetInteger(file, "PREVIEW", "Y");
	app->preview_window.window_width = IniFileGetInteger(file, "PREVIEW", "WIDTH");
	app->preview_window.window_height = IniFileGetInteger(file, "PREVIEW", "HEIGHT");

	// 参考用画像表示ウィンドウの位置とサイズを読み込む
	app->reference_window.window_x = IniFileGetInteger(file, "REFERENCE", "X");
	app->reference_window.window_y = IniFileGetInteger(file, "REFERENCE", "Y");
	app->reference_window.window_width = IniFileGetInteger(file, "REFERENCE", "WIDTH");
	app->reference_window.window_height = IniFileGetInteger(file, "REFERENCE", "HEIGHT");

	// メニューデータを読み込む
		// 新規作成
	app->menu_data.make_new.width = IniFileGetInteger(file, "NEW", "WIDTH");
	app->menu_data.make_new.height = IniFileGetInteger(file, "NEW", "HEIGHT");
	app->menu_data.make_new.resolution = IniFileGetInteger(file, "NEW", "RESOLUTION");
	if(IniFileGetString(file, "NEW", "SECOND_BG_COLOR", color_string, sizeof(color_string)) != 0)
	{
		uint8 *color_buff = (uint8*)&color;
		(void)sscanf(color_string, "%x", &color);

#if 1 // defined(BIG_ENDIAN)
		app->menu_data.make_new.second_bg_color[0] = color_buff[2];
		app->menu_data.make_new.second_bg_color[1] = color_buff[1];
		app->menu_data.make_new.second_bg_color[2] = color_buff[0];
#else
		app->menu_data.make_new.second_bg_color[0] = color_buff[0];
		app->menu_data.make_new.second_bg_color[1] = color_buff[1];
		app->menu_data.make_new.second_bg_color[2] = color_buff[2];
#endif
	}

	// 手ブレ補正
	app->tool_window.smoother.num_use = IniFileGetInteger(file, "SMOOTH", "LEVEL");
	app->tool_window.smoother.rate = IniFileGetInteger(file, "SMOOTH", "RATE");
	app->tool_window.smoother.mode = IniFileGetInteger(file, "SMOOTH", "MODE");

	// テクスチャの設定を読み込む
	app->textures.strength = IniFileGetDouble(file, "TEXTURE", "STRENGTH");
	app->textures.scale = IniFileGetDouble(file, "TEXTURE", "SCALE");

	// 環境設定を読み込む
	ReadPreference(file, &app->preference);
	// タッチイベントの処理方法を読み込む
	if(IniFileGetInteger(file, "PREFERENCE", "DRAW_WITH_TOUCH") != 0)
	{
		app->flags |= APPLICATION_DRAW_WITH_TOUCH;
	}
	// 背景色を変更するかを読み込む
	if(IniFileGetInteger(file, "PREFERENCE", "CHANGE_BACK_GROUND") != 0)
	{
		app->flags |= APPLICATION_SET_BACK_GROUND_COLOR;
	}

	// プレビューウィンドウをタスクバーに表示するかを読み込む
	if(IniFileGetInteger(file, "PREFERENCE", "SHOW_PREVIEW_WINDOW_ON_TASKBAR") != 0)
	{
		app->flags |= APPLICATION_SHOW_PREVIEW_ON_TASK_BAR;
	}

	// 使用するICCプロファイルを読み込む
	{
		char *temp_path;
		cmsHPROFILE monitor_icc;

		temp_path = IniFileStrdup(file, "ICC_PROFILE", "INPUT");
		app->input_icc_path = g_strdup(temp_path);
		if(app->input_icc_path != NULL)
		{
			if(*app->input_icc_path == '\0')
			{
				//app->input_icc = CreateDefaultSrgbProfile();
				app->input_icc = GetPrimaryMonitorProfile();
				g_free(app->input_icc_path);
				app->input_icc_path = NULL;
			}
			else
			{
				app->input_icc = cmsOpenProfileFromFile(app->input_icc_path, "r");
			}
		}
		else
		{
			//app->input_icc = CreateDefaultSrgbProfile();
			app->input_icc = GetPrimaryMonitorProfile();
		}
		MEM_FREE_FUNC(temp_path);

		temp_path = IniFileStrdup(file, "ICC_PROFILE", "OUTPUT");
		app->output_icc_path = g_strdup(temp_path);
		if(app->output_icc_path != NULL)
		{
			app->output_icc = cmsOpenProfileFromFile(app->output_icc_path, "r");
		}

		MEM_FREE_FUNC(temp_path);

		monitor_icc = GetPrimaryMonitorProfile();

		if(app->output_icc != NULL)
		{
			cmsBool bpc[] = { TRUE, TRUE, TRUE, TRUE };
			cmsHPROFILE hProfiles[] = { app->input_icc, app->output_icc, app->output_icc, monitor_icc };
			cmsUInt32Number intents[] = { INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC };
			cmsFloat64Number adaptationStates[] = { 0, 0, 0, 0 };

			app->icc_transform = cmsCreateExtendedTransform(cmsGetProfileContextID(hProfiles[1]), 4, hProfiles,
				bpc, intents, adaptationStates,
				NULL, 0, TYPE_BGRA_8, TYPE_BGRA_8, 0 /*cmsFLAGS_SOFTPROOFING*/);
		}
		else
		{
			app->icc_transform = cmsCreateTransform(app->input_icc, TYPE_BGRA_8,
				monitor_icc, TYPE_BGRA_8, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_BLACKPOINTCOMPENSATION);
		}

		cmsCloseProfile(monitor_icc);
	}

	// バックアップを作成するディレクトリを読み込む
	if(IniFileGetString(file, "BACKUP", "DIRECTORY", str, sizeof(str)) != 0)
	{
		app->backup_directory_path = g_convert(str, -1, "UTF-8", app->system_code, NULL, NULL, NULL);
	}
	else
	{
		app->backup_directory_path = g_strdup("./");
	}

	// ファイル読み込みデータを破棄
	file->delete_func(file);

	// 読み込み用のオブジェクトを削除
	g_object_unref(fp);
	g_object_unref(stream);
	g_object_unref(file_info);

	return 0;
}

/*********************************************************
* WriteInitializeFile関数                                *
* 初期化ファイルを書き込む                               *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
* file_path	: 初期化ファイルのパス                       *
* 返り値                                                 *
*	正常終了:0	失敗:負の値                              *
*********************************************************/
int WriteInitializeFile(APPLICATION* app, const char* file_path)
{
	GFile* fp = g_file_new_for_path(file_path);
	GFileOutputStream* stream =
		g_file_create(fp, G_FILE_CREATE_NONE, NULL, NULL);
	INI_FILE_PTR file;
	int32 color;
	char str[128];
	char *write_path;
	
	// ファイルオープンに失敗したら上書きを試す
	if(stream == NULL)
	{
		stream = g_file_replace(fp, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);

		if(stream == NULL)
		{
			g_object_unref(fp);
			return -1;
		}
	}

	file = CreateIniFile(stream, NULL,0, INI_WRITE);

	// システムコードを書き込む
	(void)IniFileAddString(file, "SYSTEM_CODE", "CODE", app->system_code);

	// データファイル・ディレクトリパスを書き込む
		// 通常レイヤーのブラシ
	write_path = g_convert(app->brush_file_path, -1, app->system_code, "UTF-8", NULL, NULL, NULL);
	(void)IniFileAddString(file, "BRUSH_DATA", "PATH", write_path);
	g_free(write_path);
	// ベクトルレイヤーのブラシ
	write_path = g_convert(app->vector_brush_file_path, -1, app->system_code, "UTF-8", NULL, NULL, NULL);
	(void)IniFileAddString(file, "VECTOR_BRUSH_DATA", "PATH", write_path);
	g_free(write_path);
	// 全種類のレイヤーで使えるツール
	write_path = g_convert(app->common_tool_file_path, -1, app->system_code, "UTF-8", NULL, NULL, NULL);
	(void)IniFileAddString(file, "COMMON_TOOL_DATA", "PATH", write_path);
	g_free(write_path);
	// 表示文字のデータ
	(void)IniFileAddString(file, "LANGUAGE_FILE", "PATH", app->language_file_path);
	// 各種データディレクトリ
	(void)IniFileAddString(file, "PATTERN_DIRECTORY", "PATH", app->pattern_path);
	(void)IniFileAddString(file, "STAMP_DIRECTORY", "PATH", app->stamp_path);
	(void)IniFileAddString(file, "TEXTURE_DIRECTORY", "PATH", app->texture_path);
	(void)IniFileAddString(file, "SCRIPT_DIRECTORY", "PATH", app->script_path);

	// ウィンドウの位置とサイズを書き込む
	{
		gint x, y, width, height;
		if((app->flags & (APPLICATION_FULL_SCREEN | APPLICATION_WINDOW_MAXIMIZE)) == 0)
		{
			gtk_window_get_position(GTK_WINDOW(app->window), &x, &y);
			gtk_window_get_size(GTK_WINDOW(app->window), &width, &height);
		}
		else
		{
			x = app->window_x,	y = app->window_y;
			width = app->window_width,	height = app->window_height;
		}
		(void)IniFileAddInteger(file, "WINDOW", "X", x, 10);
		(void)IniFileAddInteger(file, "WINDOW", "Y", y, 10);
		(void)IniFileAddInteger(file, "WINDOW", "WIDTH", width, 10);
		(void)IniFileAddInteger(file, "WINDOW", "HEIGHT", height, 10);
		(void)IniFileAddInteger(file, "WINDOW", "FULLSCREEN",
			((app->flags & APPLICATION_FULL_SCREEN) != 0) ? 1 : 0, 10);
		(void)IniFileAddInteger(file, "WINDOW", "MAXIMIZE",
			((app->flags & APPLICATION_WINDOW_MAXIMIZE) != 0) ? 1 : 0, 10);
		if(app->preference.theme != NULL)
		{
			(void)IniFileAddString(file, "WINDOW", "RC_FILE", app->preference.theme);
		}

		if(gtk_paned_get_child1(GTK_PANED(app->left_pane)) != NULL)
		{
			app->left_pane_position = gtk_paned_get_position(GTK_PANED(app->left_pane));
		}
		if(gtk_paned_get_child2(GTK_PANED(app->right_pane)) != NULL)
		{
			app->right_pane_position = gtk_paned_get_position(GTK_PANED(app->right_pane));
		}
		(void)IniFileAddInteger(file, "WINDOW", "LEFT_PANE", app->left_pane_position, 10);
		(void)IniFileAddInteger(file, "WINDOW", "RIGHT_PANE", app->right_pane_position, 10);

		{
			if(app->tool_window.window != NULL)
			{
				gtk_window_get_position(GTK_WINDOW(app->tool_window.window), &x, &y);
				gtk_window_get_size(GTK_WINDOW(app->tool_window.window), &width, &height);
			}
			else
			{
				x = app->tool_window.window_x, y = app->tool_window.window_y;
				width = app->tool_window.window_width, height = app->tool_window.window_height;
			}

			if(app->tool_window.ui != NULL)
			{
				app->tool_window.pane_position =
					gtk_paned_get_position(GTK_PANED(app->tool_window.pane));
			}

			(void)IniFileAddInteger(file, "TOOL_BOX", "X", x, 10);
			(void)IniFileAddInteger(file, "TOOL_BOX", "Y", y, 10);
			(void)IniFileAddInteger(file, "TOOL_BOX", "WIDTH", width, 10);
			(void)IniFileAddInteger(file, "TOOL_BOX", "HEIGHT", height, 10);
			(void)IniFileAddInteger(file, "TOOL_BOX", "PLACE", app->tool_window.place, 10);
			(void)IniFileAddInteger(file, "TOOL_BOX", "PANE", app->tool_window.pane_position, 10);

#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
			color = app->tool_window.color_chooser->rgb[0] | (app->tool_window.color_chooser->rgb[1] << 8)
				| (app->tool_window.color_chooser->rgb[2] << 16);
#else
			color = app->tool_window.color_chooser->rgb[2] | (app->tool_window.color_chooser->rgb[1] << 8)
				| (app->tool_window.color_chooser->rgb[0] << 16);
#endif
			(void)sprintf(str, "%06x", color);
			(void)IniFileAddString(file, "TOOL_BOX", "FORE_GROUND_COLOR", str);

#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
			color = app->tool_window.color_chooser->back_rgb[0] | (app->tool_window.color_chooser->back_rgb[1] << 8)
				| (app->tool_window.color_chooser->back_rgb[2] << 16);
#else
			color = app->tool_window.color_chooser->back_rgb[2] | (app->tool_window.color_chooser->back_rgb[1] << 8)
				| (app->tool_window.color_chooser->back_rgb[0] << 16);
#endif
			(void)sprintf(str, "%06x", color);
			(void)IniFileAddString(file, "TOOL_BOX", "BACK_GROUND_COLOR", str);

			(void)IniFileAddInteger(file, "TOOL_BOX", "SHOW_COLOR_CIRCLE",
				((app->tool_window.color_chooser->flags & COLOR_CHOOSER_SHOW_CIRCLE) != 0) ? 1 : 0, 10);
			(void)IniFileAddInteger(file, "TOOL_BOX", "SHOW_COLOR_PALLETE",
				((app->tool_window.color_chooser->flags & COLOR_CHOOSER_SHOW_PALLETE) != 0) ? 1 : 0, 10);
		}

		if(app->layer_window.window != NULL)
		{
			gtk_window_get_position(GTK_WINDOW(app->layer_window.window), &x, &y);
			gtk_window_get_size(GTK_WINDOW(app->layer_window.window), &width, &height);
		}
		else
		{
			x = app->layer_window.window_x, y = app->layer_window.window_y;
			width = app->layer_window.window_width, height = app->layer_window.window_height;
		}

		(void)IniFileAddInteger(file, "LAYER_VIEW", "X", x, 10);
		(void)IniFileAddInteger(file, "LAYER_VIEW", "Y", y, 10);
		(void)IniFileAddInteger(file, "LAYER_VIEW", "WIDTH", width, 10);
		(void)IniFileAddInteger(file, "LAYER_VIEW", "HEIGHT", height, 10);
		(void)IniFileAddInteger(file, "LAYER_VIEW", "PLACE", app->layer_window.place, 10);
		if(app->navi_layer_pane != NULL)
		{
			(void)IniFileAddInteger(file, "LAYER_VIEW", "POSITION",
				gtk_paned_get_position(GTK_PANED(app->navi_layer_pane)), 10);
		}
		else
		{
			(void)IniFileAddInteger(file, "LAYER_VIEW", "POSITION",
				app->layer_window.pane_position, 10);
		}

		if(app->navigation_window.window != NULL)
		{
			gtk_window_get_position(GTK_WINDOW(app->navigation_window.window), &x, &y);
			gtk_window_get_size(GTK_WINDOW(app->navigation_window.window), &width, &height);
		}
		else
		{
			x = app->navigation_window.window_x, y = app->navigation_window.window_y;
			width = app->navigation_window.window_width, y = app->navigation_window.window_height;
		}

		(void)IniFileAddInteger(file, "NAVIGATION", "X", x, 10);
		(void)IniFileAddInteger(file, "NAVIGATION", "Y", y, 10);
		(void)IniFileAddInteger(file, "NAVIGATION", "WIDTH", width, 10);
		(void)IniFileAddInteger(file, "NAVIGATION", "HEIGHT", height, 10);

		if(app->preview_window.window != NULL)
		{
			gtk_window_get_position(GTK_WINDOW(app->preview_window.window), &x, &y);
			gtk_window_get_size(GTK_WINDOW(app->preview_window.window), &width, &height);
			(void)IniFileAddInteger(file, "PREVIEW", "X", x, 10);
			(void)IniFileAddInteger(file, "PREVIEW", "Y", y, 10);
			(void)IniFileAddInteger(file, "PREVIEW", "WIDTH", width, 10);
			(void)IniFileAddInteger(file, "PREVIEW", "HEIGHT", height, 10);
		}
	}

	if(app->reference_window.data != NULL)
	{
		gtk_window_get_position(GTK_WINDOW(app->reference_window.data->window),
			&app->reference_window.window_x, &app->reference_window.window_y);
		gtk_window_get_size(GTK_WINDOW(app->reference_window.data->window),
			&app->reference_window.window_width, &app->reference_window.window_height);
	}
	(void)IniFileAddInteger(file, "REFERENCE", "X", app->reference_window.window_x, 10);
	(void)IniFileAddInteger(file, "REFERENCE", "Y", app->reference_window.window_y, 10);
	(void)IniFileAddInteger(file, "REFERENCE", "WIDTH", app->reference_window.window_width, 10);
	(void)IniFileAddInteger(file, "REFERENCE", "HEIGHT", app->reference_window.window_height, 10);

	// メニューデータを書き込む
		// 新規作成
	(void)IniFileAddInteger(file, "NEW", "WIDTH", app->menu_data.make_new.width, 10);
	(void)IniFileAddInteger(file, "NEW", "HEIGHT", app->menu_data.make_new.height, 10);
	(void)IniFileAddInteger(file, "NEW", "RESOLUTION", app->menu_data.make_new.resolution, 10);
	color = app->menu_data.make_new.second_bg_color[2] | (app->menu_data.make_new.second_bg_color[1] << 8)
		| (app->menu_data.make_new.second_bg_color[0] << 16);
	(void)sprintf(str, "%06x", color);
	(void)IniFileAddString(file, "NEW", "SECOND_BG_COLOR", str);
	// 手ブレ補正
	(void)IniFileAddInteger(file, "SMOOTH", "LEVEL", app->tool_window.smoother.num_use, 10);
	(void)IniFileAddInteger(file, "SMOOTH", "RATE", (int)app->tool_window.smoother.rate, 10);
	(void)IniFileAddInteger(file, "SMOOTH", "MODE", app->tool_window.smoother.mode, 10);
	// テクスチャ
	(void)IniFileAddDouble(file, "TEXTURE", "STRENGTH", app->textures.strength);
	(void)IniFileAddDouble(file, "TEXTURE", "SCALE", app->textures.scale);

	// 環境設定を書き込む
	WritePreference(file, &app->preference);
	// タッチイベントの処理方法を書き込む
	(void)IniFileAddInteger(file, "PREFERENCE", "DRAW_WITH_TOUCH",
		(app->flags & APPLICATION_DRAW_WITH_TOUCH) != 0 ? 1 : 0, 10);
	// 背景色を変更するかを書き込む
	(void)IniFileAddInteger(file, "PREFERENCE", "CHANGE_BACK_GROUND",
		(app->flags & APPLICATION_SET_BACK_GROUND_COLOR) != 0 ? 1 : 0, 10);
	// プレビューウィンドウをタスクバーに表示するかを書き込む
	(void)IniFileAddInteger(file, "PREFERENCE", "SHOW_PREVIEW_WINDOW_ON_TASKBAR",
		(app->flags & APPLICATION_SHOW_PREVIEW_ON_TASK_BAR) != 0 ? 1 : 0, 10);

	// 使用するICCプロファイルを書き込む
	(void)IniFileAddString(file, "ICC_PROFILE", "INPUT", app->input_icc_path);
	(void)IniFileAddString(file, "ICC_PROFILE", "OUTPUT", app->output_icc_path);

	// バックアップを作成するディレクトリを書き込む
	{
		gchar *path;
		path = g_convert(app->backup_directory_path, -1, app->system_code, "UTF-8", NULL, NULL, NULL);
		(void)IniFileAddString(file, "BACKUP", "DIRECTORY", path);
		g_free(path);
	}

	// 作成したデータをファイルに書き込む
	(void)WriteIniFile(file, (size_t (*)(void*, size_t, size_t, void*))FileWrite);

	// データを開放
	file->delete_func(file);

	// 書き出し用のオブジェクトを削除
	g_object_unref(fp);
	g_object_unref(stream);

	return 0;
}

/*******************************************************
* OnQuitApplication関数                                *
* アプリケーション終了前に呼び出されるコールバック関数 *
* 引数                                                 *
* app	: アプリケーションを管理する構造体のアドレス   *
* 返り値                                               *
*	終了中止:TRUE	終了続行:FALSE                     *
*******************************************************/
gboolean OnQuitApplication(APPLICATION* app)
{
	// 自動保存ファイル削除用
	GDir *dir;
	// フォルダへのパス(AppData作成用)
	gchar *dir_path;
	// ファイルへのパス(Mac対策)
	gchar *file_path;
	// アプリケーションデータディレクトリにデータ作成するフラグ
	gboolean make_app_dir = FALSE;
	// for文用のカウンタ
	int i;

	for(i=0; i<app->window_num; i++)
	{
		if((app->draw_window[i]->history.flags & HISTORY_UPDATED) != 0)
		{	// 保存するかどうかのダイアログを表示
			GtkWidget* dialog = gtk_dialog_new_with_buttons(
				NULL,
				GTK_WINDOW(app->window),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_SAVE,
				GTK_RESPONSE_ACCEPT,
				GTK_STOCK_NO,
				GTK_RESPONSE_NO,
				GTK_STOCK_CANCEL,
				GTK_RESPONSE_REJECT,
				NULL
			);
			// ダイアログに入れるラベル
			GtkWidget* label = gtk_label_new(app->labels->save.quit_with_unsaved_change);
			// ダイアログの選択結果
			gint result;

			// ダイアログにラベルを入れる
			gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(
				GTK_DIALOG(dialog))), label, FALSE, FALSE, 0);
			gtk_widget_show(label);

			// ダイアログを実行
			result = gtk_dialog_run(GTK_DIALOG(dialog));

			// ダイアログを閉じる
			gtk_widget_destroy(dialog);

			// ダイアログの選択によって処理を切り替え
			switch(result)
			{
			case GTK_RESPONSE_ACCEPT:	// 保存実行
				{
					int16 before_id = app->active_window;
					app->active_window = (int16)i;
					ExecuteSave(app);
					app->active_window = before_id;
				}
				break;
			case GTK_RESPONSE_REJECT:	// キャンセル
				// 閉じる操作を止める
				return TRUE;
			}
		}
	}

	// ブラシファイルを更新する
	if(app->common_tool_file_path[0] == '.' && app->common_tool_file_path[1] == '/')
	{
		file_path = g_build_filename(app->current_path, &app->common_tool_file_path[2], NULL);
		if(WriteCommonToolData(&app->tool_window, file_path, app) < 0)
		{
			g_free(file_path);
			dir_path = g_build_filename(g_get_user_config_dir(), "KABURAGI", NULL);
			make_app_dir = TRUE;
			if((dir = g_dir_open(dir_path, 0, NULL)) == NULL)
			{
				(void)g_mkdir(dir_path,
#ifdef __MAC_OS__
					S_IRUSR | S_IWUSR | S_IXUSR |
					S_IRGRP | S_IWGRP | S_IXGRP |
					S_IROTH | S_IXOTH | S_IXOTH
#elif defined(_WIN32)
					0
#else
					S_IRUSR | S_IWUSR | S_IXUSR |
					S_IRGRP | S_IWGRP | S_IXGRP |
					S_IROTH | S_IXOTH | S_IXOTH
#endif
				);
			}
			else
			{
				g_dir_close(dir);
			}

			file_path = g_build_filename(dir_path, "common_tools.ini", NULL);
			if(WriteCommonToolData(&app->tool_window, file_path, app) == 0)
			{
				g_free(app->common_tool_file_path);
				app->common_tool_file_path = file_path;
			}
			else
			{
				g_free(file_path);
				file_path = g_build_filename(app->current_path, &app->common_tool_file_path[2], NULL);
				(void)WriteCommonToolData(&app->tool_window, file_path, app);
				g_free(file_path);
			}
			g_free(dir_path);
		}
		else
		{
			g_free(file_path);
		}
	}
	else
	{
		(void)WriteCommonToolData(&app->tool_window, app->common_tool_file_path, app);
	}

	if(app->brush_file_path[0] == '.' && app->brush_file_path[1] == '/')
	{
		file_path = g_build_filename(app->current_path, &app->brush_file_path[2], NULL);
		if(WriteBrushDetailData(&app->tool_window, file_path, app) < 0)
		{
			g_free(file_path);
			dir_path = g_build_filename(g_get_user_config_dir(), "KABURAGI", NULL);
			make_app_dir = TRUE;
			if((dir = g_dir_open(dir_path, 0, NULL)) == NULL)
			{
				(void)g_mkdir(dir_path,
#ifdef __MAC_OS__
					S_IRUSR | S_IWUSR | S_IXUSR |
					S_IRGRP | S_IWGRP | S_IXGRP |
					S_IROTH | S_IXOTH | S_IXOTH
#elif defined(_WIN32)
					0
#else
					S_IRUSR | S_IWUSR | S_IXUSR |
					S_IRGRP | S_IWGRP | S_IXGRP |
					S_IROTH | S_IXOTH | S_IXOTH
#endif
				);
			}
			else
			{
				g_dir_close(dir);
			}

			file_path = g_build_filename(dir_path, "brushes.ini", NULL);
			if(WriteBrushDetailData(&app->tool_window, file_path, app) == 0)
			{
				g_free(app->brush_file_path);
				app->brush_file_path = file_path;
			}
			else
			{
				g_free(file_path);
				file_path = g_build_filename(app->current_path, &app->brush_file_path[2], NULL);
				(void)WriteBrushDetailData(&app->tool_window, file_path, app);
				g_free(file_path);
			}
			g_free(dir_path);
		}
		else
		{
			g_free(file_path);
		}
	}
	else
	{
		(void)WriteBrushDetailData(&app->tool_window, app->brush_file_path, app);
	}

	if(app->vector_brush_file_path[0] == '.' && app->vector_brush_file_path[1] == '/')
	{
		file_path = g_build_filename(app->current_path, &app->vector_brush_file_path[2], NULL);
		if(WriteVectorBrushData(&app->tool_window, file_path, app) < 0)
		{
			g_free(file_path);
			dir_path = g_build_filename(g_get_user_config_dir(), "KABURAGI", NULL);
			make_app_dir = TRUE;
			if((dir = g_dir_open(dir_path, 0, NULL)) == NULL)
			{
				(void)g_mkdir(dir_path,
#ifdef __MAC_OS__
					S_IRUSR | S_IWUSR | S_IXUSR |
					S_IRGRP | S_IWGRP | S_IXGRP |
					S_IROTH | S_IXOTH | S_IXOTH
#elif defined(_WIN32)
					0
#else
					S_IRUSR | S_IWUSR | S_IXUSR |
					S_IRGRP | S_IWGRP | S_IXGRP |
					S_IROTH | S_IXOTH | S_IXOTH
#endif
				);
			}
			else
			{
				g_dir_close(dir);
			}

			file_path = g_build_filename(dir_path, "vector_brushes.ini", NULL);
			if(WriteVectorBrushData(&app->tool_window, file_path, app) == 0)
			{
				g_free(app->vector_brush_file_path);
				app->vector_brush_file_path = file_path;
			}
			else
			{
				g_free(file_path);
				file_path = g_build_filename(app->current_path, &app->vector_brush_file_path[2], NULL);
				(void)WriteVectorBrushData(&app->tool_window, file_path, app);
				g_free(file_path);
			}
			g_free(dir_path);
		}
		else
		{
			g_free(file_path);
		}
	}
	else
	{
		(void)WriteVectorBrushData(&app->tool_window, app->vector_brush_file_path, app);
	}

	// 初期化ファイルを更新する
	if(make_app_dir == FALSE)
	{
		dir_path = g_strdup(app->current_path);
	}
	else
	{
		dir_path = g_build_filename(g_get_user_config_dir(), "KABURAGI", NULL);

		if((dir = g_dir_open(dir_path, 0, NULL)) == NULL)
		{
			(void)g_mkdir(dir_path,
#ifdef __MAC_OS__
				S_IRUSR | S_IWUSR | S_IXUSR |
				S_IRGRP | S_IWGRP | S_IXGRP |
				S_IROTH | S_IXOTH | S_IXOTH
#elif defined(_WIN32)
				0
#else
				S_IRUSR | S_IWUSR | S_IXUSR |
				S_IRGRP | S_IWGRP | S_IXGRP |
				S_IROTH | S_IXOTH | S_IXOTH
#endif
			);
		}
		else
		{
			g_dir_close(dir);
		}
	}

	file_path = g_build_filename(dir_path, INITIALIZE_FILE_NAME, NULL);
	if(WriteInitializeFile(app, file_path) != 0)
	{
		g_free(file_path);
		file_path = g_build_filename(app->current_path, INITIALIZE_FILE_NAME, NULL);
		(void)WriteInitializeFile(app, file_path);
	}
	g_free(file_path);

	// パレットファイルを更新する
	file_path = g_build_filename(dir_path, PALLETE_FILE_NAME, NULL);
	if(WritePalleteFile(app->tool_window.color_chooser, file_path) != 0)
	{
		g_free(file_path);
		file_path = g_build_filename(app->current_path, PALLETE_FILE_NAME, NULL);
		WritePalleteFile(app->tool_window.color_chooser, file_path);
	}
	g_free(file_path);

	g_free(dir_path);

	// 自動保存ファイルを削除
	dir = g_dir_open(app->backup_directory_path, 0, NULL);
	if(dir != NULL)
	{
		gchar *file_name;
		size_t name_length;

		while((file_name = (gchar*)g_dir_read_name(dir)) != NULL)
		{
			name_length = strlen(file_name);
			if(name_length >= 4)
			{
				if(StringCompareIgnoreCase(&file_name[name_length-4], ".kbt") == 0)
				{
					char *system_path;
					file_path = g_build_filename(app->current_path, file_name, NULL);
					system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
					(void)remove(system_path);
					g_free(file_path);
					g_free(system_path);
				}
			}
		}

		g_dir_close(dir);
	}

	// アプリケーション終了
	gtk_main_quit();

	return FALSE;
}

/*********************************************************
* OnCloseMainWindow関数                                  *
* メインウィンドウが閉じられるときのコールバック関数     *
* 引数                                                   *
* window	: ウィンドウウィジェット                     *
* event		: ウィジェット削除のイベント情報             *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	終了中止:TRUE	終了続行:FALSE                       *
*********************************************************/
static gboolean OnCloseMainWindow(
	GtkWindow* window,
	GdkEvent* event,
	APPLICATION* app
)
{
	return OnQuitApplication(app);
}

/*********************************************************
* OnChangeMainWindowState関数                            *
* ウィンドウのフルスクリーン・最大化が切り替わった時に   *
*   呼び出されるコールバック関数                         *
* window                                                 *
* state                                                  *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	常にFALSE                                            *
*********************************************************/
static gboolean OnChangeMainWindowState(
	GtkWindow* window,
	GdkEventWindowState* state,
	APPLICATION* app
)
{
	if((state->new_window_state & GDK_WINDOW_STATE_FULLSCREEN) != 0)
	{
		app->flags |= APPLICATION_FULL_SCREEN;
	}
	else
	{
		app->flags &= ~(APPLICATION_FULL_SCREEN);
	}

	if((state->new_window_state & GDK_WINDOW_STATE_MAXIMIZED) != 0)
	{
		app->flags |= APPLICATION_WINDOW_MAXIMIZE;
	}
	else
	{
		app->flags &= ~(APPLICATION_WINDOW_MAXIMIZE);
	}

	return FALSE;
}

/*************************************************************
* Move2ActiveLayer                                           *
* レイヤービューをアクティブなレイヤーにスクロール           *
* widget		: アクティブレイヤーのウィジェット           *
* allocation	: ウィジェットに割り当てられたサイズ         *
* app			: アプリケーションを管理する構造体のアドレス *
*************************************************************/
void Move2ActiveLayer(GtkWidget* widget, GdkRectangle* allocation, APPLICATION* app)
{
	GtkAllocation place;
	LAYER *top, *layer;

	top = app->draw_window[app->active_window]->layer;
	while(top->next != NULL)
	{
		top = top->next;
	}
	layer = top;

	while(layer != NULL)
	{
		if(layer->layer_type == TYPE_LAYER_SET && (layer->flags & LAYER_SET_CLOSE) != 0)
		{
			LAYER *prev = layer->prev;
			LayerSetHideChildren(layer, &prev);
			layer = prev;
		}
		else
		{
			layer = layer->prev;
		}
	}

	gtk_widget_get_allocation(widget, &place);
	gtk_range_set_value(GTK_RANGE(gtk_scrolled_window_get_vscrollbar(
		GTK_SCROLLED_WINDOW(app->layer_window.scrolled_window))), place.y);
	g_signal_handler_disconnect(G_OBJECT(widget),
		GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(widget), "signal_id")));
}

/*****************************************************************
* OnChangeCurrentTab関数                                         *
* 描画領域のタブが変わった時の関数                               *
* 引数                                                           *
* note_book			: タブウィジェット                           *
* note_book_page	: タブの情報                                 *
* page				: タブのID                                   *
* app				: アプリケーションを管理する構造体のアドレス *
*****************************************************************/
static void OnChangeCurrentTab(
	GtkNotebook* note_book,
	GtkWidget* note_book_page,
	gint page,
	APPLICATION* app
)
{
	// レイヤービューのウィジェットを全て削除
		// レイヤービューのリスト
	ClearLayerView(&app->layer_window);

	// 描画領域変更中のフラグを立てる
	app->flags |= APPLICATION_IN_SWITCH_DRAW_WINDOW;

	// 描画領域の新規作成中でなければ新たにアクティブになる描画領域のレイヤーをビューに追加
	if((app->flags & APPLICATION_IN_MAKE_NEW_DRAW_AREA) == 0)
	{
		DRAW_WINDOW* window;	// アクティブにする描画領域
		LAYER* layer;			// ビューに追加するレイヤー
		int i;					// for文用のカウンタ

		// アクティブな描画領域をセット
		app->active_window = (int16)page;
		window = GetActiveDrawWindow(app);

		if(window->focal_window != NULL)
		{
			window = window->focal_window;
		}

		// 一番下のレイヤーを設定
		layer = window->layer;

		// ビューに全てのレイヤーを追加
		for(i=0; i<window->num_layer && layer != NULL; i++)
		{
			LayerViewAddLayer(layer, window->layer,
				app->layer_window.view, i+1);
			layer = layer->next;
		}
		window->num_layer = (uint16)i;

		// アクティブレイヤーをセット
		LayerViewSetActiveLayer(window->active_layer, app->layer_window.view);

		// ナビゲーションの拡大縮小率を変更
		gtk_adjustment_set_value(app->navigation_window.zoom_slider,
			window->zoom);

		// 表示を反転の設定
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->menu_data.reverse_horizontally),
			window->flags & DRAW_WINDOW_DISPLAY_HORIZON_REVERSE);

		// 背景色変更のメニューの状態を設定
		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->menus.change_back_ground_menu),
			window->flags & DRAW_WINDOW_SECOND_BG);
		if(app->layer_window.change_bg_button != NULL)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->layer_window.change_bg_button),
				window->flags & DRAW_WINDOW_SECOND_BG);
		}

		// 描画領域があればナビゲーション、レイヤーウィンドウを更新
		if(app->window_num > 0)
		{
			ChangeNavigationDrawWindow(&app->navigation_window, window);
			FillTextureLayer(window->texture, &app->textures);
			g_object_set_data(G_OBJECT(window->active_layer->widget->box), "signal_id",
				GUINT_TO_POINTER(g_signal_connect(G_OBJECT(window->active_layer->widget->box), "size-allocate",
					G_CALLBACK(Move2ActiveLayer), app)));
			ChangeActiveLayer(window, window->active_layer);
			gtk_window_set_title(GTK_WINDOW(app->window), app->draw_window[app->active_window]->file_name);

			// 表示用フィルターの状態を設定
			app->display_filter.filter_func = app->tool_window.color_chooser->filter_func =
				g_display_filter_funcs[window->display_filter_mode];
			app->display_filter.filter_data = app->tool_window.color_chooser->filter_data = (void*)app;

			gtk_widget_queue_draw(app->tool_window.color_chooser->widget);
			UpdateColorBox(app->tool_window.color_chooser);
			gtk_widget_queue_draw(app->tool_window.color_chooser->pallete_widget);

			gtk_check_menu_item_set_active(
				GTK_CHECK_MENU_ITEM(app->menus.display_filter_menus[window->display_filter_mode]),
				TRUE
			);
		}
		else
		{	// 描画領域が無ければメインウィンドウのキャプションを変更
			char window_title[512];
			(void)sprintf(window_title, "Paint Soft %s %d.%d.%d.%d",
				(GetHas3DLayer(app) == FALSE) ? "KABURAGI" : "MIAKDO",
					MAJOR_VERSION, MINOR_VERSION, RELEASE_VERSION, BUILD_VERSION);
			gtk_window_set_title(GTK_WINDOW(app->window), window_title);
		}
	}

	// 描画領域切り替え終了
	app->flags &= ~(APPLICATION_IN_SWITCH_DRAW_WINDOW);
}

/*******************************
* CompareFontFamilyName関数    *
* フォントソート用の比較関数   *
* 引数                         *
* font1	: フォントデータ1      *
* font2	: フォントデータ2      *
* 返り値                       *
*	フォント名文字列比較の結果 *
*******************************/
static int CompareFontFamilyName(PangoFontFamily** font1, PangoFontFamily** font2)
{
	// フォントからフォントの名前を取り出す
	const gchar* font_name1, *font_name2;
	font_name1 = pango_font_family_get_name(*font1);
	font_name2 = pango_font_family_get_name(*font2);

	// どちらかがNULLなら終了
	if(font_name1 == NULL)
	{
		return 1;
	}
	if(font_name2 == NULL)
	{
		return -1;
	}

	return strcmp(font_name1, font_name2);
}

/***********************************************************
* ChangeSmoothMethod関数                                   *
* 手ブレ補正の方式を変更するラジオボタンのコールバック関数 *
* 引数                                                     *
* button	: ラジオボタンウィジェット                     *
* smoother	: 手ブレ補正の情報を管理する構造体のアドレス   *
***********************************************************/
static void ChangeSmoothMethod(GtkRadioButton* button, SMOOTHER* smoother)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		smoother->mode = GPOINTER_TO_INT(g_object_get_data(
			G_OBJECT(button), "smooth_method"));
	}
}

/***********************************************************
* ChangeSmootherQuality関数                                *
* 手ブレ補正に使用するサンプル数変更時のコールバック関数   *
* 引数                                                     *
* spin		: 手ブレ補正の品質変更ウィジェットのアジャスタ *
* smoother	: 手ブレ補正の情報を管理する構造体のアドレス   *
***********************************************************/
static void ChangeSmootherQuality(GtkAdjustment* spin, SMOOTHER* smoother)
{
	smoother->num_use = (int)gtk_adjustment_get_value(spin);

#if GTK_MAJOR_VERSION >= 3
	{
		APPLICATION *app = (APPLICATION*)g_object_get_data(
			G_OBJECT(spin), "application");
		int i;
		for(i=0; i<MAX_TOUCH; i++)
		{
			app->tool_window.touch_smoother[i].num_use = smoother->num_use;
		}
	}
#endif
}

/***************************************************************
* ChangeSmootherRate関数                                       *
* 手ブレ補正の適用割合変更時のコールバック関数                 *
* 引数                                                         *
* spin		: 手ブレ補正の適用割合変更ウィジェットのアジャスタ *
* smoother	: 手ブレ補正の情報を管理する構造体のアドレス       *
***************************************************************/
static void ChangeSmootherRate(GtkAdjustment* spin, SMOOTHER* smoother)
{
	smoother->rate = gtk_adjustment_get_value(spin);

#if GTK_MAJOR_VERSION >= 3
	{
		APPLICATION *app = (APPLICATION*)g_object_get_data(
			G_OBJECT(spin), "application");
		int i;
		for(i=0; i<MAX_TOUCH; i++)
		{
			app->tool_window.touch_smoother[i].rate = smoother->rate;
		}
	}
#endif
}

/*********************************************************
* DisplayReverseButtonClicked関数                        *
* 左右反転表示ボタンがクリックされた時のコールバック関数 *
* 引数                                                   *
* button	: 左右反転表示ボタン                         *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void DisplayReverseButtonClicked(GtkWidget* button, APPLICATION* app)
{
	gboolean state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));
	char mark_up_buff[256];

	if((app->flags & APPLICATION_IN_REVERSE_OPERATION) == 0)
	{
		app->flags |= APPLICATION_IN_REVERSE_OPERATION;

		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->menu_data.reverse_horizontally), state);

		app->flags &= ~(APPLICATION_IN_REVERSE_OPERATION);
	}

	if(state == FALSE)
	{
		(void)sprintf(mark_up_buff, "%s", app->labels->window.normal);
	}
	else
	{
		(void)sprintf(mark_up_buff, "<span color=\"red\">%s</span>", app->labels->window.reverse);
	}

	gtk_label_set_markup(GTK_LABEL(app->reverse_label), mark_up_buff);
}

/*********************************************************
* DisplayEditSelectionButtonClicked関数                  *
* 選択範囲編集ボタンがクリックされた時のコールバック関数 *
* 引数                                                   *
* button	: 選択範囲編集ボタン                         *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
static void DisplayEditSelectionButtonClicked(GtkWidget* button, APPLICATION* app)
{
	gboolean state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button));

	if((app->flags & APPLICATION_IN_EDIT_SELECTION) == 0)
	{

		app->flags |= APPLICATION_IN_EDIT_SELECTION;

		gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->menu_data.edit_selection), state);

		app->flags &= ~(APPLICATION_IN_EDIT_SELECTION);
	}

	app->draw_window[app->active_window]->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

/**********************************************************************
* DragDataRecieveCallBack関数                                         *
* ファイルがドラッグ&ドロップされたときに呼び出されるコールバック関数 *
* 引数                                                                *
* widget			: ドロップ先のウィジェット                        *
* context			: ドラッグ&ドロップの情報                         *
* x					: ドロップされたときのマウスのX座標               *
* y					: ドロップされたときのマウスのY座標               *
* selection_data	: ドラッグ&ドロップのデータ                       *
* target_type		: ドロップされたデータのタイプ                    *
* time_stamp		: ドロップされた時間                              *
* app				: アプリケーションを管理する構造体のアドレス      *
**********************************************************************/
static void DragDataRecieveCallBack(
	GtkWidget* widget,
	GdkDragContext* context,
	gint x,
	gint y,
	GtkSelectionData* selection_data,
	guint target_type,
	guint time_stamp,
	APPLICATION* app
)
{
	if((selection_data != NULL) && (gtk_selection_data_get_length(selection_data) >= 0))
	{
		if(target_type == DROP_URI)
		{
			gchar *uri = (gchar*)gtk_selection_data_get_data(selection_data);
			gchar *file_path = g_filename_from_uri(uri, NULL, NULL);
			gchar *c = file_path;

			while(*c != '\0')
			{
				if(*c == '\r')
				{
					*c = '\0';
					break;
				}
				c = g_utf8_next_char(c);
			}

			OpenFile(file_path, app);

			g_free(file_path);
		}

		gtk_drag_finish(context, TRUE, TRUE, time_stamp);
	}
}

/***************************************************************
* MainWindowRealizeCallBack関数                                *
* メインウィンドウが表示された時に呼び出されるコールバック関数 *
* 引数                                                         *
* window		: ウィンドウウィジェット                       *
* allocation	: ウィンドウのサイズ                           *
* realize_data	: 処理用のデータ                               *
***************************************************************/
static void MainWindowRealizeCallBack(
	GtkWidget* window,
	GdkRectangle* allocation,
	REALIZE_DATA* realize_data
)
{
	GtkWidget *child;
	APPLICATION *app = realize_data->app;

	child = gtk_paned_get_child1(GTK_PANED(app->left_pane));
	if(child != NULL)
	{
		gtk_paned_set_position(GTK_PANED(app->left_pane), app->left_pane_position);
	}

	child = gtk_paned_get_child2(GTK_PANED(app->right_pane));
	if(child != NULL)
	{
		gtk_paned_set_position(GTK_PANED(app->right_pane), app->right_pane_position);
	}

	gtk_paned_set_position(GTK_PANED(app->tool_window.pane), app->tool_window.pane_position);

	g_signal_handler_disconnect(G_OBJECT(window), realize_data->signal_id);

	MEM_FREE_FUNC(realize_data);
}

/*********************************************************************
* InitializeApplication関数                                          *
* アプリケーションの初期化                                           *
* 引数                                                               *
* app				: アプリケーション全体を管理する構造体のアドレス *
* init_file_name	: 初期化ファイルの名前                           *
*********************************************************************/
void InitializeApplication(APPLICATION* app, char* init_file_name)
{
	// 各種ディレクトリへのパス(Mac対策)
	gchar *file_path;
	// スプラッシュウィンドウのデータ
	SPLASH_WINDOW splash = {app, NULL, NULL, 0};
	// 初期化データ
	INITIALIZE_DATA init_data;
	// メニューバー
	GtkWidget* menu;
	// フォントソート用
	PangoContext* context;
	// ファイルドロップ対応用
	GtkTargetEntry target_list[] = {{"text/uri-list", 0, DROP_URI}};
	// シングルウィンドウ用の左右のパッキングボックス
	GtkWidget *left_box, *right_box;
	// ウィンドウのタイトル
	char window_title[512];
	// アプリケーションデータのディレクトリ
	char *app_dir_path;

	app_dir_path = g_build_filename(g_get_user_config_dir(), "KABURAGI", NULL);

	// 初期化ファイルを読み込む
		// まずはアプリケーションデータのディレクトリから
	file_path = g_build_filename(app_dir_path, init_file_name, NULL);
	if(ReadInitializeFile(app, file_path, &init_data) != 0)
	{	// 失敗したらプログラムファイルのディレクトリ
		g_free(file_path);
		file_path =  g_build_filename(app->current_path, init_file_name, NULL);
		if(ReadInitializeFile(app, file_path, &init_data) != 0)
		{	// それも失敗したら終了
			GtkWidget *dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
				GTK_MESSAGE_ERROR, GTK_BUTTONS_OK, "Failed to read data file.");
			exit(EXIT_FAILURE);
		}
	}
	g_free(file_path);

	// トップレベルウィンドウを作成
	app->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

	// スプラッシュウィンドウ表示でスタートアップが
		// 終了しないようにする
	gtk_window_set_auto_startup_notification(FALSE);
	// スプラッシュウィンドウを表示
	InitializeSplashWindow(&splash);
	// スプラッシュウィンドウを表示したのでスタートアップの
		// 設定を元に戻す
	gtk_window_set_auto_startup_notification(TRUE);

	// アプリケーション名の設定
	(void)sprintf(window_title, "Paint Soft %s",
		(GetHas3DLayer(app) == FALSE) ? "KABURAGI" : "MIKADO");
	g_set_application_name(window_title);

	// フルスクリーンの設定
	if((app->flags & APPLICATION_FULL_SCREEN) != 0)
	{
		gtk_window_fullscreen(GTK_WINDOW(app->window));
	}
	// 最大化の設定
	if((app->flags & APPLICATION_WINDOW_MAXIMIZE) != 0)
	{
		gtk_window_maximize(GTK_WINDOW(app->window));
	}

	file_path = g_build_filename(app->current_path, "image/icon.png", NULL);
	gtk_window_set_default_icon_from_file(file_path, NULL);
	g_free(file_path);

	// ウィンドウタイトルを作成する
	(void)sprintf(window_title, "Paint Soft %s %d.%d.%d.%d",
		(GetHas3DLayer(app) == FALSE) ? "KABURAGI" : "MIAKDO",
			MAJOR_VERSION, MINOR_VERSION, RELEASE_VERSION, BUILD_VERSION);

	// ラベルの文字列を読み込む
	if(app->language_file_path != NULL)
	{
		if(app->language_file_path[0] == '.'
			&& app->language_file_path[1] == '/')
		{
			file_path = g_build_filename(app->current_path,
				&app->language_file_path[2], NULL);
			LoadLabels(app->labels, app->fractal_labels, file_path);
			g_free(file_path);
		}
		else
		{
			LoadLabels(app->labels, app->fractal_labels, app->language_file_path);
		}
	}

	// 拡張デバイスを有効に
#if GTK_MAJOR_VERSION <= 2
	gtk_widget_set_extension_events(app->window, GDK_EXTENSION_EVENTS_CURSOR);
#endif
	// ウィンドウタイトルを設定
	gtk_window_set_title(GTK_WINDOW(app->window), window_title);
	// ウィンドウの位置を設定
	gtk_window_move(GTK_WINDOW(app->window), app->window_x, app->window_y);
	gtk_window_resize(GTK_WINDOW(app->window), app->window_width, app->window_height);
	// ウィンドウが閉じるときのコールバック関数をセット
	(void)g_signal_connect(G_OBJECT(app->window), "delete_event",
		G_CALLBACK(OnCloseMainWindow), app);
	// キーボードのコールバック関数をセット
	(void)g_signal_connect(G_OBJECT(app->window), "key-press-event",
		G_CALLBACK(KeyPressEvent), app);
	(void)g_signal_connect(G_OBJECT(app->window), "key-release-event",
		G_CALLBACK(KeyPressEvent), app);

	// パターンを初期化
	if(app->pattern_path[0] == '.' && app->pattern_path[1] == '/')
	{
		int buffer_size = 0;
		file_path = g_build_filename(app->current_path, &app->pattern_path[2], NULL);
		InitializePattern(&app->patterns, file_path, &buffer_size);
		g_free(file_path);
	}
	else
	{
		int buffer_size = 0;
		InitializePattern(&app->patterns, app->pattern_path, &buffer_size);
	}
	app->patterns.scale = 100;

	// テクスチャを初期化
	if(app->texture_path[0] == '.' && app->texture_path[1] == '/')
	{
		file_path = g_build_filename(app->current_path, &app->texture_path[2], NULL);
		LoadTexture(&app->textures, file_path);
		g_free(file_path);
	}
	else
	{
		LoadTexture(&app->textures, app->texture_path);
	}

	// スタンプを初期化
	if(app->stamp_path[0] == '.' && app->stamp_path[1] == '/')
	{
		int buffer_size = 0;
		file_path = g_build_filename(app->current_path, &app->stamp_path[2], NULL);
		InitializePattern(&app->stamps, file_path, &buffer_size);
		g_free(file_path);
	}
	else
	{
		int buffer_size = 0;
		InitializePattern(&app->stamps, app->stamp_path, &buffer_size);
	}
	app->stamp_shape = (uint8*)MEM_ALLOC_FUNC(app->stamps.pattern_max_byte);
	app->stamp_buff_size = app->stamps.pattern_max_byte;

	// メニューバーと描画領域をパッキングするボックスを作成
	app->vbox = gtk_vbox_new(FALSE, 0);

	// ファイルのドロップを設定
	gtk_drag_dest_set(
		app->window,
		GTK_DEST_DEFAULT_ALL,
		target_list,
		NUM_DROP_LIST,
		GDK_ACTION_COPY
	);
	gtk_drag_dest_add_uri_targets(app->window);
	(void)g_signal_connect(G_OBJECT(app->window), "drag-data-received",
		G_CALLBACK(DragDataRecieveCallBack), app);

	// フルスクリーン・最大化変更時のコールバック関数を設定
	(void)g_signal_connect(G_OBJECT(app->window), "window_state_event",
		G_CALLBACK(OnChangeMainWindowState), app);

	// スクリプトの読み込み
	if(app->script_path[0] == '.' && app->script_path[1] == '/')
	{
		file_path = g_build_filename(app->current_path, &app->script_path[2], NULL);
		InitializeScripts(&app->scripts, file_path);
		g_free(file_path);
	}
	else
	{
		InitializeScripts(&app->scripts, app->script_path);
	}

	// メニューバーを作成
	app->menu_bar = menu = GetMainMenu(app, app->window, app->language_file_path);
	// 描画領域のタブを作成
	app->note_book = gtk_notebook_new();
	// タブは下側に
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(app->note_book), GTK_POS_BOTTOM);
	// タブの高さを設定
#if GTK_MAJOR_VERSION <= 2
	gtk_notebook_set_tab_vborder(GTK_NOTEBOOK(app->note_book), 0);
#endif
	// ウィンドウに追加
	gtk_container_add(
		GTK_CONTAINER(app->window), app->vbox);
	// ボックスにメニューバーを追加
	gtk_box_pack_start(GTK_BOX(app->vbox), menu, FALSE, FALSE, 0);

	// ステータスバーを作成し、ウィンドウ下側に配置
	app->status_bar = gtk_statusbar_new();
	gtk_box_pack_end(GTK_BOX(app->vbox), app->status_bar, FALSE, FALSE, 0);
	// プログレスバーは必要に応じて色を変更
	app->progress = gtk_progress_bar_new();
	gtk_box_pack_start(GTK_BOX(app->status_bar), app->progress, FALSE, FALSE, 20);
	{
		GtkStyle *window_style, *progress_style;

		window_style = gtk_widget_get_style(app->window);
		progress_style = gtk_widget_get_style(app->window);
		if(window_style->bg[GTK_STATE_PRELIGHT].red == progress_style->bg[GTK_STATE_PRELIGHT].red
			&& window_style->bg[GTK_STATE_PRELIGHT].green == progress_style->bg[GTK_STATE_PRELIGHT].green
			&& window_style->bg[GTK_STATE_PRELIGHT].blue == progress_style->bg[GTK_STATE_PRELIGHT].blue
		)
		{
			gtk_widget_modify_bg(app->progress, GTK_STATE_PRELIGHT, &progress_style->bg[GTK_STATE_SELECTED]);
		}
	}

	// 手ブレ補正の設定変更、拡大・縮小、表示反転、回転、選択範囲編集ウィジェットを作成
	{
		// 手ブレ補正設定変更ウィジェット用のパッキングボックス
		GtkWidget* smooth_box = gtk_hbox_new(FALSE, 0);
		// 手ブレ補正の設定変更ウィジェット
		GtkWidget* smooth_spin;
		// 拡大・縮小用ボタン
		GtkWidget* button;
		// 手ブレ補正の設定の変更用アジャスタ
		GtkAdjustment* smooth_adjustment;
		// 左右反転用のバッファ
		GdkPixbuf *image_buf;
		uint8 *reverse_buf, *image_pixels;
		// ラベルに入れる文字列
		gchar str[256];
		// 画像の幅、高さ、チャンネル数
		gint image_width, image_height;
		gint image_channel;
		int i, j, k;	// for文用のカウンタ

		// 「手ブレ補正」のラベル
		(void)sprintf(str, "%s : ", app->labels->tool_box.smooth);
		gtk_box_pack_start(GTK_BOX(smooth_box),
			gtk_label_new(str), FALSE, FALSE, 0);

		// 手ブレ補正の方式選択ラジオボタン
		button = gtk_radio_button_new_with_label(NULL, app->labels->tool_box.smooth_gaussian);
		g_object_set_data(G_OBJECT(button), "smooth_method", GINT_TO_POINTER(0));
		(void)g_signal_connect(G_OBJECT(button), "toggled",
			G_CALLBACK(ChangeSmoothMethod), &app->tool_window.smoother);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 0);
		button = gtk_radio_button_new_with_label(gtk_radio_button_get_group(
			GTK_RADIO_BUTTON(button)), app->labels->tool_box.smooth_average);
		g_object_set_data(G_OBJECT(button), "smooth_method", GINT_TO_POINTER(1));
		(void)g_signal_connect(G_OBJECT(button), "toggled",
			G_CALLBACK(ChangeSmoothMethod), &app->tool_window.smoother);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 0);
		if(app->tool_window.smoother.mode == SMOOTH_AVERAGE)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
		}

		// 補正レベル変更ウィジェット
		smooth_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(app->tool_window.smoother.num_use, 0,
			SMOOTHER_POINT_BUFFER_SIZE, 1, 1, 0));
		smooth_spin = gtk_spin_button_new(smooth_adjustment, 1, 1);
#if GTK_MAJOR_VERSION <= 2
		GTK_WIDGET_UNSET_FLAGS(smooth_spin, GTK_CAN_DEFAULT);
#else
		g_object_set_data(G_OBJECT(smooth_adjustment), "application", app);
#endif
		// コールバック関数の設定
		(void)g_signal_connect(G_OBJECT(smooth_adjustment), "value_changed",
			G_CALLBACK(ChangeSmootherQuality), &app->tool_window.smoother);
		// 小数点以下は表示しない
		gtk_spin_button_set_digits(GTK_SPIN_BUTTON(smooth_spin), 0);
		gtk_box_pack_start(GTK_BOX(smooth_box), gtk_label_new(
			app->labels->tool_box.smooth_quality), FALSE, FALSE, 1);
		gtk_box_pack_start(GTK_BOX(smooth_box), smooth_spin, FALSE, TRUE, 0);

		// 適用割合変更ウィジェット
		smooth_adjustment = GTK_ADJUSTMENT(gtk_adjustment_new(app->tool_window.smoother.rate, 0,
			SMOOTHER_RATE_MAX, 1, 1, 0));
		smooth_spin = gtk_spin_button_new(smooth_adjustment, 1, 1);
		gtk_box_pack_start(GTK_BOX(smooth_box), gtk_label_new(
			app->labels->tool_box.smooth_rate), FALSE, FALSE, 1);
		gtk_box_pack_start(GTK_BOX(smooth_box), smooth_spin, FALSE, TRUE, 0);
#if GTK_MAJOR_VERSION >= 3
		g_object_set_data(G_OBJECT(smooth_adjustment), "application", app);
#endif
		// コールバック関数の設定
		(void)g_signal_connect(G_OBJECT(smooth_adjustment), "value_changed",
			G_CALLBACK(ChangeSmootherRate), &app->tool_window.smoother);

		// 拡大ボタン
		button = gtk_button_new_with_label(app->labels->menu.zoom_in);
		gtk_widget_set_sensitive(button, FALSE);
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteZoomIn), app);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 3);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 縮小ボタン
		button = gtk_button_new_with_label(app->labels->menu.zoom_out);
		gtk_widget_set_sensitive(button, FALSE);
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteZoomOut), app);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 0);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 等倍表示ボタン
		button = gtk_button_new_with_label(app->labels->menu.zoom_reset);
		gtk_widget_set_sensitive(button, FALSE);
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteZoomReset), app);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 0);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 表示反転ウィジェット
		app->reverse_label = gtk_label_new(app->labels->window.normal);
		app->reverse_button = gtk_toggle_button_new();
		gtk_widget_set_sensitive(app->reverse_button, FALSE);
		(void)g_signal_connect(G_OBJECT(app->reverse_button), "toggled",
			G_CALLBACK(DisplayReverseButtonClicked), app);
		gtk_container_add(GTK_CONTAINER(app->reverse_button), app->reverse_label);
		gtk_box_pack_start(GTK_BOX(smooth_box), app->reverse_button, FALSE, FALSE, 3);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = app->reverse_button;
		app->menus.num_disable_if_no_open++;

		// 反時計回りボタン
		button = gtk_button_new();
		file_path = g_build_filename(app->current_path, "image/counter_clockwise.png", NULL);
		gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_file(file_path));
		g_free(file_path);
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteRotateCounterClockwise), app);
		gtk_widget_set_sensitive(button, FALSE);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 3);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 時計回りボタン
		button = gtk_button_new();
		file_path = g_build_filename(app->current_path, "image/clockwise.png", NULL);
		gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_file(file_path));
		g_free(file_path);
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteRotateClockwise), app);
		gtk_widget_set_sensitive(button, FALSE);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 0);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 回転リセットボタン
		button = gtk_button_new_with_label(app->labels->menu.reset_rotate);
		g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteRotateReset), app);
		gtk_widget_set_sensitive(button, FALSE);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 0);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 元に戻す・やり直しボタン
		file_path = g_build_filename(app->current_path, "image/arrow.png", NULL);
		image_buf = gdk_pixbuf_new_from_file(file_path, NULL);
		g_free(file_path);
		button = gtk_button_new();
		gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_pixbuf(image_buf));
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteUndo), app);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 3);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 画像の情報を取得して左右反転
		image_buf = gdk_pixbuf_copy(image_buf);
		image_width = gdk_pixbuf_get_width(image_buf);
		image_height = gdk_pixbuf_get_height(image_buf);
		image_channel = gdk_pixbuf_get_rowstride(image_buf);
		reverse_buf = (uint8*)MEM_ALLOC_FUNC(image_channel);
		image_channel /= image_width;
		image_pixels = gdk_pixbuf_get_pixels(image_buf);

		for(i=0; i<image_height; i++)
		{
			for(j=0; j<image_width; j++)
			{
				for(k=0; k<image_channel; k++)
				{
					reverse_buf[j*image_channel+k] = image_pixels[
						i*image_width*image_channel+(image_width-j-1)*image_channel+k];
				}
			}

			(void)memcpy(&image_pixels[i*image_width*image_channel], reverse_buf, image_width*image_channel);
		}
		MEM_FREE_FUNC(reverse_buf);

		button = gtk_button_new();
		gtk_container_add(GTK_CONTAINER(button), gtk_image_new_from_pixbuf(image_buf));
		(void)g_signal_connect_swapped(G_OBJECT(button), "clicked",
			G_CALLBACK(ExecuteRedo), app);
		gtk_box_pack_start(GTK_BOX(smooth_box), button, FALSE, FALSE, 0);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = button;
		app->menus.num_disable_if_no_open++;

		// 選択範囲編集ボタン
		app->edit_selection = gtk_toggle_button_new_with_label(app->labels->window.edit_selection);
		gtk_widget_set_sensitive(app->edit_selection, FALSE);
		gtk_box_pack_end(GTK_BOX(smooth_box), app->edit_selection, FALSE, FALSE, 0);
		(void)g_signal_connect(G_OBJECT(app->edit_selection), "toggled",
			G_CALLBACK(DisplayEditSelectionButtonClicked), app);
		app->menus.disable_if_no_open[app->menus.num_disable_if_no_open] = app->edit_selection;
		app->menus.num_disable_if_no_open++;

		gtk_box_pack_start(GTK_BOX(app->vbox), smooth_box, FALSE, FALSE, 0);
	}

	// 描画領域のタブを追加
		// 先に左右のペーンに入れるパッキングボックスを作成
	app->left_pane = gtk_hpaned_new();
	app->right_pane = gtk_hpaned_new();
	// 左右のペーンに作成したパッキングボックスを追加
	left_box = gtk_hbox_new(FALSE, 0);
	right_box = gtk_hbox_new(FALSE, 0);
	gtk_paned_pack1(GTK_PANED(app->left_pane), left_box, TRUE, FALSE);
	gtk_paned_pack2(GTK_PANED(app->left_pane), app->right_pane, TRUE, TRUE);
	gtk_paned_pack2(GTK_PANED(app->right_pane), right_box, TRUE, FALSE);
	// ペーンの間に描画領域のタブを入れる
	gtk_paned_pack1(GTK_PANED(app->right_pane), app->note_book, TRUE, TRUE);
	// ウィンドウに追加
	gtk_box_pack_start(GTK_BOX(app->vbox), app->left_pane, TRUE, TRUE, 0);

	// デフォルトのウィジェットを空にする
		// (手ぶれ補正にショートカット使用時にフォーカスを防止)
	gtk_window_set_default(GTK_WINDOW(app->window), NULL);
	gtk_window_set_focus(GTK_WINDOW(app->window), NULL);

	// タブ変更時のコールバック関数をセット
	(void)g_signal_connect(G_OBJECT(app->note_book), "switch_page",
		G_CALLBACK(OnChangeCurrentTab), app);

	// フォントのリストを取得
	context = gtk_widget_get_pango_context(app->window);
	pango_context_list_families(context, &app->font_list, &app->num_font);

	// 文字列順で並べ替える
	qsort(app->font_list, app->num_font, sizeof(*app->font_list),
		(int (*)(const void*, const void*))CompareFontFamilyName);

	// ツールボックスウィンドウを作成
	app->tool_window.window = CreateToolBoxWindow(app, app->window);
	// パレット情報を読み込む
	file_path = g_build_filename(app_dir_path, PALLETE_FILE_NAME, NULL);
	if(LoadPalleteFile(app->tool_window.color_chooser, file_path) != 0)
	{
		g_free(file_path);
		file_path = g_build_filename(app->current_path, PALLETE_FILE_NAME, NULL);
		(void)LoadPalleteFile(app->tool_window.color_chooser, file_path);
	}
	g_free(file_path);

	// レイヤービューをドッキングするならば
	if((app->layer_window.flags & LAYER_WINDOW_DOCKED) != 0)
	{
		app->navi_layer_pane = gtk_vpaned_new();
		gtk_paned_set_position(GTK_PANED(app->navi_layer_pane), app->layer_window.pane_position);

		if((app->layer_window.flags & LAYER_WINDOW_PLACE_RIGHT) == 0)
		{
			gtk_box_pack_start(GTK_BOX(left_box), app->navi_layer_pane, TRUE, TRUE, 0);
			gtk_box_reorder_child(GTK_BOX(left_box), app->navi_layer_pane, 0);
		}
		else
		{
			gtk_box_pack_end(GTK_BOX(right_box), app->navi_layer_pane, TRUE, TRUE, 0);
		}
	}

	// レイヤーウィンドウを作成
	app->layer_window.window =
		CreateLayerWindow(app, app->window, &app->layer_window.view);

	// ナビゲーションウィンドウを作成
	InitializeNavigation(&app->navigation_window, app, app->navi_layer_pane);

	// プレビューウィンドウを作成
	InitializePreviewWindow(&app->preview_window, app);

	// ドラッグ&ドロップの間隔を設定
	g_object_set(gtk_settings_get_default(), "gtk-dnd-drag-threshold",
		DND_THRESHOLD, NULL);

	// ウィンドウを表示
	if((app->tool_window.flags & TOOL_DOCKED) == 0)
	{
		gtk_widget_show_all(app->tool_window.window);
	}
	if((app->layer_window.flags & LAYER_WINDOW_DOCKED) == 0)
	{
		gtk_widget_show_all(app->layer_window.window);
		gtk_widget_show_all(app->navigation_window.window);
	}

	// 色データを設定
	{
		HSV hsv;
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
		{
			uint8 temp;
			temp = init_data.fg_color[0];
			init_data.fg_color[0] = init_data.fg_color[2];
			init_data.fg_color[2] = temp;

			temp = init_data.bg_color[0];
			init_data.bg_color[0] = init_data.bg_color[2];
			init_data.bg_color[2] = temp;
		}
#endif
		(void)memcpy(app->tool_window.color_chooser->rgb, init_data.fg_color, 3);
		(void)memcpy(app->tool_window.color_chooser->back_rgb, init_data.bg_color, 3);
		RGB2HSV_Pixel(init_data.bg_color, &hsv);
		app->tool_window.color_chooser->back_hsv = hsv;
		RGB2HSV_Pixel(init_data.fg_color, &hsv);
		SetColorChooserPoint(app->tool_window.color_chooser, &hsv, TRUE);
	}
	
	// メインウィンドウを表示
	gtk_widget_show_all(app->window);

	// 色選択ウィジェットの表示・非表示を設定
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->tool_window.color_chooser->circle_button),
		app->tool_window.flags & TOOL_SHOW_COLOR_CIRCLE);
	if((app->tool_window.flags & TOOL_SHOW_COLOR_CIRCLE) != 0)
	{
		app->tool_window.color_chooser->flags |= COLOR_CHOOSER_SHOW_CIRCLE;
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->tool_window.color_chooser->pallete_button),
		app->tool_window.flags & TOOL_SHOW_COLOR_PALLETE);
	if((app->tool_window.flags & TOOL_SHOW_COLOR_PALLETE) != 0)
	{
		app->tool_window.color_chooser->flags |= COLOR_CHOOSER_SHOW_PALLETE;
	}

	// レイヤー合成の設定
	SetLayerBlendFunctions(app->layer_blend_functions);
	SetPartLayerBlendFunctions(app->part_layer_blend_functions);

	// 入力デバイスの設定
	{
		GList *device_list;
		GdkDevice *device;
		const gchar *device_name;

#if GTK_MAJOR_VERSION <= 2
		device_list = gdk_devices_list();
#else
		device = gdk_device_manager_get_client_pointer(
			gdk_display_get_device_manager(gdk_display_get_default()));
		device_list = gdk_device_list_slave_devices(device);
#endif

#ifdef _WIN32
# if GTK_MAJOR_VERSION <= 2
		while(device_list != NULL)
		{
			device = (GdkDevice*)device_list->data;
			device_name = gdk_device_get_name(device);

			if(StringStringIgnoreCase(device_name, "ERASER") != NULL)
			{
				gdk_device_set_source(device, GDK_SOURCE_ERASER);
			}
			device_list = device_list->next;
		}
# else
		{
			GList *check_list = device_list;

			while(check_list != NULL)
			{
				device = (GdkDevice*)check_list->data;
				device_name = gdk_device_get_name(device);

				if(StringStringIgnoreCase(device_name, "ERASER") != NULL)
				{
					g_object_set(G_OBJECT(device), "input-source",
						GINT_TO_POINTER(GDK_SOURCE_ERASER), NULL);
					//gdk_device_set_source(device, GDK_SOURCE_ERASER);
				}

				check_list = check_list->next;
			}
		}
# endif
#else
		app->num_device = 0;
		while(device_list != NULL)
		{
			app->num_device++;
			device_list = device_list->next;
		}

		app->input_sources = (GdkInputSource*)MEM_ALLOC_FUNC(
				sizeof(*app->input_sources)*app->num_device);
		app->set_input_modes = (gboolean*)MEM_ALLOC_FUNC(
				sizeof(*app->set_input_modes)*app->num_device);

#if GTK_MAJOR_VERSION <= 2
		device_list = gdk_devices_list();
#else
		device_list = gdk_device_list_slave_devices(NULL);
#endif
		app->num_device = 0;

		while(device_list != NULL)
		{
			device = (GdkDevice*)device_list->data;
			device_name = gdk_device_get_name(device);
			if(StringStringIgnoreCase(device_name, "CORE") == NULL)
			{
				app->set_input_modes[app->num_device] = TRUE;
			}
			else
			{
				app->set_input_modes[app->num_device] = FALSE;
			}

#if GTK_MAJOR_VERSION <= 2
			if(StringStringIgnoreCase(device_name, "STYLUS") != NULL)
			{
				gdk_device_set_source(device, GDK_SOURCE_PEN);
			}
			else if(StringStringIgnoreCase(device_name, "ERASER") != NULL)
			{
				gdk_device_set_source(device, GDK_SOURCE_ERASER);
#endif
			}
			app->input_sources[app->num_device] = gdk_device_get_source(device);

			device_list = device_list->next;
			app->num_device++;
		}
#endif

#if GTK_MAJOR_VERSION >= 3
		g_list_free(device_list);

		{
			GList *check_list = device_list;
			device_list = gdk_device_manager_list_devices(
				gdk_display_get_device_manager(gdk_display_get_default()), GDK_DEVICE_TYPE_MASTER);
			check_list = device_list;
			while(check_list != NULL)
			{
				device = (GdkDevice*)check_list->data;
				(void)gdk_device_set_mode(device, GDK_MODE_SCREEN);
				check_list = check_list->next;
			}
			g_list_free(device_list);
		}
#endif
	}

	// 左右のペーンにウィジェットが入っていなければ削除
	{
		GList *child;
		REALIZE_DATA *realize = (REALIZE_DATA*)MEM_ALLOC_FUNC(sizeof(*realize));

		(void)memset(realize, 0, sizeof(*realize));
		realize->app = app;

		child = gtk_container_get_children(GTK_CONTAINER(left_box));
		if(child == NULL)
		{
			gtk_widget_destroy(left_box);
		}
		else
		{
			gtk_paned_set_position(GTK_PANED(app->left_pane), app->left_pane_position);
		}
		g_list_free(child);

		child = gtk_container_get_children(GTK_CONTAINER(right_box));
		if(child == NULL)
		{
			gtk_widget_destroy(right_box);
		}
		else
		{
			gtk_paned_set_position(GTK_PANED(app->right_pane), app->right_pane_position);
		}
		g_list_free(child);

		// ウィンドウが表示された時にもう一度ペーンの位置を調整
		realize->signal_id = g_signal_connect(G_OBJECT(app->window), "size-allocate",
			G_CALLBACK(MainWindowRealizeCallBack), realize);
	}

	// 3Dモデリングの準備
	if(GetHas3DLayer(app) != FALSE)
	{
		app->modeling = ApplicationContextNew(app->window_width, app->window_height,
			app->current_path);
		// ラベルの文字列を読み込む
		if(app->language_file_path != NULL)
		{
			if(app->language_file_path[0] == '.'
				&& app->language_file_path[1] == '/')
			{
				file_path = g_build_filename(app->current_path,
					&app->language_file_path[2], NULL);
				Load3dModelingLabels(app, file_path);

				g_free(file_path);
			}
			else
			{
				Load3dModelingLabels(app, app->language_file_path);
			}
		}
	}

	// 初期化済みのフラグを立てる
	app->flags |= APPLICATION_INITIALIZED;

	// バックアップファイルを復元する
	RecoverBackUp(app);
}

/*****************************************************
* RecoverBackUp関数                                  *
* バックアップファイルが合った場合に復元する         *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void RecoverBackUp(APPLICATION* app)
{
	// 実行ファイルのディレクトリをチェック
	GDir *dir = g_dir_open(app->backup_directory_path, 0, NULL);
	// ファイルパス
	gchar *file_path;
	// ファイル名の長さ
	size_t name_length;
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
		name_length = strlen(file_name);
		if(name_length >= 4)
		{
			file_path = g_build_filename(app->backup_directory_path, file_name, NULL);
			if(StringCompareIgnoreCase(&file_name[name_length-4], ".kbt") == 0)
			{	// 一時保存ファイル発見
					// 復元するか尋ねる
				GtkWidget *dialog = gtk_message_dialog_new(
					GTK_WINDOW(app->window), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_YES_NO, app->labels->save.recover_backup);
				gchar *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
				if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES)
				{
					// ファイル読み込みストリーム
					FILE *fp = fopen(system_path, "rb");
					// ファイルサイズ
					size_t data_size;
					// ファイル名作成用
					char *str;
					int i;	// for文用のカウンタ

					(void)fseek(fp, 0, SEEK_END);
					data_size = (size_t)ftell(fp);
					rewind(fp);

					// ファイルパスからファイル名だけを取り出す
					file_name = str = file_path;
					while(*str != '\0')
					{
						if(*str == '/' || *str == '\\')
						{
							file_name = str+1;
						}

						str = g_utf8_next_char(str);
					}

					// ファイルオープン中のフラグを立てることでウィジェットのコールバックによる
						// セグメンテーションフォールトを回避
					app->flags |= APPLICATION_IN_OPEN_OPERATION;
					//app->active_window = app->window_num;
					app->draw_window[app->window_num] =
						ReadOriginalFormat(fp, (stream_func_t)fread, data_size, app, file_name);
					app->flags &= ~(APPLICATION_IN_OPEN_OPERATION);

					if(app->draw_window[app->window_num] != NULL)
					{
						app->active_window = app->window_num;
						// ファイルパスを設定
						app->draw_window[app->active_window]->file_path = MEM_STRDUP_FUNC(file_path);

						// ナビゲーションの表示を更新
						ChangeNavigationDrawWindow(&app->navigation_window, app->draw_window[app->active_window]);

						// 無効にしていた一部のメニューを有効に
						for(i=0; i<app->menus.num_disable_if_no_open; i++)
						{
							gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
						}
						gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
						gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);

						// ウィンドウのタイトルを画像名に
						gtk_window_set_title(GTK_WINDOW(app->window), file_name);

						// 描画領域のカウンタを更新
						app->window_num++;
					}

					(void)fclose(fp);
				}

				(void)remove(system_path);
				g_free(system_path);

				gtk_widget_destroy(dialog);
			}

			g_free(file_path);
		}
	}
}

/*********************************************************
* OpenFile関数                                           *
* 受けっとたパスのファイルを開く                         *
* 引数                                                   *
* file_path	: ファイルパス                               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
void OpenFile(char *file_path, APPLICATION* app)
{
	// ピクセルバッファに変換
	GdkPixbuf* pixbuf;
	// 拡張子判定用
	gchar* str;
	// 拡張子
	gchar *extention;
	// ファイル名
	gchar* file_name;
	// for文用のカウンタ
	int i, j;

	// 拡張子取得
	str = file_path + strlen(file_path) - 1;
	while(*str != '.' && str > file_path)
	{
		str--;
	}
	extention = str;

	if(StringCompareIgnoreCase(str, ".kab") == 0)
	{
		// ファイル読み込みストリーム
		gchar *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
		FILE *fp = fopen(system_path, "rb");
		// ファイルサイズ
		size_t data_size;
		// 読込中メッセージのID
		guint context_id, message_id;
		// 表示イベント処理用
		GdkEvent *queued_event;

		// 読込中のメッセージを表示
		context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(app->status_bar), "Loading");
		message_id = gtk_statusbar_push(GTK_STATUSBAR(app->status_bar),
			context_id, app->labels->window.loading);
		// イベントを回してメッセージを表示
#if GTK_MAJOR_VERSION <= 2
		gdk_window_process_updates(app->status_bar->window, TRUE);
#else
		gdk_window_process_updates(gtk_widget_get_window(app->status_bar), TRUE);
#endif

		while(gdk_events_pending() != FALSE)
		{
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event != NULL)
			{
#if GTK_MAJOR_VERSION <= 2
				if(queued_event->any.window == app->status_bar->window
#else
				if(queued_event->any.window == gtk_widget_get_window(app->status_bar)
#endif
					&& queued_event->any.type == GDK_EXPOSE)
				{
					gdk_event_free(queued_event);
					break;
				}
				else
				{
					gdk_event_free(queued_event);
				}
			}
		}

		(void)fseek(fp, 0, SEEK_END);
		data_size = (size_t)ftell(fp);
		rewind(fp);

		g_free(system_path);
		
		// ファイルパスからファイル名だけを取り出す
		file_name = str = file_path;
		while(*str != '\0')
		{
			if(*str == '/' || *str == '\\')
			{
				file_name = str+1;
			}

			str = g_utf8_next_char(str);
		}

		// ファイルオープン中のフラグを立てることでウィジェットのコールバックによる
			// セグメンテーションフォールトを回避
		app->flags |= APPLICATION_IN_OPEN_OPERATION;
		//app->active_window = app->window_num;
		app->draw_window[app->window_num] =
			ReadOriginalFormat(fp, (stream_func_t)fread, data_size, app, file_name);
		app->flags &= ~(APPLICATION_IN_OPEN_OPERATION);

		if(app->draw_window[app->window_num] == NULL)
		{
			return;
		}

		app->active_window = app->window_num;
		// ファイルパスを設定
		app->draw_window[app->active_window]->file_path = MEM_STRDUP_FUNC(file_path);

		// ナビゲーションの表示を更新
		ChangeNavigationDrawWindow(&app->navigation_window, app->draw_window[app->active_window]);

		// 読み込みストリーム削除
		(void)fclose(fp);

		// 無効にしていた一部のメニューを有効に
		for(i=0; i<app->menus.num_disable_if_no_open; i++)
		{
			gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
		}
		gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
		gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);

		// ウィンドウのタイトルを画像名に
		gtk_window_set_title(GTK_WINDOW(app->window), file_name);

		// アクティブなレイヤーの情報をコントロールに設定
		gtk_combo_box_set_active(GTK_COMBO_BOX(app->layer_window.layer_control.mode), app->draw_window[app->active_window]->active_layer->layer_mode);
		gtk_adjustment_set_value(app->layer_window.layer_control.opacity, app->draw_window[app->active_window]->active_layer->alpha);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->layer_window.layer_control.mask_width_under),
			app->draw_window[app->active_window]->active_layer->flags & LAYER_MASKING_WITH_UNDER_LAYER);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->layer_window.layer_control.lock_opacity),
			app->draw_window[app->active_window]->active_layer->flags & LAYER_LOCK_OPACITY);

		// 描画領域のカウンタを更新
		app->window_num++;

		// メッセージ表示を終了
		gtk_statusbar_remove(GTK_STATUSBAR(app->status_bar), context_id, message_id);
	}
	else if(StringCompareIgnoreCase(str, ".tlg") == 0)
	{
		// ファイル読み込みストリーム
		gchar *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
		FILE *fp = fopen(system_path, "rb");

		if(fp != NULL)
		{
			uint8 *pixels;
			int width, height;
			int channel;

			pixels = ReadTlgStream((void*)fp, (size_t (*)(void*, size_t, size_t, void*))fread,
				(int (*)(void*, long, int))fseek, (long (*)(void*))ftell, &width, &height, &channel);

			if(pixels != NULL)
			{
				// ピクセルデータを写すレイヤー
				LAYER *target;
				// 1行分のバイト数
				int stride = width * 4;

				// ファイルパスからファイル名だけを取り出す
				file_name = str = file_path;
				while(*str != '\0')
				{
					if(*str == '/' || *str == '\\')
					{
						file_name = str+1;
					}

					str = g_utf8_next_char(str);
				}

				if(channel == 1)
				{
					uint8 *old_pixels = pixels;
					pixels = (uint8*)MEM_ALLOC_FUNC(width*height*4);
					for(i=0; i<width*height; i++)
					{
						pixels[i*4] = old_pixels[i];
						pixels[i*4+1] = old_pixels[i];
						pixels[i*4+2] = old_pixels[i];
						pixels[i*4+3] = 0xff;
					}

					MEM_FREE_FUNC(old_pixels);
				}
				else if(channel == 2)
				{
					uint8 *old_pixels = pixels;
					pixels = (uint8*)MEM_ALLOC_FUNC(width*height*4);
					for(i=0; i<width*height; i++)
					{
						pixels[i*4] = old_pixels[i*2];
						pixels[i*4+1] = old_pixels[i*2];
						pixels[i*4+2] = old_pixels[i*2];
						pixels[i*4+3] = old_pixels[i*2+1];
					}

					MEM_FREE_FUNC(old_pixels);
				}
				else if(channel == 3)
				{
					uint8 *old_pixels = pixels;
					pixels = (uint8*)MEM_ALLOC_FUNC(width*height*4);
					for(i=0; i<width*height; i++)
					{
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
						pixels[i*4] = old_pixels[i*3+2];
						pixels[i*4+1] = old_pixels[i*3+1];
						pixels[i*4+2] = old_pixels[i*3];
#else
						pixels[i*4] = old_pixels[i*3];
						pixels[i*4+1] = old_pixels[i*3+1];
						pixels[i*4+2] = old_pixels[i*3+2];
#endif
						pixels[i*4+3] = 0xff;
					}

					MEM_FREE_FUNC(old_pixels);
				}
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
				else
				{
					uint8 b;
					for(i=0; i<width*height; i++)
					{
						b = pixels[i*4];
						pixels[i*4] = pixels[i*4+2];
						pixels[i*4+2] = b;
					}
				}
#endif

				// 描画領域を新たに追加
				app->draw_window[app->window_num] = CreateDrawWindow(
					width, height, 4, file_name, app->note_book, app->window_num, app);

				app->active_window = app->window_num;
				app->window_num++;

				// ファイルパスを設定
				app->draw_window[app->active_window]->file_path = MEM_STRDUP_FUNC(file_path);

				// ナビゲーションの表示を更新
				ChangeNavigationDrawWindow(&app->navigation_window, app->draw_window[app->active_window]);

				// 画像データを一番下のレイヤーにコピー
				target = app->draw_window[app->active_window]->active_layer;

				for(i=0; i<height; i++)
				{
					for(j=0; j<width; j++)
					{
						target->pixels[target->stride*i+target->channel*j] =
							pixels[(height-i-1)*stride+j*target->channel+2];
						target->pixels[target->stride*i+target->channel*j+1] =
							pixels[(height-i-1)*stride+j*target->channel+1];
						target->pixels[target->stride*i+target->channel*j+2] =
							pixels[(height-i-1)*stride+j*target->channel];
						target->pixels[target->stride*i+target->channel*j+3] =
							pixels[(height-i-1)*stride+j*target->channel+3];
					}
				}

				// 一番下のレイヤーをアクティブ表示に
				LayerViewSetActiveLayer(
					app->draw_window[app->active_window]->active_layer, app->layer_window.view
				);

				// 無効にしていた一部のメニューを有効に
				for(i=0; i<app->menus.num_disable_if_no_open; i++)
				{
					gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
				}
				gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
				gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);

				// ウィンドウのタイトルを画像名に
				gtk_window_set_title(GTK_WINDOW(app->window), file_name);
			}
		}

		g_free(system_path);
	}
	else if(StringCompareIgnoreCase(str, ".psd") == 0)
	{
		// ファイル読み込みストリーム
		gchar *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
		FILE *fp = fopen(system_path, "rb");
		// キャンバスデータ
		DRAW_WINDOW *window;
		// PSDデータのレイヤー
		LAYER *layer;

		layer = ReadPhotoShopDocument((void*)fp, (stream_func_t)fread,
			(seek_func_t)fseek, (long (*)(void*))ftell);
		if(layer != NULL)
		{
			// KABURAGIは全てのレイヤーの位置が同じ
				// かつ、同じサイズなので調整を行う
			LAYER *target;
			LAYER *canvas_layer;
			int32 min_x = layer->x, min_y = layer->y;
			int32 max_x = layer->x + layer->width, max_y = layer->y + layer->height;
			int32 width, height;
			uint8 alpha;
			target = layer->next;
			while(target != NULL)
			{
				if(min_x > target->x)
				{
					min_x = target->x;
				}
				if(min_y > target->y)
				{
					min_y = target->y;
				}
				if(max_x < target->x + target->width)
				{
					max_x = target->x + target->width;
				}
				if(max_y < target->y + target->height)
				{
					max_y = target->y + target->height;
				}
				target = target->next;
			}
			width = max_x - min_x;
			height = max_y - min_y;

			// ファイルパスからファイル名だけを取り出す
			file_name = str = file_path;
			while(*str != '\0')
			{
				if(*str == '/' || *str == '\\')
				{
					file_name = str+1;
				}

				str = g_utf8_next_char(str);
			}
			app->draw_window[app->window_num] = window =
				CreateDrawWindow(width, height, 4, file_name,
					app->note_book, app->window_num, app);
			DeleteLayer(&window->layer);
			window->layer = CreateLayer(0, 0, window->width, window->height,
				4, TYPE_NORMAL_LAYER, NULL, NULL,
				layer->name, window);
			window->layer->flags = layer->flags;
			target = layer->next;
			canvas_layer = window->layer;
			while(target != NULL)
			{
				canvas_layer->next = CreateLayer(0, 0, window->width, window->height, 4, TYPE_NORMAL_LAYER,
					canvas_layer, NULL, target->name, window);
				canvas_layer = canvas_layer->next;
				canvas_layer->flags = target->flags;
				canvas_layer->alpha = target->alpha;
				target = target->next;
			}
			target = layer;
			window->num_layer = 0;
			// レイヤー数の数え上げと透明色の調整
			while(target != NULL)
			{
				window->num_layer++;
				for(i=0; i<target->width * target->height; i++)
				{
					alpha = target->pixels[i*4+3];
					target->pixels[i*4+0] = MINIMUM(alpha, target->pixels[i*4+0]);
					target->pixels[i*4+1] = MINIMUM(alpha, target->pixels[i*4+1]);
					target->pixels[i*4+2] = MINIMUM(alpha, target->pixels[i*4+2]);
				}
				target = target->next;
			}
			target = layer;
			canvas_layer = window->layer;
			while(target != NULL)
			{
				int copy_start_x, copy_start_y;
				copy_start_x = target->x - min_x;
				copy_start_y = target->y - min_y;
				for(i=0; i<target->height; i++)
				{
					(void)memcpy(&canvas_layer->pixels[(copy_start_y+i)*canvas_layer->stride + copy_start_x*4],
						&target->pixels[i*target->stride], target->stride);
				}
				canvas_layer = canvas_layer->next;
				target = target->next;
			}

			// 読み込みに使ったレイヤー情報を削除
			target = layer;
			while(target != NULL)
			{
				layer = target->next;
				DeleteTempLayer(&target);
				target = layer;
			}
			(void)fclose(fp);

			app->active_window = app->window_num;
			app->window_num++;

			layer = window->layer;
			for(i=0; i<window->num_layer; i++)
			{
				if(layer == NULL)
				{
					window->num_layer = i;
					break;
				}
				LayerViewAddLayer(layer, window->layer, app->layer_window.view, i+1);
				layer = layer->next;
			}

			// ファイルパスを設定
			window->file_path = MEM_STRDUP_FUNC(file_path);

			// ナビゲーションの表示を更新
			ChangeNavigationDrawWindow(&app->navigation_window, window);

			// 一番下のレイヤーをアクティブ表示に
			LayerViewSetActiveLayer(
				window->active_layer, app->layer_window.view
			);

			// 無効にしていた一部のメニューを有効に
			for(i=0; i<app->menus.num_disable_if_no_open; i++)
			{
				gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
			}
			gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
			gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);

			// ウィンドウのタイトルを画像名に
			gtk_window_set_title(GTK_WINDOW(app->window), file_name);
		}
	}
	else if(StringCompareIgnoreCase(str, ".dds") == 0)
	{
		// ファイル読み込みストリーム
		gchar *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
		FILE *fp = fopen(system_path, "rb");

		if(fp != NULL)
		{
			uint8 *pixels;
			int width, height;
			int channel;
			size_t file_size;

			(void)fseek(fp, 0, SEEK_END);
			file_size = ftell(fp);
			rewind(fp);

			pixels = ReadDdsStream((void*)fp, (stream_func_t)fread,
				(seek_func_t)fseek, file_size, &width, &height, &channel);

			if(pixels != NULL)
			{
				// ピクセルデータを写すレイヤー
				LAYER *target;
				// 1行分のバイト数
				int stride = width * 4;
				// 上下反転用
				uint8 *trans = (uint8*)MEM_ALLOC_FUNC(width*height*4);

				// ファイルパスからファイル名だけを取り出す
				file_name = str = file_path;
				while(*str != '\0')
				{
					if(*str == '/' || *str == '\\')
					{
						file_name = str+1;
					}

					str = g_utf8_next_char(str);
				}

				if(channel == 1)
				{
					uint8 *old_pixels = pixels;
					pixels = (uint8*)MEM_ALLOC_FUNC(width*height*4);
					for(i=0; i<width*height; i++)
					{
						pixels[i*4] = old_pixels[i];
						pixels[i*4+1] = old_pixels[i];
						pixels[i*4+2] = old_pixels[i];
						pixels[i*4+3] = 0xff;
					}

					MEM_FREE_FUNC(old_pixels);
				}
				else if(channel == 2)
				{
					uint8 *old_pixels = pixels;
					pixels = (uint8*)MEM_ALLOC_FUNC(width*height*4);
					for(i=0; i<width*height; i++)
					{
						pixels[i*4] = old_pixels[i*2];
						pixels[i*4+1] = old_pixels[i*2];
						pixels[i*4+2] = old_pixels[i*2];
						pixels[i*4+3] = old_pixels[i*2+1];
					}

					MEM_FREE_FUNC(old_pixels);
				}
				else if(channel == 3)
				{
					uint8 *old_pixels = pixels;
					pixels = (uint8*)MEM_ALLOC_FUNC(width*height*4);
					for(i=0; i<width*height; i++)
					{
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
						pixels[i*4] = old_pixels[i*3];
						pixels[i*4+1] = old_pixels[i*3+1];
						pixels[i*4+2] = old_pixels[i*3+2];
#else
						pixels[i*4] = old_pixels[i*3+2];
						pixels[i*4+1] = old_pixels[i*3+1];
						pixels[i*4+2] = old_pixels[i*3];
#endif
						pixels[i*4+3] = 0xff;
					}

					MEM_FREE_FUNC(old_pixels);
				}
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
				else
				{
					uint8 b;
					for(i=0; i<width*height; i++)
					{
						b = pixels[i*4];
						pixels[i*4] = pixels[i*4+2];
						pixels[i*4+2] = b;
					}
				}
#endif
				for(i=0; i<height; i++)
				{
					(void)memcpy(&trans[stride*i], &pixels[(height-i-1)*stride], stride);
				}
				MEM_FREE_FUNC(pixels);
				pixels = trans;

				// 描画領域を新たに追加
				app->draw_window[app->window_num] = CreateDrawWindow(
					width, height, 4, file_name, app->note_book, app->window_num, app);

				app->active_window = app->window_num;
				app->window_num++;

				// ファイルパスを設定
				app->draw_window[app->active_window]->file_path = MEM_STRDUP_FUNC(file_path);

				// ナビゲーションの表示を更新
				ChangeNavigationDrawWindow(&app->navigation_window, app->draw_window[app->active_window]);

				// 画像データを一番下のレイヤーにコピー
				target = app->draw_window[app->active_window]->active_layer;

				for(i=0; i<height; i++)
				{
					for(j=0; j<width; j++)
					{
						target->pixels[target->stride*i+target->channel*j] =
							pixels[(height-i-1)*stride+j*target->channel+2];
						target->pixels[target->stride*i+target->channel*j+1] =
							pixels[(height-i-1)*stride+j*target->channel+1];
						target->pixels[target->stride*i+target->channel*j+2] =
							pixels[(height-i-1)*stride+j*target->channel];
						target->pixels[target->stride*i+target->channel*j+3] =
							pixels[(height-i-1)*stride+j*target->channel+3];
					}
				}

				// 一番下のレイヤーをアクティブ表示に
				LayerViewSetActiveLayer(
					app->draw_window[app->active_window]->active_layer, app->layer_window.view
				);

				// 無効にしていた一部のメニューを有効に
				for(i=0; i<app->menus.num_disable_if_no_open; i++)
				{
					gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
				}
				gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
				gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);

				// ウィンドウのタイトルを画像名に
				gtk_window_set_title(GTK_WINDOW(app->window), file_name);
			}
		}

		g_free(system_path);
	}
	else
	{
		GError *error = NULL;
		LAYER *target;
		pixbuf = gdk_pixbuf_new_from_file(file_path, &error);

		// 変換成功
		if(pixbuf != NULL)
		{	// 画像の幅、高さ、ピクセルのデータを取得
			int32 width = gdk_pixbuf_get_width(pixbuf),
				height = gdk_pixbuf_get_height(pixbuf);
			uint8* pixels;
			int stride;
			const int channel = 4;

			// ファイルパスからファイル名だけを取り出す
			file_name = str = file_path;
			while(*str != '\0')
			{
				if(*str == '/' || *str == '\\')
				{
					file_name = str+1;
				}

				str = g_utf8_next_char(str);
			}

			// αチャンネルがなければ追加
			if(gdk_pixbuf_get_has_alpha(pixbuf) == FALSE)
			{
				GdkPixbuf *old_buf = pixbuf;
				pixbuf = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
				g_object_unref(old_buf);
			}

			// ピクセルと画像一列分のバイト数を取得
			pixels = gdk_pixbuf_get_pixels(pixbuf);
			stride = gdk_pixbuf_get_rowstride(pixbuf);

			// 描画領域を新たに追加
			app->draw_window[app->window_num] = CreateDrawWindow(
				width, height, 4, file_name, app->note_book, app->window_num, app);

			app->active_window = app->window_num;
			// ファイルパスを設定
			app->draw_window[app->active_window]->file_path = MEM_STRDUP_FUNC(file_path);

			// ナビゲーションの表示を更新
			ChangeNavigationDrawWindow(&app->navigation_window, app->draw_window[app->active_window]);

			// 画像データを一番下のレイヤーにコピー
			target = app->draw_window[app->active_window]->active_layer;
			for(i=0; i<height; i++)
			{
				for(j=0; j<width; j++)
				{
					target->pixels[target->stride*i+target->channel*j] =
						pixels[i*stride+j*target->channel+2];
					target->pixels[target->stride*i+target->channel*j+1] =
						pixels[i*stride+j*target->channel+1];
					target->pixels[target->stride*i+target->channel*j+2] =
						pixels[i*stride+j*target->channel];
					target->pixels[target->stride*i+target->channel*j+3] =
						pixels[i*stride+j*target->channel+3];
				}
			}

			// 一番下のレイヤーをアクティブ表示に
			LayerViewSetActiveLayer(
				app->draw_window[app->active_window]->active_layer, app->layer_window.view
			);

			// ピクセルバッファは不要
			g_object_unref(pixbuf);

			// 無効にしていた一部のメニューを有効に
			for(i=0; i<app->menus.num_disable_if_no_open; i++)
			{
				gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
			}
			gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
			gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);

			// ウィンドウのタイトルを画像名に
			gtk_window_set_title(GTK_WINDOW(app->window), file_name);

			// 解像度データを仮で設定
			app->draw_window[app->active_window]->resolution = DEFALUT_RESOLUTION;

			// 解像度データ、ICCプロファイルデータを可能なら読み込む
			if(StringCompareIgnoreCase(extention, ".jpg") == 0
				|| StringCompareIgnoreCase(extention, ".jpeg") == 0)
			{
				char *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
				int resolution = 0;
				uint8 *icc_profile_data;
				int32 icc_profile_size;

				ReadJpegHeader(system_path, NULL, NULL, &resolution,
					&icc_profile_data, &icc_profile_size);
				if(resolution != 0)
				{
					app->draw_window[app->active_window]->resolution = resolution;
				}

				if(icc_profile_data != NULL)
				{
					app->draw_window[app->active_window]->icc_profile_data = icc_profile_data;
					DrawWindowSetIccProfile(app->draw_window[app->active_window], icc_profile_size, TRUE);
				}

				g_free(system_path);
			}
			else if(StringCompareIgnoreCase(extention, ".png") == 0)
			{
				char *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
				FILE *fp = fopen(system_path, "rb");
				int32 resolution = 0;
				uint8 *icc_profile_data;
				int32 icc_profile_size;

				ReadPNGHeader((void*)fp, (stream_func_t)fread, NULL, NULL, NULL,
					&resolution, NULL, &icc_profile_data, &icc_profile_size);

				if(resolution != 0)
				{
					app->draw_window[app->active_window]->resolution = (int16)resolution;
				}

				if(icc_profile_data != NULL)
				{
					app->draw_window[app->active_window]->icc_profile_data = icc_profile_data;
					DrawWindowSetIccProfile(app->draw_window[app->active_window], icc_profile_size, TRUE);
				}

				g_free(system_path);
				(void)fclose(fp);
			}
			else if(StringCompareIgnoreCase(extention, ".tif") == 0
				|| StringCompareIgnoreCase(extention, ".tiff") == 0)
			{
				char *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
				FILE *fp = fopen(system_path, "rb");
				int resolution = 0;
				uint8 *icc_profile_data = NULL;
				int32 icc_profile_size = 0;

				ReadTiffTagData(system_path, &resolution, &icc_profile_data, &icc_profile_size);

				if(resolution != 0)
				{
					app->draw_window[app->active_window]->resolution = (int16)resolution;
				}

				if(icc_profile_data != NULL)
				{
					app->draw_window[app->active_window]->icc_profile_data = icc_profile_data;
					DrawWindowSetIccProfile(app->draw_window[app->active_window], icc_profile_size, TRUE);
				}

				g_free(system_path);
			}

			// 描画領域のカウンタを更新
			app->window_num++;
		}
		else
		{
			(void)g_printerr("File Open Error : %s\n", error->message);
		}

		g_error_free(error);
	}
}

/*********************************************************
* ExecuteOpenFile関数                                    *
* ファイルを開く                                         *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void ExecuteOpenFile(APPLICATION* app)
{	// ファイルを開くダイアログ
	GtkWidget* chooser = gtk_file_chooser_dialog_new(
		app->labels->menu.open,
		GTK_WINDOW(app->window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL
	);

	// プレビューをセット
	SetFileChooserPreview(chooser);

	if(gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT)
	{	// ファイルが開かれた
		gchar* file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
		
		OpenFile(file_path, app);

		g_free(file_path);
	}

	// ファイル選択ダイアログを閉じる
	gtk_widget_destroy(chooser);
}

/*********************************************************
* ExecuteOpenFileAsLayer関数                             *
* レイヤーとしてファイルを開く                           *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void ExecuteOpenFileAsLayer(APPLICATION* app)
{	// ファイルを開くダイアログ
	GtkWidget* chooser = gtk_file_chooser_dialog_new(
		app->labels->menu.open,
		GTK_WINDOW(app->window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL
	);

	// プレビューをセット
	SetFileChooserPreview(chooser);

	if(gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT)
	{	// ファイルが開かれた
		gchar* file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
		// ピクセルバッファに変換
		GdkPixbuf *pixbuf;
		// レイヤーを追加するキャンバス
		DRAW_WINDOW *window = GetActiveDrawWindow(app);
		// 追加するレイヤー
		LAYER *layer;
		// ファイル名取得用
		gchar *str;
		// ファイル名
		gchar *file_name;
		// レイヤーの名前
		char layer_name[4096];
		// for文用のカウンタ
		int i;

		pixbuf = gdk_pixbuf_new_from_file(file_path, NULL);

		// 変換成功
		if(pixbuf != NULL)
		{	// 画像の幅、高さ、ピクセルのデータを取得
			int32 width = gdk_pixbuf_get_width(pixbuf),
				height = gdk_pixbuf_get_height(pixbuf);
			uint8* pixels;
			int stride, original_stride;
			const int channel = 4;

			// ファイルパスからファイル名だけを取り出す
			file_name = file_path;
			str = file_path;
			while(*str != '\0')
			{
				if(*str == '/' || *str == '\\')
				{
					file_path = str+1;
				}

				str = g_utf8_next_char(str);
			}

			// αチャンネルがなければ追加
			if(gdk_pixbuf_get_has_alpha(pixbuf) == FALSE)
			{
				pixbuf = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
			}

			// ピクセルと画像一列分のバイト数を取得
			pixels = gdk_pixbuf_get_pixels(pixbuf);
			original_stride = stride = gdk_pixbuf_get_rowstride(pixbuf);

			// レイヤーの名前を作成
			i = 1;
			(void)strcpy(layer_name, file_name);
			while(CorrectLayerName(app->draw_window[app->active_window]->layer, layer_name) == 0)
			{
				(void)sprintf(layer_name, "%s (%d)", file_name, i);
			}

			// アクティブな描画領域にレイヤーを追加
			layer = CreateLayer(0, 0, app->draw_window[app->active_window]->width,
				window->height, 4, TYPE_NORMAL_LAYER,
				window->active_layer,
				window->active_layer->next,
				layer_name,
				window
			);
			window->num_layer++;

#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			{
				uint8 r;
				for(i=0; i<width*height; i++)
				{
					r = pixels[i*4];
					pixels[i*4] = pixels[i*4+2];
					pixels[i*4+2] = r;
				}
			}
#endif

			// 画像データを追加したレイヤーにコピー
			if(width > layer->width)
			{
				width = layer->width;
				stride = layer->width * 4;
			}

			if(height > layer->height)
			{
				height = layer->height;
			}

			for(i=0; i<height; i++)
			{
				(void)memcpy(&layer->pixels[layer->stride*i],
						&pixels[i*original_stride], stride
				);
			}

			// 履歴データを追加
			AddNewLayerWithImageHistory(layer, pixels, width, height,
				original_stride, 4, app->labels->menu.open_as_layer);

			// 追加したレイヤーをアクティブに
			LayerViewAddLayer(layer, window->layer,
				app->layer_window.view, window->num_layer);
			ChangeActiveLayer(window, layer);
			LayerViewSetActiveLayer(
				window->active_layer, app->layer_window.view
			);

			// ピクセルバッファは不要
			g_object_unref(pixbuf);
		}

		g_free(file_path);
	}

	// ファイル選択ダイアログを閉じる
	gtk_widget_destroy(chooser);
}

/*********************************************************
* ExecuteSave関数                                        *
* 上書き保存                                             *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void ExecuteSave(APPLICATION* app)
{
	// ファイルパスがアクティブな描画領域に設定されてい
		// あるいは書き出しサポート外の拡張子なら名前を付けて保存
	// 拡張子
	char* extention;
	// 名前を付けて保存するフラグ
	int save_as = 0;
	
	if(app->draw_window[app->active_window]->file_path == NULL)
	{
		save_as++;
	}
	else
	{
		// 作業用ポインタ
		char* str;

		extention = str = app->draw_window[app->active_window]->file_path;
		while(*str != '\0')
		{
			if(*str == '.')
			{
				extention = str+1;
			}

			str = g_utf8_next_char(str);
		}

		if(IsSupportFileType(extention) == 0)
		{
			save_as++;
		}
	}

	// 名前を付けて保存
	if(save_as != 0)
	{
		ExecuteSaveAs(app);
	}
	else
	{	// 上書き保存
		Save(app, app->draw_window[app->active_window],
			app->draw_window[app->active_window]->file_path, extention);
	}
}

/*********************************************************
* ExecuteSaveAs関数                                      *
* 名前を付けて保存                                       *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void ExecuteSaveAs(APPLICATION* app)
{	// ファイル選択ダイアログ
	GtkWidget* chooser;
	// ファイルタイプのフィルター
	GtkFileFilter* filter;

	if(app->window_num < 1)
	{
		return;
	}

	chooser = gtk_file_chooser_dialog_new(
		app->labels->menu.save_as,
		GTK_WINDOW(app->window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		NULL
	);

	// 上書きする前に警告を出す
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser), TRUE);
	// ファイルタイプのフィルターをセット
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Original Format");
	gtk_file_filter_add_pattern(filter, "*.kab");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Portable Network Graphic (PNG)");
	gtk_file_filter_add_pattern(filter, "*.png");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Joint Photographic Experts Group (JPEG)");
	gtk_file_filter_add_pattern(filter, "*.jpg");
	gtk_file_filter_add_pattern(filter, "*.jpeg");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "PhotoShop Document (PSD)");
	gtk_file_filter_add_pattern(filter, "*.psd");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Tagged Image File Format (TIFF)");
	gtk_file_filter_add_pattern(filter, "*.tif");
	gtk_file_filter_add_pattern(filter, "*.tiff");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "TLG");
	gtk_file_filter_add_pattern(filter, "*.tlg");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

	// プレビューを設定
	SetFileChooserPreview(chooser);

	if(app->draw_window[app->active_window]->file_path != NULL)
	{
		char *directly = g_path_get_dirname(app->draw_window[app->active_window]->file_path);
		gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), directly);
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(chooser),
			app->draw_window[app->active_window]->file_name);
		g_free(directly);
	}

	if(gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT)
	{	// 保存ファイル名が決定された
			// ファイルパス
		const gchar* path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
		// ファイルフィルタの情報
		const gchar* filter_name;
		// ファイル名
		const char *file_name;
		// ファイル形式
		const char *file_type;
		// 作業用ポインタ
		const char* str;

		// 選択されているファイルのフィルターを取得
		filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(chooser));
		filter_name = gtk_file_filter_get_name(filter);
		if(strcmp(filter_name, "Original Format") == 0)
		{	// 独自形式
			file_type = "kab";
		}
		else if(strcmp(filter_name, "Portable Network Graphic (PNG)") == 0)
		{	// PNG
			file_type = "png";
		}
		else if(strcmp(filter_name, "Joint Photographic Experts Group (JPEG)") == 0)
		{	// JPEG
			file_type = "jpg";
		}
		else if(strcmp(filter_name, "PhotoShop Document (PSD)") == 0)
		{	// PSD
			file_type = "psd";
		}
		else if(strcmp(filter_name, "Tagged Image File Format (TIFF)") == 0)
		{	// TIFF
			file_type = "tif";
		}
		else if(strcmp(filter_name, "TLG") == 0)
		{	// TLG
			file_type = "tlg";
		}

		// 書き出し実行
		path = Save(app, app->draw_window[app->active_window], path, file_type);

		// ファイル名を取得
		str = path;
		while(*str != '\0')
		{
			if(*str == '/' || *str == '\\')
			{
				file_name = str+1;
			}

			str = g_utf8_next_char(str);
		}

		// ファイル名、ファイルパスをを更新
		MEM_FREE_FUNC(app->draw_window[app->active_window]->file_name);
		MEM_FREE_FUNC(app->draw_window[app->active_window]->file_path);

		app->draw_window[app->active_window]->file_name = MEM_STRDUP_FUNC(file_name);
		app->draw_window[app->active_window]->file_path = (char*)path;

		// ウィンドウのタイトルを変更
		gtk_window_set_title(GTK_WINDOW(app->window), file_name);
	}

	gtk_widget_destroy(chooser);
}

/*********************************************************
* ExecuteClose関数                                       *
* アクティブな描画領域を閉じる                           *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void ExecuteClose(APPLICATION* app)
{
	// 閉じるタブのID
	int close_page = app->active_window;

	if(OnCloseDrawWindow((void*)app->draw_window[app->active_window],
		app->active_window) == FALSE)
	{
		gtk_notebook_remove_page(GTK_NOTEBOOK(app->note_book), close_page);
	}
}

/*********************************************************
* DeleteActiveLayer関数                                  *
* 現在のアクティブレイヤーを削除する                     *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void DeleteActiveLayer(APPLICATION* app)
{
	// レイヤー削除を実行する描画領域
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// アクティブレイヤーの次のレイヤー、削除するレイヤー
	LAYER* next_active, *delete_layer = window->active_layer;
	// for文用のカウンタ
	int i;

	//AUTO_SAVE(app->draw_window[app->active_window]);
	// レイヤーが0個にならないようにチェック
	if(window->num_layer <= 1)
	{
		return;
	}

	// 削除履歴を残す
	AddDeleteLayerHistory(window, delete_layer);

	// 局所キャンバスモードなら消したレイヤーの名前を覚える
	if((window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		LAYER *parent_delete = SearchLayer(app->draw_window[app->active_window]->layer, delete_layer->name);
		if(parent_delete != NULL)
		{
			DeleteLayer(&parent_delete);
		}
		app->draw_window[app->active_window]->num_layer--;
	}

	if(window->layer == window->active_layer)
	{	// 一番下のレイヤーを削除する場合描画領域の一番下のレイヤーを入れ替え
			// 削除後の一番下のレイヤーをアクティブにする
		next_active = window->layer->next;
		ChangeActiveLayer(window, next_active);
		DeleteLayer(&delete_layer);
		window->layer = next_active;
	}
	else
	{	// アクティブレイヤーを削除して削除されたレイヤーの下のレイヤーをアクティブに
		next_active = window->active_layer->prev;
		ChangeActiveLayer(window, next_active);
		DeleteLayer(&delete_layer);
	}
	window->num_layer--;

	// アクティブレイヤーの表示を更新
	LayerViewSetActiveLayer(next_active, app->layer_window.view);

	// レイヤーが一個のみになったらウィジェットの無効化処理
	if(window->num_layer <= 1)
	{
		for(i=0; i<app->menus.num_disable_if_single_layer; i++)
		{
			gtk_widget_set_sensitive(app->menus.disable_if_single_layer[i], FALSE);
		}
	}
}

/***************************************
* FILL_FORE_GROUND_COLOR_HISTORY構造体 *
* 描画色で塗り潰す履歴データ           *
***************************************/
typedef struct _FILL_FORE_GROUND_COLOR_HISTORY
{
	uint8 color[3];		// 塗り潰す色
	size_t data_size;	// ピクセルデータのサイズ
	int16 name_length;	// 塗り潰すレイヤーの名前の長さ
	char *layer_name;	// 塗り潰すレイヤーの名前
	uint8 *pixel_data;	// 塗り潰す前のピクセルデータ
} FILL_FORE_GROUND_COLOR_HISTORY;

void FillForeGroundColorUndo(DRAW_WINDOW* window, void* p)
{
	FILL_FORE_GROUND_COLOR_HISTORY* history = (FILL_FORE_GROUND_COLOR_HISTORY*)p;
	MEMORY_STREAM stream;
	LAYER *target = window->layer;
	int32 width, height, stride;
	uint8 *buff = (uint8*)p;
	uint8 *image = &buff[offsetof(FILL_FORE_GROUND_COLOR_HISTORY, layer_name) + history->name_length+1];
	uint8 *pixels;

	while(strcmp(target->name, (const char*)&buff[offsetof(FILL_FORE_GROUND_COLOR_HISTORY, layer_name)]) != 0)
	{
		target = target->next;
	}

	stream.buff_ptr = image;
	stream.data_size = history->data_size;
	stream.data_point = 0;

	pixels = ReadPNGStream(&stream, (stream_func_t)MemRead, &width, &height, &stride);
	(void)memcpy(target->pixels, pixels, target->stride*target->height);

	MEM_FREE_FUNC(pixels);
}

void FillForeGroundColorRedo(DRAW_WINDOW* window, void* p)
{
	FILL_FORE_GROUND_COLOR_HISTORY* history = (FILL_FORE_GROUND_COLOR_HISTORY*)p;
	LAYER *target = window->layer;
	uint8 *buff = (uint8*)p;

	while(strcmp(target->name, (const char*)&buff[offsetof(FILL_FORE_GROUND_COLOR_HISTORY, layer_name)]) != 0)
	{
		target = target->next;
	}

	FillLayerColor(target, history->color);
}

void AddFillForeGourndColorHistory(LAYER* target, uint8 color[3])
{
	MEMORY_STREAM_PTR image = CreateMemoryStream(target->stride*target->height);
	MEMORY_STREAM_PTR data = CreateMemoryStream(target->stride*target->height);
	FILL_FORE_GROUND_COLOR_HISTORY history;

	// ピクセルデータをPNG圧縮
	WritePNGStream(image, (stream_func_t)MemWrite, NULL, target->pixels,
		target->width, target->height, target->stride, 4, 0, target->window->app->preference.compress);

	(void)memcpy(history.color, color, 3);
	history.data_size = image->data_point;
	history.name_length = (int16)strlen(target->name)+1;
	(void)MemWrite(&history, 1, offsetof(FILL_FORE_GROUND_COLOR_HISTORY, layer_name), data);
	(void)MemWrite(target->name, 1, history.name_length+1, data);

	(void)MemWrite(image->buff_ptr, 1, image->data_point, data);

	AddHistory(&target->window->history, target->window->app->labels->menu.fill_layer_fg_color,
		data->buff_ptr, (uint32)data->data_point, FillForeGroundColorUndo, FillForeGroundColorRedo);

	(void)DeleteMemoryStream(image);
	(void)DeleteMemoryStream(data);
}

/*********************************************************
* FillForeGroundColor関数                                *
* 描画色でアクティブレイヤーを塗りつぶす                 *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void FillForeGroundColor(APPLICATION* app)
{
	if(app->draw_window[app->active_window]->active_layer->layer_type != TYPE_NORMAL_LAYER)
	{
		return;
	}

	AddFillForeGourndColorHistory(app->draw_window[app->active_window]->active_layer,
		app->tool_window.color_chooser->rgb);

	FillLayerColor(
		app->draw_window[app->active_window]->active_layer,
		app->tool_window.color_chooser->rgb
	);
}

typedef struct _FILL_PATTERN_HISTORY_DATA
{
	size_t before_data_size;	// 塗り潰し前のピクセルデータのサイズ
	size_t after_data_size;		// 塗り潰し後ピクセルデータのサイズ
	int16 name_length;			// 塗り潰すレイヤーの名前の長さ
	char *layer_name;			// 塗り潰すレイヤーの名前
	uint8 *pixel_data;			// 塗り潰す前のピクセルデータ
} FILL_PATTERN_HISTORY_DATA;

void FillPatternUndo(DRAW_WINDOW* window, void* p)
{
	FILL_PATTERN_HISTORY_DATA* history = (FILL_PATTERN_HISTORY_DATA*)p;
	MEMORY_STREAM stream;
	LAYER *target = window->layer;
	int32 width, height, stride;
	uint8 *buff = (uint8*)p;
	uint8 *image = &buff[offsetof(FILL_PATTERN_HISTORY_DATA, layer_name) + history->name_length+1];
	uint8 *pixels;

	while(strcmp(target->name, (const char*)&buff[offsetof(FILL_PATTERN_HISTORY_DATA, layer_name)]) != 0)
	{
		target = target->next;
	}

	stream.buff_ptr = image;
	stream.data_size = history->before_data_size;
	stream.data_point = 0;

	pixels = ReadPNGStream(&stream, (stream_func_t)MemRead, &width, &height, &stride);
	(void)memcpy(target->pixels, pixels, target->stride*target->height);

	MEM_FREE_FUNC(pixels);
}

void FillPatternRedo(DRAW_WINDOW* window, void* p)
{
	FILL_PATTERN_HISTORY_DATA* history = (FILL_PATTERN_HISTORY_DATA*)p;
	MEMORY_STREAM stream;
	LAYER *target = window->layer;
	int32 width, height, stride;
	uint8 *buff = (uint8*)p;
	uint8 *image = &buff[offsetof(FILL_PATTERN_HISTORY_DATA, layer_name)
		+ history->name_length+1 + history->before_data_size];
	uint8 *pixels;

	while(strcmp(target->name, (const char*)&buff[offsetof(FILL_PATTERN_HISTORY_DATA, layer_name)]) != 0)
	{
		target = target->next;
	}

	stream.buff_ptr = image;
	stream.data_size = history->after_data_size;
	stream.data_point = 0;

	pixels = ReadPNGStream(&stream, (stream_func_t)MemRead, &width, &height, &stride);
	(void)memcpy(target->pixels, pixels, target->stride*target->height);

	MEM_FREE_FUNC(pixels);
}

void AddFillPatternHistory(LAYER* target, LAYER* after)
{
	MEMORY_STREAM_PTR before_image = CreateMemoryStream(target->stride*target->height);
	MEMORY_STREAM_PTR after_image = CreateMemoryStream(target->stride*target->height);
	MEMORY_STREAM_PTR data = CreateMemoryStream(target->stride*target->height);
	FILL_PATTERN_HISTORY_DATA history = {0};

	// 塗り潰し前のピクセルデータをPNG圧縮
	WritePNGStream(before_image, (stream_func_t)MemWrite, NULL, target->pixels,
		target->width, target->height, target->stride, 4, 0, target->window->app->preference.compress);
	
	// 塗り潰し後のピクセルデータをPNG圧縮
	WritePNGStream(after_image, (stream_func_t)MemWrite, NULL, after->pixels,
		target->width, target->height, target->stride, 4, 0, target->window->app->preference.compress);

	history.before_data_size = before_image->data_point;
	history.after_data_size = after_image->data_point;
	history.name_length = (int16)strlen(target->name)+1;
	(void)MemWrite(&history, 1, offsetof(FILL_PATTERN_HISTORY_DATA, layer_name), data);
	(void)MemWrite(target->name, 1, history.name_length+1, data);
	(void)MemWrite(before_image->buff_ptr, 1, before_image->data_point, data);
	(void)MemWrite(after_image->buff_ptr, 1, after_image->data_point, data);

	AddHistory(&target->window->history, target->window->app->labels->menu.fill_layer_fg_color,
		data->buff_ptr, (uint32)data->data_point, FillPatternUndo, FillPatternRedo);

	(void)DeleteMemoryStream(before_image);
	(void)DeleteMemoryStream(after_image);
	(void)DeleteMemoryStream(data);
}

/*********************************************************
* FillPattern関数                                        *
* アクティブなパターンでアクティブなレイヤーを塗り潰す   *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void FillPattern(APPLICATION* app)
{
	DRAW_WINDOW *window = app->draw_window[app->active_window];

	if(window->active_layer->layer_type != TYPE_NORMAL_LAYER)
	{
		return;
	}

	(void)memcpy(window->work_layer->pixels, window->active_layer->pixels,
		window->pixel_buf_size);

	FillLayerPattern(
		window->work_layer,
		&app->patterns,
		app,
		app->tool_window.color_chooser->rgb
	);

	AddFillPatternHistory(window->active_layer, window->work_layer);

	(void)memcpy(window->active_layer->pixels, window->work_layer->pixels,
		window->pixel_buf_size);
	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
}

/*********************************************************
* FlipImageHorizontally関数                              *
* 描画領域を左右反転する                                 *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void FlipImageHorizontally(APPLICATION* app)
{
	FlipDrawWindowHorizontally(app->draw_window[app->active_window]);

	AddHistory(&app->draw_window[app->active_window]->history, app->labels->menu.flip_canvas_horizontally,
		NULL, 0, (history_func)FlipDrawWindowHorizontally, (history_func)FlipDrawWindowHorizontally);
}

/*********************************************************
* FlipImageVertically関数                                *
* 描画領域を上下反転する                                 *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void FlipImageVertically(APPLICATION* app)
{
	FlipDrawWindowVertically(app->draw_window[app->active_window]);

	AddHistory(&app->draw_window[app->active_window]->history, app->labels->menu.flip_canvas_vertically,
		NULL, 0, (history_func)FlipDrawWindowVertically, (history_func)FlipDrawWindowVertically);
}

/*****************************************************
* SwitchSecondBackColor関数                          *
* 背景色を2つめのものと入れ替える                    *
* 引数                                               *
* menu	: メニューウィジェット                       *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void SwitchSecondBackColor(GtkWidget* menu, APPLICATION* app)
{
	if((app->flags & APPLICATION_IN_SWITCH_DRAW_WINDOW) == 0)
	{
		if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu)) == FALSE)
		{
			(void)memset(app->draw_window[app->active_window]->back_ground,
				0xff, app->draw_window[app->active_window]->pixel_buf_size);
			app->draw_window[app->active_window]->flags &= ~(DRAW_WINDOW_SECOND_BG);
		}
		else
		{
			int i;

			for(i=0; i<app->draw_window[app->active_window]->width * app->draw_window[app->active_window]->height; i++)
			{
				app->draw_window[app->active_window]->back_ground[i*4+0] = app->draw_window[app->active_window]->second_back_ground[0];
				app->draw_window[app->active_window]->back_ground[i*4+1] = app->draw_window[app->active_window]->second_back_ground[1];
				app->draw_window[app->active_window]->back_ground[i*4+2] = app->draw_window[app->active_window]->second_back_ground[2];
				app->draw_window[app->active_window]->back_ground[i*4+3] = 0xff;
			}

			app->draw_window[app->active_window]->flags |= DRAW_WINDOW_SECOND_BG;
		}

		app->draw_window[app->active_window]->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
		gtk_widget_queue_draw(app->draw_window[app->active_window]->window);

		if(app->layer_window.change_bg_button != NULL)
		{
			app->flags |= APPLICATION_IN_SWITCH_DRAW_WINDOW;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->layer_window.change_bg_button),
				gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu)));
			app->flags &= ~(APPLICATION_IN_SWITCH_DRAW_WINDOW);
		}
	}
}

/*****************************************************
* Change2ndBackColor関数                             *
* 2つめの背景色を変更する                            *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void Change2ndBackColor(APPLICATION* app)
{
	DRAW_WINDOW *window = app->draw_window[app->active_window];
	GtkWidget *dialog = gtk_color_selection_dialog_new(app->labels->menu.change_2nd_bg_color);
	GtkWidget *selection = gtk_color_selection_dialog_get_color_selection(
		GTK_COLOR_SELECTION_DIALOG(dialog));
	GdkColor color;

	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);

	color.red = (app->menu_data.make_new.second_bg_color[0] << 8) | app->menu_data.make_new.second_bg_color[0];
	color.green = (app->menu_data.make_new.second_bg_color[1] << 8) | app->menu_data.make_new.second_bg_color[1];
	color.blue = (app->menu_data.make_new.second_bg_color[2] << 8) | app->menu_data.make_new.second_bg_color[2];

	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(selection), &color);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(selection), &color);
		app->menu_data.make_new.second_bg_color[0] = color.red / 256;
		app->menu_data.make_new.second_bg_color[1] = color.green / 256;
		app->menu_data.make_new.second_bg_color[2] = color.blue / 256;

#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
		window->second_back_ground[2] = color.red / 256;
		window->second_back_ground[1] = color.green / 256;
		window->second_back_ground[0] = color.blue / 256;
#else
		window->second_back_ground[0] = color.red / 256;
		window->second_back_ground[1] = color.green / 256;
		window->second_back_ground[2] = color.blue / 256;
#endif
	}

	gtk_widget_destroy(dialog);
}

/*********************************************************
* MergeDownActiveLayer関数                               *
* アクティブなレイヤーを下のレイヤーと結合する           *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void MergeDownActiveLayer(APPLICATION* app)
{
	// アクティブな描画領域
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// 結合後にアクティブになるレイヤー
	LAYER* next_active = window->active_layer->prev;

	// 局所キャンバスモードなら親キャンバスを先に処理
	if((window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		DRAW_WINDOW *parent = window->focal_window;
		LAYER *parent_target = SearchLayer(parent->layer, window->active_layer->name);
		LAYER *parent_next_active = parent->active_layer;
		if(parent_target != NULL)
		{
			if(parent_next_active == parent_target)
			{
				parent_next_active = parent_target->prev;
			}
			if(parent_next_active == NULL)
			{
				parent_next_active = parent->layer;
			}
			// 履歴データの作成
			AddLayerMergeDownHistory(
				parent,
				parent_target
			);

			// レイヤー結合を実行して
			LayerMergeDown(parent_target);

			// アクティブなレイヤーを変更する
			parent->active_layer = parent_next_active;
		}
	}

	// 履歴データの作成
	AddLayerMergeDownHistory(
		window,
		window->active_layer
	);

	// レイヤー結合を実行して
	LayerMergeDown(window->active_layer);

	// アクティブレイヤーを変更する
	ChangeActiveLayer(window, next_active);
	LayerViewSetActiveLayer(next_active, app->layer_window.view);
}

/*********************************************************
* FlattenImage関数                                       *
* 画像の統合を実行                                       *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void FlattenImage(APPLICATION* app)
{
	AUTO_SAVE(app->draw_window[app->active_window]);

	MergeAllLayer(app->draw_window[app->active_window]);
}

/*********************************************************
* ActiveLayerAlpha2SelectionArea関数                     *
* アクティブレイヤーの不透明部分を選択範囲にする         *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void ActiveLayerAlpha2SelectionArea(APPLICATION* app)
{
	LayerAlpha2SelectionArea(GetActiveDrawWindow(app));
}

/*********************************************************
* ActiveLayerAlphaAddSelectionArea関数                   *
* アクティブレイヤーの不透明部分を選択範囲に加える       *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void ActiveLayerAlphaAddSelectionArea(APPLICATION* app)
{
	LayerAlphaAddSelectionArea(GetActiveDrawWindow(app));
}

/*********************************************************
* ExecuteCopyLayer関数                                   *
* レイヤーの複製を実行する                               *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void ExecuteCopyLayer(APPLICATION* app)
{
	// アクティブレイヤーのコピー作成
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	LAYER *layer = CreateLayerCopy(window->active_layer);
	window->num_layer++;

	// 局所キャンバスモードなら戻る時に備えてフラグを立てる
	if((window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		DRAW_WINDOW *parent = app->draw_window[app->active_window];
		LAYER *parent_prev = SearchLayer(parent->layer, window->active_layer->name);
		LAYER *parent_next = (parent_prev == NULL) ? parent->layer : parent_prev->next;
		LAYER *parent_new = CreateLayer(0, 0, parent->width, parent->height, parent->channel,
			(eLAYER_TYPE)layer->layer_type, parent_prev, parent_next, layer->name, parent);
		parent->num_layer++;
		AddNewLayerHistory(parent_new, parent_new->layer_type);
	}

	LayerViewAddLayer(layer, window->layer,
		app->layer_window.view, window->num_layer);
	ChangeActiveLayer(window, layer);
	LayerViewSetActiveLayer(layer, app->layer_window.view);
}

/*********************************************************
* ExecuteVisible2Layer関数                               *
* 可視部をレイヤーにする                                 *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void ExecuteVisible2Layer(APPLICATION* app)
{
	// レイヤーを追加する描画領域
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	char layer_name[256];		// 作成するレイヤーの名前
	LAYER *visible, *new_layer;	// 可視部のコピーと新規作成のレイヤー
	int layer_id = 1;			// レイヤーの名前に付加する数値

	// レイヤーの名前が重複しないように設定
	do
	{
		(void)sprintf(layer_name, "%s%d", app->labels->menu.visible_copy, layer_id);
		layer_id++;
	} while(CorrectLayerName(window->layer, layer_name) == 0);

	// 可視部の合成を行う
	visible = MixLayerForSave(window);
	// レイヤー作成
	new_layer = CreateLayer(visible->x, visible->y, visible->width,
		visible->height, visible->channel, TYPE_NORMAL_LAYER,
		window->active_layer, window->active_layer->next,
		layer_name, window
	);
	// レイヤーのピクセルデータに合成したデータをコピー
	(void)memcpy(new_layer->pixels, visible->pixels, visible->height*visible->stride);
	window->num_layer++;

	// 局所キャンバスモードなら親キャンバスでもレイヤー作成
	if((window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		DRAW_WINDOW *parent = app->draw_window[app->active_window];
		LAYER *parent_prev = SearchLayer(parent->layer, window->active_layer->name);
		LAYER *parent_next = (parent_prev == NULL) ? parent->layer : parent_prev->next;
		LAYER *parent_new = CreateLayer(0, 0, parent->width, parent->height, parent->channel,
			(eLAYER_TYPE)new_layer->layer_type, parent_prev, parent_next, new_layer->name, parent);
		parent->num_layer++;
		AddNewLayerHistory(parent_new, parent_new->layer_type);
	}

	// レイヤーウィンドウに追加してアクティブ化
	LayerViewAddLayer(new_layer, window->layer,
		app->layer_window.view, window->num_layer);

	// 合成用に作ったレイヤーを削除
	DeleteLayer(&visible);
}

/*********************************************************
* RasterizeActiveLayer関数                               *
* アクティブレイヤーをラスタライズする                   *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void RasterizeActiveLayer(APPLICATION* app)
{
	RasterizeLayer(GetActiveDrawWindow(app)->active_layer);
}

/*********************************************************
* ExecuteSelectAll関数                                   *
* 全て選択を実行                                         *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
void ExecuteSelectAll(APPLICATION* app)
{
	// 描画領域の情報
	DRAW_WINDOW* window = GetActiveDrawWindow(app);

	// 一時保存レイヤーに現在の選択範囲を写す
	(void)memcpy(window->temp_layer->pixels, window->selection->pixels,
		window->width * window->height);

	// 選択範囲を255で埋める
	(void)memset(window->selection->pixels, 0xff,
		window->width * window->height);

	// 選択範囲変化の履歴データ作成
	AddSelectionAreaChangeHistory(
		window, app->labels->menu.select_all, 0, 0, window->width, window->height);

	// 選択範囲の領域を更新
	(void)UpdateSelectionArea(&window->selection_area, window->selection, window->temp_layer);

	window->flags |= DRAW_WINDOW_HAS_SELECTION_AREA;
}

/*****************************************************
* ExecuteChangeResolution関数                        *
* キャンバスの解像度変更を実行                       *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteChangeResolution(APPLICATION* app)
{
	// 画像の幅と高さを決めるダイアログボックスを作成
	GtkWidget *dialog =
		gtk_dialog_new_with_buttons(
			app->labels->menu.change_resolution,
			GTK_WINDOW(app->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			NULL
		);
	// ウィジェットの配置用
	GtkWidget *table, *spin1, *spin2, *label;
	GtkWidget *dpi;
	GtkAdjustment *adjust;
	// OK, キャンセルの結果を受ける
	gint ret;
	// 作成する画像の幅と高さ
	int32 width, height;

	table = gtk_table_new(3, 3, FALSE);

	// ダイアログボックスに幅のラベルと値設定用のウィジェットを登録
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table);
	label = gtk_label_new(app->labels->make_new.width);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->draw_window[app->active_window]->original_width,
		1, LONG_MAX, 1, 10, 0));
	spin1 = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), spin1, 1, 2, 0, 1);

	// ダイアログボックスに高さのラベルと値設定用のウィジェットを登録
	label = gtk_label_new(app->labels->make_new.height);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->draw_window[app->active_window]->original_height,
		1, LONG_MAX, 1, 10, 0));
	spin2 = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), spin2, 1, 2, 1, 2);

	// 解像度設定用のウィジェットを登録
	label = gtk_label_new(app->labels->unit.resolution);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->draw_window[app->active_window]->resolution,
		1, 1200, 1, 1, 0));
	dpi = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), dpi, 1, 2, 2, 3);
	label = gtk_label_new("dpi");
	gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 2, 3);

	// ダイアログボックスを表示
	gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));

	// ユーザーが「OK」、「キャンセル」をクリックまで待つ
	ret = gtk_dialog_run(GTK_DIALOG(dialog));

	// 「OK」が押されたら
	if(ret == GTK_RESPONSE_ACCEPT)
	{	// 入力された幅と高さの値を取得
		width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin1));
		height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin2));

		// 入力値を記憶
		app->draw_window[app->active_window]->original_width = width;
		app->draw_window[app->active_window]->original_height = height;

		// 4の倍数に揃える
		width += (4 - (width % 4)) % 4;
		height += (4 - (height % 4)) % 4;

		// 現在の幅、高さと入力された値が異なれば変更を実行
		if(width != app->draw_window[app->active_window]->width
			|| height != app->draw_window[app->active_window]->height)
		{
			// 先に履歴をとっておく
			AddChangeDrawWindowResolutionHistory(
				app->draw_window[app->active_window], width, height);

			// 解像度変更の実行
			ChangeDrawWindowResolution(app->draw_window[app->active_window],
				width, height);
		}

		app->draw_window[app->active_window]->resolution =
			(int16)gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dpi));
	}

	// ダイアログボックスを閉じる
	gtk_widget_destroy(dialog);
}

/*****************************************************
* ExecuteChangeCanvasSize関数                        *
* キャンバスのサイズ変更を実行                       *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteChangeCanvasSize(APPLICATION* app)
{
	// 画像の幅と高さを決めるダイアログボックスを作成
	GtkWidget *dialog =
		gtk_dialog_new_with_buttons(
			app->labels->menu.change_canvas_size,
			GTK_WINDOW(app->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			NULL
		);
	// ウィジェットの配置用
	GtkWidget *table, *spin1, *spin2, *label;
	GtkAdjustment *adjust;
	// OK, キャンセルの結果を受ける
	gint ret;
	// 作成する画像の幅と高さ
	int32 width, height;

	table = gtk_table_new(3, 3, FALSE);

	// ダイアログボックスに幅のラベルと値設定用のウィジェットを登録
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table);
	label = gtk_label_new(app->labels->make_new.width);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->draw_window[app->active_window]->original_width,
		1, LONG_MAX, 1, 10, 100));
	spin1 = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), spin1, 1, 2, 0, 1);

	// ダイアログボックスに高さのラベルと値設定用のウィジェットを登録
	label = gtk_label_new(app->labels->make_new.height);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->draw_window[app->active_window]->original_height, 1, LONG_MAX, 1, 10, 100));
	spin2 = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), spin2, 1, 2, 1, 2);

	// ダイアログボックスを表示
	gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));

	// ユーザーが「OK」、「キャンセル」をクリックまで待つ
	ret = gtk_dialog_run(GTK_DIALOG(dialog));

	// 「OK」が押されたら
	if(ret == GTK_RESPONSE_ACCEPT)
	{	// 入力された幅と高さの値を取得
		width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin1));
		height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin2));

		// 入力値を記憶
		app->draw_window[app->active_window]->original_width = width;
		app->draw_window[app->active_window]->original_height = height;

		// 4の倍数に揃える
		width += (4 - (width % 4)) % 4;
		height += (4 - (height % 4)) % 4;

		// 現在の幅、高さと入力された値が異なれば変更を実行
		if(width != app->draw_window[app->active_window]->width
			|| height != app->draw_window[app->active_window]->height)
		{
			// 先に履歴をとっておく
			AddChangeDrawWindowSizeHistory(
				app->draw_window[app->active_window], width, height);

			// 解像度変更の実行
			ChangeDrawWindowSize(app->draw_window[app->active_window],
				width, height);
		}
	}

	// ダイアログボックスを閉じる
	gtk_widget_destroy(dialog);
}

/*****************************************************
* ExecuteChangeCanvasIccProfile関数                  *
* 引数                                               *
* menu	: メニューウィジェット                       *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteChangeCanvasIccProfile(GtkWidget* menu, APPLICATION* app)
{
	DRAW_WINDOW *window = app->draw_window[app->active_window];
	cmsHPROFILE *monitor_profile;
	char *before_path = NULL;
	GtkWidget *dialog;
	
	if(window->icc_profile_path != NULL)
	{
		before_path = g_strdup(window->icc_profile_path);
	}

	dialog = IccProfileChooserDialogNew(PROFILE_USAGE_RGB_DOCUMENT, &window->icc_profile_path);

	IccProfileChooserDialogRun(dialog);

	if(window->icc_profile_path != NULL)
	{
		FILE *fp;

		if(window->icc_transform != NULL)
		{
			cmsDeleteTransform(window->icc_transform);
		}
		MEM_FREE_FUNC(window->icc_profile_data);
		window->icc_profile_data = NULL;

		fp = fopen(window->icc_profile_path, "rb");
		if(fp != NULL)
		{
			(void)fseek(fp, 0, SEEK_END);
			window->icc_profile_size = (int32)ftell(fp);
			rewind(fp);

			window->icc_profile_data = MEM_ALLOC_FUNC(window->icc_profile_size);
			(void)fread(window->icc_profile_data, 1, window->icc_profile_size, fp);

			window->input_icc = cmsOpenProfileFromMem(window->icc_profile_data, window->icc_profile_size);

			(void)fclose(fp);
		}

		monitor_profile = GetPrimaryMonitorProfile();

		if(app->output_icc != NULL)
		{
			cmsBool bpc[] = {TRUE, TRUE, TRUE, TRUE};
			cmsHPROFILE h_profiles[] = {window->input_icc, app->output_icc, app->output_icc, monitor_profile};
			cmsUInt32Number intents[] = { INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC };
			cmsFloat64Number adaptation_states[] = {0, 0, 0, 0};

			window->icc_transform = cmsCreateExtendedTransform(cmsGetProfileContextID(h_profiles[1]), 4, h_profiles,
				bpc, intents, adaptation_states, NULL, 0, TYPE_BGRA_8, TYPE_BGRA_8, 0);
		}
		else
		{
			window->icc_transform = cmsCreateTransform(window->input_icc, TYPE_BGRA_8,
				monitor_profile, TYPE_BGRA_8, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_BLACKPOINTCOMPENSATION);
		}

		cmsCloseProfile(monitor_profile);
	}

	g_free(before_path);
}

/*****************************************************
* DisplayVersion関数                                 *
* バージョン情報を表示する                           *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void DisplayVersion(APPLICATION* app)
{
	// バージョン情報を表示するダイアログ
	GtkWidget *dialog =
		gtk_about_dialog_new();
	// 年の文字列取得用
	char *year;
	char *date = __DATE__;
	// 作者情報
	const char *authors[] = {"umi", NULL};
	// アイコン表示
	GdkPixbuf *pixbuf;
	gchar *logo_file_path;
	// 表示する文字列
	char str[4096];

	gtk_window_set_title(GTK_WINDOW(dialog), app->labels->menu.version);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(app->window));

	// 著作権表示
	year = &date[strlen(date)];
	while(year > date)
	{
		if(*year == ' ')
		{
			break;
		}
		year--;
	}
	(void)sprintf(str, "Copyright \xC2\xA9 %s", year);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), str);

	// 作者表示
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);

	// ウェブサイト
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog),
		"http://gameprogrammingbyumi.blogspot.jp/");

	// ウェブサイトのラベル
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(dialog),
		"\xE8\xB6\xA3\xE5\x91\xB3\xE3\x83\xBB\xE3\x82\xB2\xE3\x83\xBC\xE3\x83\xA0\xE3\x83\x97\xE3\x83\xAD\xE3\x82\xB0\xE3\x83\xA9\xE3\x83\x9F\xE3\x83\xB3\xE3\x82\xB0\xE3\x81\xAE\xE3\x81\xBE\xE3\x81\xA3\xE3\x81\x9F\xE3\x82\x8A\xE3\x83\x96\xE3\x83\xAD\xE3\x82\xB0");

	// ライセンス
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog),
		"GNU General Public License v3\nhttp://www.gnu.org/licenses/gpl-3.0.html");

	// ロゴ
	logo_file_path = g_build_filename(app->current_path, "image/icon.png", NULL);
	pixbuf = gdk_pixbuf_new_from_file(logo_file_path, NULL);
	gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), pixbuf);
	g_object_unref(pixbuf);
	g_free(logo_file_path);

	// バージョン
	(void)sprintf(str, "%d.%d.%d.%d",
		MAJOR_VERSION, MINOR_VERSION, RELEASE_VERSION, BUILD_VERSION);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), str);

	// コメント
	(void)sprintf(str, "Build : %s", __DATE__);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), str);

	gtk_widget_show_all(dialog);

	(void)gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
}

static void TextureChange(GtkIconView* icon_view, APPLICATION* app)
{
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	GList *list = gtk_icon_view_get_selected_items(icon_view);
	GtkTreePath *path;
	gint *indices;

	if(list == NULL)
	{
		return;
	}

	path = (GtkTreePath*)list->data;
	indices = gtk_tree_path_get_indices(path);
	if(indices == NULL)
	{
		app->textures.active_texture = 0;
		gtk_label_set_text(GTK_LABEL(app->texture_label), app->labels->tool_box.no_texture);
	}
	else
	{
		app->textures.active_texture = indices[0];
		if(app->textures.active_texture == 0)
		{
			gtk_label_set_text(GTK_LABEL(app->texture_label), app->labels->tool_box.no_texture);
		}
		else
		{
			gtk_label_set_text(GTK_LABEL(app->texture_label),
				app->textures.texture[app->textures.active_texture-1].name);
		}
	}

	if(app->window_num > 0)
	{
		FillTextureLayer(window->texture, &app->textures);
	}

	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);
}

static void ChangeTextureStrength(GtkAdjustment* scale, APPLICATION* app)
{
	app->textures.strength = gtk_adjustment_get_value(scale);

	if(app->window_num > 0)
	{
		FillTextureLayer(GetActiveDrawWindow(app)->texture, &app->textures);
	}
}

static void ChangeTextureScale(GtkAdjustment* scale, APPLICATION* app)
{
	app->textures.scale = gtk_adjustment_get_value(scale);

	if(app->window_num > 0)
	{
		FillTextureLayer(GetActiveDrawWindow(app)->texture, &app->textures);
	}
}

static void ChangeTextureRotate(GtkAdjustment* scale, APPLICATION* app)
{
	app->textures.angle = gtk_adjustment_get_value(scale);

	if(app->window_num > 0)
	{
		FillTextureLayer(GetActiveDrawWindow(app)->texture, &app->textures);
	}
}

static void ClickedCloseButton(GtkWidget* button, GtkWidget* window)
{
	gtk_widget_destroy(window);
}

/*********************************************************
* CreateTextureChooser関数                               *
* テクスチャを選択するウィジェットを作成する             *
* 引数                                                   *
* texture	: テクスチャを管理する構造体のアドレス       *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	テクスチャを選択するウィンドウウィジェット           *
*********************************************************/
GtkWidget* CreateTextureChooser(TEXTURES* textures, APPLICATION* app)
{
	GdkPixbuf *no_texture;
	GtkWidget *ret = gtk_window_new(GTK_WINDOW_POPUP);
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *scroll;
	GtkWidget *icon_view;
	GtkWidget *scale;
	GtkWidget *button;
	GtkAdjustment *adjust;
	GtkListStore *list_store;
	GtkTreeIter iter, active;
	GtkTreePath *path;
	uint8 *pixels;
	int data_size;
	gint window_x, window_y;
	int i;

	// テクスチャ選択中は描画させない
	gtk_window_set_modal(GTK_WINDOW(ret), TRUE);

	// スクロールウィンドウを作成
	scroll = gtk_scrolled_window_new(NULL, NULL);
	// スクロールをウィンドウに登録
	gtk_container_add(GTK_CONTAINER(ret), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, FALSE, FALSE, 0);
	// スクロールバーは必要に応じて表示
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	// アイコンビューに表示するリストを作成
	list_store = gtk_list_store_new(
		2, G_TYPE_STRING, GDK_TYPE_PIXBUF);
	// リストにアイコン(テクスチャ)とファイル名を登録
		// 最初にテクスチャ無しを登録
	no_texture = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE,
		8, TEXTURE_ICON_SIZE, TEXTURE_ICON_SIZE);
	pixels = gdk_pixbuf_get_pixels(no_texture);
	data_size = gdk_pixbuf_get_rowstride(no_texture) * TEXTURE_ICON_SIZE;
	(void)memset(pixels, 0xff, data_size);
	gtk_list_store_append(list_store, &iter);
	active = iter;
	gtk_list_store_set(list_store, &iter,
		0, app->labels->tool_box.no_texture, 1, no_texture, -1);

	// 初期化時に読み込んだテクスチャのアイコンを登録
	for(i=0; i<textures->num_texture; i++)
	{
		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter,
			0, textures->texture[i].name,
			1, textures->texture[i].thumbnail,
			-1
		);
		if(i+1 == textures->active_texture)
		{
			active = iter;
		}
	}

	// アイコンビューを作成
		// 先ほど作成したリストを使用
	icon_view = gtk_icon_view_new_with_model(
		GTK_TREE_MODEL(list_store));
	// アイコンビューをスクロールウィンドウに登録
	gtk_container_add(GTK_CONTAINER(scroll), icon_view);

	// アイコンビューのアイコン列とテキスト列を指定
	gtk_icon_view_set_text_column(GTK_ICON_VIEW(icon_view), 0);
	gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(icon_view), 1);
	// 一度に一つのアイコンしか選択出来ないようにする
	gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(icon_view),
		GTK_SELECTION_SINGLE);

	// ポップアップウィンドウの座標をメインウィンドウの左上に
	gtk_window_get_position(GTK_WINDOW(app->window),
		&window_x, &window_y);
	gtk_window_move(GTK_WINDOW(ret), window_x, window_y);

	// アクティブなテクスチャを設定
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(list_store), &active);
	gtk_icon_view_select_path(GTK_ICON_VIEW(icon_view), path);
	// アイコン選択時のコールバック関数をセット
	(void)g_signal_connect(G_OBJECT(icon_view), "selection_changed",
		G_CALLBACK(TextureChange), app);

	// テクスチャの強さを設定するウィジェットを作成
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(textures->strength, 0, 100, 1, 1, 0));
	scale = SpinScaleNew(adjust, app->labels->tool_box.texture_strength, 1);
	(void)g_signal_connect(G_OBJECT(adjust), "value_changed",
		G_CALLBACK(ChangeTextureStrength), app);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	// テクスチャの拡大率を設定するウィジェットを作成
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(textures->scale, 0.1, 10, 0.1, 1, 0));
	scale = SpinScaleNew(adjust, app->labels->menu.zoom, 1);
	(void)g_signal_connect(G_OBJECT(adjust), "value_changed",
		G_CALLBACK(ChangeTextureScale), app);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	// テクスチャの回転角度を設定するウィジェットを作成
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(textures->angle, -180, 180, 1, 1, 0));
	scale = SpinScaleNew(adjust, app->labels->menu.rotate, 1);
	(void)g_signal_connect(G_OBJECT(adjust), "value_changed",
		G_CALLBACK(ChangeTextureRotate), app);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	// 「閉じる」ボタンを作成
	button = gtk_button_new_with_label(app->labels->window.close);
	(void)g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(ClickedCloseButton), ret);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	gtk_widget_set_size_request(scroll, 320, 320);

	return ret;
}

/*********************************************************
* Change2LoupeMode関数                                   *
* ルーペモードへ移行する                                 *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
void Change2LoupeMode(APPLICATION* app)
{
	Change2FocalMode(app->draw_window[app->active_window]);
}

/*********************************************************
* ReturnFromLoupeMode関数                                *
* ルーペモードから戻る                                   *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
void ReturnFromLoupeMode(APPLICATION* app)
{
	ReturnFromFocalMode(app->draw_window[app->active_window]);
}

/**********************************************************
* MemoryAllocate関数                                      *
* KABURAGI / MIKADOで使用するメモリアロケータでメモリ確保 *
* 引数                                                    *
* size	: 確保するバイト数                                *
* 返り値                                                  *
*	確保したメモリのアドレス                              *
**********************************************************/
void* MemoryAllocate(size_t size)
{
	return MEM_ALLOC_FUNC(size);
}

/**************************************************************
* MemoryFree関数                                              *
* KABURAGI / MIKADOで確保されたメモリを開放する(プラグイン用) *
* 引数                                                        *
* memory	: 開放するメモリのポインタ                        *
**************************************************************/
void MemoryFree(void* memory)
{
	MEM_FREE_FUNC(memory);
}

/*********************************************************
* SetHas3DLayer関数                                      *
* 3Dモデリングの有効/無効を設定する                      *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
* enable	: 有効:TRUE	無効:FALSE                       *
*********************************************************/
void SetHas3DLayer(APPLICATION* app, int enable)
{
	if(enable == FALSE)
	{
		app->flags &= ~(APPLICATION_HAS_3D_LAYER);
	}
	else
	{
		app->flags |= APPLICATION_HAS_3D_LAYER;
	}
}

/*****************************************************
* GetHas3DLayer関数                                  *
* 3Dモデリングの有効/無効を返す                      *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
* 返り値                                             *
*	3Dモデリング有効:TRUE	無効:FALSE               *
*****************************************************/
int GetHas3DLayer(APPLICATION* app)
{
	if((app->flags & APPLICATION_HAS_3D_LAYER) != 0)
	{
		return TRUE;
	}
	return FALSE;
}

#ifdef _MSC_VER

#if 1
# pragma comment(lib, "OpenGL32.lib")
# pragma comment(lib, "GlU32.lib")
# ifdef TEST //_DEBUG
#  pragma comment(lib, "tbb_debug.lib")
#  pragma comment(lib, "tbb_preview_debug.lib")
#  pragma comment(lib, "tbbmalloc_debug.lib")
#  pragma comment(lib, "tbbproxy_debug.lib")
#  pragma comment(lib, "tbbmalloc_proxy_debug.lib")
#  pragma comment(lib, "BulletCollision_vs2008_debug.lib")
#  pragma comment(lib, "BulletDynamics_vs2008_debug.lib")
#  pragma comment(lib, "BulletSoftBody_vs2008_debug.lib")
#  pragma comment(lib, "ConvexDecomposition_vs2008_debug.lib")
#  pragma comment(lib, "LinearMath_vs2008_debug.lib")
#  pragma comment(lib, "OpenGLSupport_vs2008_debug.lib")
# else
#  pragma comment(lib, "tbb.lib")
#  pragma comment(lib, "tbb_preview.lib")
#  pragma comment(lib, "tbbmalloc.lib")
#  pragma comment(lib, "tbbproxy.lib")
#  pragma comment(lib, "tbbmalloc_proxy.lib")
#  pragma comment(lib, "BulletCollision_vs2008.lib")
#  pragma comment(lib, "BulletDynamics_vs2008.lib")
#  pragma comment(lib, "BulletSoftBody_vs2008.lib")
#  pragma comment(lib, "ConvexDecomposition_vs2008.lib")
#  pragma comment(lib, "LinearMath_vs2008.lib")
#  pragma comment(lib, "OpenGLSupport_vs2008.lib")
# endif
#endif

# ifndef BUILD64BIT
#  if defined(USE_STATIC_LIB) && USE_STATIC_LIB != 0
#   ifdef _DEBUG
#    pragma comment(linker, "/NODEFAULTLIB:LIBCMTD.lib")
#    pragma comment(lib, "gtk_libsd.lib")
#    pragma comment(lib, "Dnsapi.lib")
#    pragma comment(lib, "ws2_32.lib")
#    pragma comment(lib, "Shlwapi.lib")
#    pragma comment(lib, "imm32.lib")
#    pragma comment(lib, "zlib.lib")
#    pragma comment(lib, "libpng.lib")
#    pragma comment(lib, "usp10.lib")
cairo_surface_t *
cairo_image_surface_create_from_png (const char	*filename)
{
	return 0;
}
/*
#    pragma comment(linker, "/NODEFAULTLIB:LIBCMTD.lib")
#    pragma comment(lib, "Dnsapi.lib")
#    pragma comment(lib, "ws2_32.lib")
#    pragma comment(lib, "pcre3d.lib")
#    pragma comment(lib, "Shlwapi.lib")
#    pragma comment(lib, "atk-1.0.lib")
#    pragma comment(lib, "cairo.lib")
#    pragma comment(lib, "expat.lib")
#    pragma comment(lib, "fontconfig.lib")
#    pragma comment(lib, "freetype.lib")
#    pragma comment(lib, "gailutil.lib")
#    pragma comment(lib, "gdk_pixbuf-2.0.lib")
#    pragma comment(lib, "gdk-win32-2.0.lib")
#    pragma comment(lib, "./STATIC_LIB/giod.lib")
#    pragma comment(lib, "./STATIC_LIB/glibd.lib")
#    pragma comment(lib, "./STATIC_LIB/gmoduled.lib")
#    pragma comment(lib, "./STATIC_LIB/gobjectd.lib")
#    pragma comment(lib, "./STATIC_LIB/gthreadd.lib")
#    pragma comment(lib, "gtk-win32-2.0.lib")
#    pragma comment(lib, "intl.lib")
#    pragma comment(lib, "libpng.lib")
#    pragma comment(lib, "pango-1.0.lib")
#    pragma comment(lib, "pangocairo-1.0.lib")
#    pragma comment(lib, "pangoft2-1.0.lib")
#    pragma comment(lib, "pangowin32-1.0.lib")
#    pragma comment(lib, "zlib.lib")
*/
#   else
#    pragma comment(lib, "./STATIC_LIB/libatk-1.0.a")
#    pragma comment(lib, "./STATIC_LIB/libcairo.a")
#    pragma comment(lib, "./STATIC_LIB/libcairo-gobject.a")
#    pragma comment(lib, "./STATIC_LIB/libcairo-script-interpreter.a")
#    pragma comment(lib, "./STATIC_LIB/libcharset.a")
#    pragma comment(lib, "./STATIC_LIB/libexpat.a")
#    pragma comment(lib, "./STATIC_LIB/libffi.a")
#    pragma comment(lib, "./STATIC_LIB/libfontconfig.a")
#    pragma comment(lib, "./STATIC_LIB/libfreetype.a")
#    pragma comment(lib, "./STATIC_LIB/libgailutil.a")
#    pragma comment(lib, "./STATIC_LIB/libgdk_pixbuf-2.0.a")
#    pragma comment(lib, "./STATIC_LIB/libgdk-win32-2.0.a")
#    pragma comment(lib, "./STATIC_LIB/libgio-2.0.a")
#    pragma comment(lib, "./STATIC_LIB/libglib-2.0.a")
#    pragma comment(lib, "./STATIC_LIB/libgmodule-2.0.a")
#    pragma comment(lib, "gobject-2.0.lib")
#    pragma comment(lib, "gthread-2.0.lib")
#    pragma comment(lib, "gtk-win32-2.0.lib")
#    pragma comment(lib, "intl.lib")
#    pragma comment(lib, "libpng.lib")
#    pragma comment(lib, "pango-1.0.lib")
#    pragma comment(lib, "pangocairo-1.0.lib")
#    pragma comment(lib, "pangoft2-1.0.lib")
#    pragma comment(lib, "pangowin32-1.0.lib")
#    pragma comment(lib, "zlib.lib")
#   endif
#  else
#  ifdef _DEBUG
#   pragma comment(linker, "/NODEFAULTLIB:LIBCMT")
#  else
#   if _MSC_VER >= 1600
#    pragma comment(linker, "/NODEFAULTLIB:LIBCMT")
#   endif
#  endif
#   if GTK_MAJOR_VERSION <= 2
#    pragma comment(lib, "atk-1.0.lib")
#    pragma comment(lib, "cairo.lib")
#    pragma comment(lib, "expat.lib")
#    pragma comment(lib, "fontconfig.lib")
#    pragma comment(lib, "freetype.lib")
#    pragma comment(lib, "gailutil.lib")
#    pragma comment(lib, "gdk_pixbuf-2.0.lib")
#    pragma comment(lib, "gdk-win32-2.0.lib")
#    pragma comment(lib, "gio-2.0.lib")
#    pragma comment(lib, "glib-2.0.lib")
#    pragma comment(lib, "gmodule-2.0.lib")
#    pragma comment(lib, "gobject-2.0.lib")
#    pragma comment(lib, "gthread-2.0.lib")
#    pragma comment(lib, "gtk-win32-2.0.lib")
#    pragma comment(lib, "intl.lib")
#    pragma comment(lib, "libpng.lib")
#    pragma comment(lib, "pango-1.0.lib")
#    pragma comment(lib, "pangocairo-1.0.lib")
#    pragma comment(lib, "pangoft2-1.0.lib")
#    pragma comment(lib, "pangowin32-1.0.lib")
#    if defined(_M_X64) || defined(_M_IA64)
#     pragma comment(lib, "zdll.lib")
#    else
#     pragma comment(lib, "zlib.lib")
#    endif
#   else
#    ifdef _DEBUG
//#     pragma comment(linker, "/NODEFAULTLIB:MSVCRTD")
#    else
#     pragma comment(linker, "/NODEFAULTLIB:MSVCRT")
#    endif

#    if defined(_M_X64) || defined(_M_IA64)
#     pragma comment(lib, "libpng.lib")
#    else
#     pragma comment(lib, "libpng.lib")
#    endif
#    pragma comment(lib, "zlib.lib")
#    pragma comment(lib, "cairo.lib")
#    pragma comment(lib, "atk-1.0.lib")
#    pragma comment(lib, "fontconfig.lib")
#    pragma comment(lib, "gailutil.lib")
#    pragma comment(lib, "gdk_pixbuf-2.0.lib")
#    pragma comment(lib, "gdk-win32-3.0.lib")
#    pragma comment(lib, "gio-2.0.lib")
#    pragma comment(lib, "glib-2.0.lib")
#    pragma comment(lib, "gmodule-2.0.lib")
#    pragma comment(lib, "gobject-2.0.lib")
#    pragma comment(lib, "gthread-2.0.lib")
#    pragma comment(lib, "gtk-win32-3.0.lib")
#    pragma comment(lib, "pango-1.0.lib")
#    pragma comment(lib, "pangocairo-1.0.lib")
#    pragma comment(lib, "pangoft2-1.0.lib")
#    pragma comment(lib, "pangowin32-1.0.lib")
#    pragma comment(lib, "libgettextsrc-0-18-2.lib")
#    pragma comment(lib, "libgettextpo-0.lib")
#    pragma comment(lib, "libgettextlib-0-18-2.lib")
#    pragma comment(lib, "libintl-8.lib")

#    pragma comment(lib, "libatk-1.0-0.lib")
#    pragma comment(lib, "libcairo-2.lib")
#    pragma comment(lib, "libcairo-gobject-2.lib")
#    pragma comment(lib, "libcairo-script-interpreter-2.lib")
#    pragma comment(lib, "libcroco-0.6-3.lib")
#    pragma comment(lib, "libffi-6.lib")
#    pragma comment(lib, "libfontconfig-1.lib")
#    pragma comment(lib, "libfreetype-6.lib")
#    pragma comment(lib, "libgailutil-3-0.lib")
#    pragma comment(lib, "libgdk_pixbuf-2.0-0.lib")
#    pragma comment(lib, "libgdk-3-0.lib")
#    pragma comment(lib, "libgio-2.0-0.lib")
#    pragma comment(lib, "libglib-2.0-0.lib")
#    pragma comment(lib, "libgmodule-2.0-0.lib")
#    pragma comment(lib, "libgthread-2.0-0.lib")
#    pragma comment(lib, "libgtk-3-0.lib")
#    pragma comment(lib, "libjasper-1.lib")
#    pragma comment(lib, "libjpeg-9.lib")
#    pragma comment(lib, "liblzma-5.lib")
#    pragma comment(lib, "libpango-1.0-0.lib")
#    pragma comment(lib, "libpangocairo-1.0-0.lib")
#    pragma comment(lib, "libpangoft2-1.0-0.lib")
#    pragma comment(lib, "libpangowin32-1.0-0.lib")
#    pragma comment(lib, "libpixman-1-0.lib")
#    pragma comment(lib, "librsvg-2-2.lib")
#    pragma comment(lib, "libtiff-5.lib")
#    pragma comment(lib, "libxml2-2.lib")
#    pragma comment(lib, "zdll.lib")

#   endif
#  endif
# else
#  ifdef _DEBUG
#   pragma comment(linker, "/NODEFAULTLIB:LIBCMTD")
#  else
//#   pragma comment(linker, "/NODEFAULTLIB:LIBCMT")
#  endif
#  if GTK_MAJOR_VERSION <= 2
#   pragma comment(lib, "atk-1.0.lib")
#   pragma comment(lib, "cairo.lib")
#   pragma comment(lib, "expat.lib")
#   pragma comment(lib, "fontconfig.lib")
#   pragma comment(lib, "freetype.lib")
#   pragma comment(lib, "gailutil.lib")
#   pragma comment(lib, "gdk_pixbuf-2.0.lib")
#   pragma comment(lib, "gdk-win32-2.0.lib")
#   pragma comment(lib, "gio-2.0.lib")
#   pragma comment(lib, "glib-2.0.lib")
#   pragma comment(lib, "gmodule-2.0.lib")
#   pragma comment(lib, "gobject-2.0.lib")
#   pragma comment(lib, "gthread-2.0.lib")
#   pragma comment(lib, "gtk-win32-2.0.lib")
#   pragma comment(lib, "intl.lib")
#   pragma comment(lib, "libpng.lib")
#   pragma comment(lib, "pango-1.0.lib")
#   pragma comment(lib, "pangocairo-1.0.lib")
#   pragma comment(lib, "pangoft2-1.0.lib")
#   pragma comment(lib, "pangowin32-1.0.lib")
#   if defined(_M_X64) || defined(_M_IA64)
#    pragma comment(lib, "zdll.lib")
#   else
#    pragma comment(lib, "zlib.lib")
#   endif
#  else
#    pragma comment(lib, "libpng15-15.lib")
#    pragma comment(lib, "zlib.lib")
#    pragma comment(lib, "libz.dll.a")
#    pragma comment(lib, "cairo.lib")
#    pragma comment(lib, "atk-1.0.lib")
#    pragma comment(lib, "fontconfig.lib")
#    pragma comment(lib, "gailutil.lib")
#    pragma comment(lib, "gdk_pixbuf-2.0.lib")
#    pragma comment(lib, "gdk-win32-3.0.lib")
#    pragma comment(lib, "gio-2.0.lib")
#    pragma comment(lib, "glib-2.0.lib")
#    pragma comment(lib, "gmodule-2.0.lib")
#    pragma comment(lib, "gobject-2.0.lib")
#    pragma comment(lib, "gthread-2.0.lib")
#    pragma comment(lib, "gtk-win32-3.0.lib")
#    pragma comment(lib, "pango-1.0.lib")
#    pragma comment(lib, "pangocairo-1.0.lib")
#    pragma comment(lib, "pangoft2-1.0.lib")
#    pragma comment(lib, "pangowin32-1.0.lib")
#    pragma comment(lib, "intl.lib")

#    pragma comment(lib, "libatk-1.0-0.lib")
#    pragma comment(lib, "libcairo.dll.a")
#    pragma comment(lib, "libcairo-gobject.dll.a")
#    pragma comment(lib, "libcairo-script-interpreter.dll.a")
#    pragma comment(lib, "libcroco-0.6.dll.a")
#    pragma comment(lib, "libffi.dll.a")
#    pragma comment(lib, "libfontconfig.dll.a")
#    pragma comment(lib, "libfreetype.dll.a")
#    pragma comment(lib, "libgailutil-3.dll.a")
#    pragma comment(lib, "libgdk_pixbuf-2.0.dll.a")
#    pragma comment(lib, "libgdk-3.dll.a")
#    pragma comment(lib, "libgio-2.0.dll.a")
#    pragma comment(lib, "libglib-2.0.dll.a")
#    pragma comment(lib, "libgmodule-2.0.dll.a")
#    pragma comment(lib, "libgthread-2.0.dll.a")
#    pragma comment(lib, "libgtk-3.dll.a")
#    pragma comment(lib, "libjasper.dll.a")
#    pragma comment(lib, "libjpeg.dll.a")
#    pragma comment(lib, "liblzma.dll.a")
#    pragma comment(lib, "libpango-1.0.dll.a")
#    pragma comment(lib, "libpangocairo-1.0.dll.a")
#    pragma comment(lib, "libpangoft2-1.0.dll.a")
#    pragma comment(lib, "libpangowin32-1.0.dll.a")
#    pragma comment(lib, "libpixman-1.dll.a")
#    pragma comment(lib, "librsvg-2.dll.a")
#    pragma comment(lib, "libtiff.dll.a")
#    pragma comment(lib, "libtiffxx.dll.a")
#    pragma comment(lib, "libxml2.dll.a")
#  endif
# endif

#endif

#ifdef __cplusplus
}
#endif
