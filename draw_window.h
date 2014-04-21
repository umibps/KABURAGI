#ifndef _INCLUDED_DRAW_WINDOW_H_
#define _INCLUDED_DRAW_WINDOW_H_

#include "configure.h"
#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
# include <GL/glew.h>
#endif
#include <gtk/gtk.h>
#include "lcms/lcms2.h"
#include "layer.h"
#include "history.h"
#include "selection_area.h"
#include "memory_stream.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _eDRAW_WINDOW_FLAGS
{
	DRAW_WINDOW_HAS_SELECTION_AREA = 0x01,
	DRAW_WINDOW_UPDATE_ACTIVE_UNDER = 0x02,
	DRAW_WINDOW_UPDATE_ACTIVE_OVER = 0x04,
	DRAW_WINDOW_DISPLAY_HORIZON_REVERSE = 0x08,
	DRAW_WINDOW_EDIT_SELECTION = 0x10,
	DRAW_WINDOW_UPDATE_PART = 0x20,
	DRAW_WINDOW_DRAWING_STRAIGHT = 0x40,
	DRAW_WINDOW_SECOND_BG = 0x80,
	DRAW_WINDOW_TOOL_CHANGING = 0x100,
	DRAW_WINDOW_EDITTING_3D_MODEL = 0x200,
	DRAW_WINDOW_IS_FOCAL_WINDOW = 0x400
} eDRAW_WINDOW_FLAGS;

typedef struct _UPDATE_RECTANGLE
{
	FLOAT_T x, y;
	FLOAT_T width, height;
	cairo_t *cairo_p;
	cairo_surface_t *surface_p;
} UPDATE_RECTANGLE;

typedef struct _CALLBACK_IDS
{
	unsigned int display;
	unsigned int mouse_button_press;
	unsigned int mouse_move;
	unsigned int mouse_button_release;
	unsigned int mouse_wheel;
	unsigned int configure;
	unsigned int enter;
	unsigned int leave;
} CALLBACK_IDS;

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
typedef struct _GL_DRAW_POINT
{
	GLfloat texture_coord[2];
	GLfloat vertex[3];
} GL_DRAW_POINT;

typedef struct _GL_DRAW_DATA
{
	GLuint texture_name;
	GL_DRAW_POINT points[4];
	float length;
	float angle;
	struct
	{
		double x1, y1, x2, y2;
	} update_rectangle, before_rectangle;
} GL_DRAW_DATA;

#endif

