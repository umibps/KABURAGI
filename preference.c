// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>
#include <zlib.h>
#include "application.h"
#include "preference.h"
#include "ini_file.h"
#include "memory.h"
#include "utils.h"

#if !defined(USE_QT) || (defined(USE_QT) && USE_QT != 0)
# include "gui/GTK/utils_gtk.h"
# include "gui/GTK/gtk_widgets.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NUM_SETTING_ITEM 2

typedef struct _SET_BRUSH_CHAIN
{
	GList *chain_list;
} SET_BRUSH_CHAIN;

typedef struct _EDIT_BRUSH_CHAIN
{
	GList *edit_chain;
} EDIT_BRUSH_CHAIN;

typedef struct _SET_PREFERENCE
{
	GtkWidget *hbox, *vbox;
	int16 current_setting;
	PREFERENCE preference;
	GtkWidget *setting_widget;
	char *input_icc_path;
	char *output_icc_path;
	char *language_name;
	char *language_path;
	char *backup_path;
	GtkWidget *preference_dialog;
	APPLICATION *app;
	FLOAT_T gui_scale;
	uint32 flags;
} SET_PREFERENCE;

typedef enum _eSET_PREFERENCE_ITEM
{
	SET_PREFERENCE_BASE_SETTING,
	SET_PREFERENCE_THEME,
	SET_PREFERENCE_ICC_PROFILE,
	SET_PREFERENCE_LANGUAGE
} eSET_PREFERENCE_ITEM;

typedef enum _eSET_PREFERENCE_FLAGS
{
	SET_PREFERENCE_CHANGE_THEME = 0x01,
	SET_PREFERENCE_DRAW_WITH_TOUCH = 0x02,
	SET_PREFERENCE_CHANGE_BACK_GROUND_COLOR = 0x04,
	SET_PREFERENCE_SHOW_PREVIEW_ON_TASK_BAR = 0x08,
	SET_PREFERENCE_LAYER_WINDOW_SCROLLBAR_PLACE_LEFT = 0x10
} eSET_PREFERENCE_FLAGS;

typedef enum _eSET_PREFERENCE_TOUCH_MODE
{
	SET_PREFERENCE_TOUCH_MODE_SCALE_AND_MOVE_WITH_TOUCH,
	SET_PREFERENCE_TOUCH_MODE_DRAW_WITH_TOUCH
} eSET_PREFERENCE_TOUCH_MODE;

/*************************************************************
* CreateChooseThemeWidget関数                                *
* テーマ選択のウィジェットを作成する                         *
* 引数                                                       *
* preference	: 環境設定情報                               *
* app			: アプリケーションを管理する構造体のアドレス *
* 返り値                                                     *
*	作成したウィジェット                                     *
*************************************************************/
static GtkWidget* CreateChooseThemeWidget(SET_PREFERENCE* preference, APPLICATION* app);

/*************************************************************
* CreateChooseLanguageWidget関数                             *
* 言語選択用ウィジェットを作成する                           *
* 引数                                                       *
* preference	: 環境設定情報                               *
* app			: アプリケーションを管理する構造体のアドレス *
* 返り値                                                     *
*	作成したウィジェット                                     *
*************************************************************/
static GtkWidget* CreateChooseLanguageWidget(SET_PREFERENCE* preference, APPLICATION* app);

static void ChangeCompression(GtkSpinButton* spin, SET_PREFERENCE* setting)
{
	setting->preference.compress =
		(int8)gtk_spin_button_get_value(spin);
}

static void OnClickedInputDialogClose(GtkWidget* button, GtkWidget** dialog)
{
	gtk_widget_destroy(*dialog);
	*dialog = NULL;
}

static void AutoSaveCheckButtonClicked(GtkWidget* button, SET_PREFERENCE* setting)
{
	GtkWidget *spin_button = GTK_WIDGET(g_object_get_data(
		G_OBJECT(button), "interval_button"));

	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
	{
		setting->preference.auto_save = 1;
		setting->preference.auto_save_time =
			(int32)gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin_button));
		gtk_widget_set_sensitive(spin_button, TRUE);
	}
	else
	{
		setting->preference.auto_save = 0;
		gtk_widget_set_sensitive(spin_button, FALSE);
	}
}

static void AutoSaveSetInterval(GtkAdjustment* spin, SET_PREFERENCE* setting)
{
	setting->preference.auto_save_time =
		(int32)gtk_adjustment_get_value(spin) * 60;
}

static void BackupDirectoryChanged(GtkFileChooser* chooser, SET_PREFERENCE* preference)
{
	gchar *path;

	path = gtk_file_chooser_get_filename(chooser);
	if(path != NULL)
	{
		g_free(preference->backup_path);
		preference->backup_path = path;
	}
}

#if GTK_MAJOR_VERSION <= 2
static void InputDeviceButtonClicked(GtkWidget* button, SET_PREFERENCE* setting)
{
	GtkWidget *dialog = gtk_input_dialog_new();

	gtk_widget_hide(GTK_INPUT_DIALOG(dialog)->save_button);
	g_signal_connect(G_OBJECT(GTK_INPUT_DIALOG(dialog)->close_button), "clicked",
		G_CALLBACK(OnClickedInputDialogClose), &dialog);

	(void)gtk_dialog_run(GTK_DIALOG(dialog));

	if(dialog != NULL)
	{
		gtk_widget_destroy(dialog);
	}
}
#else
static void ChangeTouchMode(GtkWidget* radio_button, SET_PREFERENCE* setting)
{
	eSET_PREFERENCE_TOUCH_MODE touch_mode =
		(eSET_PREFERENCE_TOUCH_MODE)GPOINTER_TO_INT(g_object_get_data(
			G_OBJECT(radio_button), "touch_mode"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_button)) != FALSE)
	{
		if(touch_mode == SET_PREFERENCE_TOUCH_MODE_SCALE_AND_MOVE_WITH_TOUCH)
		{
			setting->flags &= ~(SET_PREFERENCE_DRAW_WITH_TOUCH);
		}
		else
		{
			setting->flags |= SET_PREFERENCE_DRAW_WITH_TOUCH;
		}
	}
}
#endif

static void SetLayerWindowScrollbarPlaceLeftFlag(GtkWidget* button,SET_PREFERENCE* setting)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		setting->flags &= ~(SET_PREFERENCE_LAYER_WINDOW_SCROLLBAR_PLACE_LEFT);
	}
	else
	{
		setting->flags |= SET_PREFERENCE_LAYER_WINDOW_SCROLLBAR_PLACE_LEFT;
	}
}


static void SetChangeBackGroundColorFlag(GtkWidget* button, SET_PREFERENCE* setting)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		setting->flags &= ~(SET_PREFERENCE_CHANGE_BACK_GROUND_COLOR);
	}
	else
	{
		setting->flags |= SET_PREFERENCE_CHANGE_BACK_GROUND_COLOR;
	}
}

static void SetBackGroundColor(GtkWidget* button, SET_PREFERENCE* setting)
{
	GdkColor color;

	gtk_color_button_get_color(GTK_COLOR_BUTTON(button), &color);
	setting->preference.canvas_back_ground[0] = color.red / 256;
	setting->preference.canvas_back_ground[1] = color.green / 256;
	setting->preference.canvas_back_ground[2] = color.blue / 256;
}

static void SetShowPreviewWindowOnTaskbar(GtkWidget* button, SET_PREFERENCE* setting)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		setting->flags &= ~(SET_PREFERENCE_SHOW_PREVIEW_ON_TASK_BAR);
	}
	else
	{
		setting->flags |= SET_PREFERENCE_SHOW_PREVIEW_ON_TASK_BAR;
	}
}

static void OnChangeUserInterfaceScale(GtkWidget* spin, SET_PREFERENCE* setting)
{
	setting->gui_scale = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin)) * 0.01;
}

#if defined(GTK_MAJOR_VERSION) && GTK_MAJOR_VERSION <= 2
static void OnClickInputSettingButton(void* dummy)
{
	GtkWidget *dialog;
	dialog = gtk_input_dialog_new();
	(void)gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}
#endif

