#ifndef _INCLUDED_VECTOR_H_
#define _INCLUDED_VECTOR_H_

#include <gtk/gtk.h>
#include "types.h"
#include "memory_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VECTOR_LINE_BUFFER_SIZE 32

typedef struct _VECTOR_LINE_LAYER
{
	int32 x, y, width, height, stride;
	uint8* pixels;
	cairo_t* cairo_p;
	cairo_surface_t* surface_p;
} VECTOR_LINE_LAYER;

typedef struct _VECTOR_BASE_DATA
{
	uint8 vector_type;			// ベクトルデータのタイプ(直線or曲線or形状)
	void *prev, *next;			// 前後のベクトルデータ
	VECTOR_LINE_LAYER *layer;	// ラスタライズ後のデータ
								// ラスタライズ処理軽減用
} VECTOR_BASE_DATA;

typedef enum _eVECTOR_TYPE
{
	VECTOR_TYPE_STRAIGHT,
	VECTOR_TYPE_BEZIER_OPEN,
	VECTOR_TYPE_STRAIGHT_CLOSE,
	VECTOR_TYPE_BEZIER_CLOSE,
	NUM_VECTOR_LINE_TYPE,
	VECTOR_TYPE_SHAPE = NUM_VECTOR_LINE_TYPE,
	VECTOR_TYPE_SQUARE,
	VECTOR_TYPE_RHOMBUS,
	VECTOR_TYPE_ECLIPSE,
	VECTOR_SHAPE_END,
	VECTOR_TYPE_SCRIPT = VECTOR_SHAPE_END
} eVECTOR_TYPE;

typedef enum _eVECTOR_POINT_FLAGS
{
	VECTOR_POINT_SELECTED = 0x01,
} eVECTOR_POINT_FLAGS;

typedef struct _VECTOR_POINT
{
	uint8 vector_type;
	uint8 flags;
	uint8 color[4];
	gdouble pressure, size;
	gdouble x, y;
} VECTOR_POINT;

typedef enum _eVECTOR_LINE_FLAGS
{
	VECTOR_LINE_ANTI_ALIAS = 0x01,
	VECTOR_LINE_FIX = 0x02,
	VECTOR_LINE_MARKED = 0x04
} eVECTOR_LINE_FLAGS;

/*************************************
* VECTOR_LINE構造体                  *
* ベクトルレイヤーの線一本分のデータ *
*************************************/
typedef struct _VECTOR_LINE
{
	VECTOR_BASE_DATA base_data;	// 線のタイプ等の基本情報
	uint8 flags;				// アンチエイリアス等のフラグ
	uint16 num_points;			// 制御点の数
	uint32 buff_size;			// 制御点バッファのサイズ
	gdouble blur;				// ボケ足の開始オフセット
	gdouble outline_hardness;	// 輪郭の硬さ
	VECTOR_POINT *points;		// 制御点座標配列
} VECTOR_LINE;

/****************************************
* VECTOR_SQUARE構造体                   *
* ベクトルレイヤーの四角形1つ分のデータ *
****************************************/
typedef struct _VECTOR_SQUARE
{
	VECTOR_BASE_DATA base_data;	// 線のタイプ等の基本情報
	uint8 flags;				// 角の形状等のフラグ
	uint8 line_joint;			// 線の繋ぎ目
	FLOAT_T left;				// 長方形の左上のX座標
	FLOAT_T top;				// 長方形の左上のY座標
	FLOAT_T width;				// 長方形の幅
	FLOAT_T height;				// 長方形の高さ
	FLOAT_T rotate;				// 回転
	FLOAT_T line_width;			// 線の太さ
	uint8 line_color[4];		// 線の色
	uint8 fill_color[4];		// 塗り潰しの色
} VECTOR_SQUARE;

/****************************************************
* VECTOR_SQUARE_BASE構造体                          *
* ベクトルレイヤーの四角形1つ分のデータ(書き出し用) *
****************************************************/
typedef struct _VECTOR_SQUARE_BASE
{
	uint8 flags;				// 角の形状等のフラグ
	uint8 line_joint;			// 線の繋ぎ目
	FLOAT_T left;				// 長方形の左上のX座標
	FLOAT_T top;				// 長方形の左上のY座標
	FLOAT_T width;				// 長方形の幅
	FLOAT_T height;				// 長方形の高さ
	FLOAT_T rotate;				// 回転
	FLOAT_T line_width;			// 線の太さ
	uint8 line_color[4];		// 線の色
	uint8 fill_color[4];		// 塗り潰しの色
} VECTOR_SQUARE_BASE;

