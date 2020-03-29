#ifndef _INCLUDED_TEXT_LAYER_H_
#define _INCLUDED_TEXT_LAYER_H_

#include <gtk/gtk.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TEXT_LAYER_SELECT_POINT_DISTANCE 25	// テキスト描画範囲を変更する際の受付範囲

typedef enum _eTEXT_STYLE
{
	TEXT_STYLE_NORMAL,
	TEXT_STYLE_ITALIC,
	TEXT_STYLE_OBLIQUE
} eTEXT_STYLE;

/*********************************
* eTEXT_LAYER_FLAGS列挙体        *
* テキストレイヤーの表示用フラグ *
*********************************/
typedef enum _eTEXT_LAYER_FLAGS
{
	TEXT_LAYER_VERTICAL = 0x01,					// 縦書き
	TEXT_LAYER_BOLD = 0x02,						// 太字
	TEXT_LAYER_ITALIC = 0x04,					// イタリック体
	TEXT_LAYER_OBLIQUE = 0x08,					// 斜体
	TEXT_LAYER_BALLOON_HAS_EDGE = 0x10,			// 吹き出しに角があるか
	TEXT_LAYER_CENTERING_HORIZONTALLY = 0x20,	// 水平方向を中央揃え
	TEXT_LAYER_CENTERING_VERTICALLY = 0x40,		// 垂直方向を中央揃え
	TEXT_LAYER_ADJUST_RANGE_TO_TEXT = 0x80		// テキストに合わせて描画範囲を調整
} eTEXT_LAYER_FLAGS;

/*********************************
* eTEXT_LAYER_BALLOON_TYPE列挙体 *
* 吹き出しのタイプ               *
*********************************/
typedef enum _eTEXT_LAYER_BALLOON_TYPE
{
	TEXT_LAYER_BALLOON_TYPE_SQUARE,
	TEXT_LAYER_BALLOON_TYPE_ECLIPSE,
	TEXT_LAYER_BALLOON_TYPE_ECLIPSE_THINKING,
	TEXT_LAYER_BALLOON_TYPE_CRASH,
	TEXT_LAYER_BALLOON_TYPE_SMOKE,
	TEXT_LAYER_BALLOON_TYPE_SMOKE_THINKING1,
	TEXT_LAYER_BALLOON_TYPE_SMOKE_THINKING2,
	TEXT_LAYER_BALLOON_TYPE_BURST,
	NUM_TEXT_LAYER_BALLOON_TYPE
} eTEXT_LAYER_BALLOON_TYPE;

/********************************
* TEXT_LAYER_BALLOON_DATA構造体 *
* 吹き出しの形状を決めるデータ  *
********************************/
typedef struct _TEXT_LAYER_BALLOON_DATA
{
	uint16 num_edge;				// 尖りの数
	uint16 num_children;			// 引き出し線の小さい吹き出しの数
	FLOAT_T edge_size;				// 尖りのサイズ
	gint32 random_seed;				// 乱数の初期値
	FLOAT_T edge_random_size;		// 尖りのサイズのランダム変化量
	FLOAT_T edge_random_distance;	// 尖りの間隔のランダム変化量
	FLOAT_T start_child_size;		// 引き出し線の小さい吹き出しの開始サイズ
	FLOAT_T end_child_size;			// 引き出し線の小さい吹き出しの終了サイズ
} TEXT_LAYER_BALLOON_DATA;

/*********************************************
* TEXT_LAYER_BALLOON_DATA_WIDGETS構造体      *
* 吹き出しの詳細形状を設定するウィジェット群 *
*********************************************/
typedef struct _TEXT_LAYER_BALLOON_DATA_WIDGETS
{
	GtkAdjustment *num_edge;
	GtkAdjustment *edge_size;
	GtkAdjustment *random_seed;
	GtkAdjustment *edge_random_size;
	GtkAdjustment *edge_random_distance;
	GtkAdjustment *num_children;
	GtkAdjustment *start_child_size;
	GtkAdjustment *end_child_size;
} TEXT_LAYER_BALLOON_DATA_WIDGETS;

/*********************************************
* TEXT_LAYER_POINTS構造体                    *
* テキストレイヤー上でのドラッグ処理のデータ *
*********************************************/
typedef struct _TEXT_LAYER_POINTS
{
	int point_index;
	gdouble points[9][2];
} TEXT_LAYER_POINTS;

/*******************************
* TEXT_LAYER構造体             *
* テキストレイヤーの情報を格納 *
*******************************/
typedef struct _TEXT_LAYER
{
	GtkWidget* text_field;								// テキストビューウィジェット
	GtkTextBuffer* buffer;								// 描画テキストバッファ
	char* text;											// バッファから取り出したテキスト情報
	gdouble x, y;										// テキスト表示領域左上の座標
	gdouble width, height;								// テキスト表示領域の幅、高さ
	uint16 balloon_type;								// 吹き出しのタイプ
	gdouble line_width;									// 線の太さ
	gdouble edge_position[3][2];						// 吹き出しの尖り先
	gdouble arc_start, arc_end;							// 吹き出しの円弧開始と終了角度
	TEXT_LAYER_POINTS *drag_points;						// ドラッグ処理のデータ
	TEXT_LAYER_BALLOON_DATA balloon_data;				// 吹き出し描画用のデータ
	TEXT_LAYER_BALLOON_DATA_WIDGETS balloon_widgets;	// 吹き出しの詳細設定ウィジェット
	int base_size;										// フォントサイズの倍率
	gdouble font_size;									// 表示テキストのサイズ
	int32 font_id;										// フォントファイル名のID
	uint8 color[3];										// 表示文字色
	uint8 back_color[4];								// 背景色
	uint8 line_color[4];								// 吹き出し枠の色
	uint32 flags;										// 縦書き、太字等のフラグ
} TEXT_LAYER;