typedef struct _DRAW_WINDOW
{
	uint8 channel;		// チャンネル数
	uint8 color_mode;	// カラーモード(現在はRGBA32のみ)

	int16 degree;		// 回転角(度数法)

	gchar* file_name;	// ファイル名

	gchar* file_path;	// ファイルパス

	// 時間で再描画を呼ぶ関数のID
	guint timer_id;
	guint auto_save_id;

	// タブ切り替え前のレイヤービューの位置
	int layer_view_position;

	// 画像のオリジナル幅、高さ
	int original_width, original_height;
	// 描画領域の幅、高さ(4の倍数)
	int width, height;
	// 一行分のバイト数、レイヤー一枚分のバイト数
	int stride, pixel_buf_size;
	// 回転用にマージンをとった描画領域の広さ、一行分のバイト数
	int disp_size, disp_stride;
	// 回転後の座標計算用
	FLOAT_T half_size;
	FLOAT_T angle;		// 回転角(弧度法)
	// 回転処理用の三角関数の値
	FLOAT_T sin_value, cos_value;
	// 回転処理での移動量
	FLOAT_T trans_x, trans_y;
	// カーソル座標
	FLOAT_T cursor_x, cursor_y;
	FLOAT_T before_cursor_x, before_cursor_y;
	// 前回チェック時のカーソル座標
	FLOAT_T before_x, before_y;
	// 最後にクリックまたはドラッグされた座標
	FLOAT_T last_x, last_y;
	// 最後にクリックまたはドラッグされた時の筆圧
	FLOAT_T last_pressure;
	// カーソル座標補正用
	FLOAT_T add_cursor_x, add_cursor_y;
	// 平均法手ブレ補正での座標変換用
	FLOAT_T rev_add_cursor_x, rev_add_cursor_y;
	// 表示用のパターン
	cairo_pattern_t *rotate;
	// 画面部分更新用
	UPDATE_RECTANGLE update, temp_update;
	// 描画領域スクロールの座標
	int scroll_x, scroll_y;
	// 画面更新時のクリッピング用
	int update_clip_area[4][2];

	// WindowsとLinuxでイベントの呼ばれる順序が異なるようなので回避用
	GdkModifierType state;

	// コールバック関数のIDを記憶
	CALLBACK_IDS callbacks;

	uint8 *back_ground;		// 背景のピクセルデータ
	uint8 *brush_buffer;	// ブラシ用のバッファ

	LAYER* layer;			// 一番下のレイヤー
	// 表示用、エフェクト用、ブラシカーソル表示用の一時保存
	LAYER* disp_layer, *effect, *disp_temp, *scaled_mixed;
	// アクティブなレイヤー&レイヤーセットへのポインタ
		// 及び表示レイヤーを合成したレイヤー
	LAYER* active_layer, *active_layer_set, *mixed_layer;
	// 作業用、一時保存用、選択範囲、アクティブレイヤーより下のレイヤー
	LAYER *work_layer, *temp_layer,
		*selection, *under_active;
	// マスクとマスク適用前の一時保存用
	LAYER* mask, *mask_temp;
	// テクスチャ用
	LAYER* texture;
	// 表示用パターン
	cairo_pattern_t *mixed_pattern;
	// αチャンネルのみのマスク用サーフェース
	cairo_surface_t *alpha_surface, *alpha_temp, *gray_mask_temp;
	// αチャンネルのみのイメージ
	cairo_t *alpha_cairo, *alpha_temp_cairo, *gray_mask_cairo;

	uint16 num_layer;		// レイヤーの数
	uint16 zoom;			// 拡大・縮小率
	FLOAT_T zoom_rate;		// 浮動小数点型の拡大・縮小率
	FLOAT_T rev_zoom;		// 拡大・縮小率の逆数

	// 解像度
	int16 resolution;

	// 選択範囲アニメーション用のタイマー
	GTimer *timer;

	// 自動保存用のタイマー
	GTimer *auto_save_timer;

	// 選択範囲表示用データ
	SELECTION_AREA selection_area;

	// 履歴データ
	HISTORY history;

	// 描画領域のウィジェット
	GtkWidget* window;

	// 描画領域のスクロール
	GtkWidget* scroll;

	// 表示更新等のフラグ
	unsigned int flags;

	// 変形処理用のデータ
	struct _TRANSFORM_DATA* transform;

	// 2つめの背景色
	uint8 second_back_ground[3];

	// 表示フィルターの状態
	uint8 display_filter_mode;

	// ICCプロファイルのデータ
	char *icc_profile_path;
	void *icc_profile_data;
	int32 icc_profile_size;
	// ICCプロファイルの内容
	cmsHPROFILE input_icc;
	// ICCプロファイルによる色変換用
	cmsHTRANSFORM icc_transform;

	// 局所キャンバス
	struct _DRAW_WINDOW *focal_window;

	// 追加データ
	void *extra_data;

	// アプリケーション全体管理用構造体へのポインタ
	struct _APPLICATION* app;

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
	// 3Dモデリング用データ
	void *first_project;
	// 頂点・テクスチャデータ
	GL_DRAW_DATA gl_data;
#endif
} DRAW_WINDOW;

// 関数のプロトタイプ宣言
/***************************************************************
* CreateDrawWindow関数                                         *
* 描画領域を作成する                                           *
* 引数                                                         *
* width		: キャンバスの幅                                   *
* height	: キャンバスの高さ                                 *
* channel	: キャンバスのチャンネル数(RGB:3, RGBA:4)          *
* name		: キャンバスの名前                                 *
* note_book	: 描画領域タブウィジェット                         *
* window_id	: 描画領域配列中のID                               *
* app		: アプリケーションの情報を管理する構造体のアドレス *
* 返り値                                                       *
*	描画領域の情報を管理する構造体のアドレス                   *
***************************************************************/
extern DRAW_WINDOW* CreateDrawWindow(
	int32 width,
	int32 height,
	uint8 channel,
	const gchar* name,
	GtkWidget* note_book,
	uint16 window_id,
	struct _APPLICATION* app
);

