#ifndef _INCLUDED_BRUSHES_H_
#define _INCLUDED_BRUSHES_H_

#include <gtk/gtk.h>
#include "types.h"
#include "brush_core.h"
#include "ini_file.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MINIMUM_PARALLEL_SIZE 50

typedef enum _eBRUSH_FLAGS
{
	BRUSH_FLAG_SIZE = 0x01,
	BRUSH_FLAG_FLOW = 0x02,
	BRUSH_FLAG_ENTER = 0x04,
	BRUSH_FLAG_OUT = 0x08,
	BRUSH_FLAG_ROTATE = 0x10,
	BRUSH_FLAG_ANTI_ALIAS = 0x20,
	BRUSH_FLAG_USE_OLD_ANTI_ALIAS = 0x40
} eBRUSH_FLAGS;

typedef struct _PENCIL
{
	BRUSH_CORE *core;
	int base_scale;
	gdouble r;
	gdouble alpha;
	gdouble outline_hardness;
	gdouble minimum_pressure;
	gdouble before_x, before_y;

	int8 channel;
	uint8 shape;
	uint8 brush_mode;
	uint8 flags;

	uint16 blend_mode;
} PENCIL;

#define BRUSH_POINT_BUFFER_SIZE 256

typedef struct _HARD_PEN
{
	BRUSH_CORE *core;
	int base_scale;
	gdouble r;
	gdouble alpha;
	gdouble minimum_pressure;
	gdouble before_x, before_y;
	gdouble before_alpha;
	gdouble before_r;

	unsigned int flags;

	int num_points;

	uint16 blend_mode;
} HARD_PEN;

typedef struct _AIR_BRUSH
{
	BRUSH_CORE *core;
	int base_scale;
	gdouble r, opacity;
	gdouble outline_hardness;
	gdouble before_x, before_y;
	gdouble blur;
	FLOAT_T points[BRUSH_POINT_BUFFER_SIZE][4];
	FLOAT_T sum_distance, travel;
	FLOAT_T finish_length;
	FLOAT_T draw_start;
	FLOAT_T enter_length;
	FLOAT_T enter_size;
	uint16 blend_mode;
	int num_point;
	int draw_finished;
	int ref_point;
	gdouble enter, out;
	int flags;
} AIR_BRUSH;

typedef enum _eBLEND_BRUSH_TARGET
{
	BLEND_BRUSH_TARGET_UNDER_LAYER,
	BLEND_BRUSH_TARGET_CANVAS
} eBLEND_BRUSH_TARGET;

typedef struct _BLEND_BRUSH
{
	BRUSH_CORE *core;
	int base_scale;
	gdouble r, opacity;
	gdouble outline_hardness;
	gdouble before_x, before_y;
	gdouble blur;
	FLOAT_T points[BRUSH_POINT_BUFFER_SIZE][4];
	FLOAT_T sum_distance, travel;
	FLOAT_T finish_length;
	FLOAT_T draw_start;
	FLOAT_T enter_length;
	FLOAT_T enter_size;
	uint16 blend_mode;
	uint8 target;
	int num_point;
	int draw_finished;
	int ref_point;
	gdouble enter, out;
	int flags;
} BLEND_BRUSH;

typedef struct _OLD_AIR_BRUSH
{
	gdouble r, opacity;
	gdouble outline_hardness;
	gdouble before_x, before_y;
	gdouble blur;
	int flags;
} OLD_AIR_BRUSH;

typedef struct _WATER_COLOR_BRUSH
{
	BRUSH_CORE *core;
	int base_scale;
	FLOAT_T r, alpha;
	FLOAT_T outline_hardness;
	FLOAT_T blur;
	FLOAT_T extend;
	FLOAT_T mix;
	FLOAT_T enter, out;
	uint8 before_color[4];
	FLOAT_T points[BRUSH_POINT_BUFFER_SIZE][4];
	FLOAT_T before_x, before_y;
	FLOAT_T last_draw_x, last_draw_y;
	FLOAT_T sum_distance, travel;
	FLOAT_T finish_length;
	FLOAT_T draw_start;
	FLOAT_T enter_length;
	FLOAT_T enter_size;
	int num_point;
	int draw_finished;
	int ref_point;
	int flags;
} WATER_COLOR_BRUSH;