static void OnBrushChainKeyChanged(GtkWidget* editable, BRUSH_CHAIN* brush_chain)
{
	brush_chain->key = ((const char*)gtk_entry_get_text(GTK_ENTRY(editable)))[0];
}

static void OnChangeBrushChainKeyChanged(GtkWidget* editable, BRUSH_CHAIN* brush_chain)
{
	brush_chain->change_key = ((const char*)gtk_entry_get_text(GTK_ENTRY(editable)))[0];
}

static void OnEditBrushChainItemButtonPressed(GtkWidget* button, SET_PREFERENCE* setting)
{
	GtkWidget *parent_dialog;
	GtkWidget *dialog;
	GtkWidget *label;
	GtkWidget *vbox;
	GtkWidget *select;
	POINTER_ARRAY *brush_names;
	char buffer[4096];
	const char *current;
	int num_selectable = 0;
	int x, y;

	parent_dialog = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "chain_dialog"));
	label = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "label_widget"));
	brush_names = PointerArrayNew(BRUSH_TABLE_HEIGHT*BRUSH_TABLE_WIDTH+1);

	current = gtk_label_get_text(GTK_LABEL(label));

	dialog = gtk_dialog_new_with_buttons("", GTK_WINDOW(parent_dialog),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL
	);

	vbox = gtk_vbox_new(FALSE, 0);

#if GTK_MAJOR_VERSION <= 2
	select = gtk_combo_box_new_text();
#else
	select = gtk_combo_box_text_new();
#endif

	for(y=0; y<BRUSH_TABLE_HEIGHT; y++)
	{
		for(x=0; x<BRUSH_TABLE_WIDTH; x++)
		{
			if(setting->app->tool_window.brushes[y][x].name != NULL)
			{
				PointerArrayAppend(brush_names, setting->app->tool_window.brushes[y][x].name);
				StringRemoveTargetString(setting->app->tool_window.brushes[y][x].name, "\\n", buffer);
#if GTK_MAJOR_VERSION <= 2
				gtk_combo_box_append_text(GTK_COMBO_BOX(select),
#else
				gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(select),
#endif
					buffer);
				if(current != NULL)
				{
					if(strcmp(current, buffer) == 0)
					{
						gtk_combo_box_set_active(GTK_COMBO_BOX(select), num_selectable);
					}
				}
				num_selectable++;
			}
		}
	}
	gtk_box_pack_start(GTK_BOX(vbox), select, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox, FALSE, FALSE, 0);
	gtk_widget_show_all(dialog);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		int active = gtk_combo_box_get_active(GTK_COMBO_BOX(select));
		char *name = MEM_STRDUP_FUNC((const char*)brush_names->buffer[active]);
		char **item_name = (char**)g_object_get_data(G_OBJECT(button), "brush_name");

		MEM_FREE_FUNC(*item_name);
		*item_name = name;

		label = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "label_widget"));
		gtk_label_set_label(GTK_LABEL(label),
#if GTK_MAJOR_VERSION <= 2
			gtk_combo_box_get_active_text(GTK_COMBO_BOX(select))
#else
			gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(select))
#endif
		);
		g_object_set_data(G_OBJECT(g_object_get_data(G_OBJECT(button), "delete_button")),
			"delete_name", name);
	}

	PointerArrayDestroy(&brush_names, NULL);

	gtk_widget_destroy(dialog);
}

static void OnDeleteChainItemButtonPressed(GtkWidget* button, GtkWidget* box)
{
	BRUSH_CHAIN_ITEM *item = (BRUSH_CHAIN_ITEM*)g_object_get_data(G_OBJECT(button), "brush_chain_item");
	char *delete_name = (char*)g_object_get_data(G_OBJECT(button), "delete_name");

	PointerArrayRemoveByData(item->names, delete_name, NULL);
	gtk_widget_destroy(box);
}

static void OnAddBrushChainItemButtonPressed(GtkWidget* button, SET_PREFERENCE* setting)
{
	GtkWidget *parent_dialog;
	GtkWidget *dialog;
	GtkWidget *chains_vbox;
	GtkWidget *table_vbox;
	GtkWidget *vbox;
	GtkWidget *hbox;
	GtkWidget *select;
	GtkWidget *label;
	GtkWidget *edit_button;
	GtkWidget *delete_button;
	BRUSH_CHAIN_ITEM *item = (BRUSH_CHAIN_ITEM*)g_object_get_data(G_OBJECT(button), "chain_item");
	POINTER_ARRAY *brush_names;
	char buffer[4096];
	char *current = NULL;
	int num_selectable = 0;
	int x, y;

	if(item == NULL)
	{
		return;
	}

	chains_vbox = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "chains_vbox"));
	table_vbox = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "table_vbox"));
	parent_dialog = GTK_WIDGET(g_object_get_data(G_OBJECT(chains_vbox), "chain_dialog"));
	brush_names = PointerArrayNew(BRUSH_TABLE_HEIGHT*BRUSH_TABLE_WIDTH+1);

	dialog = gtk_dialog_new_with_buttons("", GTK_WINDOW(parent_dialog),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL
	);

	vbox = gtk_vbox_new(FALSE, 0);

#if GTK_MAJOR_VERSION <= 2
	select = gtk_combo_box_new_text();
#else
	select = gtk_combo_box_text_new();
#endif

	for(y=0; y<BRUSH_TABLE_HEIGHT; y++)
	{
		for(x=0; x<BRUSH_TABLE_WIDTH; x++)
		{
			if(setting->app->tool_window.brushes[y][x].name != NULL)
			{
				PointerArrayAppend(brush_names, setting->app->tool_window.brushes[y][x].name);
				StringRemoveTargetString(setting->app->tool_window.brushes[y][x].name, "\\n", buffer);
#if GTK_MAJOR_VERSION <= 2
				gtk_combo_box_append_text(GTK_COMBO_BOX(select),
#else
				gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(select),
#endif
					buffer);
				if(current != NULL)
				{
					if(strcmp(current, buffer) == 0)
					{
						gtk_combo_box_set_active(GTK_COMBO_BOX(select), num_selectable);
					}
				}
				num_selectable++;
			}
		}
	}
	if(current == NULL)
	{
		gtk_combo_box_set_active(GTK_COMBO_BOX(select), 0);
	}
	gtk_box_pack_start(GTK_BOX(vbox), select, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox, FALSE, FALSE, 0);
	gtk_widget_show_all(dialog);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		int active = gtk_combo_box_get_active(GTK_COMBO_BOX(select));
		char *name = MEM_STRDUP_FUNC((const char*)brush_names->buffer[active]);
		PointerArrayAppend(item->names, name);

		hbox = gtk_hbox_new(FALSE, 0);
		label = gtk_label_new(
#if GTK_MAJOR_VERSION <= 2
			gtk_combo_box_get_active_text(GTK_COMBO_BOX(select))
#else
			gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(select))
#endif
		);
		edit_button = gtk_button_new_with_label(setting->app->labels->menu.edit);
		g_object_set_data(G_OBJECT(edit_button), "brush_name", &item->names[item->names->num_data-1]);
		g_object_set_data(G_OBJECT(edit_button), "chain_dialog", (void*)parent_dialog);
		g_object_set_data(G_OBJECT(edit_button), "label_widget", (void*)label);
		(void)g_signal_connect(G_OBJECT(edit_button), "clicked",
			G_CALLBACK(OnEditBrushChainItemButtonPressed), setting);
		delete_button = gtk_button_new_with_label(setting->app->labels->unit._delete);
		g_object_set_data(G_OBJECT(delete_button), "brush_chain_item", item);
		g_object_set_data(G_OBJECT(delete_button), "delete_name", name);
		(void)g_signal_connect(G_OBJECT(delete_button), "clicked",
			G_CALLBACK(OnDeleteChainItemButtonPressed), hbox);
		g_object_set_data(G_OBJECT(edit_button), "delete_button", delete_button);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), edit_button, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(hbox), delete_button, FALSE, FALSE, 0);

		gtk_box_pack_start(GTK_BOX(table_vbox), hbox, FALSE, FALSE, 0);
		gtk_widget_show_all(hbox);
	}

	PointerArrayDestroy(&brush_names, NULL);

	gtk_widget_destroy(dialog);
}