/***************************************************************
* CreateTempDrawWindow関数                                     *
* 一時的な描画領域を作成する                                   *
* 引数                                                         *
* width		: キャンバスの幅                                   *
* height	: キャンバスの高さ                                 *
* channel	: キャンバスのチャンネル数(RGB:3, RGBA:4)          *
* name		: キャンバスの名前                                 *
* note_book	: 描画領域タブウィジェット                         *
* window_id	: 描画領域配列中のID                               *
* app		: アプリケーションの情報を管理する構造体のアドレス *
* 返り値                                                       *
*	描画領域の情報を管理する構造体のアドレス                   *
***************************************************************/
extern DRAW_WINDOW* CreateTempDrawWindow(
	int32 width,
	int32 height,
	uint8 channel,
	const gchar* name,
	GtkWidget* note_book,
	uint16 window_id,
	struct _APPLICATION* app
);

/***************************************
* DeleteDrawWindow関数                 *
* 描画領域の情報を削除                 *
* 引数                                 *
* window	: 描画領域の情報のアドレス *
***************************************/
extern void DeleteDrawWindow(DRAW_WINDOW** window);

/*****************************************************
* SwapDrawWindowFromMemoryStream関数                 *
* メモリー上の描画領域データと入れ替える             *
* 引数                                               *
* window	: 描画領域の情報                         *
* stream	: メモリー上の描画領域のデータストリーム *
*****************************************************/
extern void SwapDrawWindowFromMemoryStream(DRAW_WINDOW* window, MEMORY_STREAM_PTR stream);

/***********************************************
* OnCloseDrawWindow関数                        *
* タブが閉じられるときのコールバック関数       *
* 引数                                         *
* data	: 描画領域のデータ                     *
* page	: 閉じるタブのID                       *
* 返り値                                       *
*	閉じる操作の中止:TRUE 閉じる操作続行:FALSE *
***********************************************/
extern gboolean OnCloseDrawWindow(void* data, gint page);

/***********************************************************
* GetWindowID関数                                          *
* 描画領域のIDを取得する                                   *
* 引数                                                     *
* window	: 描画領域の情報                               *
* app		: 	アプリケーションを管理する構造体のアドレス *
* 返り値                                                   *
*	描画領域のID (不正な描画領域ならば-1)                  *
***********************************************************/
int GetWindowID(DRAW_WINDOW* window, struct _APPLICATION* app);

/*********************************
* DrawWindowChangeZoom関数       *
* 描画領域の拡大縮小率を変更する *
* 引数                           *
* window	: 描画領域の情報     *
* zoom		: 新しい拡大縮小率   *
*********************************/
extern void DrawWindowChangeZoom(
	DRAW_WINDOW* window,
	int16 zoom
);

/*****************************************
* FlipDrawWindowHorizontally関数         *
* 描画領域を水平反転する                 *
* 引数                                   *
* window	: 水平反転する描画領域の情報 *
*****************************************/
extern void FlipDrawWindowHorizontally(DRAW_WINDOW* window);

/*****************************************
* FlipDrawWindowVertically関数           *
* 描画領域を垂直反転する                 *
* 引数                                   *
* window	: 垂直反転する描画領域の情報 *
*****************************************/
extern void FlipDrawWindowVertically(DRAW_WINDOW* window);

/***************************************
* LayerAlpha2SelectionArea関数         *
* レイヤーの不透明部分を選択範囲にする *
* 引数                                 *
* window	: 描画領域の情報           *
***************************************/
extern void LayerAlpha2SelectionArea(DRAW_WINDOW* window);

/*****************************************
* LayerAlphaAddSelectionArea関数         *
* レイヤーの不透明部分を選択範囲に加える *
* 引数                                   *
* window	: 描画領域の情報             *
*****************************************/
extern void LayerAlphaAddSelectionArea(DRAW_WINDOW* window);

/*****************************
* MergeAllLayer関数          *
* 全てのレイヤーを結合する   *
* 引数                       *
* window	: 描画領域の情報 *
*****************************/
extern void MergeAllLayer(DRAW_WINDOW* window);

/*******************************************
* ChangeDrawWindowResolution関数           *
* 解像度を変更する                         *
* 引数                                     *
* window		: 解像度を変更する描画領域 *
* new_width		: 新しい幅                 *
* new_height	: 新しい高さ               *
*******************************************/
extern void ChangeDrawWindowResolution(DRAW_WINDOW* window, int32 new_width, int32 new_height);

