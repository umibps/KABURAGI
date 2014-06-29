#ifndef _INCLUDED_APPLICATION_H_
#define _INCLUDED_APPLICATION_H_

#include "configure.h"

#if !defined(USE_3D_LAYER) || USE_3D_LAYER == 0
# define APPLICATION_NAME "KABURAGI"
#else
# define APPLICATION_NAME "MIKADO"
#endif

#define MAJOR_VERSION 1

#if MAJOR_VERSION == 1
# define MINOR_VERSION 3
# define RELEASE_VERSION 1
# define BUILD_VERSION 6
#elif MAJOR_VERSION == 2
# define MINOR_VERSION 0
# define RELEASE_VERSION 1
# define BUILD_VERSION 1
#endif

#define FILE_VERSION 4

#include "draw_window.h"
// 描画領域の最大数
#define MAX_DRAW_WINDOW 128
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

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
# include "MikuMikuGtk+/mikumikugtk.h"
# include "MikuMikuGtk+/ui_label.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

// 初期化ファイルのファイルパス
#define INITIALIZE_FILE_NAME "application.ini"
// パレットファイルのパス
#define PALLETE_FILE_NAME "pallete.kpl"
// デフォルトの解像度
#define DEFALUT_RESOLUTION 96

typedef enum _eUTILITY_PLACE
{
	UTILITY_PLACE_WINDOW,
	UTILITY_PLACE_LEFT,
	UTILITY_PLACE_RIGHT
} eUTILITY_PLACE;

typedef enum _eTOOL_WINDOW_FLAGS
{
	TOOL_USING_BRUSH = 0x01,
	TOOL_DOCKED = 0x02,
	TOOL_PLACE_RIGHT = 0x04,
	TOOL_SHOW_COLOR_CIRCLE = 0x08,
	TOOL_SHOW_COLOR_PALLETE = 0x10
} eTOOL_WINDOW_FLAGS;