static void OnDeleteBrushChainButtonPressed(GtkWidget* button, SET_PREFERENCE* setting)
{
	BRUSH_CHAIN_ITEM *item = (BRUSH_CHAIN_ITEM*)g_object_get_data(G_OBJECT(button), "delete_item");

	PointerArrayDestroy(&item->names, MEM_FREE_FUNC);
	PointerArrayRemoveByData(setting->app->tool_window.brush_chain.chains, item, NULL);

	gtk_widget_destroy(GTK_WIDGET(g_object_get_data(G_OBJECT(button), "delete_box")));
}

static void OnAddBrushChainButtonPressed(GtkWidget* button, SET_PREFERENCE* setting)
{
	GtkWidget *chains_vbox;
	GtkWidget *table_vbox;
	GtkWidget *hbox;
	GtkWidget *add_button;
	GtkWidget *delete_button;
	BRUSH_CHAIN_ITEM *item;

	chains_vbox = GTK_WIDGET(g_object_get_data(G_OBJECT(button), "chains_vbox"));

	item = (BRUSH_CHAIN_ITEM*)MEM_ALLOC_FUNC(sizeof(*item));
	InitializeBrushChainItem(item);
	PointerArrayAppend(setting->app->tool_window.brush_chain.chains, item);

	table_vbox = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(chains_vbox), table_vbox, FALSE, FALSE, 0);

	hbox = gtk_hbox_new(FALSE, 0);
	add_button = gtk_button_new_with_label(setting->app->labels->unit.add);
	g_object_set_data(G_OBJECT(add_button), "chain_item", (void*)item);
	g_object_set_data(G_OBJECT(add_button), "chains_vbox", (void*)chains_vbox);
	g_object_set_data(G_OBJECT(add_button), "table_vbox", (void*)table_vbox);
	(void)g_signal_connect(G_OBJECT(add_button), "clicked",
		G_CALLBACK(OnAddBrushChainItemButtonPressed), setting);
	delete_button = gtk_button_new_with_label(setting->app->labels->unit._delete);
	g_object_set_data(G_OBJECT(delete_button), "delete_item", (void*)item);
	g_object_set_data(G_OBJECT(delete_button), "delete_box", (void*)table_vbox);
	(void)g_signal_connect(G_OBJECT(delete_button), "clicked",
		G_CALLBACK(OnDeleteBrushChainButtonPressed), setting);
	gtk_box_pack_start(GTK_BOX(hbox), add_button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), delete_button, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(chains_vbox), hbox, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(chains_vbox), gtk_hseparator_new(), FALSE, FALSE, 3);

	gtk_widget_show_all(chains_vbox);
}

static void OnBrushChainButtonPressed(GtkWidget* button, SET_PREFERENCE* setting)
{
	GtkWidget *dialog = gtk_dialog_new_with_buttons(setting->app->labels->tool_box.brush_chain, GTK_WINDOW(setting->preference_dialog),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *chains_vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *chain_vbox;
	GtkWidget *table_vbox;
	GtkWidget *add_button;
	GtkWidget *delete_button;
	GtkWidget *edit_button;
	GtkWidget *entry;
	GtkWidget *hbox;
	GtkWidget *label;
	GtkWidget *add_chain_button = gtk_button_new_with_label(setting->app->labels->preference.add_brush_chain);
	char str[4096] = {0};
	int i, j;

	(void)g_signal_connect(G_OBJECT(add_chain_button), "clicked",
		G_CALLBACK(OnAddBrushChainButtonPressed), setting);
	g_object_set_data(G_OBJECT(chains_vbox), "chain_dialog", (void*)dialog);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), vbox, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), chains_vbox, FALSE, FALSE, 0);

	entry = gtk_entry_new();
	str[0] = (char)setting->app->tool_window.brush_chain.key;
	gtk_entry_set_text(GTK_ENTRY(entry), str);
	gtk_entry_set_max_length(GTK_ENTRY(entry), 1);
	label = gtk_label_new("Key : ");
	hbox = gtk_hbox_new(FALSE, 0);
	(void)g_signal_connect(G_OBJECT(entry), "changed",
		G_CALLBACK(OnBrushChainKeyChanged), &setting->app->tool_window.brush_chain);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 3);
	(void)sprintf(str, "%s : ", setting->app->labels->tool_box.change_brush_chain_key);
	label = gtk_label_new(str);
	str[0] = (char)setting->app->tool_window.brush_chain.change_key;
	str[1] = str[2] = str[3] = 0;
	entry = gtk_entry_new();
	gtk_entry_set_text(GTK_ENTRY(entry), str);
	gtk_entry_set_max_length(GTK_ENTRY(entry), 1);
	(void)g_signal_connect(G_OBJECT(entry), "changed",
		G_CALLBACK(OnChangeBrushChainKeyChanged), &setting->app->tool_window.brush_chain);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);

	gtk_box_pack_start(GTK_BOX(chains_vbox), hbox, FALSE, FALSE, 3);

	for(i=0; i<(int)setting->app->tool_window.brush_chain.chains->num_data; i++)
	{
		BRUSH_CHAIN_ITEM *item;
		chain_vbox = gtk_vbox_new(FALSE, 0);
		table_vbox = gtk_vbox_new(FALSE, 0);

		gtk_box_pack_start(GTK_BOX(chains_vbox), chain_vbox, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(chain_vbox), table_vbox, FALSE, FALSE, 0);

		item = (BRUSH_CHAIN_ITEM*)setting->app->tool_window.brush_chain.chains->buffer[i];
		for(j=0; j<(int)item->names->num_data; j++)
		{
			hbox = gtk_hbox_new(FALSE, 0);
			StringRemoveTargetString((char*)item->names->buffer[j], "\\n", str);
			label = gtk_label_new(str);
			edit_button = gtk_button_new_with_label(setting->app->labels->menu.edit);
			g_object_set_data(G_OBJECT(edit_button), "brush_name", &item->names[item->names->num_data-1]);
			g_object_set_data(G_OBJECT(edit_button), "chain_dialog", (void*)dialog);
			g_object_set_data(G_OBJECT(edit_button), "label_widget", (void*)label);
			(void)g_signal_connect(G_OBJECT(edit_button), "clicked",
				G_CALLBACK(OnEditBrushChainItemButtonPressed), setting);
			delete_button = gtk_button_new_with_label(setting->app->labels->unit._delete);
			g_object_set_data(G_OBJECT(delete_button), "brush_chain_item", item);
			g_object_set_data(G_OBJECT(delete_button), "delete_name", item->names->buffer[j]);
			(void)g_signal_connect(G_OBJECT(delete_button), "clicked",
				G_CALLBACK(OnDeleteChainItemButtonPressed), hbox);
			g_object_set_data(G_OBJECT(edit_button), "delete_button", delete_button);
			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
			gtk_box_pack_start(GTK_BOX(hbox), edit_button, FALSE, FALSE, 0);
			gtk_box_pack_start(GTK_BOX(hbox), delete_button, FALSE, FALSE, 0);
			gtk_box_pack_start(GTK_BOX(table_vbox), hbox, FALSE, FALSE, 0);
		}

		hbox = gtk_hbox_new(FALSE, 0);
		add_button = gtk_button_new_with_label(setting->app->labels->unit.add);
		g_object_set_data(G_OBJECT(add_button), "chain_item", (void*)item);
		g_object_set_data(G_OBJECT(add_button), "chains_vbox", (void*)chains_vbox);
		g_object_set_data(G_OBJECT(add_button), "table_vbox", (void*)table_vbox);
		(void)g_signal_connect(G_OBJECT(add_button), "clicked",
			G_CALLBACK(OnAddBrushChainItemButtonPressed), setting);
		gtk_box_pack_start(GTK_BOX(hbox), add_button, FALSE, FALSE, 0);
		delete_button = gtk_button_new_with_label(setting->app->labels->unit._delete);
		g_object_set_data(G_OBJECT(delete_button), "delete_item", (void*)item);
		g_object_set_data(G_OBJECT(delete_button), "delete_box", (void*)table_vbox);
		(void)g_signal_connect(G_OBJECT(delete_button), "clicked",
			G_CALLBACK(OnDeleteBrushChainButtonPressed), setting);
		gtk_box_pack_start(GTK_BOX(hbox), delete_button, FALSE, FALSE, 0);
		gtk_box_pack_start(GTK_BOX(chain_vbox), hbox, FALSE, FALSE, 0);

		gtk_box_pack_start(GTK_BOX(chains_vbox), gtk_hseparator_new(), FALSE, FALSE, 3);
	}

	g_object_set_data(G_OBJECT(add_chain_button), "chains_vbox", (void*)chains_vbox);
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), add_chain_button, FALSE, FALSE, 0);
	delete_button = gtk_button_new_with_label(setting->app->labels->unit._delete);
	gtk_box_pack_start(GTK_BOX(hbox), delete_button, FALSE, FALSE, 0);
	gtk_box_pack_end(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

	gtk_widget_show_all(dialog);
	(void)gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
}

