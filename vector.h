#ifndef _INCLUDED_VECTOR_H_
#define _INCLUDED_VECTOR_H_

#include <gtk/gtk.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VECTOR_LINE_BUFFER_SIZE 32

typedef enum _eVECTOR_LINE_TYPE
{
	VECTOR_LINE_STRAIGHT,
	VECTOR_LINE_BEZIER_OPEN,
	VECTOR_LINE_STRAIGHT_CLOSE,
	VECTOR_LINE_BEZIER_CLOSE
} eVECTOR_LINE_TYPE;

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

typedef struct _VECTOR_LINE_LAYER
{
	int32 x, y, width, height, stride;
	uint8* pixels;
	cairo_t* cairo_p;
	cairo_surface_t* surface_p;
} VECTOR_LINE_LAYER;

/*************************************
* VECTOR_LINE構造体                  *
* ベクトルレイヤーの線一本分のデータ *
*************************************/
typedef struct _VECTOR_LINE
{
	uint8 vector_type;			// 線のタイプ
	uint8 flags;				// アンチエイリアス等のフラグ
	uint16 num_points;			// 制御点の数
	uint32 buff_size;			// 制御点バッファのサイズ
	gdouble blur;				// ボケ足の開始オフセット
	gdouble outline_hardness;	// 輪郭の硬さ
	VECTOR_POINT* points;		// 制御点座標配列
	VECTOR_LINE_LAYER* layer;	// ラスタライズ後のデータ
								// ラスタライズ処理軽減用
	// 前後の線へのポインタ
	struct _VECTOR_LINE *prev, *next;
} VECTOR_LINE;

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

extern VECTOR_LINE* CreateVectorLine(VECTOR_LINE* prev, VECTOR_LINE* next);

extern void DeleteVectorLine(VECTOR_LINE** line);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_VECTOR_H_