/*************************************************
* AddChangeDrawWindowResolutionHistory関数       *
* 解像度変更の履歴データを追加する               *
* 引数                                           *
* window		: 解像度を変更する描画領域の情報 *
* new_width		: 新しい幅                       *
* new_height	: 新しい高さ                     *
*************************************************/
extern void AddChangeDrawWindowResolutionHistory(
	DRAW_WINDOW* window,
	int32 new_width,
	int32 new_height
);

/*******************************************
* ChangeDrawWindowSize関数                 *
* 描画領域のサイズを変更する               *
* 引数                                     *
* window		: 解像度を変更する描画領域 *
* new_width		: 新しい幅                 *
* new_height	: 新しい高さ               *
*******************************************/
extern void ChangeDrawWindowSize(DRAW_WINDOW* window, int32 new_width, int32 new_height);

/*****************************************
* UpdateDrawWindowClippingArea           *
* 画面更新時にクリッピングする領域を更新 *
* 引数                                   *
* window	: 描画領域の情報             *
*****************************************/
extern void UpdateDrawWindowClippingArea(DRAW_WINDOW* window);

/*************************************************
* ClipUpdateArea関数                             *
* 画面のスクロールに入っている部分でクリッピング *
* 引数                                           *
* window	: 描画領域の情報                     *
* cairo_p	: Cairo情報                          *
*************************************************/
extern void ClipUpdateArea(DRAW_WINDOW* window, cairo_t* cairo_p);

/*******************************************************************
* DrawWindowSetIccProfile関数                                      *
* キャンバスにICCプロファイルを割り当てる                          *
* 引数                                                             *
* window	: 描画領域の情報(icc_profile_dataにデータ割り当て済み) *
* data_size	: ICCプロファイルのデータのバイト数                    *
* ask_set	: ソフトプルーフ表示を適用するかを尋ねるか否か         *
*******************************************************************/
extern void DrawWindowSetIccProfile(DRAW_WINDOW* window, int32 data_size, gboolean ask_set);

/*************************************************
* AddChangeDrawWindowSizeHistory関数             *
* キャンバスサイズ変更の履歴データを追加する     *
* 引数                                           *
* window		: 解像度を変更する描画領域の情報 *
* new_width		: 新しい幅                       *
* new_height	: 新しい高さ                     *
*************************************************/
extern void AddChangeDrawWindowSizeHistory(
	DRAW_WINDOW* window,
	int32 new_width,
	int32 new_height
);

extern void RasterizeVectorLine(
	DRAW_WINDOW* window,
	VECTOR_LAYER* layer,
	VECTOR_LINE* line,
	VECTOR_LAYER_RECTANGLE* rect
);

extern void RasterizeVectorLayer(
	DRAW_WINDOW* window,
	LAYER* target,
	VECTOR_LAYER* layer
);

/***********************************************
* IsVectorLineInSelectionArea関数              *
* 線が選択範囲内か否かを判定する               *
* 引数                                         *
* window			: 描画領域の情報           *
* selection_pixels	: 選択範囲のピクセルデータ *
* line				: 判定する線               *
* 返り値                                       *
*	選択範囲内:1	選択範囲外:0               *
***********************************************/
extern int IsVectorLineInSelectionArea(
	DRAW_WINDOW* window,
	uint8* selection_pixels,
	VECTOR_LINE* line
);

/*******************************************
* AddControlPointHistory関数               *
* 制御点追加の履歴を作成する               *
* 引数                                     *
* window	: 描画領域の情報               *
* layer		: 制御点を追加したレイヤー     *
* line		: 制御点を追加したライン       *
* point		: 追加した制御点のアドレス     *
* tool_name	: 制御点を追加したツールの名前 *
*******************************************/
extern void AddControlPointHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_LINE* line,
	VECTOR_POINT* point,
	const char* tool_name
);

/***************************************************
* AddTopLineControlPointHistory関数                *
* 折れ線ツールか曲線ツールでの制御点追加履歴を追加 *
* 引数                                             *
* window		: 描画領域の情報                   *
* layer			: 制御点を追加したレイヤー         *
* line			: 制御点を追加したライン           *
* point			: 追加した制御点                   *
* tool_name		: 制御点を追加したツールの名前     *
* add_line_flag	: ライン追加をしたといのフラグ     *
***************************************************/
extern void AddTopLineControlPointHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_LINE* line,
	VECTOR_POINT* point,
	const char* tool_name,
	uint8 add_line_flag
);

