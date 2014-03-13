#ifndef _INCLUDED_PATTERN_H_
#define _INCLUDED_PATTERN_H_

#include <gtk/gtk.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

// パターンの左右反転、上下反転のフラグ
typedef enum _PATTERN_FLAGS
{
	PATTERN_FLIP_HORIZONTALLY = 0x01,
	PATTERN_FLIP_VERTICALLY = 0x02
} PATTERN_FLAGS;

// パターンのチャンネル数が2のときのモード
typedef enum _PATTERN_MODE
{
	PATTERN_MODE_SATURATION,
	PATTERN_MODE_BRIGHTNESS,
	PATTERN_MODE_FORE_TO_BACK,
	PATTERN_MODE_NUM
} PATTERN_MODE;

#ifndef INCLUDE_WIN_DEFAULT_API
/****************************
* PATTERN構造体             *
* パターンデータ1つ分の情報 *
****************************/
typedef struct _PATTERN
{
	// 画像データの幅、高さ、一行分のバイト数
	gint32 width, height, stride;
	// 画像データのチャンネル数
	uint8 channel;
	// 画像のピクセルデータ
	uint8* pixels;
} PATTERN;
#else
#include <wingdi.h>
#endif

/*************************
* PATTERNS構造体         *
* 使用可能なパターン情報 *
*************************/
typedef struct _PATTERNS
{
	// パターン情報とアクティブなパターン
	PATTERN* patterns, *active_pattern;
	// クリップボードのパターン
	PATTERN clip_board;
	// クリップボードのパターンがある時のフラグ
	int has_clip_board_pattern;
	// 使用可能なパターンの数
	int32 num_pattern;
	// RGBAに変換したパターンのバイトデータ
	uint8* pattern_pixels;
	// チャンネル数2のときのマスク用
	uint8* pattern_mask;
	// RGBA変換時の一時保存
	uint8* pattern_pixels_temp;
	// パターン画像の最大バイト数
	size_t pattern_max_byte;
	// パターンの拡大率
	gdouble scale;
} PATTERNS;

// 関数のプロトタイプ宣言
/*************************************************************
* InitializePatterns関数                                     *
* パターンを初期化                                           *
* 引数                                                       *
* pattern			: パターン情報を管理する構造体のアドレス *
* directory_path	: パターンファイルのあるディレクトリパス *
* buffer_size		: 現在のバッファサイズ                   *
*************************************************************/
extern void InitializePattern(PATTERNS* pattern, const char* directory_path, int* buffer_size);

/*********************************************************
* UpdateClipBoardPattern関数                             *
* クリップボードから画像データを取り出してパターンにする *
* 引数                                                   *
* patterns	: パターンを管理する構造体のアドレス         *
*********************************************************/
extern void UpdateClipBoardPattern(PATTERNS* patterns);

/*************************************************
* CreatePatternSurface関数                       *
* パターン塗り潰し実行用のCAIROサーフェース作成  *
* 引数                                           *
* patterns	: パターンを管理する構造体のアドレス *
* scale		: パターンの拡大率                   *
* rgb		: 現在の描画色                       *
* back_rgb	: 現在の背景色                       *
* flags		: 左右反転、上下反転のフラグ         *
* mode		: チャンネル数が2のときの作成モード  *
* flow		: パターンの濃度                     *
* 返り値                                         *
*	作成したサーフェース                         *
*************************************************/
extern cairo_surface_t* CreatePatternSurface(
	PATTERNS* patterns,
	uint8 rgb[3],
	uint8 back_rgb[3],
	int flags,
	int mode,
	gdouble flow
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_PATTERN_H_