static GtkWidget* CreateSettingWidget(SET_PREFERENCE *setting)
{
	APPLICATION *app = (APPLICATION*)g_object_get_data(
		G_OBJECT(setting->hbox), "application_data");
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);

	switch(setting->current_setting)
	{
	case SET_PREFERENCE_BASE_SETTING:
		{
			GtkWidget *label, *spin, *hbox, *button, *chooser;
			GtkAdjustment *adjust;
			GdkColor color = {0};
			gchar str[256];

			hbox = gtk_hbox_new(FALSE, 0);
			(void)sprintf(str, "%s : ", app->labels->save.compress);
			label = gtk_label_new(str);
			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
			spin = gtk_spin_button_new_with_range(
				Z_NO_COMPRESSION, Z_BEST_COMPRESSION, 1);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), setting->preference.compress);
			gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), 0);
			(void)g_signal_connect(G_OBJECT(spin), "changed",
				G_CALLBACK(ChangeCompression), setting);
			gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, TRUE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

			button = gtk_check_button_new_with_label(app->labels->preference.auto_save);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), setting->preference.auto_save);
			gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);
			adjust = GTK_ADJUSTMENT(gtk_adjustment_new(
				setting->preference.auto_save_time / 60, 3, 180, 1, 1, 0));
			spin = gtk_spin_button_new(adjust, 1, 0);
			gtk_widget_set_sensitive(spin, setting->preference.auto_save);
			g_object_set_data(G_OBJECT(button), "interval_button", spin);
			(void)sprintf(str, "%s : ", app->labels->unit.interval);
			label = gtk_label_new(str);
			hbox = gtk_hbox_new(FALSE, 0);
			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
			gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, TRUE, 0);
			label = gtk_label_new(app->labels->unit.minute);
			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
			(void)g_signal_connect(G_OBJECT(adjust), "value_changed",
				G_CALLBACK(AutoSaveSetInterval), setting);
			(void)g_signal_connect(G_OBJECT(button), "toggled",
				G_CALLBACK(AutoSaveCheckButtonClicked), setting);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 2);

			label = gtk_label_new(app->labels->preference.backup_path);
			gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 0);
			chooser = gtk_file_chooser_button_new(
				app->labels->preference.backup_path, GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
			gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(chooser), app->backup_directory_path);
			(void)g_signal_connect(G_OBJECT(chooser), "selection-changed",
				G_CALLBACK(BackupDirectoryChanged), setting);
			gtk_box_pack_start(GTK_BOX(vbox), chooser, FALSE, FALSE, 0);

			hbox = gtk_hbox_new(FALSE, 0);
			button = gtk_check_button_new_with_label(app->labels->preference.set_back_ground);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
				setting->flags & SET_PREFERENCE_CHANGE_BACK_GROUND_COLOR);
			gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 3);
			(void)g_signal_connect(G_OBJECT(button), "toggled",
				G_CALLBACK(SetChangeBackGroundColorFlag), setting);

			color.red = setting->preference.canvas_back_ground[0]
				| (setting->preference.canvas_back_ground[0] << 8);
			color.green = setting->preference.canvas_back_ground[1]
				| (setting->preference.canvas_back_ground[1] << 8);
			color.blue = setting->preference.canvas_back_ground[2]
				| (setting->preference.canvas_back_ground[2] << 8);
			button = gtk_color_button_new_with_color(&color);
			(void)g_signal_connect(G_OBJECT(button), "color-set",
				G_CALLBACK(SetBackGroundColor), setting);
			gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 3);

			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 3);

			button = gtk_check_button_new_with_label(app->labels->preference.show_preview_on_taskbar);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button),
				setting->flags & SET_PREFERENCE_SHOW_PREVIEW_ON_TASK_BAR);
			(void)g_signal_connect(G_OBJECT(button), "toggled",
				G_CALLBACK(SetShowPreviewWindowOnTaskbar), setting);
			gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 3);

			hbox = gtk_hbox_new(FALSE, 0);
			(void)sprintf(str, "%s : ", app->labels->preference.gui_scale);
			label = gtk_label_new(str);
			spin = gtk_spin_button_new_with_range(50, 300, 1);
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), app->gui_scale * 100);
			gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), 0);
			(void)g_signal_connect(G_OBJECT(spin), "value-changed",
				G_CALLBACK(OnChangeUserInterfaceScale), setting);
			gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
			gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, FALSE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 3);

#if GTK_MAJOR_VERSION >= 3
			{
				GtkWidget *radio_buttons[2];
				hbox = gtk_hbox_new(FALSE, 0);
				radio_buttons[0] = gtk_radio_button_new_with_label(
					NULL, app->labels->preference.scale_and_move_with_touch);
				g_object_set_data(G_OBJECT(radio_buttons[0]), "touch_mode",
					GINT_TO_POINTER(SET_PREFERENCE_TOUCH_MODE_SCALE_AND_MOVE_WITH_TOUCH));
				radio_buttons[1] = gtk_radio_button_new_with_label(
					gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
						app->labels->preference.draw_with_touch);
				g_object_set_data(G_OBJECT(radio_buttons[1]), "touch_mode",
					GINT_TO_POINTER(SET_PREFERENCE_TOUCH_MODE_DRAW_WITH_TOUCH));
				if((app->flags & APPLICATION_DRAW_WITH_TOUCH) == 0)
				{
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[0]), TRUE);
				}
				else
				{
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[1]), TRUE);
				}
				(void)g_signal_connect(G_OBJECT(radio_buttons[0]), "toggled",
					G_CALLBACK(ChangeTouchMode), setting);
				(void)g_signal_connect(G_OBJECT(radio_buttons[1]), "toggled",
					G_CALLBACK(ChangeTouchMode), setting);

				gtk_box_pack_start(GTK_BOX(hbox), radio_buttons[0], FALSE, FALSE, 0);
				gtk_box_pack_start(GTK_BOX(hbox), radio_buttons[1], FALSE, FALSE, 0);
				gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
			}
#else
			{
				GtkWidget *input_button;
				input_button = gtk_button_new_with_label("Input Setting");
				(void)g_signal_connect_swapped(G_OBJECT(input_button), "clicked",
					G_CALLBACK(OnClickInputSettingButton), NULL);
				gtk_box_pack_start(GTK_BOX(vbox), input_button, FALSE, FALSE, 0);
			}
#endif
			button = gtk_button_new_with_label(setting->app->labels->tool_box.brush_chain);
			(void)g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(OnBrushChainButtonPressed), setting);
			gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

			button = gtk_check_button_new_with_label(app->labels->preference.layer_window_scrollbar_place_left);
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), setting->flags & SET_PREFERENCE_LAYER_WINDOW_SCROLLBAR_PLACE_LEFT);
			(void)g_signal_connect(G_OBJECT(button), "toggled",
				G_CALLBACK(SetLayerWindowScrollbarPlaceLeftFlag), setting);
			gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);