/*************************************************
* eAPPLICATION_FLAGS列挙体                       *
* アプリケーションのファイル操作中等で立つフラグ *
*************************************************/
typedef enum _eAPPLICATION_FLAGS
{
	APPLICATION_IN_OPEN_OPERATION = 0x01,			// ファイルオープン
	APPLICATION_IN_MAKE_NEW_DRAW_AREA = 0x02,		// 描画領域の新規作成
	APPLICATION_IN_REVERSE_OPERATION = 0x04,		// 左右反転処理中
	APPLICATION_IN_EDIT_SELECTION = 0x08,			// 選択範囲編集切り替え中
	APPLICATION_INITIALIZED = 0x10,					// 初期化済み
	APPLICATION_IN_DELETE_EVENT = 0x20,				// 削除イベント内
	APPLICATION_FULL_SCREEN = 0x40,					// フルスクリーンモード
	APPLICATION_WINDOW_MAXIMIZE = 0x80,				// ウィンドウの最大化
	APPLICATION_DISPLAY_GRAY_SCALE = 0x100,			// グレースケールでの表示
	APPLICATION_DISPLAY_SOFT_PROOF = 0x200,			// ICCプロファイルでソフトプルーフ表示
	APPLICATION_REMOVE_ACCELARATOR = 0x400,			// ショートカットキーを無効化
	APPLICATION_DRAW_WITH_TOUCH = 0x800,			// タッチイベントで描画する
	APPLICATION_SET_BACK_GROUND_COLOR = 0x1000,		// キャンバスの背景を設定する
	APPLICATION_SHOW_PREVIEW_ON_TASK_BAR = 0x2000,	// プレビューウィンドウをタスクバーに表示する
	APPLICATION_IN_SWITCH_DRAW_WINDOW = 0x4000		// 描画領域の切替中
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

typedef enum _eINPUT_DEVICE
{
	INPUT_PEN,
	INPUT_ERASER
} eINPUT_DEVICE;

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
	void* copy_brush;
	// ブラシファイルの文字コード
	char* brush_code;
	// 共通ツールのブラシテーブル
	COMMON_TOOL_CORE common_tools[COMMON_TOOL_TABLE_HEIGHT][COMMON_TOOL_TABLE_WIDTH];
	// 現在使用している共通ツールへのポインタ
	COMMON_TOOL_CORE* active_common_tool;
	// 共通ツールファイルの文字コード
	char* common_tool_code;
	// ベクトルブラシのコアテーブル
	VECTOR_BRUSH_CORE vector_brushes[VECTOR_BRUSH_TABLE_HEIGHT][VECTOR_BRUSH_TABLE_WIDTH];
	// 現在使用しているベクトルブラシへのポインタ
	VECTOR_BRUSH_CORE *active_vector_brush[2];
	// ベクトルブラシファイルの文字コード
	char* vector_brush_code;
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

/***********************
* MENU_DATA構造体      *
* メニューで扱うデータ *
***********************/
typedef struct _MENU_DATA
{
	struct
	{
		int32 width, height;
		int16 resolution;
		uint8 second_bg_color[3];
	} make_new;

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

/*************************************
* APPLICATION構造体                  *
* アプリケーション全体のデータを管理 *
*************************************/
typedef struct _APPLICATION
{
	// ウィンドウ、メニュー、タブを入れるパッキングボックス、タブ
	GtkWidget *window, *vbox, *note_book;
	// メニューバー
	GtkWidget *menu_bar;
	// ステータスバー
	GtkWidget *status_bar;
	// 保存、フィルター等の進捗状況を表すプログレスバー
	GtkWidget *progress;
	// シングルウィンドウ用にツールを入れる左右のペーン
	GtkWidget *left_pane, *right_pane;
	// ナビゲーションとレイヤービューをドッキングするためのボックス
	GtkWidget *navi_layer_pane;
	// ウィンドウの位置、サイズ
	int window_x, window_y, window_width, window_height;
	// 左右ペーンの位置
	gint left_pane_position, right_pane_position;
	// ショートカットキー
	GtkAccelGroup *hot_key;
	// 入力デバイス
	eINPUT_DEVICE input;
	// 描画領域
	DRAW_WINDOW* draw_window[MAX_DRAW_WINDOW];
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
	// 使用テクスチャのラベル
	GtkWidget *texture_label;

	// UIに表示する文字列
	APPLICATION_LABELS *labels;

	// システムのコード
	char* system_code;
	// 実行ファイルのパス
	char* current_path;
	// 言語ファイルのパス
	char* language_file_path;
	// ブラシファイルのパス
	char* brush_file_path;
	// ベクトルブラシファイルのパス
	char* vector_brush_file_path;
	// 共通ツールファイルのパス
	char* common_tool_file_path;
	// バックアップを作成するディレクトリのパス
	char *backup_directory_path;
	// パターンファイルのあるディレクトリへのパス
	char* pattern_path;
	// スタンプファイルのあるディレクトリへのパス
	char* stamp_path;
	// テクスチャ画像のあるディレクトリへのパス
	char* texture_path;
	// スクリプトファイルのあるディレクトリへのパス
	char* script_path;

	// スクリプトのファイル情報
	SCRIPTS scripts;

	// 新規作成などのメニューで扱うデータ
	MENU_DATA menu_data;

	// 左右反転のボタンとラベル
	GtkWidget *reverse_button, *reverse_label;

	// 選択範囲編集のボタン
	GtkWidget *edit_selection;

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
	uint8* stamp_buff;
	uint8* stamp_shape;
	uint8* stamp_alpha;
	size_t stamp_buff_size;

#ifndef _WIN32
	// Ubuntuではツールの切り替えが効かなくなるので
		// デバイスの設定を記憶する必要がある?
	int num_device;
	GdkInputSource *input_sources;
	gboolean *set_input_modes;
#endif

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
	// 3Dモデリング用データ
	void *modeling;
#endif
} APPLICATION;

// 関数のプロトタイプ宣言
/*********************************************************************
* InitializeApplication関数                                          *
* アプリケーションの初期化                                           *
* 引数                                                               *
* app				: アプリケーション全体を管理する構造体のアドレス *
* init_file_name	: 初期化ファイルの名前                           *
*********************************************************************/
extern void InitializeApplication(APPLICATION* app, char* init_file_name);

/*********************************************************
* GetActiveDrawWindow関数                                *
* アクティブな描画領域を取得する                         *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	アクティブな描画領域                                 *
*********************************************************/
extern DRAW_WINDOW* GetActiveDrawWindow(APPLICATION* app);

/*****************************************************
* RecoverBackUp関数                                  *
* バックアップファイルが合った場合に復元する         *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void RecoverBackUp(APPLICATION* app);

extern GtkWidget *CreateToolBoxWindow(struct _APPLICATION* app, GtkWidget *parent);

extern void CreateBrushTable(
	APPLICATION* app,
	TOOL_WINDOW* window,
	BRUSH_CORE brush_data[BRUSH_TABLE_HEIGHT][BRUSH_TABLE_WIDTH]
);

extern void CreateVectorBrushTable(
	APPLICATION* app,
	TOOL_WINDOW* window,
	VECTOR_BRUSH_CORE brush_data[VECTOR_BRUSH_TABLE_HEIGHT][VECTOR_BRUSH_TABLE_WIDTH]
);

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
extern void CreateChange3DLayerUI(
	APPLICATION* app,
	TOOL_WINDOW* window
);
#endif

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
extern int WriteCommonToolData(
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
extern int WriteVectorBrushData(
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
extern gboolean OnQuitApplication(APPLICATION* app);

#ifndef INCLUDE_WIN_DEFAULT_API
/*********************************************************
* OpenFile関数                                           *
* 受けっとたパスのファイルを開く                         *
* 引数                                                   *
* file_path	: ファイルパス                               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
extern void OpenFile(char *file_path, APPLICATION* app);
#endif

/*********************************************************
* ExecuteOpenFile関数                                    *
* ファイルを開く                                         *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void ExecuteOpenFile(APPLICATION* app);

/*********************************************************
* ExecuteOpenFileAsLayer関数                             *
* レイヤーとしてファイルを開く                           *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void ExecuteOpenFileAsLayer(APPLICATION* app);

/*********************************************************
* ExecuteSave関数                                        *
* 上書き保存                                             *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void ExecuteSave(APPLICATION* app);

/*********************************************************
* ExecuteSaveAs関数                                      *
* 名前を付けて保存                                       *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void ExecuteSaveAs(APPLICATION* app);

/*********************************************************
* ExecuteClose関数                                       *
* アクティブな描画領域を閉じる                           *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void ExecuteClose(APPLICATION* app);

/*****************************************************
* ExecuteMakeColorLayer関数                          *
* 通常レイヤー作成を実行                             *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteMakeColorLayer(APPLICATION *app);

/*****************************************************
* ExecuteMakeVectorLayer関数                         *
* 新規ベクトルレイヤー作成を実行                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteMakeVectorLayer(APPLICATION *app);

/*****************************************************
* ExecuteMakeLayerSet関数                            *
* 新規レイヤーセット作成を実行                       *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteMakeLayerSet(APPLICATION *app);

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
/*****************************************************
* ExecuteMake3DLayer関数                             *
* 3Dモデリングレイヤー作成を実行                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteMake3DLayer(APPLICATION* app);
#endif

/*********************************************************
* DeleteActiveLayer関数                                  *
* 現在のアクティブレイヤーを削除する                     *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void DeleteActiveLayer(APPLICATION* app);

/*****************************************************
* ExecuteUpLayer関数                                 *
* レイヤーの順序を1つ上に変更する                    *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteUpLayer(APPLICATION* app);

/*****************************************************
* ExecuteDownLayer関数                               *
* レイヤーの順序を1つ下に変更する                    *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteDownLayer(APPLICATION* app);

/*********************************************************
* FillForeGroundColor関数                                *
* 描画色でアクティブレイヤーを塗りつぶす                 *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void FillForeGroundColor(APPLICATION* app);

/*********************************************************
* FillPattern関数                                        *
* アクティブなパターンでアクティブなレイヤーを塗り潰す   *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void FillPattern(APPLICATION* app);

/*********************************************************
* FlipImageHorizontally関数                              *
* 描画領域を左右反転する                                 *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void FlipImageHorizontally(APPLICATION* app);

/*********************************************************
* FlipImageVertically関数                                *
* 描画領域を上下反転する                                 *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void FlipImageVertically(APPLICATION* app);

/*****************************************************
* SwitchSecondBackColor関数                          *
* 背景色を2つめのものと入れ替える                    *
* 引数                                               *
* menu	: メニューウィジェット                       *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void SwitchSecondBackColor(GtkWidget* menu, APPLICATION* app);

/*****************************************************
* Change2ndBackColor関数                             *
* 2つめの背景色を変更する                            *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void Change2ndBackColor(APPLICATION* app);

/*********************************************************
* MergeDownActiveLayer関数                               *
* アクティブなレイヤーを下のレイヤーと結合する           *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void MergeDownActiveLayer(APPLICATION* app);

/*********************************************************
* FlattenImage関数                                       *
* 画像の統合を実行                                       *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void FlattenImage(APPLICATION* app);

/*********************************************************
* ActiveLayerAlpha2SelectionArea関数                     *
* アクティブレイヤーの不透明部分を選択範囲にする         *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void ActiveLayerAlpha2SelectionArea(APPLICATION* app);

/*********************************************************
* ActiveLayerAlphaAddSelectionArea関数                   *
* アクティブレイヤーの不透明部分を選択範囲に加える       *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void ActiveLayerAlphaAddSelectionArea(APPLICATION* app);

/*********************************************************
* ExecuteCopyLayer関数                                   *
* レイヤーの複製を実行する                               *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void ExecuteCopyLayer(APPLICATION* app);

/*********************************************************
* ExecuteVisible2Layer関数                               *
* 可視部をレイヤーにする                                 *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void ExecuteVisible2Layer(APPLICATION* app);

/*********************************************************
* RasterizeActiveLayer関数                               *
* アクティブレイヤーをラスタライズする                   *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void RasterizeActiveLayer(APPLICATION* app);

/*********************************************************
* ExecuteSelectAll関数                                   *
* 全て選択を実行                                         *
* 引数                                                   *
* app	: アプリケーション全体を管理する構造体のアドレス *
*********************************************************/
extern void ExecuteSelectAll(APPLICATION* app);

/*****************************************************
* ExecuteZoomIn関数                                  *
* 拡大を実行                                         *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteZoomIn(APPLICATION *app);

/*****************************************************
* ExecuteZoomReset関数                               *
* 等倍表示を実行                                     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteZoomReset(APPLICATION* app);

/*****************************************************
* ExecuteRotateClockwise関数                         *
* 表示を時計回りに回転                               *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteRotateClockwise(APPLICATION* app);

/*****************************************************
* ExecuteRotateCounterClockwise関数                  *
* 表示を反時計回りに回転                             *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteRotateCounterClockwise(APPLICATION* app);

/*****************************************************
* ExecuteRotateReset関数                             *
* 回転角度をリセットする                             *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteRotateReset(APPLICATION* app);

/*****************************************************
* ExecuteChangeResolution関数                        *
* キャンバスの解像度変更を実行                       *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteChangeResolution(APPLICATION* app);

/*****************************************************
* ExecuteZoomOut関数                                 *
* 縮小を実行                                         *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteZoomOut(APPLICATION *app);

/*****************************************************
* ExecuteChangeCanvasSize関数                        *
* キャンバスのサイズ変更を実行                       *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteChangeCanvasSize(APPLICATION* app);

/*****************************************************
* ExecuteChangeCanvasIccProfile関数                  *
* 引数                                               *
* menu	: メニューウィジェット                       *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteChangeCanvasIccProfile(GtkWidget* menu, APPLICATION* app);

extern void ExecuteUndo(struct _APPLICATION* app);
extern void ExecuteRedo(struct _APPLICATION* app);

/*********************************************************
* FillLayerPattern関数                                   *
* レイヤーをパターンで塗り潰す                           *
* 引数                                                   *
* target	: 塗り潰しを行うレイヤー                     *
* patterns	: パターンを管理する構造体のアドレス         *
* app		: アプリケーションを管理する構造体のアドレス *
* color		: パターンがグレースケールのときに使う色     *
*********************************************************/
extern void FillLayerPattern(
	struct _LAYER* target,
	struct _PATTERNS* patterns,
	struct _APPLICATION* app,
	uint8 color[3]
);

extern void AddLayerNameChangeHistory(
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
extern void InitializeNavigation(
	NAVIGATION_WINDOW* navigation,
	APPLICATION *app,
	GtkWidget* box
);

extern void InitializePreviewWindow(PREVIEW_WINDOW* preview, APPLICATION* app);

extern void UnSetSelectionArea(APPLICATION* app);

extern void InvertSelectionArea(APPLICATION* app);

/*****************************************************
* ReductSelectionArea関数                            *
* 選択範囲を縮小する                                 *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ReductSelectionArea(APPLICATION* app);

/*********************************************************
* ChangeEditSelectionMode関数                            *
* 選択範囲編集モードを切り替える                         *
* 引数                                                   *
* menu_item	: メニューアイテムウィジェット               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
extern void ChangeEditSelectionMode(GtkWidget* menu_item, APPLICATION* app);

/*************************************************************
* Move2ActiveLayer                                           *
* レイヤービューをアクティブなレイヤーにスクロール           *
* widget		: アクティブレイヤーのウィジェット           *
* allocation	: ウィジェットに割り当てられたサイズ         *
* app			: アプリケーションを管理する構造体のアドレス *
*************************************************************/
extern void Move2ActiveLayer(GtkWidget* widget, GdkRectangle * allocation, APPLICATION* app);

extern void TextLayerButtonPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	GdkEventButton* event
);

extern void TextLayerMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	GdkModifierType state
);

extern GtkWidget* CreateTextLayerDetailUI(APPLICATION* app, struct _LAYER* target, TEXT_LAYER* layer);

/*********************************************************
* ExecuteChangeToolWindowPlace関数                       *
* ツールボックスの位置を変更する                         *
* 引数                                                   *
* menu_item	: メニューアイテムウィジェット               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
extern void ExecuteChangeToolWindowPlace(GtkWidget* menu_item, APPLICATION* app);

/*********************************************************
* ExecuteChangeNavigationLayerWindowPlace関数            *
* ナビゲーションとレイヤービューの位置を変更する         *
* 引数                                                   *
* menu_item	: 位置変更メニューアイテムウィジェット       *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
extern void ExecuteChangeNavigationLayerWindowPlace(GtkWidget* menu_item, APPLICATION* app);

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
extern gboolean TextFieldFocusIn(GtkWidget* text_field, GdkEventFocus* focus, APPLICATION* app);

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
extern gboolean TextFieldFocusOut(GtkWidget* text_field, GdkEventFocus* focus, APPLICATION* app);

/*************************************************************
* OnDestroyTextField関数                                     *
* テキストレイヤーの編集ウィジェットが削除される時に         *
*   呼び出されるコールバック関数                             *
* 引数                                                       *
* text_field	: テキストレイヤーの編集ウィジェット         *
* app			: アプリケーションを管理する構造体のアドレス *
*************************************************************/
extern void OnDestroyTextField(GtkWidget* text_field, APPLICATION* app);

/*****************************************************
* ExtendSelectionArea関数                            *
* 選択範囲を拡大する                                 *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExtendSelectionArea(APPLICATION* app);

/*****************************************************
* ExecuteSetPreference関数                           *
* 環境設定を変更する                                 *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteSetPreference(APPLICATION* app);

/*****************************************************
* DisplayVersion関数                                 *
* バージョン情報を表示する                           *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void DisplayVersion(APPLICATION* app);

/*************************************************
* ChangeNavigationRotate関数                     *
* ナビゲーションの回転角度変更時に呼び出す関数   *
* 引数                                           *
* navigation	: ナビゲーションウィンドウの情報 *
* draw_window	: 表示する描画領域               *
*************************************************/
extern void ChangeNavigationRotate(NAVIGATION_WINDOW* navigation, DRAW_WINDOW* window);

/*************************************************
* ChangeNavigationDrawWindow関数                 *
* ナビゲーションで表示する描画領域を変更する     *
* 引数                                           *
* navigation	: ナビゲーションウィンドウの情報 *
* draw_window	: 表示する描画領域               *
*************************************************/
extern void ChangeNavigationDrawWindow(NAVIGATION_WINDOW* navigation, DRAW_WINDOW* window);

/*****************************************************
* NoDisplayFilter関数                                *
* 表示時のフィルターをオフ                           *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void NoDisplayFilter(APPLICATION* app);

/*****************************************************
* GrayScaleDisplayFilter関数                         *
* 表示時のフィルターをグレースケール変換のものへ     *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void GrayScaleDisplayFilter(APPLICATION* app);

/******************************************************************
* GrayScaleDisplayFilterYIQ関数                                   *
* 表示時のフィルターをグレースケール変換(YIQカラーモデル)のものへ *
* 引数                                                            *
* app	: アプリケーションを管理する構造体のアドレス              *
******************************************************************/
extern void GrayScaleDisplayFilterYIQ(APPLICATION* app);

/*****************************************************
* IccProfileDisplayFilter関数                        *
* 表示時のフィルターをICCプロファイル適用のものへ    *
* 引数                                               *
* menu	: メニューウィジェット                       *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void IccProfileDisplayFilter(GtkWidget* menu, APPLICATION* app);

/*********************************************************
* CreateTextureChooser関数                               *
* テクスチャを選択するウィジェットを作成する             *
* 引数                                                   *
* texture	: テクスチャを管理する構造体のアドレス       *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	テクスチャを選択するウィンドウウィジェット           *
*********************************************************/
extern GtkWidget* CreateTextureChooser(TEXTURES* textures, struct _APPLICATION* app);

/***************************************************************
* InitializeScripts関数                                        *
* スクリプトディレクトリにあるファイル一覧を作成               *
* 引数                                                         *
* scripts		: スクリプトファイルを管理する構造体のアドレス *
* scripts_path	: スクリプトディレクトリのパス                 *
***************************************************************/
extern void InitializeScripts(SCRIPTS* scripts, const char* scripts_path);

/*********************************************************
* ExecuteScript関数                                      *
* メニューからスクリプトが選択された時のコールバック関数 *
* 引数                                                   *
* menu_item	: メニューアイテムウィジェット               *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
extern void ExecuteScript(GtkWidget* menu_item, APPLICATION* app);

/*********************************************************
* Change2LoupeMode関数                                   *
* ルーペモードへ移行する                                 *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
extern void Change2LoupeMode(APPLICATION* app);

/*********************************************************
* ReturnFromLoupeMode関数                                *
* ルーペモードから戻る                                   *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
*********************************************************/
extern void ReturnFromLoupeMode(APPLICATION* app);

/*********************************************************
* LayerViewSetDrawWindow関数                             *
* レイヤービューにキャンバスの全てのレイヤーをセットする *
* 引数                                                   *
* layer_window	: レイヤービューを持つウィンドウ         *
* draw_window	: 描画領域                               *
*********************************************************/
extern void LayerViewSetDrawWindow(LAYER_WINDOW* layer_window, DRAW_WINDOW* draw_window);

/*****************************************************************
* LoupeButtonToggled関数                                         *
* ルーペモード切り替えボタンがクリックされた時のコールバック関数 *
* 引数                                                           *
* button	: ボタンウィジェット                                 *
* app		: アプリケーションを管理する構造体のアドレス         *
*****************************************************************/
extern void LoupeButtonToggled(GtkWidget* button, APPLICATION* app);

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
/*****************************************************************
* Load3dModelingLabels関数                                       *
* 3Dモデル操作時のラベルデータを読み込む                         *
* app				: アプリケーションを管理する構造体のアドレス *
* lang_file_path	: ラベルのテキストデータファイルのパス       *
*****************************************************************/
extern void Load3dModelingLabels(APPLICATION* app, const char* lang_file_path);
#endif

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_APPLICATION_H_