typedef enum _eSTAMP_FLAGS
{
	STAMP_PRESSURE_SIZE = 0x01,
	STAMP_PRESSURE_FLOW = 0x02,
	STAMP_FLIP_HORIZONAL = 0x04,
	STAMP_FLIP_VERTICAL = 0x08,
	STAMP_RANDOM_SIZE = 0x20,
	STAMP_RANDOM_ROTATE = 0x40
} eSTAMP_FLAGS;

typedef enum _eSTAMP_MODE
{
	STAMP_MODE_SATURATION,
	STAMP_MODE_BRIGHTNESS
} eSTAMP_MODE;

/*****************************
* STAMP_CORE構造体           *
* スタンプ系ツールの基本情報 *
*****************************/
typedef struct _STAMP_CORE
{
	// ブラシの基本情報
	BRUSH_CORE *brush_core;
	// スタンプの拡大縮小率、濃度
	gdouble scale, flow;
	// 拡大縮小率の逆数(行列設定が反転してる?)
	gdouble inv_scale;
	// スタンプの半径
	gdouble r;
	// スタンプの幅、高さの半分
	gdouble half_width, half_height;
	// 前に描画した座標
	gdouble before_x, before_y;
	// スタンプ間の距離
	gdouble d, stamp_distance;
	// スタンプの角度
	gdouble rotate;
	// スタンプの回転開始角度
	gdouble rotate_start;
	// スタンプの回転速度
	gdouble rotate_speed;
	// スタンプの回転方向
	int rotate_direction;

	// スタンプ描画用のサーフェース
	cairo_surface_t *brush_surface;
	// カーソル描画用のサーフェース
	cairo_surface_t *cursor_surface;

	// 使用パターンのID
	int32 stamp_id;

	// 左右反転、上下反転のフラグ
	uint8 pattern_flags;
	// 筆圧設定などのフラグ
	uint8 flags;

	// 着色モード変数のアドレス
	uint8* mode;
	// 着色モード選択ウィジェット
	GtkWidget* mode_select[3];

	// パターン選択用のボタン
	GtkWidget** buttons;
	// ボタンの数
	int32 num_button;

	// アプリケーションを管理する構造体へのポインタ
	struct _APPLICATION* app;
} STAMP_CORE;

/*****************************
* STAMP構造体                *
* スタンプツールの詳細データ *
*****************************/
typedef struct _STAMP
{
	STAMP_CORE core;
	FLOAT_T points[BRUSH_POINT_BUFFER_SIZE][4];
	uint8 mode;
} STAMP;

typedef struct _IMAGE_BRUSH
{
	STAMP_CORE core;
	FLOAT_T points[BRUSH_POINT_BUFFER_SIZE][4];
	FLOAT_T sum_distance, travel;
	FLOAT_T finish_length;
	FLOAT_T draw_start;
	FLOAT_T enter_length;
	FLOAT_T enter_size;
	FLOAT_T stamp_distance;
	FLOAT_T remain_distance;
	FLOAT_T size_range;
	FLOAT_T rotate_range;
	uint16 blend_mode;
	int16 random_rotate_range;
	int num_point;
	int draw_finished;
	int ref_point;
	gdouble enter, out;
	uint8 mode;
} IMAGE_BRUSH;

typedef struct _BLEND_IMAGE_BRUSH
{
	STAMP_CORE core;
	int base_scale;
	FLOAT_T points[BRUSH_POINT_BUFFER_SIZE][4];
	FLOAT_T sum_distance, travel;
	FLOAT_T finish_length;
	FLOAT_T draw_start;
	FLOAT_T enter_length;
	FLOAT_T enter_size;
	FLOAT_T stamp_distance;
	FLOAT_T remain_distance;
	FLOAT_T size_range;
	FLOAT_T rotate_range;
	uint16 blend_mode;
	int16 random_rotate_range;
	uint8 target;
	uint8 mode;
	int num_point;
	int draw_finished;
	int ref_point;
	gdouble enter, out;
	int flags;
} BLEND_IMAGE_BRUSH;

typedef enum _ePICKER_FLAGS
{
	PICKER_FLAG_PRESSURE_SIZE = 0x01,
	PICKER_FLAG_PRESSURE_FLOW = 0x02,
	PICKER_FLAG_CHANGE_HSV = 0x04,
	PICKER_FLAG_ANTI_ALIAS = 0x08
} ePICKER_FLAGS;

typedef enum _ePICKER_MODE
{
	PICKER_MODE_SINGLE_PIXEL,
	PICKER_MODE_AVERAGE
} ePICKER_MODE;