/*
			button = gtk_button_new_with_label("Input Device");
			g_signal_connect(button, "clicked", G_CALLBACK(InputDeviceButtonClicked), setting);
			gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 2);
*/
		}

		break;
	case SET_PREFERENCE_THEME:
		gtk_box_pack_start(GTK_BOX(vbox), CreateChooseThemeWidget(
			setting, app), FALSE, TRUE, 0);
		break;
	case SET_PREFERENCE_ICC_PROFILE:
		{
			GtkWidget *hbox;
			hbox = gtk_hbox_new(FALSE, 0);
			gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Document:"), FALSE, FALSE, 0);
			gtk_box_pack_start(GTK_BOX(hbox), IccProfileChooser(&setting->input_icc_path, PROFILE_USAGE_RGB_DOCUMENT), TRUE, TRUE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);

			hbox = gtk_hbox_new(FALSE, 0);
			gtk_box_pack_start(GTK_BOX(hbox), gtk_label_new("Target:"), FALSE, FALSE, 0);
			gtk_box_pack_start(GTK_BOX(hbox), IccProfileChooser(&setting->output_icc_path, PROFILE_USAGE_PROOF_TARGET | PROFILE_USAGE_ANY), TRUE, TRUE, 0);
			gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 3);
		}
		break;
	case SET_PREFERENCE_LANGUAGE:
		gtk_box_pack_start(GTK_BOX(vbox), CreateChooseLanguageWidget(
			setting, app), FALSE, FALSE, 0);
		break;
	}

	return vbox;
}

typedef struct _FILE_NAME_LIST
{
	char *file_name;
	char *path;
	struct _FILE_NAME_LIST *next;
} FILE_NAME_LIST;

void GetGtkRcFilePath(const char* dir_name, FILE_NAME_LIST** list, const char* top_dir_name)
{
	// ディレクトリをオープン
	GDir *dir = g_dir_open(dir_name, 0, NULL);
	// 下層ディレクトリ
	GDir *child_dir;
	// 下層ディレクトリのパス
	char child_dir_path[4096];
	// ファイル名
	const gchar *file_name;

	// ディレクトリオープン失敗
	if(dir == NULL)
	{
		return;
	}

	// ディレクトリのファイルを読み込む
	while((file_name = g_dir_read_name(dir)) != NULL)
	{
		(void)sprintf(child_dir_path, "%s/%s", dir_name, file_name);
		child_dir = g_dir_open(child_dir_path, 0, NULL);
		if(child_dir != NULL)
		{
			g_dir_close(child_dir);
			GetGtkRcFilePath(child_dir_path, list, top_dir_name);
		}
		else
		{
			// rcファイル発見
			if(StringCompareIgnoreCase(file_name, "gtkrc") == 0)
			{
				FILE_NAME_LIST *target = (FILE_NAME_LIST*)MEM_ALLOC_FUNC(sizeof(*target));

				if(*list != NULL)
				{
					(*list)->next = target;
					*list = target;
				}
				else
				{
					*list = target;
				}

				target->file_name = MEM_STRDUP_FUNC(top_dir_name);
				target->path = MEM_STRDUP_FUNC(child_dir_path);
				target->next = NULL;

				break;
			}
		}
	}

	g_dir_close(dir);
}

/***********************************************************
* SetTheme関数                                             *
* テーマ変更時に呼び出されるコールバック関数               *
* 引数                                                     *
* widget		: テーマ変更用のコンボボックスウィジェット *
* preference	: 環境設定情報                             *
***********************************************************/
static void SetTheme(GtkWidget* widget, SET_PREFERENCE* preference)
{
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(widget));
	FILE_NAME_LIST *list = (FILE_NAME_LIST*)g_object_get_data(G_OBJECT(widget), "first_list");
	int i;

	if(index == 0)
	{
		MEM_FREE_FUNC(preference->preference.theme);
		preference->preference.theme = NULL;
	}
	else
	{
		for(i=0; i<index-1; i++)
		{
			list = list->next;
		}
		MEM_FREE_FUNC(preference->preference.theme);
		preference->preference.theme = MEM_STRDUP_FUNC(list->path);
	}

	preference->flags |= SET_PREFERENCE_CHANGE_THEME;
}

/***********************************************************
* OnDestroySetThemeWidget関数                              *
* テーマ変更用のウィジェットが削除されるときに呼び出される *
* 引数                                                     *
* widget	: テーマ変更用のコンボボックスウィジェット     *
* list		: テーマファイルリスト                         *
***********************************************************/
static void OnDestroySetThemeWidget(GtkWidget* widget, FILE_NAME_LIST* list)
{
	FILE_NAME_LIST *delete_list = list;

	while(delete_list != NULL)
	{
		delete_list = list->next;
		MEM_FREE_FUNC(list->file_name);
		MEM_FREE_FUNC(list->path);
		MEM_FREE_FUNC(list);
		list = delete_list;
	}
}

/*************************************************************
* CreateChooseThemeWidget関数                                *
* テーマ選択のウィジェットを作成する                         *
* 引数                                                       *
* preference	: 環境設定情報                               *
* app			: アプリケーションを管理する構造体のアドレス *
* 返り値                                                     *
*	作成したウィジェット                                     *
*************************************************************/
static GtkWidget* CreateChooseThemeWidget(SET_PREFERENCE* preference, APPLICATION* app)
{
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	FILE_NAME_LIST *list = NULL;
	FILE_NAME_LIST *first_list = NULL;
	gchar *path = g_build_filename(app->current_path, "share/themes", NULL);
	GDir *dir = g_dir_open(path, 0, NULL);
	GtkWidget *combo;
	char dir_path[4096];
	int combo_index = 0;
	int index = 0;

	if(dir != NULL)
	{
		const gchar *file_name;

		while((file_name = g_dir_read_name(dir)) != NULL)
		{
			(void)sprintf(dir_path, "%s/%s", path, file_name);
			GetGtkRcFilePath(dir_path, &list, file_name);

			if(first_list == NULL)
			{
				first_list = list;
			}
		}

		g_dir_close(dir);
	}

	list = first_list;
#if GTK_MAJOR_VERSION <= 2
	combo = gtk_combo_box_new_text();
	gtk_combo_box_append_text(GTK_COMBO_BOX(combo), app->labels->preference.default_theme);
#else
	combo = gtk_combo_box_text_new();
	gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), app->labels->preference.default_theme);
#endif
	while(list != NULL)
	{
		index++;
#if GTK_MAJOR_VERSION <= 2
		gtk_combo_box_append_text(GTK_COMBO_BOX(combo), list->file_name);
#else
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), list->file_name);
#endif

		if(preference->preference.theme != NULL)
		{
			if(strcmp(list->path, preference->preference.theme) == 0)
			{
				combo_index = index;
			}
		}

		list = list->next;
	}
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), combo_index);
	g_object_set_data(G_OBJECT(combo), "first_list", first_list);
	(void)g_signal_connect(G_OBJECT(combo), "destroy",
		G_CALLBACK(OnDestroySetThemeWidget), first_list);
	(void)g_signal_connect(G_OBJECT(combo), "changed",
		G_CALLBACK(SetTheme), preference);
	gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, TRUE, 0);

	g_free(path);

	return vbox;
}

void SetLanguage(GtkWidget* combo, SET_PREFERENCE* preference)
{
	char **file_paths = (char**)g_object_get_data(G_OBJECT(combo), "lang_file_paths");
	int index = gtk_combo_box_get_active(GTK_COMBO_BOX(combo));

	g_free(preference->language_name);
	MEM_FREE_FUNC(preference->language_path);

#if GTK_MAJOR_VERSION <= 2
	preference->language_name = g_strdup(
		gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo)));
#else
	preference->language_name = g_strdup(
		gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo)));
#endif
	preference->language_path = MEM_STRDUP_FUNC(file_paths[index]);
}

void OnDestroyChooseLanguageWidget(GtkWidget* widget, char** file_paths)
{
	int num_files = GPOINTER_TO_INT(
		g_object_get_data(G_OBJECT(widget), "num_lang_files"));
	int i;

	for(i=0; i<num_files; i++)
	{
		MEM_FREE_FUNC(file_paths[i]);
	}

	MEM_FREE_FUNC(file_paths);
}

