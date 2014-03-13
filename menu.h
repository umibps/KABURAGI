#ifndef _INCLUDED_MENU_H_
#define _INCLUDED_MENU_H_

#include <gtk/gtk.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern GtkWidget* GetMainMenu(
	APPLICATION* app,
	GtkWidget* window,
	const char* path
);

/*************************************************
* ParentItemSelected関数                         *
* 親メニューアイテムが選択された時に呼び出される *
* 引数                                           *
* widget	: 親メニューアイテム                 *
* data		: ダミー                             *
*************************************************/
extern void ParentItemSelected(GtkWidget* widget, gpointer* data);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_MENU_H_
