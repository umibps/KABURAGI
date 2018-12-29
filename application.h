#ifndef _INCLUDED_APPLICATION_H_
#define _INCLUDED_APPLICATION_H_

#include "configure.h"

#define MAJOR_VERSION 1

#if MAJOR_VERSION == 1
# define MINOR_VERSION 4
# define RELEASE_VERSION 2
# define BUILD_VERSION 0
#elif MAJOR_VERSION == 2
# define MINOR_VERSION 0
# define RELEASE_VERSION 1
# define BUILD_VERSION 1
#endif

#define FILE_VERSION 6

#include "draw_window.h"
// 描画領域の最大数
#define MAX_DRAW_WINDOW 128
// ブラシプレビュー用キャンバスの幅
#define BRUSH_PREVIEW_CANVAS_WIDTH 500
// ブラシプレビュー用キャンバスの高さ
#define BRUSH_PREVIEW_CANVAS_HEIGHT 500
// タッチイベント同時検出の最大数
#define MAX_TOUCH 10
// 3Dモデル操作時のスクロールバー分のマージン
#define SCROLLED_WINDOW_MARGIN 32

#include "lcms/lcms2.h"
#include "labels.h"
#include "tool_box.h"
#include "layer_window.h"
#include "navigation.h"
#include "preview_window.h"
#include "reference_window.h"
#include "brush_core.h"
#include "common_tools.h"
#include "vector_brush_core.h"
#include "vector_brushes.h"
#include "color.h"
#include "pattern.h"
#include "texture.h"
#include "smoother.h"
#include "preference.h"
#include "display_filter.h"
#include "fractal_label.h"
#include "ght_hash_table.h"

#include "MikuMikuGtk+/mikumikugtk.h"
#include "MikuMikuGtk+/ui_label.h"

#if !defined(USE_QT) || (defined(USE_QT) && USE_QT != 0)
# include "gui/GTK/color_gtk.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// 初期化ファイルのファイルパス
#define INITIALIZE_FILE_NAME "application.ini"
// パレットファイルのパス
#define PALLETE_FILE_NAME "pallete.kpl"
// プラグインのパス
#define PLUG_IN_DIRECTORY "plug-in"
// デフォルトの解像度
#define DEFALUT_RESOLUTION 96

typedef enum _eUTILITY_PLACE
{
	UTILITY_PLACE_WINDOW,
	UTILITY_PLACE_LEFT,
	UTILITY_PLACE_RIGHT,
	UTILITY_PLACE_POPUP_LEFT,
	UTILITY_PLACE_POPUP_RIGHT
} eUTILITY_PLACE;

typedef enum _eTOOL_WINDOW_FLAGS
{
	TOOL_USING_BRUSH = 0x01,
	TOOL_DOCKED = 0x02,
	TOOL_PLACE_RIGHT = 0x04,
	TOOL_SHOW_COLOR_CIRCLE = 0x08,
	TOOL_SHOW_COLOR_PALLETE = 0x10,
	TOOL_POP_UP = 0x20,
	TOOL_CHANGING_BRUSH = 0x40,
	TOOL_BUTTON_STOPPED = 0x80
} eTOOL_WINDOW_FLAGS;

/*************************************************
* eAPPLICATION_FLAGS列挙体                       *
* アプリケーションのファイル操作中等で立つフラグ *
*************************************************/
typedef enum _eAPPLICATION_FLAGS
{
	APPLICATION_IN_OPEN_OPERATION = 0x01,				// ファイルオープン
	APPLICATION_IN_MAKE_NEW_DRAW_AREA = 0x02,			// 描画領域の新規作成
	APPLICATION_IN_REVERSE_OPERATION = 0x04,			// 左右反転処理中
	APPLICATION_IN_EDIT_SELECTION = 0x08,				// 選択範囲編集切り替え中
	APPLICATION_INITIALIZED = 0x10,						// 初期化済み
	APPLICATION_IN_DELETE_EVENT = 0x20,					// 削除イベント内
	APPLICATION_FULL_SCREEN = 0x40,						// フルスクリーンモード
	APPLICATION_WINDOW_MAXIMIZE = 0x80,					// ウィンドウの最大化
	APPLICATION_DISPLAY_GRAY_SCALE = 0x100,				// グレースケールでの表示
	APPLICATION_DISPLAY_SOFT_PROOF = 0x200,				// ICCプロファイルでソフトプルーフ表示
	APPLICATION_REMOVE_ACCELARATOR = 0x400,				// ショートカットキーを無効化
	APPLICATION_DRAW_WITH_TOUCH = 0x800,				// タッチイベントで描画する
	APPLICATION_SET_BACK_GROUND_COLOR = 0x1000,			// キャンバスの背景を設定する
	APPLICATION_SHOW_PREVIEW_ON_TASK_BAR = 0x2000,		// プレビューウィンドウをタスクバーに表示する
	APPLICATION_IN_SWITCH_DRAW_WINDOW = 0x4000,			// 描画領域の切替中
	APPLICATION_WRITE_PROGRAM_DATA_DIRECTORY = 0x8000,	// ファイルの書出しをProgram Dataフェオルダにする
	APPLICATION_HAS_3D_LAYER = 0x10000,					// 3Dモデリングの使用可否
} eAPPLICATION_FLAGS;

#define DND_THRESHOLD 20

#define BRUSH_TABLE_WIDTH (4)
#define BRUSH_TABLE_HEIGHT (16)

#define MAX_BRUSH_SIZE 500
#define MAX_BRUSH_STRIDE ((MAX_BRUSH_SIZE) * 4)