/*************************************************************
* CreateChooseLanguageWidget関数                             *
* 言語選択用ウィジェットを作成する                           *
* 引数                                                       *
* preference	: 環境設定情報                               *
* app			: アプリケーションを管理する構造体のアドレス *
* 返り値                                                     *
*	作成したウィジェット                                     *
*************************************************************/
static GtkWidget* CreateChooseLanguageWidget(SET_PREFERENCE* preference, APPLICATION* app)
{
#define MAX_STR_SIZE 512
	INI_FILE_PTR ini;
	GtkWidget *vbox;
	GtkWidget *combo;
	GDir *dir;
	GFile *fp;
	GFileInputStream *stream;
	GFileInfo *file_info;
	size_t data_size;
	gchar *path = g_build_filename(app->current_path, "lang", NULL);
	const gchar *file_name;
	gchar *disp_str;
	char file_path[4096];
	char lang_name[MAX_STR_SIZE];
	char *file_names[1024];
	int num_files = 0;
	char code[MAX_STR_SIZE];

	dir = g_dir_open(path, 0, NULL);

	if(dir == NULL)
	{
		g_free(path);

		return NULL;
	}

	vbox = gtk_vbox_new(FALSE, 0);
#if GTK_MAJOR_VERSION <= 2
	combo = gtk_combo_box_new_text();
#else
	combo = gtk_combo_box_text_new();
#endif

	while((file_name = g_dir_read_name(dir)) != NULL)
	{
		(void)sprintf(file_path, "%s/%s", path, file_name);
		fp = g_file_new_for_path(file_path);
		stream = g_file_read(fp, NULL, NULL);

		if(stream != NULL)
		{
			file_info = g_file_input_stream_query_info(stream,
				G_FILE_ATTRIBUTE_STANDARD_SIZE, NULL, NULL);
			data_size = (size_t)g_file_info_get_size(file_info);

			ini = CreateIniFile((void*)stream,
				(size_t (*)(void*, size_t, size_t, void*))FileRead,
					data_size, INI_READ
			);

			lang_name[0] = '\0';
			(void)IniFileGetString(ini, "CODE", "CODE_TYPE", code, MAX_STR_SIZE);
			(void)IniFileGetString(ini, "LANGUAGE", "LANGUAGE", lang_name, MAX_STR_SIZE);

			if(lang_name[0] != '\0')
			{
				disp_str = g_convert(lang_name, -1, "UTF-8", code, NULL, NULL, NULL);
#if GTK_MAJOR_VERSION <= 2
				gtk_combo_box_append_text(GTK_COMBO_BOX(combo), disp_str);
#else
				gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), disp_str);
#endif
				if(strcmp(disp_str, preference->language_name) == 0)
				{
					gtk_combo_box_set_active(GTK_COMBO_BOX(combo), num_files);
				}

				g_free(disp_str);

				(void)sprintf(file_path, "./lang/%s", file_name);
				file_names[num_files] = MEM_STRDUP_FUNC(file_path);
				num_files++;
			}

			ini->delete_func(ini);

			g_object_unref(file_info);
		}

		g_object_unref(stream);
		g_object_unref(fp);
	}

	{
		char **set_data = (char**)MEM_ALLOC_FUNC(sizeof(*set_data)*num_files);
		int i;

		for(i=0; i<num_files; i++)
		{
			set_data[i] = file_names[i];
		}

		g_object_set_data(G_OBJECT(combo), "lang_file_paths", set_data);
		g_object_set_data(G_OBJECT(combo), "num_lang_files", GINT_TO_POINTER(num_files));
		(void)g_signal_connect(G_OBJECT(combo), "destroy",
			G_CALLBACK(OnDestroyChooseLanguageWidget), set_data);
		(void)g_signal_connect(G_OBJECT(combo), "changed",
			G_CALLBACK(SetLanguage), preference);
	}

	gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 0);

	g_free(path);

	return vbox;
}

/*************************************************************
* PreferenceItemChanged関数                                  *
* 環境設定の項目が変更された時に呼び出されるコールバック関数 *
* 引数                                                       *
* tree			: ツリービューウィジェット                   *
* preference	: 環境設定の情報                             *
*************************************************************/
static void PreferenceItemChanged(GtkWidget* tree, SET_PREFERENCE* preference)
{
	APPLICATION *app = (APPLICATION*)g_object_get_data(
		G_OBJECT(tree), "application_data");
	GtkTreeModel *model;
	GtkTreeIter iter;
	char *value;

	if(gtk_tree_selection_get_selected(GTK_TREE_SELECTION(tree),
		&model, &iter) != FALSE)
	{
		int index = 0;

		gtk_tree_model_get(model, &iter, 0, &value, -1);

		if(strcmp(value, app->labels->preference.base_setting) == 0)
		{
			index = SET_PREFERENCE_BASE_SETTING;
		}
		else if(strcmp(value, app->labels->preference.theme) == 0)
		{
			index = SET_PREFERENCE_THEME;
		}
		else if(strcmp(value, "ICC Profile") == 0)
		{
			index = SET_PREFERENCE_ICC_PROFILE;
		}
		else
		{
			index = SET_PREFERENCE_LANGUAGE;
		}

		gtk_widget_destroy(preference->setting_widget);
		preference->current_setting = (int16)index;
		preference->setting_widget = CreateSettingWidget(preference);
		gtk_box_pack_start(GTK_BOX(preference->hbox),
			preference->setting_widget, TRUE, TRUE, 10);
		gtk_widget_show_all(preference->setting_widget);
	}
}

/*************************************************************
* SetPreferenceTreeView関数                                  *
* 環境設定での項目選択用のツリービューを作成                 *
* 引数                                                       *
* tree			: ツリービューウィジェット                   *
* preference	: 環境設定の情報                             *
* app			: アプリケーションを管理する構造体のアドレス *
*************************************************************/
static void SetPreferenceTreeView(
	GtkWidget* tree,
	SET_PREFERENCE* preference,
	APPLICATION* app
)
{
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkListStore *store;
	GtkTreeIter iter;
	GtkTreeSelection *selection;

	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(tree), FALSE);

	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", 0);
	gtk_tree_view_append_column(GTK_TREE_VIEW(tree), column);

	store = gtk_list_store_new(1, G_TYPE_STRING);

	gtk_tree_view_set_model(GTK_TREE_VIEW(tree), GTK_TREE_MODEL(store));
	g_object_unref(store);

	store = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree)));

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, app->labels->preference.base_setting, -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, app->labels->preference.theme, -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, "ICC Profile", -1);

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, 0, app->labels->preference.language);

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));

	(void)g_object_set_data(G_OBJECT(selection), "application_data", app);
	(void)g_signal_connect(G_OBJECT(selection), "changed",
		G_CALLBACK(PreferenceItemChanged), preference);
}