/**************************************
* VECTOR_ECLIPSE構造体                *
* ベクトルレイヤーの楕円1つ分のデータ *
**************************************/
typedef struct _VECTOR_ECLIPSE
{
	VECTOR_BASE_DATA base_data;	// 線のタイプ等の基本情報
	uint8 flags;				// 角の形状等のフラグ
	uint8 line_joint;			// 線の繋ぎ目
	FLOAT_T x;					// 楕円の中心のX座標
	FLOAT_T y;					// 楕円の中心のY座標
	FLOAT_T radius;				// 半径
	FLOAT_T ratio;				// 幅:高さ
	FLOAT_T rotate;				// 回転
	FLOAT_T line_width;			// 線の太さ
	uint8 line_color[4];		// 線の色
	uint8 fill_color[4];		// 塗り潰しの色
} VECTOR_ECLIPSE;

/**************************************************
* VECTOR_ECLIPSE_BASE構造体                       *
* ベクトルレイヤーの楕円1つ分のデータ(書き出し用) *
**************************************************/
typedef struct _VECTOR_ECLIPSE_BASE
{
	uint8 flags;				// 角の形状等のフラグ
	FLOAT_T x;					// 楕円の左上のX座標
	FLOAT_T y;					// 楕円の左上のY座標
	FLOAT_T radius;				// 半径
	FLOAT_T ratio;				// 幅:高さ
	FLOAT_T rotate;				// 回転
	FLOAT_T line_width;			// 線の太さ
	uint8 line_color[4];		// 線の色
	uint8 fill_color[4];		// 塗り潰しの色
} VECTOR_ECLIPSE_BASE;

/****************************************************************
* VECTOR_SCRIPT構造体                                           *
* ベクトルレイヤーのスクリプトによるベクトルデータ1つ分のデータ *
****************************************************************/
typedef struct _VECTOR_SCRIPT
{
	VECTOR_BASE_DATA base_data;	// ベクトルタイプ等の基本情報
	char *script_data;			// スクリプトの文字列
} VECTOR_SCRIPT;

/****************************
* VECTOR_DATA構造体         *
* ベクトル形状1つ分のデータ *
****************************/
typedef union _VECTOR_DATA
{
	VECTOR_BASE_DATA base_data;
	VECTOR_LINE line;
	VECTOR_SQUARE square;
	VECTOR_ECLIPSE eclipse;
	VECTOR_SCRIPT script;
} VECTOR_DATA;

/*********************************************************
* VECTOR_LINE_BASE_DATA構造体                            *
* ベクトルレイヤーの線一本分の基本情報(データ書き出し用) *
*********************************************************/
typedef struct _VECTOR_LINE_BASE_DATA
{
	uint8 vector_type;			// 線のタイプ
	uint8 flags;				// 線に設定されているフラグ
	uint16 num_points;			// 制御点の数
	gdouble blur;				// ボケ足の開始オフセット
	gdouble outline_hardness;	// 輪郭の硬さ
} VECTOR_LINE_BASE_DATA;

typedef struct _VECTOR_LAYER_RECTANGLE
{
	gdouble min_x, min_y, max_x, max_y;
} VECTOR_LAYER_RECTANGLE;

typedef struct _DIVIDE_LINE_CHANGE_DATA
{
	size_t data_size;
	int8 added;
	int8 line_type;
	uint16 before_num_points;
	uint16 after_num_points;

	int index;
	VECTOR_POINT *before;
	VECTOR_POINT *after;
} DIVIDE_LINE_CHANGE_DATA;

typedef struct _DIVIDE_LINE_ADD_DATA
{
	size_t data_size;
	int8 line_type;
	uint16 num_points;

	int index;
	VECTOR_POINT *after;
} DIVIDE_LINE_ADD_DATA;

typedef struct _DIVIDE_LINE_DATA
{
	int num_change;
	int num_add;
	int change_size;
	int layer_name_length;

	char *layer_name;
	DIVIDE_LINE_CHANGE_DATA *change;
	DIVIDE_LINE_ADD_DATA *add;
} DIVIDE_LINE_DATA;

EXTERN VECTOR_LINE* CreateVectorLine(VECTOR_LINE* prev, VECTOR_LINE* next);

EXTERN void DeleteVectorLine(VECTOR_LINE** line);

EXTERN VECTOR_DATA* CreateVectorShape(VECTOR_BASE_DATA* prev, VECTOR_BASE_DATA* next, uint8 vector_type);

EXTERN VECTOR_DATA* CreateVectorScript(VECTOR_BASE_DATA* prev, VECTOR_BASE_DATA* next, const char* script);

EXTERN void DeleteVectorShape(VECTOR_BASE_DATA** shape);

EXTERN void DeleteVectorScript(VECTOR_SCRIPT** script);

EXTERN void ReadVectorShapeData(VECTOR_DATA* data, MEMORY_STREAM* stream);

EXTERN void WriteVectorShapeData(VECTOR_DATA* data, MEMORY_STREAM* stream);

EXTERN void ReadVectorScriptData(VECTOR_SCRIPT* script, MEMORY_STREAM* stream);

EXTERN void WriteVectorScriptData(VECTOR_SCRIPT* script, MEMORY_STREAM* stream);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_VECTOR_H_