#define VECTOR_BRUSH_TABLE_WIDTH (4)
#define VECTOR_BRUSH_TABLE_HEIGHT (8)

#define COMMON_TOOL_TABLE_WIDTH 5
#define COMMON_TOOL_TABLE_HEIGHT 2

#define THUMBNAIL_SIZE 128

#define PALLETE_WIDTH 16
#define PALLETE_HEIGHT 16

#define NUM_LAYER_BLEND_MODE 17

typedef enum _eINPUT_DEVICE
{
	INPUT_PEN,
	INPUT_ERASER
} eINPUT_DEVICE;

typedef struct _INITIALIZE_DATA
{
	uint8 fg_color[3], bg_color[3];
} INITIALIZE_DATA;

/*************************
* TOOL_WINDOW構造体      *
* ツールボックスのデータ *
*************************/
typedef struct _TOOL_WINDOW
{
	// ウィンドウ、ブラシテーブル、ブラシテーブルのスクロール
	GtkWidget *window, *common_tool_table, *brush_table, *brush_scroll;
	// メニューウィジェット
	GtkWidget *menu_item;
	// ブラシテーブルと詳細設定を区切るペーン
	GtkWidget *pane;
	// ウィジェットの配置
	int place;
	// ウィンドウの位置、サイズ
	int window_x, window_y, window_width, window_height;
	// ブラシテーブルと詳細設定を区切るペーンの位置
	gint pane_position;
	// ブラシボタンに使用するフォント名
	char* font_file;
	// 色選択用のデータ
	COLOR_CHOOSER *color_chooser;
	// ブラシのコアテーブル
	BRUSH_CORE brushes[BRUSH_TABLE_HEIGHT][BRUSH_TABLE_WIDTH];
#if GTK_MAJOR_VERSION >= 3
	// タッチイベント用のデータ
	BRUSH_CORE touch[MAX_TOUCH];
	// 指判定用
	GdkEventSequence *fingers[MAX_TOUCH];
	// 現在のタッチイベント中の指の数
	int num_touch;
#endif
	// 現在使用しているブラシへのポインタ
	BRUSH_CORE *active_brush[2];
	// ブラシ画像のピクセルデータ
	uint8 *brush_pattern_buff, *temp_pattern_buff;
	// コピーするブラシへのポインタ
	void *copy_brush;
	// ブラシファイルの文字コード
	char *brush_code;
	// プラグインブラシのモジュール
	GModule *plug_in_brush;
	// 共通ツールのブラシテーブル
	COMMON_TOOL_CORE common_tools[COMMON_TOOL_TABLE_HEIGHT][COMMON_TOOL_TABLE_WIDTH];
	// 現在使用している共通ツールへのポインタ
	COMMON_TOOL_CORE* active_common_tool;
	// 共通ツールファイルの文字コード
	char *common_tool_code;
	// ベクトルブラシのコアテーブル
	VECTOR_BRUSH_CORE vector_brushes[VECTOR_BRUSH_TABLE_HEIGHT][VECTOR_BRUSH_TABLE_WIDTH];
	// 現在使用しているベクトルブラシへのポインタ
	VECTOR_BRUSH_CORE *active_vector_brush[2];
	// ベクトルブラシファイルの文字コード
	char *vector_brush_code;
	// 通常レイヤーでControlキーが押されているときはスポイトツールに偏差させるため
		// スポイトツールのデータを保持しておく
	COMMON_TOOL_CORE color_picker_core;
	COLOR_PICKER color_picker;
	// ベクトルレイヤーでControl、Shiftキーが押されているときには制御点ツールに変化させるため
		// 制御点ツールのデータを保持しておく
	VECTOR_BRUSH_CORE vector_control_core;
	CONTROL_POINT_TOOL vector_control;
	// ブラシのUIウィジェット
	GtkWidget *ui, *detail_ui, *detail_ui_scroll;
	// 手ブレ補正の情報
	SMOOTHER smoother;
	// タッチイベント用の手ブレ補正
#if GTK_MAJOR_VERSION >= 3
	SMOOTHER touch_smoother[MAX_TOUCH];
#endif
	// 簡易ブラシ切り替えのデータ
	BRUSH_CHAIN brush_chain;
	// パレットの情報
	uint8 pallete[(PALLETE_WIDTH*PALLETE_HEIGHT)][3];
	uint8 pallete_use[((PALLETE_WIDTH*PALLETE_HEIGHT)+7)/8];
	// ツールボックス全体で扱うフラグ
	unsigned int flags;
} TOOL_WINDOW;

/*************************************************************************
* APPLICATION_MENUS構造体                                                *
* アプリケーションで描画領域の有無等で有効、無効が変化するメニューを記憶 *
*************************************************************************/
typedef struct _APPLICATION_MENUS
{	// 描画領域が無いときに無効
	GtkWidget *disable_if_no_open[80];
	// 選択範囲が無いときに無効
	GtkWidget *disable_if_no_select[32];
	// レイヤーが一枚しかないときに無効
	GtkWidget *disable_if_single_layer[8];
	// 通常のレイヤーのときに無効
	GtkWidget *disable_if_normal_layer[8];
	uint8 num_disable_if_no_open;
	uint8 menu_start_disable_if_no_open;
	uint8 num_disable_if_no_select;
	uint8 menu_start_disable_if_no_select;
	uint8 num_disable_if_single_layer;
	uint8 menu_start_disable_if_single_layer;
	uint8 num_disable_if_normal_layer;
	uint8 menu_start_disable_if_normal_layer;
	// 下のレイヤーと結合メニュー
	GtkWidget *merge_down_menu;
	// 背景色切り替えメニュー
	GtkWidget *change_back_ground_menu;
	// 表示フィルター切り替えメニュー
	GtkWidget *display_filter_menus[NUM_DISPLAY_FUNC_TYPE];
} APPLICATION_MENUS;

