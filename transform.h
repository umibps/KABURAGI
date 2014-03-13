#ifndef _INCLUDED_TRANSFORM_H_
#define _INCLUDED_TRANSFORM_H_

#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************
* TRANSFORM_DATA構造体 *
* 変形処理のデータ     *
***********************/
typedef struct _TRANSFORM_DATA
{
	int32 x, y;						// 変形領域の左上の座標
	int32 width, height;			// 変形領域の幅と高さ
	int stride;						// 元画像データ1行分のバイト数
	uint8 **source_pixels;			// 変形領域のピクセルデータ
	uint8 **before_pixels;			// 変形前のピクセルデータ
	LAYER **layers;					// 変形処理を行うレイヤー
	uint16 num_layers;				// 変形処理を行うレイヤーの数
	uint16 trans_point;				// 選択中のポイント
	uint8 selected_mode;			// 選択されているモード
	uint8 trans_mode;				// 変形モード
	uint8 trans_type;				// 変形方法
	uint8 flags;					// 上下・左右反転のフラグ
	FLOAT_T move_x, move_y;			// アフィン変換での移動先の座標
	FLOAT_T angle;					// アフィン変換での回転量
	gdouble before_x, before_y;		// マウス操作の座標を記憶
	FLOAT_T before_angle;			// 一つ前の状態での角度
	FLOAT_T trans[8][2];			// 変形で指定されている4点の座標
	// 射影変換を行う範囲
	int min_x[4], max_x[4], min_y[4], max_y[4];
} TRANSFORM_DATA;

typedef enum _eTRANSFORM_FLAGS
{
	TRANSFORM_REVERSE_HORIZONTALLY = 0x01,
	TRANSFORM_REVERSE_VERTICALLY = 0x02
} eTRANSFORM_FLAGS;

typedef enum _eTRANSFORM_POINT
{
	TRANSFORM_LEFT_UP,
	TRANSFORM_LEFT,
	TRANSFORM_LEFT_DOWN,
	TRANSFORM_DOWN,
	TRANSFORM_RIGHT_DOWN,
	TRANSFORM_RIGHT,
	TRANSFORM_RIGHT_UP,
	TRANSFORM_UP,
	TRANSFORM_POINT_NONE
} eTRANSFORM_POINT;

typedef enum _eTRANSFORM_MODE
{
	TRANSFORM_FREE,
	TRANSFORM_SCALE,
	TRANSFORM_FREE_SHAPE,
	TRANSFORM_ROTATE,
	TRANSFORM_MOVE
} eTRANSFORM_MODE;

typedef enum _eTRANSFORM_TYPE
{
	TRANSFORM_PROJECTION,
	TRANSFORM_PATTERN
} eTRANSFORM_TYPE;

// 関数のプロトタイプ宣言
/*************************************
* CreateTransformData関数            *
* 変形処理用のデータを作成する       *
* 引数                               *
* window	: 変形処理を行う描画領域 *
*************************************/
extern TRANSFORM_DATA *CreateTransformData(DRAW_WINDOW* window);

/***************************************
* DeleteTransformData関数              *
* 変形処理用のデータを開放             *
* 引数                                 *
* transform	: 開放するデータのアドレス *
***************************************/
extern void DeleteTransformData(TRANSFORM_DATA** transform);

/***********************************************
* Transform関数                                *
* 変形処理を実行                               *
* 引数                                         *
* transform			: 変形処理用のデータ       *
* projection_param	: 射影変換用のパラメーター *
***********************************************/
extern void Transform(TRANSFORM_DATA* transform);

/***********************************************
* ProjectionTransform関数                      *
* 射影変形処理を実行                           *
* 引数                                         *
* transform			: 変形処理用のデータ       *
* projection_param	: 射影変換用のパラメーター *
***********************************************/
extern void ProjectionTransform(TRANSFORM_DATA* transform, FLOAT_T projection_param[4][9]);

/*****************************************************
* ExecuteTransform関数                               *
* アプリケーションメニューからの変形処理の呼び出し   *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
extern void ExecuteTransform(APPLICATION* app);

/*******************************************************
* ExecuteProjection関数                                *
* アプリケーションメニューからの射影変換処理の呼び出し *
* 引数                                                 *
* app	: アプリケーションを管理する構造体のアドレス   *
*******************************************************/
extern void ExecuteProjection(APPLICATION* app);

extern void TransformButtonPress(TRANSFORM_DATA* transform, gdouble x, gdouble y);

extern void DisplayTransform(DRAW_WINDOW *window);

/***********************************************
* TransformMotionNotifyCallBack関数            *
* 変形処理時のマウスドラッグのコールバック関数 *
* 引数                                         *
* window	: 描画領域の情報                   *
* transform	: 変形処理の情報                   *
* x			: マウスのX座標                    *
* y			: マウスのY座標                    *
* state		: マウス・キーボードの状態         *
***********************************************/
extern void TransformMotionNotifyCallBack(
	DRAW_WINDOW* window,
	TRANSFORM_DATA* transform,
	gdouble x,
	gdouble y,
	void* state
);

/*****************************************************
* CreateTransformDetailUI関数                        *
* 変形処理時のUIウィジェットを作成                   *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
* 返り値                                             *
*	作成したUIウィジェット                           *
*****************************************************/
extern GtkWidget* CreateTransformDetailUI(APPLICATION* app);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_TRANSFORM_H_