typedef struct _PICKER_BRUSH
{
	BRUSH_CORE *core;

	int base_scale;
	FLOAT_T r;
	FLOAT_T blur;
	FLOAT_T outline_hardness;
	FLOAT_T alpha;
	FLOAT_T before_x, before_y;

	int add_h, add_s, add_v;

	uint16 blend_mode;
	uint8 picker_mode;
	uint8 picker_source;
	unsigned int flags;
} PICKER_BRUSH;

typedef struct _PICKER_IMAGE_BRUSH
{
	STAMP_CORE core;
	FLOAT_T points[BRUSH_POINT_BUFFER_SIZE][4];
	FLOAT_T sum_distance, travel;
	FLOAT_T finish_length;
	FLOAT_T draw_start;
	FLOAT_T enter_length;
	FLOAT_T enter_size;
	FLOAT_T stamp_distance;
	FLOAT_T remain_distance;
	FLOAT_T size_range;
	FLOAT_T rotate_range;
	uint16 blend_mode;
	int16 random_rotate_range;
	uint8 picker_mode;
	uint8 picker_source;
	int num_point;
	int draw_finished;
	int ref_point;
	int add_h, add_s, add_v;
	gdouble enter, out;
	uint8 flags;
} PICKER_IMAGE_BRUSH;

typedef struct _ERASER
{
	BRUSH_CORE *core;
	int base_scale;
	gdouble r;
	gdouble alpha;
	gdouble outline_hardness;
	gdouble blur;
	gdouble before_x, before_y;

	int8 channel;
	uint8 shape;
	uint8 brush_mode;
	uint8 flags;
} ERASER;

typedef enum _eBUCKET_SELECT_MODE
{
	BUCKET_RGB,
	BUCKET_RGBA,
	BUCKET_ALPHA
} eBUCKET_SELECT_MODE;

typedef enum _eBUCKET_TARGET
{
	BUCKET_TARGET_ACTIVE_LAYER,
	BUCKET_TARGET_CANVAS
} eBUCKET_TARGET;

typedef enum _eBUCKET_FLAGS
{
	BUCKET_FLAG_ANTI_ALIAS = 0x01
} eBUCKET_FLAGS;

typedef struct _BUCKET
{
	uint16 threshold;
	int16 extend;
	uint8 mode;
	uint8 target;
	uint8 select_direction;
	uint8 flags;
} BUCKET;

typedef enum _ePATTERN_FILL_SELECT_MODE
{
	PATTERN_FILL_RGB,
	PATTERN_FILL_RGBA,
	PATTERN_FILL_ALPHA
} ePATTERN_FILL_SELECT_MODE;

typedef enum _ePATTERN_FILL_TARGET
{
	PATTERN_FILL_TARGET_ACTIVE_LAYER,
	PATTERN_FILL_TARGET_CANVAS
} ePATTERN_FILL_TARGET;

typedef enum _ePATTERN_FILL_FLAGS
{
	PATTERN_FILL_FLAG_ANTI_ALIAS = 0x01
} ePATTERN_FILL_FLAGS;

/*******************************
* PATTERN_FILL構造体           *
* パターン塗り潰しの詳細データ *
*******************************/
typedef struct _PATTERN_FILL
{
	// アプリケーションの情報
	struct _APPLICATION* app;
	gdouble scale;					// パターンの拡大率
	gdouble flow;					// 塗り潰す濃度
	GtkWidget** buttons;			// パターン選択用のボタン配列
	GtkWidget *mode_select[2];		// チャンネル数2のときの合成モード選択ウィジェット
	int32 num_button;				// パターン選択用のボタンの数
	int32 pattern_id;				// 使用中のパターンID
	uint16 threshold;				// 塗り潰し範囲決定用の閾値
	uint8 mode;						// 塗り潰し範囲決定モード
	uint8 target;					// 塗り潰し範囲決定ターゲット
	uint8 pattern_flags;			// パターンの左右反転、上下反転のフラグ
	uint8 pattern_mode;				// パターンのチャンネル数が2の時のモード
	uint8 area_detect_direction;	// 塗り潰す範囲の検出方向
	uint8 flags;					// アンチエイリアス等のフラグ
	int16 extend;					// 塗りつぶし範囲の拡大縮小
} PATTERN_FILL;