/**********************
* MAKE_NEW_DATA構造体 *
* 新規作成時のデータ  *
**********************/
typedef struct _MAKE_NEW_DATA
{
	char *name;
	int32 width, height;
	int16 resolution;
} MAKE_NEW_DATA;

/***********************
* MENU_DATA構造体      *
* メニューで扱うデータ *
***********************/
typedef struct _MENU_DATA
{
	MAKE_NEW_DATA *make_new;
	uint8 second_bg_color[3];
	int16 num_make_new_data;

	GtkWidget *reverse_horizontally;
	GtkWidget *edit_selection;
} MENU_DATA;

/*****************************************
* SCRIPTS構造体                          *
* スクリプトディレクトリのスクリプト情報 *
*****************************************/
typedef struct _SCRIPTS
{
	gchar **file_paths;
	gchar **file_names;
	int num_script;
} SCRIPTS;

// フィルター関数配列のインデックス
typedef enum _eFILTER_FUNC_ID
{
	FILTER_FUNC_BLUR,
	FILTER_FUNC_MOTION_BLUR,
	FILTER_FUNC_GAUSSIAN_BLUR,
	FILTER_FUNC_BRIGHTNESS_CONTRAST,
	FILTER_FUNC_HUE_SATURATION,
	FILTER_FUNC_COLOR_LEVEL_ADJUST,
	FILTER_FUNC_TONE_CURVE,
	FILTER_FUNC_LUMINOSITY2OPACITY,
	FILTER_FUNC_COLOR2ALPHA,
	FILTER_FUNC_COLORIZE_WITH_UNDER,
	FILTER_FUNC_GRADATION_MAP,
	FILTER_FUNC_FILL_WITH_VECTOR,
	FILTER_FUNC_PERLIN_NOISE,
	FILTER_FUNC_FRACTAL,
	FILTER_FUNC_TRACE,
	NUM_FILTER_FUNC
} eFILTER_FUNC_ID;

typedef void (*filter_func)(struct _DRAW_WINDOW* window, struct _LAYER** layers,
							uint16 num_layer, void* data);
typedef void (*selection_filter_func)(struct _DRAW_WINDOW* window, void* data);

typedef struct _MAIN_WINDOW_WIDGETS* MAIN_WINDOW_WIDGETS_PTR;

typedef struct _WRITE_APPLICATION_DATA
{
	int main_window_x, main_window_y;
	int main_window_width, main_window_height;
} WRITE_APPLICATIOIN_DATA;

/*************************************
* APPLICATION構造体                  *
* アプリケーション全体のデータを管理 *
*************************************/
typedef struct _APPLICATION
{
	// OpenMPの最大スレッド数
	int max_threads;
	// メインウィンドウのGUIウィジェット
	MAIN_WINDOW_WIDGETS_PTR widgets;
	// ウィンドウの位置、サイズ
	int window_x, window_y, window_width, window_height;
	// 左右ペーンの位置
	int left_pane_position, right_pane_position;
	// GUIの拡大率
	FLOAT_T gui_scale;
	// 入力デバイス
	eINPUT_DEVICE input;
	// 描画領域
	DRAW_WINDOW *draw_window[MAX_DRAW_WINDOW];
	// ツールウィンドウ
	TOOL_WINDOW tool_window;
	// レイヤーウィンドウ
	LAYER_WINDOW layer_window;
	// ナビゲーションウィンドウ
	NAVIGATION_WINDOW navigation_window;
	// プレビューウィンドウ
	PREVIEW_WINDOW preview_window;
	// 参考用画像表示ウィンドウ
	REFERENCE_WINDOW reference_window;
	// 環境設定
	PREFERENCE preference;
	// 描画領域の状態に合わせて有効、無効が切り替わるメニュー
	APPLICATION_MENUS menus;
	// 表示時に適用するフィルターのデータ
	DISPLAY_FILTER display_filter;
	// 描画領域の数
	int window_num;
	// 現在アクティブな描画領域
	int active_window;

	// 使用可能なフォントリスト
	PangoFontFamily** font_list;
	// 使用可能なフォントの数
	int num_font;

	// ブラシテクスチャ用
	TEXTURES textures;

	// UIに表示する文字列
	APPLICATION_LABELS *labels;
	FRACTAL_LABEL *fractal_labels;

	// レイヤーをまとめて作成するデータ
	LAYER_GROUP_TEMPLATES layer_group_templates;

	// システムのコード
	char *system_code;
	// 実行ファイルのパス
	char *current_path;
	// 言語ファイルのパス
	char *language_file_path;
	// 新規作成データファイルのパス
	char *make_new_file_path;
	// ブラシファイルのパス
	char *brush_file_path;
	// ベクトルブラシファイルのパス
	char *vector_brush_file_path;
	// 共通ツールファイルのパス
	char *common_tool_file_path;
	// バックアップを作成するディレクトリのパス
	char *backup_directory_path;
	// パターンファイルのあるディレクトリへのパス
	char *pattern_path;
	// スタンプファイルのあるディレクトリへのパス
	char *stamp_path;
	// テクスチャ画像のあるディレクトリへのパス
	char *texture_path;
	// スクリプトファイルのあるディレクトリへのパス
	char *script_path;

	// スクリプトのファイル情報
	SCRIPTS scripts;

	// 新規作成などのメニューで扱うデータ
	MENU_DATA menu_data;

	// ファイルオープン中等の操作中に立つフラグ
	unsigned int flags;

	// 印刷用のデータ
	GtkPrintSettings* print_settings;
	// 印刷するサーフェース
	cairo_surface_t* print_surface;
	// 印刷するピクセルデータ
	uint8* print_pixels;

	// 表示中のツール
	uint16 current_tool;

	// ICCプロファイルへのパス
	char *input_icc_path;
	char *output_icc_path;

	// ICCプロファイルの内容
	cmsHPROFILE input_icc;
	cmsHPROFILE output_icc;
	// ICCプロファイルによる色変換用
	cmsHTRANSFORM icc_transform;

	// パターン塗り潰し用
	PATTERNS patterns;
	// スタンプ用
	PATTERNS stamps;
	uint8 *stamp_buff;
	uint8 *stamp_shape;
	uint8 *stamp_alpha;
	size_t stamp_buff_size;

	// レイヤー合成用の関数ポインタ配列
	void (*layer_blend_functions[NUM_LAYER_BLEND_FUNCTIONS])(LAYER* src, LAYER* dst);
	void (*part_layer_blend_functions[NUM_LAYER_BLEND_FUNCTIONS])(LAYER* src, UPDATE_RECTANGLE* update);

	// 合成モードの定数配列
	cairo_operator_t layer_blend_operators[NUM_LAYER_BLEND_FUNCTIONS];

	// フィルター関数ポインタ配列
	filter_func filter_funcs[NUM_FILTER_FUNC];
	selection_filter_func selection_filter_funcs[NUM_FILTER_FUNC];

	// 吹き出しを描画する関数ポインタ配列
	void (*draw_balloon_functions[NUM_TEXT_LAYER_BALLOON_TYPE])(TEXT_LAYER*, LAYER*, DRAW_WINDOW*);

	// ブラシプレビュー用のキャンバス
	DRAW_WINDOW *brush_preview_canvas;

	// スクリプト・プラグイン用の文字列ハッシュテーブル
	ght_hash_table_t *label_table;

	// 3Dモデリング用データ
	void *modeling;

#ifndef _WIN32
	// Ubuntuではツールの切り替えが効かなくなるので
		// デバイスの設定を記憶する必要がある?
	int num_device;
	GdkInputSource *input_sources;
	gboolean *set_input_modes;
#endif
} APPLICATION;

