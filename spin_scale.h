#ifndef _INCLUDED_SPIN_SCALE_H_
#define _INCLUDED_SPIN_SCALE_H_

#include <gtk/gtk.h>

#ifdef _MSC_VER
# ifdef __cplusplus
#  define EXTERN extern "C" __declspec(dllexport)
# else
#  define EXTERN extern __declspec(dllexport)
# endif
#else
# define EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************
* SPIN_SCALE構造体                                 *
* スピンボタンとスケールの混合ウィジェットのデータ *
***************************************************/
typedef struct _SPIN_SCALE
{
	GtkSpinButton parent_instance;
} SPIN_SCALE;

// 関数のプロトタイプ宣言
/*****************************************************
* SpinScaleNew関数                                   *
* スピンボタンとスケールバーの混合ウィジェットを生成 *
* 引数                                               *
* adjustment	: スケールに使用するアジャスタ       *
* label			: スケールのラベル                   *
* digits		: 表示する桁数                       *
* 返り値                                             *
*	生成したウィジェット                             *
*****************************************************/
EXTERN GtkWidget* SpinScaleNew(
	GtkAdjustment *adjustment,
	const gchar *label,
	gint digits
);

/***************************************************
* SpinScaleSetScaleLimits関数                      *
* スケールの上下限値を設定する                     *
* 引数                                             *
* scale	: スピンボタンとスケールの混合ウィジェット *
* lower	: 下限値                                   *
* upper	: 上限値                                   *
***************************************************/
EXTERN void SpinScaleSetScaleLimits(
	SPIN_SCALE *scale,
	gdouble lower,
	gdouble upper
);

/*****************************************************
* SpinScaleGetScaleLimits関数                        *
* 上下限値を取得する                                 *
* 引数                                               *
* scale	: スピンボタンとスケールの混合ウィジェット   *
* lower	: 下限値を格納する倍精度浮動小数点のアドレス *
* upper	: 上限値を格納する倍精度浮動小数点のアドレス *
* 返り値                                             *
*	取得成功:TRUE	取得失敗:FALSE                   *
*****************************************************/
EXTERN gboolean SpinScaleGetScaleLimits(
	SPIN_SCALE *scale,
	gdouble *lower,
	gdouble *upper
);

/***************************************************
* SpinScaleSetStep関数                             *
* 矢印キーでの増減の値を設定する                   *
* 引数                                             *
* scale	: スピンボタンとスケールの混合ウィジェット *
* step	: 増減の値                                 *
***************************************************/
EXTERN void SpinScaleSetStep(SPIN_SCALE *scale, gdouble step);

/***************************************************
* SpinScaleSetPase関数                             *
* Pase Up/Downキーでの増減の値を設定する           *
* 引数                                             *
* scale	: スピンボタンとスケールの混合ウィジェット *
* page	: 増減の値                                 *
***************************************************/
EXTERN void SpinScaleSetPage(SPIN_SCALE *scale, gdouble page);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_SPIN_SCALE_H_