typedef enum _eGRADATION_MODE
{
	GRADATION_DRAW_RGB_TO_BACK_RGB,
	GRADATION_DRAW_RGB_TO_TRANSPARENT,
	GRADATION_DRAW_RGB_TO_BACK_RGB_BILINEAR,
	GRADATION_DRAW_RGB_TO_TRANSPARENT_BILINEAR,
	GRADATION_DRAW_RGB_TO_BACK_RGB_REPEAT,
	GRADATION_DRAW_RGB_TO_TRANSPARENT_REPEAT,
	GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY,
	GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY,
	GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_BILINEAR,
	GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_BILINEAR,
	GRADATION_DRAW_RGB_TO_BACK_RGB_RADIALLY_REPEAT,
	GRADATION_DRAW_RGB_TO_TRANSPARENT_RADIALLY_REPEAT,
	GRADATION_DRAW_IRIDESCENE_LINEAR,
	GRADATION_DRAW_IRIDESCENE_RADIALLY,
	GRADATION_MODE_NUM
} eDRADATION_MODE;

typedef enum _eGRADATION_FLAGS
{
	GRADATION_STARTED = 0x01,
	GRADATION_COLOR_REVERSE = 0x02
} eGRADATION_FLAGS;

#define GRADATION_PATTERN_TABLE_WIDTH 4
#define GRADATION_PATTERN_TABLE_HEIGHT 4

typedef struct _GRADATION
{
	GtkWidget* button_table[GRADATION_PATTERN_TABLE_HEIGHT][
		GRADATION_PATTERN_TABLE_WIDTH];
	struct _APPLICATION *app;
	gdouble start_x, start_y;
	gdouble end_x, end_y;
	uint16 flags;
	uint16 mode;
} GRADATION;

typedef struct _BLUR_TOOL
{
	BRUSH_CORE *core;
	int base_scale;
	gdouble r;
	gdouble alpha;
	gdouble outline_hardness;
	gdouble opacity;
	gdouble blur;
	gdouble color_extend;
	gdouble before_x, before_y;

	int8 channel;
	uint8 shape;
	uint8 brush_mode;
	uint8 flags;
} BLUR_TOOL;

typedef enum _eSMUDGE_FLAGS
{
	SMUDGE_PRESSURE_SIZE = 0x01,
	SMUDGE_PRESSURE_EXTENTION = 0x02,
	SMUDGE_INITIALIZED = 0x04,
	SMUDGE_DRAW_STARTED = 0x08
} eSMUDGE_FLAGS;

typedef struct _SMUDGE
{
	BRUSH_CORE *core;
	int base_scale;
	gdouble r, before_r;
	gdouble before_x, before_y;
	gdouble blur, opacity;
	gdouble outline_hardness;
	gdouble extention;
	uint32 flags;
} SMUDGE;

typedef struct _MIX_BRUSH
{
	int base_scale;				// ブラシサイズの倍率
	gdouble r;					// ブラシサイズ
	gdouble alpha;				// ブラシ濃度
	gdouble blur;				// ボケ足
	gdouble outline_hardness;	// 輪郭の硬さ
	gdouble before_x, before_y;	// 1ステップ前の座標
	int flags;					// 筆圧設定等のフラグ
} MIX_BRUSH;

typedef enum _eCUSTOM_BRUSH_SHAPES
{
	CUSTOM_BRUSH_SHAPE_CIRCLE,
	CUSTOM_BRUSH_SHAPE_TRIANGLE,
	CUSTOM_BRUSH_SHAPE_SQUARE,
	CUSTOM_BRUSH_SHAPE_HEXIAGON,
	CUSTOM_BRUSH_SHAPE_STAR,
	CUSTOM_BRUSH_SHAPE_PATTERN,
	CUSTOM_BRUSH_SHAPE_IMAGE,
	NUM_CUSTOM_BRUSH_SHAPES
} eCUSTOM_BRUSH_SHAPE;

typedef enum _eCUSTOM_BRUSH_MODE
{
	CUSTOM_BRUSH_MODE_NORMAL,
	CUSTOM_BRUSH_MODE_BLEND,
	CUSTOM_BRUSH_MODE_SMUDGE,
	CUSTOM_BRUSH_MODE_PICKER,
	CUSTOM_BRUSH_MODE_WATER_BRUSH,
	NUM_CUSTOM_BRUSH_MODE
} eCUSTOM_BRUSH_MODE;