// 関数のプロトタイプ宣言
/*********************************************************************
* InitializeApplication関数                                          *
* アプリケーションの初期化                                           *
* 引数                                                               *
* app				: アプリケーション全体を管理する構造体のアドレス *
* argv				: main関数の第一引数                             *
* argc				: main関数の第二引数                             *
* init_file_name	: 初期化ファイルの名前                           *
*********************************************************************/
EXTERN void InitializeApplication(APPLICATION* app, char** argv, int argc, char* init_file_name);

/*********************************************************
* CreateMainWindowWidgets関数                            *
* アプリケーションのメインウィンドウを作成する           *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
* 返り値                                                 *
*	作成したメインウィンドウウィジェットデータのアドレス *
*********************************************************/
EXTERN MAIN_WINDOW_WIDGETS_PTR CreateMainWindowWidgets(APPLICATION* app);

/*************************************************************************
* GetWriteMainWindowData関数                                             *
* 初期化ファイルに書き出すメインウィンドウウィジェットのデータを取得する *
* 引数                                                                   *
* write_data	: 初期化ファイルへの書き出し専用データのアドレス         *
* app			: アプリケーション全体を管理する構造体のアドレス         *
*************************************************************************/
EXTERN void GetWriteMainWindowData(WRITE_APPLICATIOIN_DATA* write_data, APPLICATION* app);

/*********************************************************
* ReadInitializeFile関数                                 *
* 初期化ファイルを読み込む                               *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
* file_path	: 初期化ファイルのパス                       *
* 返り値                                                 *
*	正常終了:0	失敗:負の値                              *
*********************************************************/
EXTERN int ReadInitializeFile(APPLICATION* app, const char* file_path, INITIALIZE_DATA* init_data);

/*********************************************************************
* UpdateWindowTitle関数                                              *
* ウインドウタイトルの更新                                           *
* 引数                                                               *
* app				: アプリケーション全体を管理する構造体のアドレス *
*********************************************************************/
EXTERN void UpdateWindowTitle(APPLICATION* app);

/*********************************************************
* GetActiveDrawWindow関数                                *
* アクティブな描画領域を取得する                         *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	アクティブな描画領域                                 *
*********************************************************/
EXTERN DRAW_WINDOW* GetActiveDrawWindow(APPLICATION* app);

/*****************************************************
* RecoverBackUp関数                                  *
* バックアップファイルが合った場合に復元する         *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void RecoverBackUp(APPLICATION* app);

EXTERN GtkWidget *CreateToolBoxWindow(struct _APPLICATION* app, GtkWidget *parent);

EXTERN void CreateBrushTable(
	APPLICATION* app,
	TOOL_WINDOW* window,
	BRUSH_CORE brush_data[BRUSH_TABLE_HEIGHT][BRUSH_TABLE_WIDTH]
);

EXTERN void CreateVectorBrushTable(
	APPLICATION* app,
	TOOL_WINDOW* window,
	VECTOR_BRUSH_CORE brush_data[VECTOR_BRUSH_TABLE_HEIGHT][VECTOR_BRUSH_TABLE_WIDTH]
);

EXTERN void CreateAdjustmentLayerWidget(LAYER* layer);

EXTERN void CreateChange3DLayerUI(
	APPLICATION* app,
	TOOL_WINDOW* window
);

/*********************************************************
* WriteCommonToolData関数                                *
* 共通ツールのデータを書き出す                           *
* 引数                                                   *
* window	: ツールボックス                             *
* file_path	: 書き出すファイルのパス                     *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	正常終了:0 失敗:負の値                               *
*********************************************************/
EXTERN int WriteCommonToolData(
	TOOL_WINDOW* window,
	const char* file_path,
	APPLICATION* app
);