/*****************************************
* TEXT_LAYER_BASE_DATA構造体             *
* テキストレイヤーの基本情報(書き出し用) *
*****************************************/
typedef struct _TEXT_LAYER_BASE_DATA
{
	gdouble x, y;							// テキスト表示領域左上の座標
	gdouble width, height;					// テキスト表示領域の幅、高さ
	uint16 balloon_type;					// 吹き出しのタイプ
	gdouble font_size;						// 表示テキストのサイズ
	uint8 base_size;						// 文字サイズの倍率
	uint8 color[3];							// 表示文字色
	gdouble edge_position[3][2];			// 吹き出しの尖り先
	gdouble arc_start;						// 吹き出しの円弧開始角
	gdouble arc_end;						// 吹き出しの円弧終了角
	uint8 back_color[4];					// 吹き出しの背景色
	uint8 line_color[4];					// 吹き出しの線の色
	gdouble line_width;						// 吹き出しの線の太さ
	TEXT_LAYER_BALLOON_DATA balloon_data;	// 吹き出しの詳細データ
	guint32 flags;							// 縦書き、太字等のフラグ
} TEXT_LAYER_BASE_DATA;

/*********************************************
* CreateTextLayer関数                        *
* テキストレイヤーのデータメモリ確保と初期化 *
* 引数                                       *
* window		: キャンバスの情報           *
* x				: レイヤーのX座標            *
* y				: レイヤーのY座標            *
* width			: レイヤーの幅               *
* height		: レイヤーの高さ             *
* base_size		: 文字サイズの倍率           *
* font_size		: 文字サイズ                 *
* color			: 文字色                     *
* balloon_type	: 吹き出しのタイプ           *
* back_color	: 吹き出しの背景色           *
* line_color	: 吹き出しの線の色           *
* line_width	: 吹き出しの線の太さ         *
* balloon_data	: 吹き出し形状の詳細データ   *
* flags			: テキスト表示のフラグ       *
* 返り値                                     *
*	初期化された構造体のアドレス             *
*********************************************/
EXTERN TEXT_LAYER* CreateTextLayer(
	void* window,
	gdouble x,
	gdouble y,
	gdouble width,
	gdouble height,
	int base_size,
	gdouble font_size,
	int32 font_id,
	const uint8 color[3],
	uint16 balloon_type,
	const uint8 back_color[4],
	const uint8 line_color[4],
	gdouble line_width,
	TEXT_LAYER_BALLOON_DATA* balloon_data,
	uint32 flags
);

/*****************************************************
* DeleteTextLayer関数                                *
* テキストレイヤーのデータを削除する                 *
* 引数                                               *
* layer	: テキストレイヤーのデータポインタのアドレス *
*****************************************************/
EXTERN void DeleteTextLayer(TEXT_LAYER** layer);

/***************************************************
* TextLayerGetBalloonArcPoint関数                  *
* 吹き出しの尖り開始座標を取得する                 *
* layer	: テキストレイヤーの情報                   *
* angle	: 吹き出しの中心に対する尖り開始位置の角度 *
* point	: 座標を取得する配列                       *
***************************************************/
EXTERN void TextLayerGetBalloonArcPoint(
	TEXT_LAYER* layer,
	FLOAT_T angle,
	FLOAT_T point[2]
);

/***********************************************************************
* TextLayerSelectBalloonTypeWidgetNew関数                              *
* 吹き出しのタイプを選択するウィジェットを作成する                     *
* 引数                                                                 *
* balloon_type			: 吹き出しのタイプを記憶する変数のアドレス     *
* select_callback		: タイプ選択時のコールバック関数の関数ポインタ *
* select_callback_data	: タイプ選択時のコールバック関数で使うデータ   *
* widgets				: 詳細設定を行うウィジェット群を記憶する構造体 *
* application			: アプリケーションを管理するデータ             *
* 返り値                                                               *
*	吹き出しのタイプを選択するボタンウィジェットのテーブルウィジェット *
***********************************************************************/
EXTERN GtkWidget* TextLayerSelectBalloonTypeWidgetNew(
	uint16* balloon_type,
	void (*select_callback)(void*),
	void* select_callback_data,
	TEXT_LAYER_BALLOON_DATA_WIDGETS* widgets,
	void* application
);

/*********************************************************************
* CreateBalloonDetailSettingWidget関数                               *
* 吹き出しの詳細形状を設定するウィジェットを作成する                 *
* 引数                                                               *
* data					: 詳細設定データ                             *
* widgets				: 詳細設定用のウィジェット群                 *
* setting_callback		: 設定変更時のコールバック関数の関数ポインタ *
* setting_callback_data	: 設定変更時のコールバック関数に使うデータ   *
* application			: アプリケーションを管理するデータ           *
* 返り値                                                             *
*	詳細設定用ウィジェット                                           *
*********************************************************************/
EXTERN GtkWidget* CreateBalloonDetailSettinWidget(
	TEXT_LAYER_BALLOON_DATA* data,
	TEXT_LAYER_BALLOON_DATA_WIDGETS* widgets,
	void (*setting_callback)(void*),
	void* setting_callback_data,
	void* application
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_TEXT_LAYER_H_