typedef enum _eCUSTOM_BRUSH_FLAGS
{
	CUSTOM_BRUSH_FLAG_PRESSURE_SIZE,
	CUSTOM_BRUSH_FLAG_PRESSURE_FLOW,
	CUSTOM_BRUSH_FLAG_ROTATE_BY_CURSOR,
	CUSTOM_BRUSH_FLAG_RANDOM_SIZE,
	CUSTOM_BRUSH_FLAG_RANDOM_ROTATE,
	CUSTOM_BRUSH_FLAG_ANTI_ALIAS,
	CUSTOM_BRUSH_FLAG_IMAGE_FLIP_HORIZONTALLY,
	CUSTOM_BRUSH_FLAG_IMAGE_FLIP_VERTICALLY,
	CUSTOM_BRUSH_FLAG_ENTER_OUT_SIZE,
	CUSTOM_BRUSH_FLAG_ENTER_OUT_FLOW,
	CUSTOM_BRUSH_FLAG_DRAW_ONLY_SCATTER
} eCUSTOM_BRUSH_FLAGS;

typedef struct _CUSTOM_BRUSH
{
	BRUSH_CORE *core;					// 基本情報
	BRUSH_UPDATE_AREA update;			// キャンバスの更新範囲
	int base_scale;						// x0.1、x1、x10
	FLOAT_T r;							// ブラシ半径
	FLOAT_T scale;						// ブラシの拡大率
	FLOAT_T alpha;						// ブラシ濃度
	FLOAT_T blur;						// ボケ足
	FLOAT_T outline_hardness;			// 輪郭の硬さ
	FLOAT_T enter;						// 入り
	FLOAT_T out;						// 抜き
	FLOAT_T angle;						// 角度
	FLOAT_T rotate_speed;				// 回転速度
	FLOAT_T random_size;				// サイズに対するランダム要素
	FLOAT_T random_angle;				// 角度に対するランダム要素
	// 入り・抜き処理用のバッファ
	FLOAT_T points[BRUSH_POINT_BUFFER_SIZE][4];
	FLOAT_T sum_distance, travel;		// クリックしてからの移動量
	FLOAT_T finish_length;				// 抜きの距離
	FLOAT_T draw_start;					// 次に描画する距離
	FLOAT_T enter_length;				// 入りの距離
	FLOAT_T enter_size;					// 入りのサイズ
	FLOAT_T brush_distance;				// 描画間隔
	FLOAT_T draw_distance;				// 次の描画までの間隔
	FLOAT_T distance;					// ブラシが移動した距離
	FLOAT_T remain_distance;			// カーソル移動距離 - 描画終了距離
	FLOAT_T start_angle;				// 初期角度
	int rotate_direction;				// 回転方向
	uint16 blend_mode;					// 合成モード
	uint8 blend_target;					// 合成時の色決定用のターゲット
	uint8 brush_shape;					// ブラシの形状
	uint8 color_mode;					// 着色方法
	uint8 brush_mode;					// ブラシの動作モード
	uint8 picker_mode;					// スポイトブラシの色取得モード
	int16 hue;							// スポイトブラシモードでの色相変化量
	int16 saturation;					// スポイトブラシモードでの彩度変化量
	int16 brightness;					// スポイトブラシモードでの明度変化量
	int num_point;						// 記録した座標の数
	int draw_finished;					// 描画終了した座標の数
	int ref_point;						// 現在参照している配列のインデックス
	gdouble before_x, before_y;			// 前回描画時の座標
	uint8 *brush_pixel;					// ブラシのピクセルデータ
	uint8 *temp_pixel;					// マスク処理等の一時保存用ピクセルデータ
	guint32 flags;						// フラグ
	uint8 *image_pixel;					// 画像使用時のピクセルデータ
	int image_size;						// 画像の長辺の長さ
	int image_width;					// 画像の幅
	int image_height;					// 画像の高さ
	int image_channel;					// 画像のチャンネル数
	FLOAT_T half_width;					// 画像幅の半分(描画行列計算用)
	FLOAT_T half_height;				// 画像高さの半分(描画行列計算用)
	char *image_path;					// 画像のパス
	uint8 before_color[4];				// 水彩ブラシ用の色記憶バッファ
	FLOAT_T last_draw_x;				// 水彩ブラシ用の前回描画時のX座標
	FLOAT_T last_draw_y;				// 水彩ブラシ用の前回描画時のY座標
	uint8 mix;							// 水彩ブラシの混色の割合
	uint8 extend;						// 水彩ブラシと指先ツールの色延びの割合
	int before_width;					// 指先ツール用の前回描画時の幅
	int before_height;					// 指先ツール用の前回描画時の高さ
	int num_scatter;					// 散布量
	FLOAT_T scatter_size;				// 散布サイズ
	FLOAT_T scatter_random_size;		// 散布サイズのランダム量
	FLOAT_T scatter_range;				// 散布範囲
	FLOAT_T scatter_random_flow;		// 散布濃度のランダム量
	uint8 *cursor_pixel;				// カーソル用ピクセルデータ
	cairo_surface_t *cursor_surface;	// カーソル用サーフェース
	GtkWidget *vbox;					// 設定用ウィジェット
	GtkWidget *button_image;			// ボタン用のイメージウィジェット
	GtkWidget **pattern_buttons;		// パターン選択用のボタンウィジェット配列
	int num_pattern_buttons;			// パターン選択用のボタンの数
	int pattern_id;						// 選択しているパターン
	GtkWidget *mode_detail_setting;		// ブラシのモード毎の設定用ウィジェット
} CUSTOM_BRUSH;