/*****************************************************
* ExecuteSetPreference関数                           *
* 環境設定を変更する                                 *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecuteSetPreference(APPLICATION* app)
{
	// 環境設定用のダイアログボックスを作成
	GtkWidget *dialog =
		gtk_dialog_new_with_buttons(
			app->labels->preference.title,
			GTK_WINDOW(app->widgets->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			GTK_STOCK_APPLY,
			GTK_RESPONSE_APPLY,
			NULL
		);
	SET_PREFERENCE setting = {
		gtk_hbox_new(FALSE, 0),
		NULL,
		app->preference.current_setting
	};
	GtkWidget *tree_view;
	gint ret;
	cmsHPROFILE monitor_icc;
	int i;

	tree_view = gtk_tree_view_new();
	SetPreferenceTreeView(tree_view, &setting, app);
	setting.preference_dialog = dialog;
	setting.app = app;
	setting.gui_scale = app->gui_scale;

	setting.preference = app->preference;
	if(app->input_icc_path != NULL)
	{
		setting.input_icc_path = g_locale_to_utf8(app->input_icc_path, -1, NULL, NULL, NULL);
	}
	else
	{
		setting.input_icc_path = NULL;
	}

	if((app->layer_window.flags & SET_PREFERENCE_LAYER_WINDOW_SCROLLBAR_PLACE_LEFT) != 0)
	{
		setting.flags |= SET_PREFERENCE_LAYER_WINDOW_SCROLLBAR_PLACE_LEFT;
	}

	if(app->output_icc_path != NULL)
	{
		setting.output_icc_path = g_locale_to_utf8(app->output_icc_path, -1, NULL, NULL, NULL);
	}
	else
	{
		setting.output_icc_path = NULL;
	}

	if(app->preference.theme != NULL)
	{
		setting.preference.theme = MEM_STRDUP_FUNC(app->preference.theme);
	}
	else
	{
		setting.preference.theme = NULL;
	}

	setting.language_name = g_strdup(app->labels->language_name);
	setting.language_path = MEM_STRDUP_FUNC(app->language_file_path);
	setting.backup_path = g_strdup(app->backup_directory_path);
	if((app->flags & APPLICATION_SET_BACK_GROUND_COLOR) != 0)
	{
		setting.flags |= SET_PREFERENCE_CHANGE_BACK_GROUND_COLOR;
	}
	if((app->flags & APPLICATION_SHOW_PREVIEW_ON_TASK_BAR) != 0)
	{
		setting.flags |= SET_PREFERENCE_SHOW_PREVIEW_ON_TASK_BAR;
	}
	g_object_set_data(G_OBJECT(setting.hbox), "application_data", app);

	setting.setting_widget = CreateSettingWidget(&setting);
	gtk_box_pack_start(GTK_BOX(setting.hbox), tree_view, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(setting.hbox), setting.setting_widget, FALSE, TRUE, 10);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		setting.hbox, FALSE, TRUE, 0);
	gtk_widget_show_all(setting.hbox);

	while(1)
	{
		setting.flags &=
			SET_PREFERENCE_CHANGE_BACK_GROUND_COLOR | SET_PREFERENCE_SHOW_PREVIEW_ON_TASK_BAR
				| SET_PREFERENCE_LAYER_WINDOW_SCROLLBAR_PLACE_LEFT;
		ret = gtk_dialog_run(GTK_DIALOG(dialog));

		if(ret == GTK_RESPONSE_APPLY)
		{
			MEM_FREE_FUNC(app->preference.theme);
			if((setting.flags & SET_PREFERENCE_CHANGE_THEME) != 0)
			{
				gtk_rc_parse(setting.preference.theme);
			}

			if((setting.flags & SET_PREFERENCE_DRAW_WITH_TOUCH) != 0)
			{
				app->flags |= APPLICATION_DRAW_WITH_TOUCH;
			}
			else
			{
				app->flags &= ~(APPLICATION_DRAW_WITH_TOUCH);
			}

			if((setting.flags & SET_PREFERENCE_CHANGE_BACK_GROUND_COLOR) == 0)
			{
				app->flags &= ~(APPLICATION_SET_BACK_GROUND_COLOR);
			}
			else
			{
				app->flags |= APPLICATION_SET_BACK_GROUND_COLOR;
			}

			if((setting.flags & SET_PREFERENCE_SHOW_PREVIEW_ON_TASK_BAR) == 0)
			{
				app->flags &= ~(APPLICATION_SHOW_PREVIEW_ON_TASK_BAR);
			}
			else
			{
				app->flags |= APPLICATION_SHOW_PREVIEW_ON_TASK_BAR;
			}

			if((setting.flags & SET_PREFERENCE_LAYER_WINDOW_SCROLLBAR_PLACE_LEFT) == 0)
			{
				app->layer_window.flags &= ~(LAYER_WINDOW_SCROLLBAR_PLACE_LEFT);
			}
			else
			{
				app->layer_window.flags |= LAYER_WINDOW_SCROLLBAR_PLACE_LEFT;
			}

			app->preference = setting.preference;
			app->gui_scale = setting.gui_scale;
			if(app->preference.theme != NULL)
			{
				setting.preference.theme = MEM_STRDUP_FUNC(app->preference.theme);
			}
			else
			{
				setting.preference.theme = NULL;
			}

			if(app->labels->language_name != setting.language_name)
			{
				g_free(app->labels->language_name);
				app->labels->language_name = setting.language_name;
				MEM_FREE_FUNC(app->language_file_path);
				app->language_file_path = setting.language_path;
			}

			if(app->backup_directory_path != setting.backup_path)
			{
				g_free(app->backup_directory_path);
				app->backup_directory_path = setting.backup_path;
			}

			g_free(app->input_icc_path);
			(void)cmsCloseProfile(app->input_icc);
			if(setting.input_icc_path != NULL)
			{
				app->input_icc_path = g_locale_from_utf8(setting.input_icc_path, -1, NULL, NULL, NULL);
			}
			else
			{
				app->input_icc_path = NULL;
			}

			if(app->input_icc_path != NULL)
			{
				if(*app->input_icc_path == '\0')
				{
					//app->input_icc = CreateDefaultSrgbProfile();
					app->input_icc = GetPrimaryMonitorProfile();
					g_free(app->input_icc_path);
					app->input_icc_path = NULL;
				}
				else
				{
					app->input_icc = cmsOpenProfileFromFile(app->input_icc_path, "r");
				}
			}
			else
			{
				//app->input_icc = CreateDefaultSrgbProfile();
				app->input_icc = GetPrimaryMonitorProfile();
			}

			g_free(app->output_icc_path);
			(void)cmsCloseProfile(app->output_icc);
			if(setting.output_icc_path != NULL)
			{
				app->output_icc_path = g_locale_from_utf8(setting.output_icc_path, -1, NULL, NULL, NULL);
			}
			else
			{
				app->output_icc_path = NULL;
			}

			if(app->output_icc_path != NULL)
			{
				app->output_icc = cmsOpenProfileFromFile(app->output_icc_path, "r");
			}
			else
			{
				app->output_icc = NULL;
			}

			if(app->icc_transform != NULL)
			{
				cmsDeleteTransform(app->icc_transform);
			}

			monitor_icc = GetPrimaryMonitorProfile();

			if(app->output_icc != NULL)
			{
				cmsBool bpc[] = { TRUE, TRUE, TRUE, TRUE };
				cmsHPROFILE hProfiles[] = { app->input_icc, app->output_icc, app->output_icc, monitor_icc };
				cmsUInt32Number intents[] = { INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC };
				cmsFloat64Number adaptationStates[] = { 0, 0, 0, 0 };

				app->icc_transform = cmsCreateExtendedTransform(cmsGetProfileContextID(hProfiles[1]), 4, hProfiles,
					bpc, intents, adaptationStates,
					NULL, 0, TYPE_BGRA_8, TYPE_BGRA_8, 0 /*cmsFLAGS_SOFTPROOFING*/);

				for(i=0; i<app->window_num; i++)
				{
					if(app->draw_window[i]->input_icc != NULL)
					{
						hProfiles[0] = app->draw_window[i]->input_icc;
						if(app->draw_window[i]->icc_transform != NULL)
						{
							cmsDeleteTransform(app->draw_window[i]->icc_transform);
						}
						app->draw_window[i]->icc_transform = cmsCreateExtendedTransform(cmsGetProfileContextID(hProfiles[1]), 4, hProfiles,
							bpc, intents, adaptationStates, NULL, 0, TYPE_BGRA_8, TYPE_BGRA_8, 0 /*cmsFLAGS_SOFTPROOFING*/);
					}
				}
			}
			else
			{
				app->icc_transform = cmsCreateTransform(app->input_icc, TYPE_BGRA_8,
					monitor_icc, TYPE_BGRA_8, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_BLACKPOINTCOMPENSATION);

				for(i=0; i<app->window_num; i++)
				{
					if(app->draw_window[i]->input_icc != NULL)
					{
						if(app->draw_window[i]->icc_transform != NULL)
						{
							cmsDeleteTransform(app->draw_window[i]->icc_transform);
						}
						app->draw_window[i]->icc_transform = cmsCreateTransform(app->draw_window[i]->input_icc, TYPE_BGRA_8,
							monitor_icc, TYPE_BGRA_8, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_BLACKPOINTCOMPENSATION);
					}
				}
			}

			cmsCloseProfile(monitor_icc);

			gtk_widget_queue_draw(app->tool_window.color_chooser->widget);
			UpdateColorBox(app->tool_window.color_chooser);
			gtk_widget_queue_draw(app->tool_window.color_chooser->pallete_widget);

			if(app->window_num > 0)
			{
				app->draw_window[app->active_window]->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
				gtk_widget_queue_draw(app->draw_window[app->active_window]->window);

				UpdateWindowTitle(app);
			}
		}
		else
		{
			if(ret == GTK_RESPONSE_ACCEPT)
			{
				MEM_FREE_FUNC(app->preference.theme);
				if((setting.flags & SET_PREFERENCE_CHANGE_THEME) != 0)
				{
					gtk_rc_parse(setting.preference.theme);
				}

				app->preference = setting.preference;
				app->gui_scale = setting.gui_scale;

				if(app->labels->language_name != setting.language_name)
				{
					g_free(app->labels->language_name);
					app->labels->language_name = setting.language_name;
					MEM_FREE_FUNC(app->language_file_path);
					app->language_file_path = setting.language_path;
				}

				if(app->backup_directory_path != setting.backup_path)
				{
					g_free(app->backup_directory_path);
					app->backup_directory_path = setting.backup_path;
				}

				g_free(app->input_icc_path);
				(void)cmsCloseProfile(app->input_icc);
				if(setting.input_icc_path != NULL)
				{
					app->input_icc_path = g_locale_from_utf8(setting.input_icc_path, -1, NULL, NULL, NULL);
				}
				else
				{
					app->input_icc_path = NULL;
				}

				if(app->input_icc_path != NULL)
				{
					if(*app->input_icc_path == '\0')
					{
						//app->input_icc = CreateDefaultSrgbProfile();
						app->input_icc = GetPrimaryMonitorProfile();
						g_free(app->input_icc_path);
						app->input_icc_path = NULL;
					}
					else
					{
						app->input_icc = cmsOpenProfileFromFile(app->input_icc_path, "r");
					}
				}
				else
				{
					//app->input_icc = CreateDefaultSrgbProfile();
					app->input_icc = GetPrimaryMonitorProfile();
				}

				g_free(app->output_icc_path);
				(void)cmsCloseProfile(app->output_icc);
				if(setting.output_icc_path != NULL)
				{
					app->output_icc_path = g_locale_from_utf8(setting.output_icc_path, -1, NULL, NULL, NULL);
				}
				else
				{
					app->output_icc_path = NULL;
				}

				if(app->output_icc_path != NULL)
				{
					app->output_icc = cmsOpenProfileFromFile(app->output_icc_path, "r");
				}
				else
				{
					app->output_icc = NULL;
				}

				if(app->icc_transform != NULL)
				{
					cmsDeleteTransform(app->icc_transform);
				}

				monitor_icc = GetPrimaryMonitorProfile();

				if(app->output_icc != NULL)
				{
					cmsBool bpc[] = { TRUE, TRUE, TRUE, TRUE };
					cmsHPROFILE hProfiles[] = { app->input_icc, app->output_icc, app->output_icc, monitor_icc };
					cmsUInt32Number intents[] = { INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC };
					cmsFloat64Number adaptationStates[] = { 0, 0, 0, 0 };

					app->icc_transform = cmsCreateExtendedTransform(cmsGetProfileContextID(hProfiles[1]), 4, hProfiles,
						bpc, intents, adaptationStates,
						NULL, 0, TYPE_BGRA_8, TYPE_BGRA_8, 0 /*cmsFLAGS_SOFTPROOFING*/);
				}
				else
				{
					app->icc_transform = cmsCreateTransform(app->input_icc, TYPE_BGRA_8,
						monitor_icc, TYPE_BGRA_8, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_BLACKPOINTCOMPENSATION);
				}

				cmsCloseProfile(monitor_icc);

				if((setting.flags & SET_PREFERENCE_DRAW_WITH_TOUCH) != 0)
				{
					app->flags |= APPLICATION_DRAW_WITH_TOUCH;
				}
				else
				{
					app->flags &= ~(APPLICATION_DRAW_WITH_TOUCH);
				}

				if((setting.flags & SET_PREFERENCE_CHANGE_BACK_GROUND_COLOR) == 0)
				{
					app->flags &= ~(APPLICATION_SET_BACK_GROUND_COLOR);
				}
				else
				{
					app->flags |= APPLICATION_SET_BACK_GROUND_COLOR;
				}

				if((setting.flags & SET_PREFERENCE_SHOW_PREVIEW_ON_TASK_BAR) == 0)
				{
					app->flags &= ~(APPLICATION_SHOW_PREVIEW_ON_TASK_BAR);
				}
				else
				{
					app->flags |= APPLICATION_SHOW_PREVIEW_ON_TASK_BAR;
				}

				if((setting.flags & SET_PREFERENCE_LAYER_WINDOW_SCROLLBAR_PLACE_LEFT) == 0)
				{
					app->layer_window.flags &= ~(LAYER_WINDOW_SCROLLBAR_PLACE_LEFT);
				}
				else
				{
					app->layer_window.flags |= LAYER_WINDOW_SCROLLBAR_PLACE_LEFT;
				}
			}
			else
			{
				MEM_FREE_FUNC(setting.preference.theme);
			}

			gtk_widget_destroy(dialog);
			g_free(setting.input_icc_path);
			g_free(setting.output_icc_path);

			gtk_widget_queue_draw(app->tool_window.color_chooser->widget);
			UpdateColorBox(app->tool_window.color_chooser);
			gtk_widget_queue_draw(app->tool_window.color_chooser->pallete_widget);

			if(app->window_num > 0)
			{
				app->draw_window[app->active_window]->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
				gtk_widget_queue_draw(app->draw_window[app->active_window]->window);

				UpdateWindowTitle(app);
			}

			break;
		}
	}
}