/*********************************************************
* WriteVectorBrushData関数                               *
* ベクトルレイヤー用のブラシテーブルのデータを書き出す   *
* 引数                                                   *
* window	: ツールボックスウィンドウ                   *
* file_path	: 書き出すファイルのパス                     *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	正常終了:0	失敗:負の値                              *
*********************************************************/
EXTERN int WriteVectorBrushData(
	struct _TOOL_WINDOW* window,
	const char* file_path,
	struct _APPLICATION* app
);

/*******************************************************
* OnQuitApplication関数                                *
* アプリケーション終了前に呼び出されるコールバック関数 *
* 引数                                                 *
* app	: アプリケーションを管理する構造体のアドレス   *
* 返り値                                               *
*	終了中止:TRUE	終了続行:FALSE                     *
*******************************************************/
EXTERN gboolean OnQuitApplication(APPLICATION* app);

#ifndef INCLUDE_WIN_DEFAULT_API
/*********************************************************
* OpenFile関数                                           *
* 受けっとたパスのファイルを開く                         *
* 引数                                                   *
* file_path	: ファイルパス                               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
EXTERN void OpenFile(char *file_path, APPLICATION* app);
#endif

/*********************************************************
* ExecuteOpenFile関数                                    *
* ファイルを開く                                         *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void ExecuteOpenFile(APPLICATION* app);

/*********************************************************
* ExecuteOpenFileAsLayer関数                             *
* レイヤーとしてファイルを開く                           *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void ExecuteOpenFileAsLayer(APPLICATION* app);

/*********************************************************
* ExecuteSave関数                                        *
* 上書き保存                                             *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void ExecuteSave(APPLICATION* app);

/*********************************************************
* ExecuteSaveAs関数                                      *
* 名前を付けて保存                                       *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void ExecuteSaveAs(APPLICATION* app);

/*********************************************************
* ExecuteClose関数                                       *
* アクティブな描画領域を閉じる                           *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void ExecuteClose(APPLICATION* app);

/*****************************************************
* ExecuteMakeColorLayer関数                          *
* 通常レイヤー作成を実行                             *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteMakeColorLayer(APPLICATION *app);

/*****************************************************
* ExecuteMakeVectorLayer関数                         *
* 新規ベクトルレイヤー作成を実行                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteMakeVectorLayer(APPLICATION *app);

/*****************************************************
* ExecuteMakeLayerSet関数                            *
* 新規レイヤーセット作成を実行                       *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteMakeLayerSet(APPLICATION *app);

/*****************************************************
* ExecuteMakeAdjustmentLayer関数                     *
* 調整レイヤー作成を実行                             *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteMakeAdjustmentLayer(APPLICATION *app);

/*****************************************************
* ExecuteMake3DLayer関数                             *
* 3Dモデリングレイヤー作成を実行                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteMake3DLayer(APPLICATION* app);

/*********************************************************
* DeleteActiveLayer関数                                  *
* 現在のアクティブレイヤーを削除する                     *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void DeleteActiveLayer(APPLICATION* app);

/*****************************************************
* ExecuteUpLayer関数                                 *
* レイヤーの順序を1つ上に変更する                    *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteUpLayer(APPLICATION* app);

/*****************************************************
* ExecuteDownLayer関数                               *
* レイヤーの順序を1つ下に変更する                    *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteDownLayer(APPLICATION* app);

/*********************************************************
* FillForeGroundColor関数                                *
* 描画色でアクティブレイヤーを塗りつぶす                 *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void FillForeGroundColor(APPLICATION* app);

/*********************************************************
* FillPattern関数                                        *
* アクティブなパターンでアクティブなレイヤーを塗り潰す   *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void FillPattern(APPLICATION* app);

/*********************************************************
* FlipImageHorizontally関数                              *
* 描画領域を左右反転する                                 *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void FlipImageHorizontally(APPLICATION* app);

/*********************************************************
* FlipImageVertically関数                                *
* 描画領域を上下反転する                                 *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void FlipImageVertically(APPLICATION* app);

/*****************************************************
* SwitchSecondBackColor関数                          *
* 背景色を2つめのものと入れ替える                    *
* 引数                                               *
* menu	: メニューウィジェット                       *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void SwitchSecondBackColor(GtkWidget* menu, APPLICATION* app);

/*****************************************************
* Change2ndBackColor関数                             *
* 2つめの背景色を変更する                            *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void Change2ndBackColor(APPLICATION* app);

/*********************************************************
* MergeDownActiveLayer関数                               *
* アクティブなレイヤーを下のレイヤーと結合する           *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void MergeDownActiveLayer(APPLICATION* app);

/*********************************************************
* FlattenImage関数                                       *
* 画像の統合を実行                                       *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void FlattenImage(APPLICATION* app);

/*********************************************************
* ActiveLayerAlpha2SelectionArea関数                     *
* アクティブレイヤーの不透明部分を選択範囲にする         *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void ActiveLayerAlpha2SelectionArea(APPLICATION* app);

/*********************************************************
* ActiveLayerAlphaAddSelectionArea関数                   *
* アクティブレイヤーの不透明部分を選択範囲に加える       *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void ActiveLayerAlphaAddSelectionArea(APPLICATION* app);

/*********************************************************
* ExecuteCopyLayer関数                                   *
* レイヤーの複製を実行する                               *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void ExecuteCopyLayer(APPLICATION* app);

/*********************************************************
* ExecuteVisible2Layer関数                               *
* 可視部をレイヤーにする                                 *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void ExecuteVisible2Layer(APPLICATION* app);

/*********************************************************
* RasterizeActiveLayer関数                               *
* アクティブレイヤーをラスタライズする                   *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void RasterizeActiveLayer(APPLICATION* app);

/*********************************************************
* ExecuteSelectAll関数                                   *
* 全て選択を実行                                         *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
EXTERN void ExecuteSelectAll(APPLICATION* app);

/*****************************************************
* ExecuteZoomIn関数                                  *
* 拡大を実行                                         *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteZoomIn(APPLICATION *app);

/*****************************************************
* ExecuteZoomReset関数                               *
* 等倍表示を実行                                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteZoomReset(APPLICATION* app);

/*****************************************************
* ExecuteRotateClockwise関数                         *
* 表示を時計回りに回転                               *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteRotateClockwise(APPLICATION* app);

/*****************************************************
* ExecuteRotateCounterClockwise関数                  *
* 表示を反時計回りに回転                             *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteRotateCounterClockwise(APPLICATION* app);

/*****************************************************
* ExecuteRotateReset関数                             *
* 回転角度をリセットする                             *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteRotateReset(APPLICATION* app);

/*****************************************************
* ExecuteChangeResolution関数                        *
* キャンバスの解像度変更を実行                       *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteChangeResolution(APPLICATION* app);

/*****************************************************
* ExecuteZoomOut関数                                 *
* 縮小を実行                                         *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteZoomOut(APPLICATION *app);

/*****************************************************
* ExecuteChangeCanvasSize関数                        *
* キャンバスのサイズ変更を実行                       *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteChangeCanvasSize(APPLICATION* app);

/*****************************************************
* ExecuteChangeCanvasIccProfile関数                  *
* 引数                                               *
* menu	: メニューウィジェット                       *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteChangeCanvasIccProfile(GtkWidget* menu, APPLICATION* app);

EXTERN void ExecuteUndo(struct _APPLICATION* app);
EXTERN void ExecuteRedo(struct _APPLICATION* app);

/*********************************************************
* FillLayerPattern関数                                   *
* レイヤーをパターンで塗り潰す                           *
* 引数                                                   *
* target	: 塗り潰しを行うレイヤー                     *
* patterns	: パターンを管理する構造体のアドレス         *
* app		: アプリケーションを管理する構造体のアドレス *
* color		: パターンがグレースケールのときに使う色     *
*********************************************************/
EXTERN void FillLayerPattern(
	struct _LAYER* target,
	struct _PATTERNS* patterns,
	struct _APPLICATION* app,
	uint8 color[3]
);