/***********************************************************
* AddDeleteLinesHistory関数                                *
* 複数の線を同時に削除した際の履歴データを追加             *
* 引数                                                     *
* window		: 描画領域の情報を管理する構造体のアドレス *
* active_layer	: アクティブなレイヤー                     *
* line_data		: 削除する線のデータ                       *
* line_indexes	: 削除する線の一番下から数えたインデックス *
* num_lines		: 削除する線の数                           *
* tool_name		: 削除に使用したツールの名前               *
***********************************************************/
extern void AddDeleteLinesHistory(
	DRAW_WINDOW* window,
	LAYER* active_layer,
	VECTOR_LINE* line_data,
	uint32* line_indexes,
	uint32 num_lines,
	const char* tool_name
);

/*******************************************
* DeleteLayerSet関数                       *
* レイヤーセットの削除を行う               *
* 引数                                     *
* layer_set	: 削除するレイヤーセット       *
* window	: レイヤーセットを持つ描画領域 *
*******************************************/
extern void DeleteLayerSet(LAYER* layer_set, DRAW_WINDOW* window);

/*************************************************
* MixLayerSet関数                                *
* レイヤーセット内を合成                         *
* 引数                                           *
* bottom	: レイヤーセットの一番下のレイヤー   *
* next		: 合成後に次に合成するレイヤー       *
* window	: 描画領域を管理する構造体のアドレス *
*************************************************/
extern void MixLayerSet(LAYER* bottom, LAYER** next, DRAW_WINDOW* window);

/***************************************************************
* MixLayerSetActiveOver関数                                    *
* レイヤーセット中のアクティブレイヤー以上のレイヤーを合成する *
* 引数                                                         *
* start		: アクティブレイヤー                               *
* next		: 合成後の次に合成するレイヤー                     *
* window	: 描画領域を管理する構造体のアドレス               *
***************************************************************/
extern void MixLayerSetActiveOver(LAYER* start, LAYER** next, DRAW_WINDOW* window);

extern void RenderTextLayer(DRAW_WINDOW* window, struct _LAYER* target, TEXT_LAYER* layer);

extern void DisplayTextLayerRange(DRAW_WINDOW* window, TEXT_LAYER* layer);

extern void DisplayEditSelection(DRAW_WINDOW* window);

/***************************************
* g_part_layer_blend_funcs配列         *
* レイヤー部分合成関数へのポインタ配列 *
* 引数                                 *
* src		: 合成するレイヤー         *
* update	: 更新範囲の情報           *
***************************************/
extern void (*g_part_layer_blend_funcs[])(LAYER* src, UPDATE_RECTANGLE* update);

/*****************************************
* AutoSave関数                           *
* バックアップとしてファイルを保存する   *
* 引数                                   *
* window	: バックアップを取る描画領域 *
*****************************************/
extern void AutoSave(DRAW_WINDOW* window);

/***************************************************
* GetBlendedUnderLayer関数                         *
* 対象より下のレイヤーを合成したレイヤーを取得する *
* 引数                                             *
* target			: 対象のレイヤー               *
* window			: 描画領域の情報               *
* use_back_ground	: 背景色を使用するかどうか     *
* 返り値                                           *
*	合成したレイヤー                               *
***************************************************/
extern LAYER* GetBlendedUnderLayer(LAYER* target, DRAW_WINDOW* window, int use_back_ground);

extern void DivideLinesUndo(DRAW_WINDOW* window, void* p);
extern void DivideLinesRedo(DRAW_WINDOW* window, void* p);

extern void ScrollSizeChangeEvent(GtkWidget* scroll, GdkRectangle* size, DRAW_WINDOW* window);

extern gboolean DrawWindowConfigurEvent(GtkWidget* widget, GdkEventConfigure* event_info, DRAW_WINDOW* window);

/*********************************
* Change2FocalMode関数           *
* 局所キャンバスモードに移行する *
* 引数                           *
* parent_window	: 親キャンバス   *
*********************************/
extern void Change2FocalMode(DRAW_WINDOW* parent_window);

/*********************************
* ReturnFromFocalMode関数        *
* 局所キャンバスモードから戻る   *
* 引数                           *
* parent_window	: 親キャンバス   *
*********************************/
extern void ReturnFromFocalMode(DRAW_WINDOW* parent_window);

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
extern void InitializeGL(DRAW_WINDOW* window);
extern void ResizeGL(DRAW_WINDOW* window, int width, int height);
extern void RotateGL(DRAW_WINDOW* window, float angle);
#endif

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_DRAW_WINDOW_H_