/*************************************************************
* ReadPreference関数                                         *
* 環境設定を読み込む                                         *
* 引数                                                       *
* file			: 初期化ファイル読み込み用の構造体のアドレス *
* preference	: 環境設定を管理する構造体のアドレス         *
*************************************************************/
void ReadPreference(INI_FILE_PTR file, PREFERENCE* preference)
{
	uint32 color;
	uint8 *color_buff = (uint8*)&color;
	char color_string[128] = {0};

	preference->compress = (int8)IniFileGetInteger(file, "PREFERENCE", "COMPRESSION");
	preference->auto_save = (int8)IniFileGetInteger(file, "PREFERENCE", "AUTO_SAVE");
	preference->auto_save_time = (int32)IniFileGetInteger(file, "PREFERENCE", "AUTO_SAVE_INTERVAL") * 60;
	if(preference->auto_save_time < 300)
	{
		preference->auto_save_time = 300;
	}

	if(IniFileGetString(file, "PREFERENCE", "BACK_GROUND_COLOR", color_string, 128) > 0)
	{
		(void)sscanf(color_string, "%x", &color);
		preference->canvas_back_ground[0] = color_buff[2];
		preference->canvas_back_ground[1] = color_buff[1];
		preference->canvas_back_ground[2] = color_buff[0];
	}
}

/*************************************************************
* WritePreference関数                                        *
* 環境設定を書き込む                                         *
* 引数                                                       *
* file			: 初期化ファイル書き込み用の構造体のアドレス *
* preference	: 環境設定を管理する構造体のアドレス         *
*************************************************************/
void WritePreference(INI_FILE_PTR file, PREFERENCE* preference)
{
	int32 color;

	(void)IniFileAddInteger(file, "PREFERENCE", "COMPRESSION",
		preference->compress, 10);
	(void)IniFileAddInteger(file, "PREFERENCE", "AUTO_SAVE",
		preference->auto_save, 10);
	(void)IniFileAddInteger(file, "PREFERENCE", "AUTO_SAVE_INTERVAL",
		preference->auto_save_time / 60, 10);

	color = (preference->canvas_back_ground[0] << 16)
		| (preference->canvas_back_ground[1] << 8)
		| preference->canvas_back_ground[2];
	(void)IniFileAddInteger(file, "PREFERENCE", "BACK_GROUND_COLOR",
		color, 16);
}

#ifdef __cplusplus
}
#endif