typedef enum _eTEXT_TOOL_FLAGS
{
	TEXT_TOOL_STARTED = 0x01,
	TEXT_TOOL_VERTICAL = 0x02,
	TEXT_TOOL_BOLD = 0x04,
	TEXT_TOOL_ITALIC = 0x08,
	TEXT_TOOL_OBLIQUE = 0x10,
	TEXT_TOOL_BALLOON_HAS_EDGE = 0x20,
	TEXT_TOOL_CENTERING_HORIZONTALLY = 0x40,
	TEXT_TOOL_CENTERING_VERTICALLY = 0x80,
	TEXT_TOOL_ADJUST_RANGE_TO_TEXT = 0x100
} eTEXT_TOOL_FLAGS;

typedef struct _TEXT_TOOL
{
	gdouble start_x, start_y;
	gdouble end_x, end_y;
	int base_size;
	gdouble font_size;
	uint32 flags;
	int32 font_id;
	gdouble line_width;
	uint16 balloon_type;
	TEXT_LAYER_BALLOON_DATA balloon_data;
	TEXT_LAYER_BALLOON_DATA_WIDGETS balloon_widgets;
	uint8 back_color[4];
	uint8 line_color[4];
} TEXT_TOOL;

typedef struct _PLUG_IN_BRUSH
{
	char *plug_in_name;
	struct _APPLICATION *app;
	void *detail_data;
	GtkWidget* (*setting_widget_new)(struct _APPLICATION* app, struct _BRUSH_CORE* core);
} PLUG_IN_BRUSH;

extern void LoadBrushDetailData(
	BRUSH_CORE* core,
	INI_FILE_PTR file,
	const char* section_name,
	const char* brush_type,
	struct _APPLICATION* app
);

extern void LoadBrushDefaultData(
	BRUSH_CORE* core,
	eBRUSH_TYPE brush_type,
	struct _APPLICATION* app
);

/*********************************************************
* WriteBrushDetailData関数                               *
* ブラシの詳細設定を書き出す                             *
* 引数                                                   *
* window	: ツールボックスウィンドウ                   *
* file_path	: 書き出すファイルのパス                     *
* app		: アプリケーションを管理する構造体のアドレス *
* 返り値                                                 *
*	正常終了:0	失敗:負の値                              *
*********************************************************/
extern int WriteBrushDetailData(
	struct _TOOL_WINDOW* window,
	const char* file_path,
	struct _APPLICATION *app
);

/*****************************************************
* SetBrushCallBack関数                               *
* ブラシのマウス関連のコールバック関数を設定する     *
* 引数                                               *
* core	: コールバック関数を設定するブラシの基本情報 *
*****************************************************/
extern void SetBrushCallBack(BRUSH_CORE* core);

/*************************************************************
* SetEditSelectionCallBack関数                               *
* ブラシのマウス関連のコールバック関数を選択範囲編集用にする *
* 引数                                                       *
* core	: コールバック関数を設定するブラシの基本情報         *
*************************************************************/
extern void SetEditSelectionCallBack(BRUSH_CORE* core);

extern gboolean LoadPlugInBrushCallbacks(GModule* module, BRUSH_CORE* core, PLUG_IN_BRUSH* brush);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_BRUSHES_H_