EXTERN void AddLayerNameChangeHistory(
	APPLICATION* app,
	const gchar* before_name,
	const gchar* after_name
);

/*********************************************************************
* InitializeNavigation関数                                           *
* ナビゲーションウィンドウを初期化                                   *
* 引数                                                               *
* navigation	: ナビゲーションウィンドウを管理する構造体のアドレス *
* app			: アプリケーションを管理する構造体のアドレス         *
* box			: ドッキングする場合はボックスウィジェットを指定     *
*********************************************************************/
EXTERN void InitializeNavigation(
	NAVIGATION_WINDOW* navigation,
	APPLICATION *app,
	GtkWidget* box
);

EXTERN void InitializePreviewWindow(PREVIEW_WINDOW* preview, APPLICATION* app);

EXTERN void UnSetSelectionArea(APPLICATION* app);

EXTERN void InvertSelectionArea(APPLICATION* app);

/*****************************************************
* ReductSelectionArea関数                            *
* 選択範囲を縮小する                                 *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ReductSelectionArea(APPLICATION* app);

/*********************************************************
* ChangeEditSelectionMode関数                            *
* 選択範囲編集モードを切り替える                         *
* 引数                                                   *
* menu_item	: メニューアイテムウィジェット               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
EXTERN void ChangeEditSelectionMode(GtkWidget* menu_item, APPLICATION* app);

/*************************************************************
* Move2ActiveLayer                                           *
* レイヤービューをアクティブなレイヤーにスクロール           *
* widget		: アクティブレイヤーのウィジェット           *
* allocation	: ウィジェットに割り当てられたサイズ         *
* app			: アプリケーションを管理する構造体のアドレス *
*************************************************************/
EXTERN void Move2ActiveLayer(GtkWidget* widget, GdkRectangle * allocation, APPLICATION* app);

EXTERN void TextLayerButtonPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	GdkEventButton* event
);

EXTERN void TextLayerMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	GdkModifierType state
);

EXTERN void TextLayerButtonReleaseCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	GdkEventButton* event_info
);

EXTERN GtkWidget* CreateTextLayerDetailUI(APPLICATION* app, struct _LAYER* target, TEXT_LAYER* layer);

