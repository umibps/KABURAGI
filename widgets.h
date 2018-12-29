#ifndef _INCLUDED_WIDGETS_H_
#define _INCLUDED_WIDGETS_H_

#include <gtk/gtk.h>
#include "configure.h"
#include "types.h"
#include "spin_scale.h"
#include "cell_renderer_widget.h"

#ifdef __cplusplus
extern  "C" {
#endif

typedef struct _IMAGE_CHECK_BUTTON
{
	GtkWidget* image;
	GtkWidget* widget;
	GdkPixbuf* pixbuf;
	void* data;
	gint (*func)(struct _IMAGE_CHECK_BUTTON *button, void* data);
	int8 state;
} IMAGE_CHECK_BUTTON;

// 関数のプロトタイプ宣言
EXTERN IMAGE_CHECK_BUTTON* CreateImageCheckButton(
	GdkPixbuf* pixbuf,
	gint (*func)(IMAGE_CHECK_BUTTON* button, void* data),
	void *data
);

EXTERN void ImageCheckButtonSetState(IMAGE_CHECK_BUTTON* button, int8 state);

EXTERN GtkWidget* CreateNotebookLabel(
	const gchar* text,
	GtkWidget* note_book,
	gint page,
	gboolean (*destroy_func)(void* data, gint page),
	gpointer data
);

EXTERN GtkWidget *CreateImageButton(
	const char* image_file_path,
	const gchar* label,
	const char* font_file,
	FLOAT_T scale
);

typedef enum _ICC_PROFILE_USAGE
{
	PROFILE_USAGE_GRAYSCALE = 1,
	PROFILE_USAGE_RGB = 2,
	PROFILE_USAGE_CMYK = 4,
	PROFILE_USAGE_ANY = 7,
	PROFILE_USAGE_DOCUMENT = 8,
	PROFILE_USAGE_GRAYSCALE_DOCUMENT = 9,
	PROFILE_USAGE_RGB_DOCUMENT = 10,
	PROFILE_USAGE_CMYK_DOCUMENT = 12,
	PROFILE_USAGE_PROOF_TARGET = 16,
	PROFILE_USAGE_GRAYSCALE_TARGET = 17,
	PROFILE_USAGE_RGB_TARGET = 18,
	PROFILE_USAGE_CMYK_TARGET = 20,
} ICC_PROFILE_USAGE;

/******************************************************************************
* IccProfileChooserDialogNew関数                                              *
* ICCプロファイルを選択するダイアログウィジェットを作成                       *
* 引数                                                                        *
* usage				: 設定できるICCプロファイルの種類 ICC_PROFILE_USAGEを参照 *
* set_profile_path	: ダイアログの初期値として設定するICCプロファイルのパス   *
* 返り値                                                                      *
*	ダイアログ生成用のウィジェット                                            *
******************************************************************************/
EXTERN GtkWidget* IccProfileChooser(char** icc_path, ICC_PROFILE_USAGE usage);

/********************************************************
* IccProfileChangerDialogNew関数                        *
* ICCプロファイルを選択するダイアログウィジェットを作成 *
* 引数                                                  *
* parent			     : 親ウインドウ                 *
* workspace_profile_path : 作業用プロファイルのパス     *
* 返り値                                                *
*	ダイアログ生成用のウィジェット                      *
********************************************************/
EXTERN GtkWidget* IccProfileChangerDialogNew(GtkWindow *parent, gchar *workspace_profile_path);

/**************************************************************************
* IccProfileChooserDialogNew関数                                          *
* ICCプロファイルを選択するダイアログウィジェットを作成                   *
* 引数                                                                    *
* usage			: 設定できるICCプロファイルの種類 ICC_PROFILE_USAGEを参照 *
* profile_path	: 初期値及び設定されたICCプロファイルのパス               *
* 返り値                                                                  *
*	ダイアログ生成用のウィジェット                                        *
**************************************************************************/
EXTERN GtkWidget* IccProfileChooserDialogNew(int usage, char** profile_path);

EXTERN void IccProfileChooserDialogRun(GtkWidget* dialog);

/***************************************************************
* ToggleRadioButtonNew関数                                     *
* ラジオアクションを設定したトグルボタンウィジェットを作成     *
* 引数                                                         *
* list			: ボタンリストの入ったGList                    *
*				  先頭のボタンはNULLの入ったlistを渡す         *
*				  g_list_freeしないこと                        *
* callback_func	: ボタンが有効状態になった時のコールバック関数 *
* callback_data	: コールバック関数に渡すデータ                 *
* 返り値                                                       *
*	ボタンウィジェット                                         *
***************************************************************/
EXTERN GtkWidget* ToggleRadioButtonNew(
	GList** list,
	void (*callback_func)(GtkWidget* button, void* data),
	void* callback_data
);

#ifdef _WIN32
EXTERN cairo_t *
kaburagi_cairo_create (void* drawable);
#else
static inline kaburagi_cairo_create (void* window) {return gdk_cairo_create(window);}
#endif

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_WIDGETS_H_