/*********************************************************
* ExecuteChangeToolWindowPlace関数                       *
* ツールボックスの位置を変更する                         *
* 引数                                                   *
* menu_item	: メニューアイテムウィジェット               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
EXTERN void ExecuteChangeToolWindowPlace(GtkWidget* menu_item, APPLICATION* app);

/*********************************************************
* ExecuteChangeNavigationLayerWindowPlace関数            *
* ナビゲーションとレイヤービューの位置を変更する         *
* 引数                                                   *
* menu_item	: 位置変更メニューアイテムウィジェット       *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
EXTERN void ExecuteChangeNavigationLayerWindowPlace(GtkWidget* menu_item, APPLICATION* app);

/*************************************************************
* TextFieldFocusIn関数                                       *
* テキストレイヤーの編集ウィジェットがフォーカスされた時に   *
*   呼び出されるコールバック関数                             *
* 引数                                                       *
* text_field	: テキストレイヤーの編集ウィジェット         *
* focus			: イベントの情報                             *
* app			: アプリケーションを管理する構造体のアドレス *
* 返り値                                                     *
*	常にFALSE                                                *
*************************************************************/
EXTERN gboolean TextFieldFocusIn(GtkWidget* text_field, GdkEventFocus* focus, APPLICATION* app);

/*******************************************************************
* TextFieldFocusOut関数                                            *
* テキストレイヤーの編集ウィジェットからフォーカスが移動したた時に *
*   呼び出されるコールバック関数                                   *
* 引数                                                             *
* text_field	: テキストレイヤーの編集ウィジェット               *
* focus			: イベントの情報                                   *
* app			: アプリケーションを管理する構造体のアドレス       *
* 返り値                                                           *
*	常にFALSE                                                      *
*******************************************************************/
EXTERN gboolean TextFieldFocusOut(GtkWidget* text_field, GdkEventFocus* focus, APPLICATION* app);

/*************************************************************
* OnDestroyTextField関数                                     *
* テキストレイヤーの編集ウィジェットが削除される時に         *
*   呼び出されるコールバック関数                             *
* 引数                                                       *
* text_field	: テキストレイヤーの編集ウィジェット         *
* app			: アプリケーションを管理する構造体のアドレス *
*************************************************************/
EXTERN void OnDestroyTextField(GtkWidget* text_field, APPLICATION* app);

/*****************************************************
* ExtendSelectionArea関数                            *
* 選択範囲を拡大する                                 *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExtendSelectionArea(APPLICATION* app);

/*****************************************************
* ExecuteSetPreference関数                           *
* 環境設定を変更する                                 *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void ExecuteSetPreference(APPLICATION* app);

/*****************************************************
* DisplayVersion関数                                 *
* バージョン情報を表示する                           *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void DisplayVersion(APPLICATION* app);

/*************************************************
* ChangeNavigationRotate関数                     *
* ナビゲーションの回転角度変更時に呼び出す関数   *
* 引数                                           *
* navigation	: ナビゲーションウィンドウの情報 *
* draw_window	: 表示する描画領域               *
*************************************************/
EXTERN void ChangeNavigationRotate(NAVIGATION_WINDOW* navigation, DRAW_WINDOW* window);

/*************************************************
* ChangeNavigationDrawWindow関数                 *
* ナビゲーションで表示する描画領域を変更する     *
* 引数                                           *
* navigation	: ナビゲーションウィンドウの情報 *
* draw_window	: 表示する描画領域               *
*************************************************/
EXTERN void ChangeNavigationDrawWindow(NAVIGATION_WINDOW* navigation, DRAW_WINDOW* window);

/*****************************************************
* NoDisplayFilter関数                                *
* 表示時のフィルターをオフ                           *
* 引数                                               *
* menu	: メニューウィジェット                       *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void NoDisplayFilter(GtkWidget* menu, APPLICATION* app);

/*****************************************************
* GrayScaleDisplayFilter関数                         *
* 表示時のフィルターをグレースケール変換のものへ     *
* 引数                                               *
* menu	: メニューウィジェット                       *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void GrayScaleDisplayFilter(GtkWidget* menu, APPLICATION* app);

/******************************************************************
* GrayScaleDisplayFilterYIQ関数                                   *
* 表示時のフィルターをグレースケール変換(YIQカラーモデル)のものへ *
* 引数                                                            *
* menu	: メニューウィジェット                                    *
* app	: アプリケーションを管理する構造体のアドレス              *
******************************************************************/
EXTERN void GrayScaleDisplayFilterYIQ(GtkWidget* menu, APPLICATION* app);

/*****************************************************
* IccProfileDisplayFilter関数                        *
* 表示時のフィルターをICCプロファイル適用のものへ    *
* 引数                                               *
* menu	: メニューウィジェット                       *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
EXTERN void IccProfileDisplayFilter(GtkWidget* menu, APPLICATION* app);

/*********************************************************
* CreateTextureChooser関数                               *
* テクスチャを選択するウィジェットを作成する             *
* 引数                                                   *
* texture	: テクスチャを管理する構造体のアドレス       *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	テクスチャを選択するウィンドウウィジェット           *
*********************************************************/
EXTERN GtkWidget* CreateTextureChooser(TEXTURES* textures, struct _APPLICATION* app);

/***************************************************************
* InitializeScripts関数                                        *
* スクリプトディレクトリにあるファイル一覧を作成               *
* 引数                                                         *
* scripts		: スクリプトファイルを管理する構造体のアドレス *
* scripts_path	: スクリプトディレクトリのパス                 *
***************************************************************/
EXTERN void InitializeScripts(SCRIPTS* scripts, const char* scripts_path);

/*********************************************************
* ExecuteScript関数                                      *
* メニューからスクリプトが選択された時のコールバック関数 *
* 引数                                                   *
* menu_item	: メニューアイテムウィジェット               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
EXTERN void ExecuteScript(GtkWidget* menu_item, APPLICATION* app);

/*********************************************************
* Change2LoupeMode関数                                   *
* ルーペモードへ移行する                                 *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
EXTERN void Change2LoupeMode(APPLICATION* app);

/*********************************************************
* ReturnFromLoupeMode関数                                *
* ルーペモードから戻る                                   *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
EXTERN void ReturnFromLoupeMode(APPLICATION* app);

/*********************************************************
* LayerViewSetDrawWindow関数                             *
* レイヤービューにキャンバスの全てのレイヤーをセットする *
* 引数                                                   *
* layer_window	: レイヤービューを持つウィンドウ         *
* draw_window	: 描画領域                               *
*********************************************************/
EXTERN void LayerViewSetDrawWindow(LAYER_WINDOW* layer_window, DRAW_WINDOW* draw_window);

/***********************************************
* ExecuteLayerGroupTemplateWindow関数          *
* レイヤーをまとめて作成する                   *
* 引数                                         *
* app	: アプリケーション全体を管理するデータ *
***********************************************/
EXTERN void ExecuteLayerGroupTemplateWindow(APPLICATION* app);

/*****************************************************************
* LoupeButtonToggled関数                                         *
* ルーペモード切り替えボタンがクリックされた時のコールバック関数 *
* 引数                                                           *
* button	: ボタンウィジェット                                 *
* app		: アプリケーションを管理する構造体のアドレス         *
*****************************************************************/
EXTERN void LoupeButtonToggled(GtkWidget* button, APPLICATION* app);

/****************************************************
* GetLayerColorHistgram関数                         *
* レイヤーのRGBA、CMYK、HSVのヒストグラムを取得する *
* 引数                                              *
* histgram	: ヒストグラムのデータを受ける変数      *
* target	: ヒストグラムを取得するレイヤー        *
****************************************************/
EXTERN void GetLayerColorHistgram(COLOR_HISTGRAM* histgram, LAYER* target);

/*****************************************
* SetFilterFunctions関数                 *
* フィルター関数ポインタ配列の中身を設定 *
* 引数                                   *
* functions	: フィルター関数ポインタ配列 *
*****************************************/
extern void SetFilterFunctions(filter_func* functions);

/***********************************************************
* SetSelectionFilterFunctions関数                          *
* 選択範囲の編集中のフィルター関数ポインタ配列の中身を設定 *
* 引数                                                     *
* functions	: フィルター関数ポインタ配列                   *
***********************************************************/
extern void SetSelectionFilterFunctions(selection_filter_func* functions);

/**********************************************************
* MemoryAllocate関数                                      *
* KABURAGI / MIKADOで使用するメモリアロケータでメモリ確保 *
* 引数                                                    *
* size	: 確保するバイト数                                *
* 返り値                                                  *
*	確保したメモリのアドレス                              *
**********************************************************/
EXTERN void* MemoryAllocate(size_t size);

/******************************************************************
* MemoryReallocate関数                                            *
* KABURAGI / MIKADOで使用するメモリアロケータでバッファサイズ変更 *
* 引数                                                            *
* address	: サイズ変更するバッファ                              *
* size		: 新しいバッファサイズ                                *
* 返り値                                                          *
*	サイズ変更後のバッファのアドレス                              *
******************************************************************/
EXTERN void* MemoryReallocate(void* address, size_t size);

/**************************************************************
* MemoryFree関数                                              *
* KABURAGI / MIKADOで確保されたメモリを開放する(プラグイン用) *
* 引数                                                        *
* memory	: 開放するメモリのポインタ                        *
**************************************************************/
EXTERN void MemoryFree(void* memory);

/*******************************************************************
* GetLabelString関数                                               *
* 日本語環境なら日本語の英語環境なら英語のラベル用文字列を取得する *
* 引数                                                             *
* app	: アプリケーションを管理するデータ                         *
* key	: 基本的に英語環境時のラベル文字列                         *
* 返り値                                                           *
*	ラベル用文字列(free厳禁)                                       *
*******************************************************************/
EXTERN char* GetLabelString(APPLICATION* app, const char* key);

/*********************************************************
* SetHas3DLayer関数                                      *
* 3Dモデリングの有効/無効を設定する                      *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
* enable	: 有効:TRUE	無効:FALSE                       *
*********************************************************/
EXTERN void SetHas3DLayer(APPLICATION* app, int enable);

/*****************************************************
* GetHas3DLayer関数                                  *
* 3Dモデリングの有効/無効を返す                      *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
* 返り値                                             *
*	3Dモデリング有効:TRUE	無効:FALSE               *
*****************************************************/
EXTERN int GetHas3DLayer(APPLICATION* app);

/*****************************************************************
* Load3dModelingLabels関数                                       *
* 3Dモデル操作時のラベルデータを読み込む                         *
* app				: アプリケーションを管理する構造体のアドレス *
* lang_file_path	: ラベルのテキストデータファイルのパス       *
*****************************************************************/
EXTERN void Load3dModelingLabels(APPLICATION* app, const char* lang_file_path);

/***********************************************************
* SetActiveBrushTableButton関数                            *
* 通常レイヤー時のアクティブなブラシのボタン状態を設定する *
* (詳細設定は変更しない)                                   *
* 引数                                                     *
* tool			: ツールボックスの情報                     *
* brush_core	: アクティブにするブラシのデータ           *
***********************************************************/
EXTERN void SetActiveBrushTableButton(TOOL_WINDOW* tool, void* brush_core);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_APPLICATION_H_
