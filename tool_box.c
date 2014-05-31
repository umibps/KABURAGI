// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <ctype.h>
#include "application.h"
#include "menu.h"
#include "brush_core.h"
#include "vector_brush_core.h"
#include "brushes.h"
#include "vector_brushes.h"
#include "ini_file.h"
#include "memory.h"
#include "color.h"
#include "widgets.h"
#include "utils.h"
#include "input.h"
#include "image_read_write.h"
#include "script.h"
#include "display.h"
#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
# include "MikuMikuGtk+/ui.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

void CommonToolButtonClicked(GtkWidget* widget, gpointer data)
{
	COMMON_TOOL_CORE* core = (COMMON_TOOL_CORE*)data;
	APPLICATION* app = (APPLICATION*)g_object_get_data(G_OBJECT(widget), "application");
	int i, j;

	if(app->window_num > 0)
	{
		DRAW_WINDOW *window = app->draw_window[app->active_window];
		int do_release_func = 0;

		window->flags |= DRAW_WINDOW_TOOL_CHANGING;

		if((app->tool_window.flags & TOOL_USING_BRUSH) == 0)
		{
			if((app->tool_window.active_common_tool->flags & COMMON_TOOL_DO_RELEASE_FUNC_AT_CHANGI_TOOL) != 0)
			{
				do_release_func++;
			}
		}

		if((window->state & GDK_BUTTON1_MASK) != 0 || do_release_func != 0)
		{
			GdkEventButton event_info = {0};
			gdouble x, y;
			gdouble x0, y0;
			const gdouble pressure = 0;

			x0 = (window->before_cursor_x - window->half_size) * window->cos_value
				- (window->before_cursor_y - window->half_size) * window->sin_value + window->add_cursor_x;
			y0 = (window->before_cursor_x - window->half_size) * window->sin_value
				+ (window->before_cursor_y - window->half_size) * window->cos_value + window->add_cursor_y;
			x = window->rev_zoom * x0;
			y = window->rev_zoom * y0;

			event_info.button = 1;
			event_info.state = window->state;

			if((window->app->tool_window.flags & TOOL_USING_BRUSH) == 0)
			{
				window->app->tool_window.active_common_tool->release_func(
					window, x, y,
					window->app->tool_window.active_common_tool, (void*)&event_info
				);
			}
			else
			{
				if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
					|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
				{
					window->app->tool_window.active_brush[window->app->input]->release_func(
						window, x, y, pressure,
						window->app->tool_window.active_brush[window->app->input], (void*)&event_info
					);
				}
				else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
				{
					if((event_info.state & GDK_SHIFT_MASK) != 0)
					{
						window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
						window->app->tool_window.vector_control_core.release_func(
							window, x, y,
							pressure, &window->app->tool_window.vector_control_core, (void*)&event_info
						);
					}
					else if((event_info.state & GDK_CONTROL_MASK) != 0 ||
						(window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
					{
						window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
						window->app->tool_window.vector_control_core.release_func(
							window, x,y,
							pressure, &window->app->tool_window.vector_control_core, (void*)&event_info
						);
					}
					else
					{
						window->app->tool_window.active_vector_brush[window->app->input]->release_func(
							window, x, y,
							pressure, window->app->tool_window.active_vector_brush[window->app->input], (void*)&event_info
						);
					}
				}
			}

			window->state &= ~(GDK_BUTTON1_MASK);
		}

		window->flags &= ~(DRAW_WINDOW_TOOL_CHANGING);
	}

	if(app->tool_window.active_common_tool == core
		&& (app->tool_window.flags & TOOL_USING_BRUSH) == 0)
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
		}
		return;
	}
	else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		return;
	}

	app->tool_window.flags &= ~(TOOL_USING_BRUSH);
	gtk_widget_destroy(app->tool_window.detail_ui);
	app->tool_window.active_common_tool = core;
	app->tool_window.detail_ui = core->create_detail_ui(app, core->tool_data);
	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(app->tool_window.detail_ui_scroll), app->tool_window.detail_ui);
	gtk_widget_show_all(app->tool_window.detail_ui);

	for(i=0; i<COMMON_TOOL_TABLE_HEIGHT; i++)
	{
		for(j=0; j<COMMON_TOOL_TABLE_WIDTH; j++)
		{
			if(app->tool_window.common_tools[i][j].tool_type == TYPE_LOUPE_TOOL)
			{
				continue;
			}
			if(&app->tool_window.common_tools[i][j] == core)
			{
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
					app->tool_window.common_tools[i][j].button)) == FALSE)
				{
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
						app->tool_window.common_tools[i][j].button), TRUE);
				}
			}
			else
			{
				if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
					app->tool_window.common_tools[i][j].button)) != FALSE)
				{
					gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
						app->tool_window.common_tools[i][j].button), FALSE);
				}
			}
		}
	}

	if(app->current_tool == TYPE_NORMAL_LAYER)
	{
		for(i=0; i<BRUSH_TABLE_HEIGHT; i++)
		{
			for(j=0; j<BRUSH_TABLE_WIDTH; j++)
			{
				if(GTK_IS_TOGGLE_BUTTON(app->tool_window.brushes[i][j].button) != FALSE)
				{
					gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(app->tool_window.brushes[i][j].button), FALSE);
				}
			}
		}
	}
	else if(app->current_tool == TYPE_VECTOR_LAYER)
	{
		for(i=0; i<VECTOR_BRUSH_TABLE_HEIGHT; i++)
		{
			for(j=0; j<VECTOR_BRUSH_TABLE_WIDTH; j++)
			{
				if(GTK_IS_TOGGLE_BUTTON(app->tool_window.vector_brushes[i][j].button) != FALSE)
				{
					gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(app->tool_window.vector_brushes[i][j].button), FALSE);
				}
			}
		}
	}
}

void ChangeCommonToolPreference(APPLICATION* app)
{
	// 設定ダイアログ
	GtkWidget *dialog =
		gtk_dialog_new_with_buttons(
			app->labels->tool_box.preference,
			GTK_WINDOW(app->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			NULL
		);
	// ウィジェット整列用
	GtkWidget *table = gtk_table_new(2, 2, FALSE);
	// ショートカットキー設定用
	GtkWidget *hot_key;
	// ラベル
	GtkWidget *label;
	// 表示文字列
	char str[256];
	// 設定を変更するブラシ
	COMMON_TOOL_CORE *target = (COMMON_TOOL_CORE*)app->tool_window.copy_brush;
	int x, y;
	(void)sprintf(str, "%s : ", app->labels->window.hot_ley);
	label = gtk_label_new(str);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
		GTK_FILL, GTK_FILL, 0, 0);
	hot_key = gtk_entry_new();
	gtk_entry_set_has_frame(GTK_ENTRY(hot_key), FALSE);
	gtk_entry_set_max_length(GTK_ENTRY(hot_key), 2);
	gtk_entry_set_width_chars(GTK_ENTRY(hot_key), 2);
	str[0] = target->hot_key, str[1] = 0;
	gtk_entry_set_text(GTK_ENTRY(hot_key), str);
	gtk_table_attach(GTK_TABLE(table), hot_key, 1, 2, 0, 1,
		GTK_EXPAND, GTK_EXPAND, 0, 0);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table, FALSE, TRUE, 0);
	gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		const gchar *key_val;
		char set_key;

		key_val = gtk_entry_get_text(GTK_ENTRY(hot_key));
		if(key_val != NULL && isalnum(*key_val) != 0)
		{
			int add_cancel_flag = 0;
			set_key = (gchar)toupper(key_val[0]);

			for(y=0; y<COMMON_TOOL_TABLE_HEIGHT && add_cancel_flag == 0; y++)
			{
				for(x=0; x<COMMON_TOOL_TABLE_WIDTH && add_cancel_flag == 0; x++)
				{
					if(app->tool_window.common_tools[y][x].hot_key == set_key)
					{
						add_cancel_flag++;
					}
				}
			}

			for(y=0; y<BRUSH_TABLE_HEIGHT && add_cancel_flag == 0; y++)
			{
				for(x=0; x<BRUSH_TABLE_WIDTH && add_cancel_flag == 0; x++)
				{
					if(app->tool_window.brushes[y][x].hot_key == set_key)
					{
						add_cancel_flag++;
					}
				}
			}

			for(y=0; y<VECTOR_BRUSH_TABLE_HEIGHT && add_cancel_flag == 0; y++)
			{
				for(x=0; x<VECTOR_BRUSH_TABLE_WIDTH && add_cancel_flag == 0; x++)
				{
					if(app->tool_window.vector_brushes[y][x].hot_key == set_key)
					{
						add_cancel_flag++;
					}
				}
			}

			if(add_cancel_flag == 0)
			{
				target->hot_key = toupper(set_key);
			}
			else
			{
				GtkWidget *message = gtk_message_dialog_new(
					GTK_WINDOW(app->window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING,
						GTK_BUTTONS_CLOSE, app->labels->preference.conflict_hot_key
				);
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			}
		}
		else
		{
			target->hot_key = '\0';
		}
	}

	gtk_widget_destroy(dialog);
}

gboolean CommonToolButtonRightClicked(GtkWidget* button, GdkEventButton* event_info, COMMON_TOOL_CORE* core)
{
	if(event_info->button == 3)
	{
		APPLICATION* app = g_object_get_data(G_OBJECT(button), "application");
		GtkWidget *menu = gtk_menu_new();
		GtkWidget *menu_item;

		app->tool_window.copy_brush = (void*)core;

		menu_item = gtk_menu_item_new_with_label(app->labels->tool_box.preference);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ChangeCommonToolPreference), app);

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(app->labels->window.cancel);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event_info->button, event_info->time);

		gtk_widget_show_all(menu);
	}

	return FALSE;
}

void BrushButtonClicked(GtkWidget* widget, gpointer data)
{
	BRUSH_CORE* core = (BRUSH_CORE*)data;
	APPLICATION* app = (APPLICATION*)g_object_get_data(G_OBJECT(widget), "application");
	int i, j;

	if(app->window_num > 0)
	{
		DRAW_WINDOW *window = app->draw_window[app->active_window];
		int do_release_func = 0;

		window->flags |= DRAW_WINDOW_TOOL_CHANGING;

		if((app->tool_window.flags & TOOL_USING_BRUSH) == 0)
		{
			if((app->tool_window.active_common_tool->flags & COMMON_TOOL_DO_RELEASE_FUNC_AT_CHANGI_TOOL) != 0)
			{
				do_release_func++;
			}
		}

		if((window->state & GDK_BUTTON1_MASK) != 0 || do_release_func != 0)
		{
			GdkEventButton event_info = {0};
			gdouble x, y;
			gdouble x0, y0;
			const gdouble pressure = 0;

			x0 = (window->before_cursor_x - window->half_size) * window->cos_value
				- (window->before_cursor_y - window->half_size) * window->sin_value + window->add_cursor_x;
			y0 = (window->before_cursor_x - window->half_size) * window->sin_value
				+ (window->before_cursor_y - window->half_size) * window->cos_value + window->add_cursor_y;
			x = window->rev_zoom * x0;
			y = window->rev_zoom * y0;

			event_info.button = 1;
			event_info.state = window->state;

			if((window->app->tool_window.flags & TOOL_USING_BRUSH) == 0)
			{
				window->app->tool_window.active_common_tool->release_func(
					window, x, y,
					window->app->tool_window.active_common_tool, (void*)&event_info
				);
			}
			else
			{
				if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
					|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
				{
					window->app->tool_window.active_brush[window->app->input]->release_func(
						window, x, y, pressure,
						window->app->tool_window.active_brush[window->app->input], (void*)&event_info
					);
				}
				else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
				{
					if((event_info.state & GDK_SHIFT_MASK) != 0)
					{
						window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
						window->app->tool_window.vector_control_core.release_func(
							window, x, y,
							pressure, &window->app->tool_window.vector_control_core, (void*)&event_info
						);
					}
					else if((event_info.state & GDK_CONTROL_MASK) != 0 ||
						(window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
					{
						window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
						window->app->tool_window.vector_control_core.release_func(
							window, x,y,
							pressure, &window->app->tool_window.vector_control_core, (void*)&event_info
						);
					}
					else
					{
						window->app->tool_window.active_vector_brush[window->app->input]->release_func(
							window, x, y,
							pressure, window->app->tool_window.active_vector_brush[window->app->input], (void*)&event_info
						);
					}
				}
			}

			window->state &= ~(GDK_BUTTON1_MASK);
		}

		window->flags &= ~(DRAW_WINDOW_TOOL_CHANGING);
	}

	if(app->tool_window.active_brush[app->input] == core
		&& (app->tool_window.flags & TOOL_USING_BRUSH) != 0)
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
		}
		return;
	}
	else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		return;
	}

	app->tool_window.flags |= TOOL_USING_BRUSH;
	if(app->tool_window.detail_ui != NULL)
	{
		gtk_widget_destroy(app->tool_window.detail_ui);
	}
	app->tool_window.active_brush[app->input] = core;
	app->tool_window.detail_ui = core->create_detail_ui(app, core);
	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(app->tool_window.detail_ui_scroll), app->tool_window.detail_ui);
	gtk_widget_show_all(app->tool_window.detail_ui);

#if GTK_MAJOR_VERSION >= 3
	for(i=0; i<MAX_TOUCH; i++)
	{
		if(app->tool_window.touch[i].brush_data != NULL)
		{
			(void)memcpy(app->tool_window.touch[i].brush_data,
				app->tool_window.active_brush[INPUT_PEN]->brush_data,
				app->tool_window.active_brush[INPUT_PEN]->detail_data_size
			);
		}
	}
#endif

	for(i=0; i<BRUSH_TABLE_HEIGHT; i++)
	{
		for(j=0; j<BRUSH_TABLE_WIDTH; j++)
		{
			if(GTK_IS_TOGGLE_BUTTON(app->tool_window.brushes[i][j].button) != FALSE)
			{
				if(&app->tool_window.brushes[i][j] == core)
				{
					if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
						app->tool_window.brushes[i][j].button)) == FALSE)
					{
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
							app->tool_window.brushes[i][j].button), TRUE);
					}
				}
				else
				{
					if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
						app->tool_window.brushes[i][j].button)) != FALSE)
					{
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
							app->tool_window.brushes[i][j].button), FALSE);
					}
				}
			}
		}
	}

	for(i=0; i<COMMON_TOOL_TABLE_HEIGHT; i++)
	{
		for(j=0; j<COMMON_TOOL_TABLE_WIDTH; j++)
		{
			if(app->tool_window.common_tools[i][j].tool_type == TYPE_LOUPE_TOOL)
			{
				continue;
			}
			gtk_toggle_button_set_active(
				GTK_TOGGLE_BUTTON(app->tool_window.common_tools[i][j].button), FALSE);
		}
	}
}

static void ButtonImageFileSelectionChange(GtkWidget* combo, GtkWidget* image)
{
	gchar *directory_path = (gchar*)g_object_get_data(G_OBJECT(combo), "directory_path");
#if GTK_MAJOR_VERSION <= 2
	gchar *file_path = g_build_filename(directory_path,
		gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo)), NULL);
#else
	gchar *file_path = g_build_filename(directory_path,
		gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo)), NULL);
#endif
	gtk_image_set_from_file(GTK_IMAGE(image), file_path);
	gtk_widget_queue_draw(image);
	g_free(file_path);
}

gboolean BrushButtonRightClicked(GtkWidget* button, GdkEventButton* event_info, BRUSH_CORE* core);

void BrushBlankButtonCallBack(GtkWidget* button, BRUSH_CORE* core)
{
	APPLICATION *app = (APPLICATION*)g_object_get_data(G_OBJECT(button), "application");
	GtkWidget *dialog;
	GtkWidget *box;
	GtkWidget *label;
	GtkWidget *brush_type;
	GtkWidget *brush_name = gtk_text_view_new();
	GtkTextBuffer *name_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(brush_name));
	GtkWidget *button_image_file;
	GtkWidget *button_image;
	GDir *dir;
	GFile *fp;
	GFileInputStream *stream;
	gchar *image_directory_path;
	gchar *text;
	char *file_path;
	char *system_path;
	const char *file_name;
	guint32 width, height, stride;
	int i;

	dialog = gtk_dialog_new_with_buttons(
		app->labels->make_new.title,
		GTK_WINDOW(app->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
		NULL
	);

	box = gtk_hbox_new(FALSE, 0);
	text = g_strdup_printf("%s:", app->labels->unit.name);
	label = gtk_label_new(text);
	g_free(text);

	gtk_text_buffer_set_text(name_buffer, app->labels->tool_box.new_brush, -1);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), brush_name, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		box, FALSE, FALSE, 2);

#if MAJOR_VERSION == 1
	brush_type = gtk_combo_box_new_text();
	for(i=0; i<NUM_BRUSH_TYPE; i++)
	{
		gtk_combo_box_append_text(
			GTK_COMBO_BOX(brush_type), app->labels->tool_box.brush_default_names[i]);
	}
#else
	brush_type = gtk_combo_box_text_new();
	for(i=0; i<NUM_BRUSH_TYPE; i++)
	{
		gtk_combo_box_text_append_text(
			GTK_COMBO_BOX_TEXT(brush_type), app->labels->tool_box.brush_default_names[i]);
	}
#endif
	gtk_combo_box_set_active(GTK_COMBO_BOX(brush_type), 0);
	text = g_strdup_printf("%s:", app->labels->unit.type);
	label = gtk_label_new(text);
	g_free(text);
	box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), brush_type, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		box, FALSE, FALSE, 2);

	image_directory_path = g_build_filename(app->current_path, "image", NULL);

#if MAJOR_VERSION == 1
	button_image_file = gtk_combo_box_new_text();
#else
	button_image_file = gtk_combo_box_text_new();
#endif
	g_object_set_data(G_OBJECT(button_image_file), "directory_path", image_directory_path);
	dir = g_dir_open(image_directory_path, 0, NULL);
	while((file_name = g_dir_read_name(dir)) != NULL)
	{
		size_t name_length = strlen(file_name);
		if(name_length >= 4 && StringCompareIgnoreCase(&file_name[name_length-4], ".png") == 0)
		{
			file_path = g_build_filename(image_directory_path, file_name, NULL);
			if((fp = g_file_new_for_path(file_path)) != NULL)
			{
				if((stream = g_file_read(fp, NULL, NULL)) != NULL)
				{
					uint8 *pixels = ReadPNGStream(stream, (stream_func_t)FileRead,
						&width, &height, &stride);
					if(pixels != NULL)
					{
						if(width <= 32 && height <= 32)
						{
#if GTK_MAJOR_VERSION <= 2
							gtk_combo_box_append_text(GTK_COMBO_BOX(button_image_file), file_name);
#else
							gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(button_image_file), file_name);
#endif
						}

						MEM_FREE_FUNC(pixels);
					}
					g_object_unref(stream);
				}
				g_object_unref(fp);
			}
			g_free(file_path);
		}
	}
	g_dir_close(dir);

	gtk_combo_box_set_active(GTK_COMBO_BOX(button_image_file), 0);
#if GTK_MAJOR_VERSION <= 2
	file_path = g_build_filename(image_directory_path,
		gtk_combo_box_get_active_text(GTK_COMBO_BOX(button_image_file)), NULL);
#else
	file_path = g_build_filename(image_directory_path,
		gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(button_image_file)), NULL);
#endif
	button_image = gtk_image_new_from_file(file_path);
	g_free(file_path);
	(void)g_signal_connect(G_OBJECT(button_image_file), "changed",
		G_CALLBACK(ButtonImageFileSelectionChange), button_image);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		button_image_file, FALSE, FALSE, 2);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		button_image, FALSE, FALSE, 2);

	gtk_widget_show_all(dialog);
	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GtkWidget *initialize_dialog;
		GtkTextIter start, end;
		gchar *src;
		gchar *name;
		gchar *release;
		gchar str[4096] = {0};
		int x = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "table_x"));
		int y = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(button), "table_y"));
		int is_first_button = 0;

		if(GTK_IS_TOGGLE_BUTTON(button) == FALSE)
		{
			is_first_button = 1;
			x = y = 0;
		}

		gtk_text_buffer_get_start_iter(name_buffer, &start);
		gtk_text_buffer_get_end_iter(name_buffer, &end);
		release = src = name = g_strdup(gtk_text_buffer_get_text(name_buffer, &start, &end, TRUE));
		while(*name != '\0')
		{
			if(*name == '\n')
			{
				*name = '\0';
				(void)strcat(str, src);
				(void)strcat(str, "\\n");
				name++;
				src = name;
			}

			name = g_utf8_next_char(name);
		}
		(void)strcat(str,src);
		g_free(release);
		app->tool_window.brushes[y][x].name = g_strdup(str);

		if(gtk_combo_box_get_active(GTK_COMBO_BOX(brush_type)) == BRUSH_TYPE_SCRIPT_BRUSH)
		{
			gchar *dir_path = g_build_filename(app->current_path, "brushes", NULL);
#if GTK_MAJOR_VERSION <= 2
			GtkWidget *combo = gtk_combo_box_new_text();
#else
			GtkWidget *combo = gtk_combo_box_text_new();
#endif

			LoadBrushDefaultData(core, gtk_combo_box_get_active(
				GTK_COMBO_BOX(brush_type)), app);

			initialize_dialog = gtk_dialog_new_with_buttons(
				app->labels->tool_box.initialize,
				GTK_WINDOW(dialog),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
				GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				NULL
			);
			box = gtk_hbox_new(FALSE, 0);
			(void)sprintf(str, "%s:", app->labels->menu.file);
			label = gtk_label_new(str);
			gtk_box_pack_start(GTK_BOX(box), label, FALSE, FALSE, 0);
			gtk_box_pack_start(GTK_BOX(box), combo, TRUE, TRUE, 0);
			gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(initialize_dialog))),
				box, FALSE, FALSE, 0);

			dir = g_dir_open(dir_path, 0, NULL);
			while((file_name = g_dir_read_name(dir)) != NULL)
			{
				size_t name_length = strlen(file_name);
				if(name_length >= 4 && StringCompareIgnoreCase(&file_name[name_length-4], ".lua") == 0)
				{
#if GTK_MAJOR_VERSION <= 2
					gtk_combo_box_append_text(GTK_COMBO_BOX(combo), file_name);
#else
					gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(combo), file_name);
#endif

				}
			}
			g_dir_close(dir);
			g_free(dir_path);

			gtk_widget_show_all(initialize_dialog);
			if(gtk_dialog_run(GTK_DIALOG(initialize_dialog)) == GTK_RESPONSE_ACCEPT)
			{
				SCRIPT *script;
#if GTK_MAJOR_VERSION <= 2
				file_path = g_build_filename(app->current_path, "brushes",
					gtk_combo_box_get_active_text(GTK_COMBO_BOX(combo)), NULL);
#else
				file_path = g_build_filename(app->current_path, "brushes",
					gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(combo)), NULL);
#endif

				script = CreateScript(app, file_path);
				if(script != NULL)
				{
					core->brush_data = (void*)script;
					core->detail_data_size = sizeof(*script);
					script->brush_data = (SCRIPT_BRUSH*)MEM_ALLOC_FUNC(sizeof(*script->brush_data));
					(void)memset(script->brush_data, 0, sizeof(*script->brush_data));

					script->brush_data->zoom = 1;
					script->brush_data->r = 1;

					if(is_first_button != 0)
					{
						gtk_widget_destroy(button);
					}
					else
					{
						gtk_widget_destroy(app->tool_window.brushes[0][0].button);
					}
#if GTK_MAJOR_VERSION <= 2
					(void)sprintf(str, "./image/%s",
						gtk_combo_box_get_active_text(GTK_COMBO_BOX(button_image_file)));
#else
					(void)sprintf(str, "./image/%s",
						gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(button_image_file)));
#endif
					core->image_file_path = MEM_STRDUP_FUNC(str);
#if GTK_MAJOR_VERSION <= 2
					file_path = g_build_filename(app->current_path, "image",
						gtk_combo_box_get_active_text(GTK_COMBO_BOX(button_image_file)), NULL);
#else
					file_path = g_build_filename(app->current_path, "image",
						gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(button_image_file)), NULL);
#endif
					system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
					core->button = CreateImageButton(system_path, core->name, app->tool_window.font_file);
					g_object_set_data(G_OBJECT(core->button), "application", app);
					(void)g_signal_connect(G_OBJECT(core->button), "clicked", G_CALLBACK(BrushButtonClicked), core);
					(void)g_signal_connect(G_OBJECT(core->button), "button_press_event",
						G_CALLBACK(BrushButtonRightClicked), core);
					gtk_widget_add_events(core->button, GDK_BUTTON_PRESS_MASK);
					gtk_table_attach(GTK_TABLE(app->tool_window.brush_table), core->button,
						x, x+1, y, y+1, GTK_FILL, GTK_FILL, 0, 0);
					gtk_widget_show_all(core->button);

				}
				g_free(file_path);
				g_free(system_path);
			}
		}
		else
		{
			LoadBrushDefaultData(core, gtk_combo_box_get_active(
				GTK_COMBO_BOX(brush_type)), app);
			initialize_dialog = gtk_dialog_new_with_buttons(
				app->labels->tool_box.initialize,
				GTK_WINDOW(dialog),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_OK, GTK_RESPONSE_ACCEPT, NULL
			);
			gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(initialize_dialog))),
				core->create_detail_ui(app, core), FALSE, FALSE, 0);
			gtk_widget_show_all(initialize_dialog);
			(void)gtk_dialog_run(GTK_DIALOG(initialize_dialog));
			gtk_widget_destroy(initialize_dialog);

			if(is_first_button == 0)
			{
				gtk_widget_destroy(button);
			}
			else
			{
				gtk_widget_destroy(app->tool_window.brushes[0][0].button);
			}
#if GTK_MAJOR_VERSION <= 2
			(void)sprintf(str, "./image/%s",
				gtk_combo_box_get_active_text(GTK_COMBO_BOX(button_image_file)));
#else
			(void)sprintf(str, "./image/%s",
				gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(button_image_file)));
#endif
			core->image_file_path = MEM_STRDUP_FUNC(str);
#if GTK_MAJOR_VERSION <= 2
			file_path = g_build_filename(app->current_path, "image",
				gtk_combo_box_get_active_text(GTK_COMBO_BOX(button_image_file)), NULL);
#else
			file_path = g_build_filename(app->current_path, "image",
				gtk_combo_box_text_get_active_text(GTK_COMBO_BOX(button_image_file)), NULL);
#endif
			system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
			core->button = CreateImageButton(system_path, core->name, app->tool_window.font_file);
			g_object_set_data(G_OBJECT(core->button), "application", app);
			(void)g_signal_connect(G_OBJECT(core->button), "clicked", G_CALLBACK(BrushButtonClicked), core);
			(void)g_signal_connect(G_OBJECT(core->button), "button_press_event",
				G_CALLBACK(BrushButtonRightClicked), core);
			gtk_widget_add_events(core->button, GDK_BUTTON_PRESS_MASK);
			gtk_table_attach(GTK_TABLE(app->tool_window.brush_table), core->button,
				x, x+1, y, y+1, GTK_FILL, GTK_FILL, 0, 0);
			gtk_widget_show_all(core->button);

			g_free(file_path);
			g_free(system_path);
		}
	}

	g_free(image_directory_path);
	gtk_widget_destroy(dialog);
}

void CopyBrushData(APPLICATION* app)
{
	BRUSH_CORE *target = NULL;
	int target_x, target_y;
	int x, y;

	for(y=0; y<BRUSH_TABLE_HEIGHT && target == NULL; y++)
	{
		for(x=0; x<BRUSH_TABLE_WIDTH; x++)
		{
			if(app->tool_window.brushes[y][x].name == NULL)
			{
				target = &app->tool_window.brushes[y][x];
				target_x = x, target_y = y;
				break;
			}
		}
	}

	if(target != NULL)
	{
		BRUSH_CORE *src = (BRUSH_CORE*)app->tool_window.copy_brush;
		gchar *file_path;

		target->name = g_strdup(src->name);
		target->image_file_path = MEM_STRDUP_FUNC(src->image_file_path);

		gtk_widget_destroy(target->button);
		if(target->image_file_path[0] == '.' && target->image_file_path[1] == '/')
		{
			gchar *temp_path = g_build_filename(app->current_path,
				&target->image_file_path[2], NULL);
			file_path = g_convert(temp_path, -1, app->system_code, "UTF-8", NULL, NULL, NULL);
			g_free(temp_path);
		}
		else
		{
			file_path = g_strdup(target->image_file_path);
		}
		target->button = CreateImageButton(file_path, target->name, app->tool_window.font_file);
		g_object_set_data(G_OBJECT(target->button), "application", app);
		g_free(file_path);

		target->brush_type = src->brush_type;
		target->press_func = src->press_func;
		target->motion_func = src->motion_func;
		target->release_func = src->release_func;
		target->draw_cursor = src->draw_cursor;
		target->color_change = src->color_change;
		target->create_detail_ui = src->create_detail_ui;

		target->brush_data = MEM_ALLOC_FUNC(src->detail_data_size);
		(void)memcpy(target->brush_data, src->brush_data, src->detail_data_size);
		target->detail_data_size = src->detail_data_size;

		(void)g_signal_connect(G_OBJECT(target->button), "clicked", G_CALLBACK(BrushButtonClicked), target);
		(void)g_signal_connect(G_OBJECT(target->button), "button_press_event",
			G_CALLBACK(BrushButtonRightClicked), target);
		gtk_widget_add_events(target->button, GDK_BUTTON_PRESS_MASK);

		gtk_table_attach(GTK_TABLE(app->tool_window.brush_table), target->button,
			target_x, target_x+1, target_y, target_y+1, GTK_FILL, GTK_FILL, 0, 0);

		gtk_widget_show_all(target->button);
	}
}

void ChangeBrushPreference(APPLICATION* app)
{
	// 設定ダイアログ
	GtkWidget *dialog =
		gtk_dialog_new_with_buttons(
			app->labels->tool_box.preference,
			GTK_WINDOW(app->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			NULL
		);
	// ウィジェット整列用
	GtkWidget *table = gtk_table_new(3, 2, FALSE);
	// ブラシの名前設定用
	GtkWidget *text;
	GtkWidget *separator;
	GtkTextBuffer *buffer;
	// ショートカットキー設定用
	GtkWidget *hot_key;
	// ラベル
	GtkWidget *label;
	// 表示文字列
	char str[256];
	// 設定を変更するブラシ
	BRUSH_CORE *target = (BRUSH_CORE*)app->tool_window.copy_brush;
	// 表示する名前
	char *disp_str = g_strdup(target->name);
	char *now = disp_str, *next;
	int length = (int)strlen(disp_str);
	int x, y;
	int i;

	next = g_utf8_next_char(now);
	while(*next != '\0')
	{
		if((uint8)*now == 0x5c && *next == 'n')
		{
			*now = '\n';
			(void)memmove(next, next+1, strlen(next));
		}
		else if((uint8)*now == 0xc2 && (uint8)(*(now+1)) == 0xa5 && *next == 'n')
		{
			*now = '\n';
			(void)memmove(now+1, next+1, strlen(next));
		}

		now = next;
		next = g_utf8_next_char(next);
	}

	label = gtk_label_new("");
	(void)sprintf(str, "%s : ", app->labels->tool_box.name);
	gtk_label_set_markup(GTK_LABEL(label), str);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
		GTK_FILL, GTK_FILL, 0, 0);
	text = gtk_text_view_new();
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	gtk_text_buffer_set_text(buffer, disp_str, -1);
	gtk_table_attach(GTK_TABLE(table), text, 1, 2, 0, 1,
		GTK_EXPAND, GTK_EXPAND, 0, 0);
	g_free(disp_str);

	separator = gtk_hseparator_new();
	gtk_table_attach(GTK_TABLE(table), separator, 1, 3, 1, 2,
		GTK_FILL, GTK_FILL, 0, 0);

	(void)sprintf(str, "%s : ", app->labels->window.hot_ley);
	label = gtk_label_new(str);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
		GTK_FILL, GTK_FILL, 0, 0);
	hot_key = gtk_entry_new();
	gtk_entry_set_has_frame(GTK_ENTRY(hot_key), FALSE);
	gtk_entry_set_max_length(GTK_ENTRY(hot_key), 2);
	gtk_entry_set_width_chars(GTK_ENTRY(hot_key), 2);
	str[0] = target->hot_key, str[1] = 0;
	gtk_entry_set_text(GTK_ENTRY(hot_key), str);
	gtk_table_attach(GTK_TABLE(table), hot_key, 1, 2, 2, 3,
		GTK_EXPAND, GTK_EXPAND, 0, 0);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		table, FALSE, TRUE, 0);
	gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GtkTextIter start, end;
		gchar *name;
		const gchar *key_val;
		char set_key;

		gtk_text_buffer_get_start_iter(buffer, &start);
		gtk_text_buffer_get_end_iter(buffer, &end);
		name = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

		if(name != NULL && *name != '\0')
		{
			gchar *file_path;
			int target_x, target_y = -1;
			gboolean before_state = gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(target->button));

			g_free(target->name);
			length = (int)strlen(name);
			disp_str = (char*)MEM_ALLOC_FUNC(512);
			(void)strcpy(disp_str, name);

			for(i=0; i<length; i++)
			{
				if(disp_str[i] == '\n')
				{
					(void)memmove(&disp_str[i+2], &disp_str[i+1], length-i);
					disp_str[i] = 0x5c, disp_str[i+1] = 'n';
					length++, i++;
				}
			}

			target->name = g_strdup(disp_str);
			MEM_FREE_FUNC(disp_str);

			for(y=0; y<BRUSH_TABLE_HEIGHT && target_y < 0; y++)
			{
				for(x=0; x<BRUSH_TABLE_WIDTH; x++)
				{
					if(&app->tool_window.brushes[y][x] == target)
					{
						target_x = x, target_y = y;
						break;
					}
				}
			}

			gtk_widget_destroy(target->button);
			if(target->image_file_path[0] == '.' && target->image_file_path[1] == '/')
			{
				gchar *temp_path = g_build_filename(app->current_path,
					&target->image_file_path[2], NULL);
				file_path = g_convert(temp_path, -1, app->system_code, "UTF-8", NULL, NULL, NULL);
				g_free(temp_path);
			}
			else
			{
				file_path = g_strdup(target->image_file_path);
			}
			target->button = CreateImageButton(file_path, target->name, app->tool_window.font_file);
			g_free(file_path);
			g_object_set_data(G_OBJECT(target->button), "application", app);

			(void)g_signal_connect(G_OBJECT(target->button), "clicked", G_CALLBACK(BrushButtonClicked), target);
			(void)g_signal_connect(G_OBJECT(target->button), "button_press_event",
				G_CALLBACK(BrushButtonRightClicked), target);
			gtk_widget_add_events(target->button, GDK_BUTTON_PRESS_MASK);

			gtk_table_attach(GTK_TABLE(app->tool_window.brush_table), target->button,
				target_x, target_x+1, target_y, target_y+1, GTK_FILL, GTK_FILL, 0, 0);

			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(target->button), before_state);

			gtk_widget_show_all(target->button);
		}

		key_val = gtk_entry_get_text(GTK_ENTRY(hot_key));
		if(key_val != NULL && isalnum(*key_val) != 0)
		{
			int add_cancel_flag = 0;
			set_key = (gchar)toupper(key_val[0]);

			for(y=0; y<COMMON_TOOL_TABLE_HEIGHT && add_cancel_flag == 0; y++)
			{
				for(x=0; x<COMMON_TOOL_TABLE_WIDTH && add_cancel_flag == 0; x++)
				{
					if(app->tool_window.common_tools[y][x].hot_key == set_key)
					{
						add_cancel_flag++;
					}
				}
			}

			for(y=0; y<BRUSH_TABLE_HEIGHT && add_cancel_flag == 0; y++)
			{
				for(x=0; x<BRUSH_TABLE_WIDTH && add_cancel_flag == 0; x++)
				{
					if(app->tool_window.brushes[y][x].hot_key == set_key
						&& &app->tool_window.brushes[y][x] != target)
					{
						add_cancel_flag++;
					}
				}
			}

			if(add_cancel_flag == 0)
			{
				target->hot_key = toupper(set_key);
			}
			else
			{
				GtkWidget *message = gtk_message_dialog_new(
					GTK_WINDOW(app->window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING,
						GTK_BUTTONS_CLOSE, app->labels->preference.conflict_hot_key
				);
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			}
		}
		else
		{
			target->hot_key = '\0';
		}
	}

	gtk_widget_destroy(dialog);
}

void DeleteBrushData(APPLICATION* app)
{
	BRUSH_CORE *target = (BRUSH_CORE*)app->tool_window.copy_brush;
	int x, y;
	int target_x, target_y = -1;
	g_free(target->name);
	target->name = NULL;
	MEM_FREE_FUNC(target->image_file_path);
	target->image_file_path = NULL;
	MEM_FREE_FUNC(target->brush_data);
	target->brush_data = NULL;

	for(y=0; y<BRUSH_TABLE_HEIGHT && target_y < 0; y++)
	{
		for(x=0; x<BRUSH_TABLE_WIDTH; x++)
		{
			if(&app->tool_window.brushes[y][x] == target)
			{
				target_x = x, target_y = y;
				break;
			}
		}
	}

	gtk_widget_destroy(target->button);
	(void)memset(target, 0, sizeof(*target));
	target->button = gtk_button_new();
	g_signal_connect(G_OBJECT(target->button), "clicked",
		G_CALLBACK(BrushBlankButtonCallBack), target);
	g_object_set_data(G_OBJECT(target->button), "application", app);
	g_object_set_data(G_OBJECT(target->button), "table_x", GINT_TO_POINTER(target_x));
	g_object_set_data(G_OBJECT(target->button), "table_y", GINT_TO_POINTER(target_y));

	gtk_table_attach(GTK_TABLE(app->tool_window.brush_table), target->button,
		target_x, target_x+1, target_y, target_y+1, GTK_FILL, GTK_FILL, 0, 0);

	gtk_widget_show_all(target->button);
}

gboolean BrushButtonRightClicked(GtkWidget* button, GdkEventButton* event_info, BRUSH_CORE* core)
{
#if MAJOR_VERSION == 1
	if(event_info->device->source == GDK_SOURCE_PEN)
	{
		core->app->input = INPUT_PEN;
	}
	else if(event_info->device->source == GDK_SOURCE_ERASER)
	{
		core->app->input = INPUT_ERASER;
	}
#else
	if(gdk_device_get_source(event_info->device) == GDK_SOURCE_PEN)
	{
		core->app->input = INPUT_PEN;
	}
	else if(gdk_device_get_source(event_info->device) == GDK_SOURCE_ERASER)
	{
		core->app->input = INPUT_ERASER;
	}
#endif

	if(event_info->button == 3)
	{
		APPLICATION* app = g_object_get_data(G_OBJECT(button), "application");
		GtkWidget *menu = gtk_menu_new();
		GtkWidget *menu_item;

		app->tool_window.copy_brush = (void*)core;
		menu_item = gtk_menu_item_new_with_label(app->labels->tool_box.copy_brush);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(CopyBrushData), app);

		if(core == app->tool_window.brushes[0])
		{
			menu_item = gtk_menu_item_new_with_label(app->labels->tool_box.change_brush);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
			(void)g_signal_connect(G_OBJECT(menu_item), "activate",
				G_CALLBACK(BrushBlankButtonCallBack), core);
			g_object_set_data(G_OBJECT(menu_item), "application", app);
		}
		else
		{
			menu_item = gtk_menu_item_new_with_label(app->labels->tool_box.delete_brush);
			gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
			(void)g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
				G_CALLBACK(DeleteBrushData), app);
		}

		menu_item = gtk_menu_item_new_with_label(app->labels->tool_box.preference);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ChangeBrushPreference), app);

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(app->labels->window.cancel);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event_info->button, event_info->time);

		gtk_widget_show_all(menu);
	}

	return FALSE;
}

gboolean VectorBrushButtonRightClicked(GtkWidget* button, GdkEventButton* event_info, VECTOR_BRUSH_CORE* core);
void VectorBrushButtonClicked(GtkWidget* widget, gpointer data);

void CopyVectorBrushData(APPLICATION* app)
{
	VECTOR_BRUSH_CORE *target = NULL;
	int target_x, target_y;
	int x, y;

	for(y=0; y<VECTOR_BRUSH_TABLE_HEIGHT && target == NULL; y++)
	{
		for(x=0; x<VECTOR_BRUSH_TABLE_WIDTH; x++)
		{
			if(app->tool_window.vector_brushes[y][x].name == NULL)
			{
				target = &app->tool_window.vector_brushes[y][x];
				target_x = x, target_y = y;
				break;
			}
		}
	}

	if(target != NULL)
	{
		VECTOR_BRUSH_CORE *src = (VECTOR_BRUSH_CORE*)app->tool_window.copy_brush;
		gchar *file_path;

		target->name = g_strdup(src->name);
		target->image_file_path = MEM_STRDUP_FUNC(src->image_file_path);

		gtk_widget_destroy(target->button);
		if(target->image_file_path[0] == '.' && target->image_file_path[1] == '/')
		{
			gchar *temp_path = g_build_filename(app->current_path,
				&target->image_file_path[2], NULL);
			file_path = g_convert(temp_path, -1, app->system_code, "UTF-8", NULL, NULL, NULL);
			g_free(temp_path);
		}
		else
		{
			file_path = g_strdup(target->image_file_path);
		}
		target->button = CreateImageButton(file_path, target->name, app->tool_window.font_file);
		g_object_set_data(G_OBJECT(target->button), "application", app);
		g_free(file_path);

		target->brush_type = src->brush_type;
		target->press_func = src->press_func;
		target->motion_func = src->motion_func;
		target->release_func = src->release_func;
		target->draw_cursor = src->draw_cursor;
		target->color_change = src->color_change;
		target->create_detail_ui = src->create_detail_ui;

		target->brush_data = MEM_ALLOC_FUNC(src->detail_data_size);
		(void)memcpy(target->brush_data, src->brush_data, src->detail_data_size);
		target->detail_data_size = src->detail_data_size;

		g_signal_connect(G_OBJECT(target->button), "clicked", G_CALLBACK(VectorBrushButtonClicked), target);
		g_signal_connect(G_OBJECT(target->button), "button_press_event",
			G_CALLBACK(VectorBrushButtonRightClicked), target);
		gtk_widget_add_events(target->button, GDK_BUTTON_PRESS_MASK);

		gtk_table_attach(GTK_TABLE(app->tool_window.brush_table), target->button,
			target_x, target_x+1, target_y, target_y+1, GTK_FILL, GTK_FILL, 0, 0);

		gtk_widget_show_all(target->button);
	}
}

void ChangeVectorBrushPreference(APPLICATION* app)
{
	// 設定ダイアログ
	GtkWidget *dialog =
		gtk_dialog_new_with_buttons(
			app->labels->tool_box.preference,
			GTK_WINDOW(app->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			NULL
		);
	// ウィジェット整列用
	GtkWidget *table = gtk_table_new(3, 2, FALSE);
	// ブラシの名前設定用
	GtkWidget *text;
	GtkWidget *separator;
	GtkTextBuffer *buffer;
	// ショートカットキー設定用
	GtkWidget *hot_key;
	// ラベル
	GtkWidget *label;
	// 表示文字列
	char str[256];
	// 設定を変更するブラシ
	VECTOR_BRUSH_CORE *target = (VECTOR_BRUSH_CORE*)app->tool_window.copy_brush;
	// 表示する名前
	char *disp_str = g_strdup(target->name);
	char *now = disp_str, *next;
	int length = (int)strlen(disp_str);
	int x, y;
	int i;

	next = g_utf8_next_char(now);
	while(*next != '\0')
	{
		if((uint8)*now == 0x5c && *next == 'n')
		{
			*now = '\n';
			(void)memmove(next, next+1, strlen(next));
		}
		else if((uint8)*now == 0xc2 && (uint8)(*(now+1)) == 0xa5 && *next == 'n')
		{
			*now = '\n';
			(void)memmove(now+1, next+1, strlen(next));
		}

		now = next;
		next = g_utf8_next_char(next);
	}

	label = gtk_label_new("");
	(void)sprintf(str, "%s : ", app->labels->tool_box.name);
	gtk_label_set_markup(GTK_LABEL(label), str);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 0, 1,
		GTK_FILL, GTK_FILL, 0, 0);
	text = gtk_text_view_new();
	buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
	gtk_text_buffer_set_text(buffer, disp_str, -1);
	gtk_table_attach(GTK_TABLE(table), text, 1, 2, 0, 1,
		GTK_EXPAND, GTK_EXPAND, 0, 0);
	g_free(disp_str);

	separator = gtk_hseparator_new();
	gtk_table_attach(GTK_TABLE(table), separator, 1, 3, 1, 2,
		GTK_FILL, GTK_FILL, 0, 0);

	(void)sprintf(str, "%s : ", app->labels->window.hot_ley);
	label = gtk_label_new(str);
	gtk_table_attach(GTK_TABLE(table), label, 0, 1, 2, 3,
		GTK_FILL, GTK_FILL, 0, 0);
	hot_key = gtk_entry_new();
	gtk_entry_set_has_frame(GTK_ENTRY(hot_key), FALSE);
	gtk_entry_set_max_length(GTK_ENTRY(hot_key), 2);
	gtk_entry_set_width_chars(GTK_ENTRY(hot_key), 2);
	str[0] = target->hot_key, str[1] = 0;
	gtk_entry_set_text(GTK_ENTRY(hot_key), str);
	gtk_table_attach(GTK_TABLE(table), hot_key, 1, 2, 2, 3,
		GTK_EXPAND, GTK_EXPAND, 0, 0);

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		table, FALSE, TRUE, 0);
	gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		GtkTextIter start, end;
		gchar *name;
		const gchar *key_val;
		char set_key;

		gtk_text_buffer_get_start_iter(buffer, &start);
		gtk_text_buffer_get_end_iter(buffer, &end);
		name = gtk_text_buffer_get_text(buffer, &start, &end, TRUE);

		if(name != NULL && *name != '\0')
		{
			gchar *file_path;
			int target_x, target_y = -1;
			gboolean before_state = gtk_toggle_button_get_active(
				GTK_TOGGLE_BUTTON(target->button));

			g_free(target->name);
			length = (int)strlen(name);
			disp_str = (char*)MEM_ALLOC_FUNC(512);
			(void)strcpy(disp_str, name);

			for(i=0; i<length; i++)
			{
				if(disp_str[i] == '\n')
				{
					(void)memmove(&disp_str[i+2], &disp_str[i+1], length-i);
					disp_str[i] = 0x5c, disp_str[i+1] = 'n';
					length++, i++;
				}
			}

			target->name = g_strdup(disp_str);
			MEM_FREE_FUNC(disp_str);

			for(y=0; y<VECTOR_BRUSH_TABLE_HEIGHT && target_y < 0; y++)
			{
				for(x=0; x<VECTOR_BRUSH_TABLE_WIDTH; x++)
				{
					if(&app->tool_window.vector_brushes[y][x] == target)
					{
						target_x = x, target_y = y;
						break;
					}
				}
			}

			gtk_widget_destroy(target->button);
			if(target->image_file_path[0] == '.' && target->image_file_path[1] == '/')
			{
				gchar *temp_path = g_build_filename(app->current_path,
					&target->image_file_path[2], NULL);
				file_path = g_convert(temp_path, -1, app->system_code, "UTF-8", NULL, NULL, NULL);
				g_free(temp_path);
			}
			else
			{
				file_path = g_strdup(target->image_file_path);
			}
			target->button = CreateImageButton(file_path, target->name, app->tool_window.font_file);
			g_free(file_path);
			g_object_set_data(G_OBJECT(target->button), "application", app);

			g_signal_connect(G_OBJECT(target->button), "clicked", G_CALLBACK(VectorBrushButtonClicked), target);
			g_signal_connect(G_OBJECT(target->button), "button_press_event",
				G_CALLBACK(VectorBrushButtonRightClicked), target);
			gtk_widget_add_events(target->button, GDK_BUTTON_PRESS_MASK);

			gtk_table_attach(GTK_TABLE(app->tool_window.brush_table), target->button,
				target_x, target_x+1, target_y, target_y+1, GTK_FILL, GTK_FILL, 0, 0);

			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(target->button), before_state);

			gtk_widget_show_all(target->button);
		}

		key_val = gtk_entry_get_text(GTK_ENTRY(hot_key));
		if(key_val != NULL && isalnum(*key_val) != 0)
		{
			int add_cancel_flag = 0;
			set_key = (gchar)toupper(key_val[0]);

			for(y=0; y<COMMON_TOOL_TABLE_HEIGHT && add_cancel_flag == 0; y++)
			{
				for(x=0; x<COMMON_TOOL_TABLE_WIDTH && add_cancel_flag == 0; x++)
				{
					if(app->tool_window.common_tools[y][x].hot_key == set_key)
					{
						add_cancel_flag++;
					}
				}
			}

			for(y=0; y<VECTOR_BRUSH_TABLE_HEIGHT && add_cancel_flag == 0; y++)
			{
				for(x=0; x<VECTOR_BRUSH_TABLE_WIDTH && add_cancel_flag == 0; x++)
				{
					if(app->tool_window.vector_brushes[y][x].hot_key == set_key
						&& &app->tool_window.vector_brushes[y][x] != target)
					{
						add_cancel_flag++;
					}
				}
			}

			if(add_cancel_flag == 0)
			{
				target->hot_key = toupper(set_key);
			}
			else
			{
				GtkWidget *message = gtk_message_dialog_new(
					GTK_WINDOW(app->window), GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_WARNING,
						GTK_BUTTONS_CLOSE, app->labels->preference.conflict_hot_key
				);
				gtk_dialog_run(GTK_DIALOG(message));
				gtk_widget_destroy(message);
			}
		}
		else
		{
			target->hot_key = '\0';
		}
	}

	gtk_widget_destroy(dialog);
}

void DeleteVectorBrushData(APPLICATION* app)
{
	VECTOR_BRUSH_CORE *target = (VECTOR_BRUSH_CORE*)app->tool_window.copy_brush;
	int x, y;
	int target_x, target_y = -1;
	g_free(target->name);
	target->name = NULL;
	MEM_FREE_FUNC(target->image_file_path);
	target->image_file_path = NULL;
	MEM_FREE_FUNC(target->brush_data);
	target->brush_data = NULL;

	for(y=0; y<VECTOR_BRUSH_TABLE_HEIGHT && target_y < 0; y++)
	{
		for(x=0; x<VECTOR_BRUSH_TABLE_WIDTH; x++)
		{
			if(&app->tool_window.vector_brushes[y][x] == target)
			{
				target_x = x, target_y = y;
				break;
			}
		}
	}

	gtk_widget_destroy(target->button);
	(void)memset(target, 0, sizeof(*target));
	target->button = gtk_toggle_button_new();

	gtk_table_attach(GTK_TABLE(app->tool_window.brush_table), target->button,
		target_x, target_x+1, target_y, target_y+1, GTK_FILL, GTK_FILL, 0, 0);

	gtk_widget_show_all(target->button);
}

gboolean VectorBrushButtonRightClicked(GtkWidget* button, GdkEventButton* event_info, VECTOR_BRUSH_CORE* core)
{
#if MAJOR_VERSION == 1
	if(event_info->device->source == GDK_SOURCE_PEN)
	{
		core->app->input = INPUT_PEN;
	}
	if(event_info->device->source == GDK_SOURCE_ERASER)
	{
		core->app->input = INPUT_ERASER;
	}
#else
	if(gdk_device_get_source(event_info->device) == GDK_SOURCE_PEN)
	{
		core->app->input = INPUT_PEN;
	}
	if(gdk_device_get_source(event_info->device) == GDK_SOURCE_ERASER)
	{
		core->app->input = INPUT_ERASER;
	}
#endif

	if(event_info->button == 3)
	{
		APPLICATION* app = g_object_get_data(G_OBJECT(button), "application");
		GtkWidget *menu = gtk_menu_new();
		GtkWidget *menu_item;

		app->tool_window.copy_brush = (void*)core;
		menu_item = gtk_menu_item_new_with_label(app->labels->tool_box.copy_brush);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(CopyVectorBrushData), app);

		menu_item = gtk_menu_item_new_with_label(app->labels->tool_box.delete_brush);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		if(core == app->tool_window.vector_brushes[0])
		{
			gtk_widget_set_sensitive(GTK_WIDGET(menu_item), FALSE);
		}
		else
		{
			g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
				G_CALLBACK(DeleteVectorBrushData), app);
		}

		menu_item = gtk_menu_item_new_with_label(app->labels->tool_box.preference);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);
		g_signal_connect_swapped(G_OBJECT(menu_item), "activate",
			G_CALLBACK(ChangeVectorBrushPreference), app);

		menu_item = gtk_separator_menu_item_new();
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		menu_item = gtk_menu_item_new_with_label(app->labels->window.cancel);
		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu_item);

		gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, event_info->button, event_info->time);

		gtk_widget_show_all(menu);
	}

	return FALSE;
}

void VectorBrushButtonClicked(GtkWidget* widget, gpointer data)
{
	VECTOR_BRUSH_CORE* core = (VECTOR_BRUSH_CORE*)data;
	APPLICATION* app = g_object_get_data(G_OBJECT(widget), "application");
	int i, j;

	if(app->window_num > 0)
	{
		DRAW_WINDOW *window = app->draw_window[app->active_window];
		int do_release_func = 0;

		window->flags |= DRAW_WINDOW_TOOL_CHANGING;

		if((app->tool_window.flags & TOOL_USING_BRUSH) == 0)
		{
			if((app->tool_window.active_common_tool->flags & COMMON_TOOL_DO_RELEASE_FUNC_AT_CHANGI_TOOL) != 0)
			{
				do_release_func++;
			}
		}

		if((window->state & GDK_BUTTON1_MASK) != 0 || do_release_func != 0)
		{
			GdkEventButton event_info = {0};
			gdouble x, y;
			gdouble x0, y0;
			const gdouble pressure = 0;

			x0 = (window->before_cursor_x - window->half_size) * window->cos_value
				- (window->before_cursor_y - window->half_size) * window->sin_value + window->add_cursor_x;
			y0 = (window->before_cursor_x - window->half_size) * window->sin_value
				+ (window->before_cursor_y - window->half_size) * window->cos_value + window->add_cursor_y;
			x = window->rev_zoom * x0;
			y = window->rev_zoom * y0;

			event_info.button = 1;
			event_info.state = window->state;

			if((window->app->tool_window.flags & TOOL_USING_BRUSH) == 0)
			{
				window->app->tool_window.active_common_tool->release_func(
					window, x, y,
					window->app->tool_window.active_common_tool, (void*)&event_info
				);
			}
			else
			{
				if(window->active_layer->layer_type == TYPE_NORMAL_LAYER
					|| (window->flags & DRAW_WINDOW_EDIT_SELECTION) != 0)
				{
					window->app->tool_window.active_brush[window->app->input]->release_func(
						window, x, y, pressure,
						window->app->tool_window.active_brush[window->app->input], (void*)&event_info
					);
				}
				else if(window->active_layer->layer_type == TYPE_VECTOR_LAYER)
				{
					if((event_info.state & GDK_SHIFT_MASK) != 0)
					{
						window->app->tool_window.vector_control.mode = CONTROL_POINT_DELETE;
						window->app->tool_window.vector_control_core.release_func(
							window, x, y,
							pressure, &window->app->tool_window.vector_control_core, (void*)&event_info
						);
					}
					else if((event_info.state & GDK_CONTROL_MASK) != 0 ||
						(window->app->tool_window.vector_control.flags & CONTROL_POINT_TOOL_HAS_POINT) != 0)
					{
						window->app->tool_window.vector_control.mode = CONTROL_POINT_MOVE;
						window->app->tool_window.vector_control_core.release_func(
							window, x,y,
							pressure, &window->app->tool_window.vector_control_core, (void*)&event_info
						);
					}
					else
					{
						window->app->tool_window.active_vector_brush[window->app->input]->release_func(
							window, x, y,
							pressure, window->app->tool_window.active_vector_brush[window->app->input], (void*)&event_info
						);
					}
				}
			}

			window->state &= ~(GDK_BUTTON1_MASK);
		}

		window->flags &= ~(DRAW_WINDOW_TOOL_CHANGING);
	}

	if(app->tool_window.active_vector_brush[app->input] == core
		&& (app->tool_window.flags & TOOL_USING_BRUSH) != 0)
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
		{
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
		}
		return;
	}
	else if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		return;
	}

	app->tool_window.flags |= TOOL_USING_BRUSH;
	gtk_widget_destroy(app->tool_window.detail_ui);
	app->tool_window.active_vector_brush[app->input] = core;
	app->tool_window.detail_ui = core->create_detail_ui(app, core->brush_data);
	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(app->tool_window.detail_ui_scroll), app->tool_window.detail_ui);
	gtk_widget_show_all(app->tool_window.detail_ui);

	for(i=0; i<VECTOR_BRUSH_TABLE_HEIGHT; i++)
	{
		for(j=0; j<VECTOR_BRUSH_TABLE_WIDTH; j++)
		{
			if(GTK_IS_TOGGLE_BUTTON(app->tool_window.vector_brushes[i][j].button) != FALSE)
			{
				if(&app->tool_window.vector_brushes[i][j] == core)
				{
					if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
						app->tool_window.vector_brushes[i][j].button)) == FALSE)
					{
							gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
							app->tool_window.vector_brushes[i][j].button), TRUE);
					}
				}
				else
				{
					if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(
						app->tool_window.vector_brushes[i][j].button)) != FALSE)
					{
						gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
							app->tool_window.vector_brushes[i][j].button), FALSE);
					}
				}
			}
		}
	}

	for(i=0; i<COMMON_TOOL_TABLE_HEIGHT; i++)
	{
		for(j=0; j<COMMON_TOOL_TABLE_WIDTH; j++)
		{
			if(app->tool_window.common_tools[i][j].tool_type == TYPE_LOUPE_TOOL)
			{
				continue;
			}

			if(GTK_IS_TOGGLE_BUTTON(app->tool_window.vector_brushes[i][j].button) != FALSE)
			{
				gtk_toggle_button_set_active(
					GTK_TOGGLE_BUTTON(app->tool_window.common_tools[i][j].button), FALSE);
			}
		}
	}
}

static gboolean ToolBoxMotionNotifyEvent(
	GtkWidget* widget,
	GdkEventMotion* event_info,
	APPLICATION* app
);

void CreateBrushTable(
	APPLICATION* app,
	TOOL_WINDOW* window,
	BRUSH_CORE brush_data[BRUSH_TABLE_HEIGHT][BRUSH_TABLE_WIDTH]
)
{
	GtkWidget *button;
	int x, y;

	window->brush_table = gtk_table_new(BRUSH_TABLE_HEIGHT, BRUSH_TABLE_WIDTH, TRUE);
	// テーブルのツール間の間隔は0に
	gtk_table_set_row_spacings(GTK_TABLE(window->brush_table), 0);
	gtk_table_set_col_spacings(GTK_TABLE(window->brush_table), 0);

	for(y=0; y<BRUSH_TABLE_HEIGHT; y++)
	{
		for(x=0; x<BRUSH_TABLE_WIDTH; x++)
		{
			if(brush_data[y][x].name == NULL)
			{
				button = gtk_button_new();
				(void)g_signal_connect(G_OBJECT(button), "clicked",
					G_CALLBACK(BrushBlankButtonCallBack), &brush_data[y][x]);
				g_object_set_data(G_OBJECT(button), "table_x", GINT_TO_POINTER(x));
				g_object_set_data(G_OBJECT(button), "table_y", GINT_TO_POINTER(y));
			}
			else
			{
				gchar *file_path;
				if(brush_data[y][x].image_file_path[0] == '.'
					&& brush_data[y][x].image_file_path[1] == '/')
				{
					gchar *temp_path = g_build_filename(app->current_path,
						&brush_data[y][x].image_file_path[2], NULL);
					file_path = g_convert(temp_path, -1, app->system_code, "UTF-8", NULL, NULL, NULL);
					g_free(temp_path);
				}
				else
				{
					file_path = g_strdup(brush_data[y][x].image_file_path);
				}

				button = CreateImageButton(
					file_path,
					brush_data[y][x].name,
					window->font_file
				);
				g_free(file_path);

				(void)g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(BrushButtonClicked),
					&brush_data[y][x]);
				(void)g_signal_connect(G_OBJECT(button), "button_press_event",
					G_CALLBACK(BrushButtonRightClicked), &brush_data[y][x]);
				(void)g_signal_connect(G_OBJECT(button), "motion_notify_event",
					G_CALLBACK(ToolBoxMotionNotifyEvent), app);
				gtk_widget_add_events(button, GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
			}
			g_object_set_data(G_OBJECT(button), "application", app);

#if GTK_MAJOR_VERSION <= 2
			gtk_widget_set_extension_events(button, GDK_EXTENSION_EVENTS_ALL);
#endif
			brush_data[y][x].button = button;
			brush_data[y][x].color = &app->tool_window.color_chooser->rgb;

			gtk_table_attach(GTK_TABLE(window->brush_table), button, x, x+1, y, y+1, GTK_FILL, GTK_FILL, 0, 0);
		}
	}
}

void CreateVectorBrushTable(
	APPLICATION* app,
	TOOL_WINDOW* window,
	VECTOR_BRUSH_CORE brush_data[VECTOR_BRUSH_TABLE_HEIGHT][VECTOR_BRUSH_TABLE_WIDTH]
)
{
	GtkWidget* button;
	int x, y;

	window->brush_table = gtk_table_new(VECTOR_BRUSH_TABLE_HEIGHT, VECTOR_BRUSH_TABLE_WIDTH, TRUE);
	// テーブルのツール間の間隔は0に
	gtk_table_set_row_spacings(GTK_TABLE(window->brush_table), 0);
	gtk_table_set_col_spacings(GTK_TABLE(window->brush_table), 0);

	for(y=0; y<VECTOR_BRUSH_TABLE_HEIGHT; y++)
	{
		for(x=0; x<VECTOR_BRUSH_TABLE_WIDTH; x++)
		{
			if(brush_data[y][x].name == NULL)
			{
				button = gtk_toggle_button_new();
			}
			else
			{
				gchar *file_path;
				if(brush_data[y][x].image_file_path[0] == '.'
					&& brush_data[y][x].image_file_path[1] == '/')
				{
					gchar *temp_path = g_build_filename(app->current_path,
						&brush_data[y][x].image_file_path[2], NULL);
					file_path = g_convert(temp_path, -1, app->system_code, "UTF-8", NULL, NULL, NULL);
					g_free(temp_path);
				}
				else
				{
					file_path = g_strdup(brush_data[y][x].image_file_path);
				}

				button = CreateImageButton(
					file_path,
					brush_data[y][x].name,
					window->font_file
				);
				g_free(file_path);

				(void)g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(VectorBrushButtonClicked),
					&brush_data[y][x]);
				(void)g_signal_connect(G_OBJECT(button), "button_press_event",
					G_CALLBACK(VectorBrushButtonRightClicked), &brush_data[y][x]);
				(void)g_signal_connect(G_OBJECT(button), "motion_notify_event",
					G_CALLBACK(ToolBoxMotionNotifyEvent), app);
				gtk_widget_add_events(button, GDK_POINTER_MOTION_MASK | GDK_POINTER_MOTION_HINT_MASK);
			}
			g_object_set_data(G_OBJECT(button), "application", app);

#if GTK_MAJOR_VERSION <= 2
			gtk_widget_set_extension_events(button, GDK_EXTENSION_EVENTS_ALL);
#endif
			brush_data[y][x].button = button;
			brush_data[y][x].color = &app->tool_window.color_chooser->rgb;

			gtk_table_attach(GTK_TABLE(window->brush_table), button, x, x+1, y, y+1, GTK_FILL, GTK_FILL, 0, 0);
		}
	}
}

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
static void AfterPixelDataGet(APPLICATION* app, uint8* pixels)
{
	DRAW_WINDOW *draw_window = app->draw_window[app->active_window];
	LAYER *layer = draw_window->active_layer;
	uint8 before_num_disable_if_no_open;
	uint8 before_num_disable_if_no_select;
	uint8 before_num_disable_if_single_layer;
	uint8 before_num_disable_if_normal_layer;
	int i;

	(void)memcpy(layer->pixels, pixels, layer->stride * layer->height);
	//g_signal_handler_disconnect(G_OBJECT(draw_window->window), draw_window->callbacks.display);
	DisconnectDrawWindowCallbacks(draw_window->gl_area, draw_window);

	for(i=0; i<layer->height; i++)
	{
		(void)memcpy(&draw_window->temp_layer->pixels[(layer->height-i-1)*layer->stride],
			&layer->pixels[i*layer->stride], layer->stride);
	}
	(void)memcpy(layer->pixels, draw_window->temp_layer->pixels, layer->height * layer->stride);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
	{
		uint8 b;
		for(i=0; i<layer->width * layer->height; i++)
		{
			b = layer->pixels[i*4];
			layer->pixels[i*4] = layer->pixels[i*4+2];
			layer->pixels[i*4+2] = b;
		}
	}
#endif

	// メニューバーの復元
	gtk_widget_destroy(app->menu_bar);
	before_num_disable_if_no_open = app->menus.num_disable_if_no_open;
	before_num_disable_if_no_select = app->menus.num_disable_if_no_select;
	before_num_disable_if_single_layer = app->menus.num_disable_if_single_layer;
	before_num_disable_if_normal_layer = app->menus.num_disable_if_normal_layer;
	app->menus.num_disable_if_no_open = app->menus.menu_start_disable_if_no_open;
	app->menus.num_disable_if_no_select = app->menus.menu_start_disable_if_no_select;
	app->menus.num_disable_if_single_layer = app->menus.menu_start_disable_if_single_layer;
	app->menus.num_disable_if_normal_layer = app->menus.menu_start_disable_if_normal_layer;
	app->flags |= APPLICATION_IN_SWITCH_DRAW_WINDOW;
	app->menu_bar = GetMainMenu(app, app->window, NULL);
	app->menus.num_disable_if_no_open = before_num_disable_if_no_open;
	app->menus.num_disable_if_no_select = before_num_disable_if_no_select;
	app->menus.num_disable_if_single_layer = before_num_disable_if_single_layer;
	app->menus.num_disable_if_normal_layer = before_num_disable_if_normal_layer;
	gtk_box_pack_start(GTK_BOX(app->vbox), app->menu_bar, FALSE, FALSE, 0);
	gtk_box_reorder_child(GTK_BOX(app->vbox), app->menu_bar, 0);
	gtk_widget_show_all(app->menu_bar);

	for(i=0; i<app->menus.num_disable_if_no_open; i++)
	{
		gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
	}
	if((draw_window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
	{
		for(i=0; i<app->menus.num_disable_if_no_select; i++)
		{
			gtk_widget_set_sensitive(app->menus.disable_if_no_select[i], TRUE);
		}
	}
	if(draw_window->num_layer > 1)
	{
		for(i=0; i<app->menus.num_disable_if_single_layer; i++)
		{
			gtk_widget_set_sensitive(app->menus.disable_if_single_layer[i], TRUE);
		}
	}

	// コールバック関数をセット
	SetDrawWindowCallbacks(draw_window->window, draw_window);

	// ウィジェットの表示を切り替え
	gtk_widget_hide(draw_window->gl_area);
	gtk_widget_show_all(draw_window->window);

	DrawWindowChangeZoom(draw_window, draw_window->zoom);
	app->flags &= ~(APPLICATION_IN_SWITCH_DRAW_WINDOW);

	draw_window->flags &= ~(DRAW_WINDOW_EDITTING_3D_MODEL);
}

static void End3DLayerButtonPressed(GtkWidget* button, APPLICATION* app)
{
	DRAW_WINDOW *draw_window = app->draw_window[app->active_window];
	LAYER *layer = draw_window->active_layer;
	TOOL_WINDOW *window = &app->tool_window;
	uint8 *pixels = (uint8*)MEM_ALLOC_FUNC(layer->stride * layer->height);
	gint before_pane_position;

	if(pixels == NULL)
	{
		(void)memcpy(layer->pixels, pixels, layer->width * layer->height * 4);
		DisconnectDrawWindowCallbacks(draw_window->gl_area, draw_window);

		// 描画用のコールバック関数をセット
		SetDrawWindowCallbacks(draw_window->window, draw_window);

		// ウィジェットの表示を切り替え
		gtk_widget_hide(draw_window->gl_area);
		gtk_widget_show_all(draw_window->window);

		DrawWindowChangeZoom(draw_window, draw_window->zoom);

		draw_window->flags &= ~(DRAW_WINDOW_EDITTING_3D_MODEL);
	}
	else
	{
		RenderForPixelData(layer->layer_data.project,
			layer->width, layer->height, pixels, (void (*)(void*, uint8*))AfterPixelDataGet, app);
	}

	gtk_widget_destroy(app->layer_window.vbox);
	before_pane_position = app->layer_window.pane_position;
	ExecuteChangeToolWindowPlace(NULL, app);
	ExecuteChangeNavigationLayerWindowPlace(NULL, app);
	app->layer_window.pane_position = before_pane_position;
	gtk_paned_set_position(GTK_PANED(app->navi_layer_pane), app->layer_window.pane_position);

	{
		LAYER* layer;	// ビューに追加するレイヤー
		int i;			// for文用のカウンタ

		// 一番下のレイヤーを設定
		layer = draw_window->layer;

		// ビューに全てのレイヤーを追加
		for(i=0; i<draw_window->num_layer; i++)
		{
			LayerViewAddLayer(layer, draw_window->layer,
				app->layer_window.view, i+1);
			layer = layer->next;
		}

		// アクティブレイヤーをセット
		LayerViewSetActiveLayer(draw_window->active_layer, app->layer_window.view);
	}
}

static void Change3DLayerButtonPressed(GtkWidget* button, APPLICATION* app)
{
	DRAW_WINDOW *draw_window = app->draw_window[app->active_window];
	LAYER *layer = draw_window->active_layer;
	TOOL_WINDOW *window = &app->tool_window;
	GtkAllocation allocation;
	GtkWidget *return_button;
	GtkWidget *camera_light_widget;
	gtk_widget_destroy(window->brush_table);
	window->brush_table = (GtkWidget*)ModelControlWidgetNew(
		app->modeling);
	gtk_widget_destroy(window->ui);
	return_button = gtk_button_new_with_label(app->labels->tool_box.end_edit_3d);
	(void)g_signal_connect(G_OBJECT(return_button), "pressed",
		G_CALLBACK(End3DLayerButtonPressed), app);
	window->ui = gtk_vbox_new(FALSE, 0);
	gtk_box_pack_end(GTK_BOX(window->ui), return_button, FALSE, FALSE, 0);

	window->brush_scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_box_pack_start(GTK_BOX(window->ui), window->brush_scroll, TRUE, TRUE, 0);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(window->brush_scroll), window->brush_table);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(window->brush_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	DisconnectDrawWindowCallbacks(draw_window->window, draw_window);
	gtk_widget_hide(draw_window->window);

#if GTK_MAJOR_VERSION <= 2
	draw_window->callbacks.display = g_signal_connect(G_OBJECT(draw_window->gl_area), "expose-event",
		G_CALLBACK(ProjectDisplayEvent), layer->layer_data.project);
#else
	draw_window->callbacks.display = g_signal_connect(G_OBJECT(draw_window->gl_area), "draw",
		G_CALLBACK(ProjectDisplayEvent), layer->layer_data.project);
#endif
	draw_window->callbacks.mouse_button_press = g_signal_connect(G_OBJECT(draw_window->gl_area), "button-press-event",
		G_CALLBACK(MouseButtonPressEvent), layer->layer_data.project);
	draw_window->callbacks.mouse_move = g_signal_connect(G_OBJECT(draw_window->gl_area), "motion-notify-event",
		G_CALLBACK(MouseMotionEvent), layer->layer_data.project);
	draw_window->callbacks.mouse_button_release = g_signal_connect(G_OBJECT(draw_window->gl_area), "button-release-event",
		G_CALLBACK(MouseButtonReleaseEvent), layer->layer_data.project);
	draw_window->callbacks.mouse_wheel = g_signal_connect(G_OBJECT(draw_window->gl_area), "scroll-event",
		G_CALLBACK(MouseWheelScrollEvent), layer->layer_data.project);
	draw_window->callbacks.configure = g_signal_connect(G_OBJECT(draw_window->gl_area), "configure-event",
		G_CALLBACK(ConfigureEvent), layer->layer_data.project);

#if GTK_MAJOR_VERSION >= 3
	gtk_widget_get_allocation(draw_window->scroll, &allocation);
#else
	allocation = draw_window->scroll->allocation;
#endif
	if(allocation.width > SCROLLED_WINDOW_MARGIN)
	{
		allocation.width -= SCROLLED_WINDOW_MARGIN;
	}
	if(allocation.height > SCROLLED_WINDOW_MARGIN)
	{
		allocation.height -= SCROLLED_WINDOW_MARGIN;
	}
	gtk_widget_set_size_request(draw_window->gl_area, allocation.width, allocation.height);
	gtk_widget_show_all(draw_window->gl_area);

	if((window->flags & TOOL_DOCKED) == 0)
	{
		gtk_container_add(GTK_CONTAINER(window->window), window->ui);
	}
	else if((window->flags & TOOL_PLACE_RIGHT) == 0)
	{
		gtk_box_pack_start(GTK_BOX(gtk_paned_get_child1(GTK_PANED(app->left_pane))),
			window->ui, TRUE, TRUE, 0);
	}
	else
	{
		gtk_box_pack_start(GTK_BOX(gtk_paned_get_child2(GTK_PANED(app->right_pane))),
			window->ui, TRUE, TRUE, 0);
	}
	gtk_widget_show_all(window->ui);

	camera_light_widget = CameraLightControlWidgetNew(app->modeling);
	if((app->layer_window.flags & LAYER_WINDOW_DOCKED) == 0)
	{
		gtk_widget_destroy(app->layer_window.vbox);
		app->layer_window.vbox = camera_light_widget;
		gtk_container_add(GTK_CONTAINER(app->layer_window.window), app->layer_window.vbox);
	}
	else
	{
		gtk_widget_destroy(app->navi_layer_pane);
		app->layer_window.vbox = camera_light_widget;
		if((app->layer_window.flags & LAYER_WINDOW_PLACE_RIGHT) != 0)
		{
			gtk_box_pack_start(GTK_BOX(gtk_paned_get_child2(GTK_PANED(app->right_pane))),
				app->layer_window.vbox, TRUE, TRUE, 0);
		}
		else
		{
			gtk_box_pack_start(GTK_BOX(gtk_paned_get_child1(GTK_PANED(app->left_pane))),
				app->layer_window.vbox, TRUE, TRUE, 0);
		}
	}
	gtk_widget_show_all(app->layer_window.vbox);

	gtk_widget_destroy(app->menu_bar);
	app->menu_bar = MakeMenuBar(app->modeling, app->hot_key);
	gtk_box_pack_start(GTK_BOX(app->vbox), app->menu_bar, FALSE, FALSE, 0);
	gtk_box_reorder_child(GTK_BOX(app->vbox), app->menu_bar, 0);
	gtk_widget_show_all(app->menu_bar);

	draw_window->flags |= DRAW_WINDOW_EDITTING_3D_MODEL;
}

void CreateChange3DLayerUI(
	APPLICATION* app,
	TOOL_WINDOW* window
)
{
	GtkWidget *button;
	window->brush_table = gtk_vbox_new(FALSE, 0);
	button = gtk_button_new_with_label(app->labels->tool_box.start_edit_3d);
	(void)g_signal_connect(G_OBJECT(button), "pressed",
		G_CALLBACK(Change3DLayerButtonPressed), app);
	gtk_box_pack_start(GTK_BOX(window->brush_table), button, FALSE, FALSE, 0);
}
#endif

static void ColorChangeCallBack(GtkWidget* widget, const uint8 color[3], void* data)
{
	APPLICATION *app = (APPLICATION*)data;

	switch(app->current_tool)
	{
	case TYPE_NORMAL_LAYER:
		if(app->tool_window.active_brush[app->input]->color_change != NULL)
		{
			app->tool_window.active_brush[app->input]->color_change(
				color, app->tool_window.active_brush[app->input]->brush_data);
		}
		break;
	case TYPE_VECTOR_LAYER:
		if(app->tool_window.active_vector_brush[app->input]->color_change != NULL)
		{
			app->tool_window.active_vector_brush[app->input]->color_change(
				color, app->tool_window.active_vector_brush[app->input]->brush_data);
		}
		break;
	}
}

/***************************************************************
* TextureButtonCallBack関数                                    *
* テクスチャ選択用のボタンがクリックされた時のコールバック関数 *
* 引数                                                         *
* button	: テクスチャ選択用のボタンウィジェット             *
* app		: アプリケーションを管理する構造体のアドレス       *
***************************************************************/
static void TextureButtonCallBack(GtkWidget* button, APPLICATION* app)
{
	GtkWidget *popup = CreateTextureChooser(&app->textures, app);
	gtk_widget_show_all(popup);
}

/*************************************************************
* ToolBoxMotionNotifyEvent関数                               *
* ツールボックス上でペン・消しゴムの判定をして切り替える     *
* 引数                                                       *
* widget                                                     *
* event_info	: カーソル移動イベントの情報                 *
* app			: アプリケーションを管理する構造体のアドレス *
* 返り値                                                     *
*	常にTRUE                                                 *
*************************************************************/
static gboolean ToolBoxMotionNotifyEvent(
	GtkWidget* widget,
	GdkEventMotion* event_info,
	APPLICATION* app
)
{
#if MAJOR_VERSION == 1
	if(event_info->device->source == GDK_SOURCE_ERASER)
#else
	if(gdk_device_get_source(event_info->device) == GDK_SOURCE_ERASER)
#endif
	{	// 消しゴム
		if(app->input == INPUT_PEN)
		{
			app->input = INPUT_ERASER;
			if(app->window_num == 0)
			{
				gtk_toggle_button_set_active(
					GTK_TOGGLE_BUTTON(app->tool_window.active_brush[INPUT_ERASER]->button), TRUE);
			}
			else
			{
				if(app->draw_window[app->active_window]->active_layer->layer_type == TYPE_NORMAL_LAYER
					|| ((app->draw_window[app->active_window]->flags & DRAW_WINDOW_EDIT_SELECTION) != 0))
				{
					gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(app->tool_window.active_brush[INPUT_ERASER]->button), TRUE);
				}
				else if(app->draw_window[app->active_window]->active_layer->layer_type == TYPE_VECTOR_LAYER)
				{
					gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(app->tool_window.active_brush[INPUT_ERASER]->button), TRUE);
				}
			}
		}
	}
#if MAJOR_VERSION == 1
	else if(event_info->device->source == GDK_SOURCE_PEN)
#else
	else if(gdk_device_get_source(event_info->device) == GDK_SOURCE_PEN)
#endif
	{	// ペンまたはマウス
		if(app->input != INPUT_PEN)
		{
			app->input = INPUT_PEN;
			if(app->window_num == 0)
			{
				gtk_toggle_button_set_active(
					GTK_TOGGLE_BUTTON(app->tool_window.active_brush[INPUT_PEN]->button), TRUE);
			}
			else
			{
				if(app->draw_window[app->active_window]->active_layer->layer_type == TYPE_NORMAL_LAYER
					|| ((app->draw_window[app->active_window]->flags & DRAW_WINDOW_EDIT_SELECTION) != 0))
				{
					gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(app->tool_window.active_brush[INPUT_PEN]->button), TRUE);
				}
				else if(app->draw_window[app->active_window]->active_layer->layer_type == TYPE_VECTOR_LAYER)
				{
					gtk_toggle_button_set_active(
						GTK_TOGGLE_BUTTON(app->tool_window.active_brush[INPUT_PEN]->button), TRUE);
				}
			}
		}
	}

	return FALSE;
}

GtkWidget *CreateToolBox(
	APPLICATION* app,
	TOOL_WINDOW* window,
	const char* brush_data_file_path,
	const char* vector_brush_data_file_path,
	const char* common_tools_data_file_path,
	BRUSH_CORE brush_data[BRUSH_TABLE_HEIGHT][BRUSH_TABLE_WIDTH],
	VECTOR_BRUSH_CORE vector_brush_data[VECTOR_BRUSH_TABLE_HEIGHT][VECTOR_BRUSH_TABLE_WIDTH],
	COMMON_TOOL_CORE common_tool_data[COMMON_TOOL_TABLE_HEIGHT][COMMON_TOOL_TABLE_WIDTH]
)
{
#define MAX_STR_LENGTH 128
#define COLOR_CIRCLE_SIZE (32 * 4 + 16)
#define COLOR_CIRCLE_WIDTH 15
#define DETAIL_UI_HEIGHT 128

	// 絶対パス作成用(Mac対策)
	gchar *file_path;
	// 垂直ボックスにツールを並べる
	GtkWidget *widget = gtk_vbox_new(FALSE, 0);
	// 共通ツールのテーブル
	GtkWidget *table;
	// ブラシのボタン
	GtkWidget *button;
	// テクスチャ選択用の水平ボックス
	GtkWidget *hbox;
	// 詳細設定用のパッキングボックス
	GtkWidget *detail_box = gtk_vbox_new(FALSE, 0);
	// 文字コード
	char code[MAX_STR_LENGTH];
	// ショートカットキー
	char hot_key[4];
	// テーブル配置用のカウンタ
	int x, y;
	// for文用のカウンタ
	int i;

	// ファイル読み込みストリーム
	GFile* fp;
	GFileInputStream* stream;
	// ファイルサイズ取得用
	GFileInfo *file_info;
	// ファイルサイズ
	size_t data_size;
	// ファイルデータ解析用
	INI_FILE_PTR file;
	INI_FILE_PTR vector_file;
	INI_FILE_PTR common_tool;

	if((app->flags & APPLICATION_INITIALIZED) == 0)
	{
		if(brush_data_file_path[0] == '.' && brush_data_file_path[1] == '/')
		{
			file_path = g_build_filename(app->current_path,
				&brush_data_file_path[2], NULL);
			fp = g_file_new_for_path(file_path);
			g_free(file_path);
		}
		else
		{
			fp = g_file_new_for_path(brush_data_file_path);
		}
		stream = g_file_read(fp, NULL, NULL);

		// ファイルサイズを取得
		file_info = g_file_input_stream_query_info(stream,
			G_FILE_ATTRIBUTE_STANDARD_SIZE, NULL, NULL);
		data_size = (size_t)g_file_info_get_size(file_info);

		file = CreateIniFile(stream,
			(size_t (*)(void*, size_t, size_t, void*))FileRead, data_size, INI_READ);

		// 一度不要になったオブジェクトを削除
		g_object_unref(fp);
		g_object_unref(stream);
		g_object_unref(file_info);

		if(vector_brush_data_file_path[0] == '.' && vector_brush_data_file_path[1] == '/')
		{
			file_path = g_build_filename(app->current_path,
				&vector_brush_data_file_path[2], NULL);
			fp = g_file_new_for_path(file_path);
			g_free(file_path);
		}
		else
		{
			fp = g_file_new_for_path(vector_brush_data_file_path);
		}
		stream = g_file_read(fp, NULL, NULL);
		// ファイルサイズを取得
		file_info = g_file_input_stream_query_info(stream,
			G_FILE_ATTRIBUTE_STANDARD_SIZE, NULL, NULL);
		data_size = (size_t)g_file_info_get_size(file_info);

		vector_file = CreateIniFile(stream,
			(size_t (*)(void*, size_t, size_t, void*))FileRead, data_size, INI_READ);

		// 一度不要になったオブジェクトを削除
		g_object_unref(fp);
		g_object_unref(stream);
		g_object_unref(file_info);

		if(common_tools_data_file_path[0] == '.' && common_tools_data_file_path[1] == '/')
		{
			file_path = g_build_filename(app->current_path,
				&common_tools_data_file_path[2], NULL);
			fp = g_file_new_for_path(file_path);
			g_free(file_path);
		}
		else
		{
			fp = g_file_new_for_path(common_tools_data_file_path);
		}
		stream = g_file_read(fp, NULL, NULL);
		// ファイルサイズを取得
		file_info = g_file_input_stream_query_info(stream,
			G_FILE_ATTRIBUTE_STANDARD_SIZE, NULL, NULL);
		data_size = (size_t)g_file_info_get_size(file_info);

		common_tool = CreateIniFile(stream,
			(size_t (*)(void*, size_t, size_t, void*))FileRead, data_size, INI_READ);

		// 不要になったオブジェクトを削除
		g_object_unref(fp);
		g_object_unref(stream);
		g_object_unref(file_info);
	}

	// ボックスのサイズをセット
	gtk_widget_set_size_request(widget,
		32*4+74, 32*8+16 + COLOR_CIRCLE_SIZE
		+ 32*COMMON_TOOL_TABLE_HEIGHT + DETAIL_UI_HEIGHT);

	// カラーサークルを作成
	window->color_chooser =
		CreateColorChooser(COLOR_CIRCLE_SIZE, COLOR_CIRCLE_SIZE, COLOR_CIRCLE_WIDTH, 0,
		window->pallete, window->pallete_use, app->current_path, app->labels);
	// 作成したウィジェットを追加
	gtk_box_pack_start(GTK_BOX(widget),
		window->color_chooser->widget, FALSE, FALSE, 0);
	// 色変更時のコールバック関数を設定
	SetColorChangeCallBack(window->color_chooser, ColorChangeCallBack, (void*)app);

	// 共通ツールのテーブル作成
	if((app->flags & APPLICATION_INITIALIZED) == 0)
	{	// 文字コードの読み込み
		(void)IniFileGetString(common_tool, "CODE", "CODE_TYPE", code, MAX_STR_LENGTH);
		window->common_tool_code = MEM_STRDUP_FUNC(code);
	}
	window->common_tool_table = table = gtk_table_new(COMMON_TOOL_TABLE_HEIGHT, COMMON_TOOL_TABLE_WIDTH, TRUE);
	{
		char temp[MAX_STR_LENGTH];
		size_t length;

		if((app->flags & APPLICATION_INITIALIZED) == 0)
		{
			for(i=0; i<common_tool->section_count; i++)
			{
				if((length = IniFileGetString(common_tool, common_tool->section[i].section_name,
					"NAME", temp, MAX_STR_LENGTH)) != 0)
				{
					x = IniFileGetInt(common_tool, common_tool->section[i].section_name, "X");
					y = IniFileGetInt(common_tool, common_tool->section[i].section_name, "Y");
					common_tool_data[y][x].name = g_convert(
						temp, length, "UTF-8", code, NULL, NULL, NULL);
					common_tool_data[y][x].image_file_path =
						IniFileStrdup(common_tool, common_tool->section[i].section_name, "IMAGE");
					if(IniFileGetString(common_tool, common_tool->section[i].section_name, "HOT_KEY", hot_key, 3) != 0)
					{
						common_tool_data[y][x].hot_key = hot_key[0];
					}
					(void)IniFileGetString(common_tool, common_tool->section[i].section_name,
						"TYPE", temp, MAX_STR_LENGTH);
					LoadCommonToolDetailData(&common_tool_data[y][x], common_tool,
						common_tool->section[i].section_name, temp, app);
				}
			}
		}

		for(y=0; y<COMMON_TOOL_TABLE_HEIGHT; y++)
		{
			for(x=0; x<COMMON_TOOL_TABLE_WIDTH; x++)
			{
				if(common_tool_data[y][x].name == NULL)
				{
					button = gtk_toggle_button_new();
				}
				else
				{
					gchar *file_path;
					if(common_tool_data[y][x].image_file_path[0] == '.'
						&& common_tool_data[y][x].image_file_path[1] == '/')
					{
						gchar *temp_path = g_build_filename(app->current_path,
							&common_tool_data[y][x].image_file_path[2], NULL);
						file_path = g_convert(temp_path, -1, app->system_code, "UTF-8", NULL, NULL, NULL);
						g_free(temp_path);
					}
					else
					{
						file_path = g_strdup(common_tool_data[y][x].image_file_path);
					}
					button = CreateImageButton(file_path, NULL, NULL);
					g_free(file_path);

					if(common_tool_data[y][x].tool_type == TYPE_LOUPE_TOOL)
					{
						(void)g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(LoupeButtonToggled), app);
					}
					else
					{
						(void)g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(CommonToolButtonClicked),
							&common_tool_data[y][x]);
						(void)g_signal_connect(G_OBJECT(button), "button_press_event",
							G_CALLBACK(CommonToolButtonRightClicked), &common_tool_data[y][x]);
					}
					gtk_widget_set_tooltip_text(button, common_tool_data[y][x].name);
				}
				g_object_set_data(G_OBJECT(button), "application", app);
				common_tool_data[y][x].button = button;

				gtk_table_attach(GTK_TABLE(table), button, x, x+1, y, y+1, GTK_FILL, GTK_FILL, 0, 0);
			}
		}

		gtk_box_pack_start(GTK_BOX(widget), table, FALSE, TRUE, 0);
	}

	// ブラシテーブルと詳細設定を区切るペーンを作成
	window->pane = gtk_vpaned_new();
	gtk_box_pack_start(GTK_BOX(widget), window->pane, TRUE, TRUE, 0);
	// スクロールドウィンドウの作成
	window->brush_scroll = gtk_scrolled_window_new(NULL, NULL);
	// スクロールドウィンドウのサイズをセット
	gtk_widget_set_size_request(window->brush_scroll, 32*4+64, 32*4+16);
	// ウィジェットの間隔をセット
	gtk_container_set_border_width(GTK_CONTAINER(window->brush_scroll), 0);
	// スクロールバーを表示する条件をセット
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(window->brush_scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
#if GTK_MAJOR_VERSION <= 2
	// ペーンに追加
	gtk_paned_pack1(GTK_PANED(window->pane), window->brush_scroll, TRUE, TRUE);
#else
	// ペーンに追加
	gtk_paned_pack1(GTK_PANED(window->pane), window->brush_scroll, TRUE, FALSE);
#endif

	// ブラシデータファイルを読み込む
	if((app->flags & APPLICATION_INITIALIZED) == 0)
	{
		char temp[MAX_STR_LENGTH];
		size_t length;

		// 文字コードの読み込み
		(void)IniFileGetString(file, "CODE", "CODE_TYPE", code, MAX_STR_LENGTH);
		window->brush_code = MEM_STRDUP_FUNC(code);

		window->font_file = IniFileStrdup(file, "FONT", "FONT_FILE");

		for(i=1; i<file->section_count; i++)
		{
			if((length = IniFileGetString(
				file, file->section[i].section_name, "NAME", temp, MAX_STR_LENGTH))
				!= 0
			)
			{
				x = IniFileGetInt(file, file->section[i].section_name, "X");
				y = IniFileGetInt(file, file->section[i].section_name, "Y");
				hot_key[0] = 0;
				if(IniFileGetString(file, file->section[i].section_name, "HOT_KEY", hot_key, 3) != 0)
				{
					brush_data[y][x].hot_key = hot_key[0];
				}
				brush_data[y][x].app = app;
				brush_data[y][x].name =
					g_convert(temp, length, "UTF-8", code, NULL, NULL, NULL);
				brush_data[y][x].image_file_path =
					IniFileStrdup(file, file->section[i].section_name, "IMAGE");
				(void)IniFileGetString(file, file->section[i].section_name, "TYPE", temp, MAX_STR_LENGTH);
				LoadBrushDetailData(&brush_data[y][x], file, file->section[i].section_name, temp, app);
			}
		}
	}

	// ベクトルブラシデータファイルを読み込む
	if((app->flags & APPLICATION_INITIALIZED) == 0)
	{
		char temp[MAX_STR_LENGTH];
		size_t length;

		// 文字コードの読み込み
		(void)IniFileGetString(vector_file, "CODE", "CODE_TYPE", code, MAX_STR_LENGTH);
		window->vector_brush_code = MEM_STRDUP_FUNC(code);

		for(i=1; i<vector_file->section_count; i++)
		{
			if((length = IniFileGetString(
				vector_file, vector_file->section[i].section_name, "NAME", temp, MAX_STR_LENGTH))
				!= 0
			)
			{
				x = IniFileGetInt(vector_file, vector_file->section[i].section_name, "X");
				y = IniFileGetInt(vector_file, vector_file->section[i].section_name, "Y");
				vector_brush_data[y][x].app = app;
				vector_brush_data[y][x].name =
					g_convert(temp, length, "UTF-8", code, NULL, NULL, NULL);
				vector_brush_data[y][x].image_file_path =
					IniFileStrdup(vector_file, vector_file->section[i].section_name, "IMAGE");
				(void)IniFileGetString(vector_file, vector_file->section[i].section_name, "TYPE", temp, MAX_STR_LENGTH);
				LoadVectorBrushDetailData(&vector_brush_data[y][x], vector_file, vector_file->section[i].section_name, temp);
			}
		}
	}

	// 16 x 4 のテーブルを作成してブラシ等のツールを入れる
	CreateBrushTable(app, window, brush_data);
	// スクロールドウィンドウにテーブルを追加
	gtk_scrolled_window_add_with_viewport(
		GTK_SCROLLED_WINDOW(window->brush_scroll), window->brush_table);
	gtk_widget_show_all(window->brush_scroll);

	// 詳細設定用のパッキングボックスをペーンに追加
	gtk_paned_pack2(GTK_PANED(window->pane), detail_box, TRUE, TRUE);

	// テクスチャ選択用
	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(detail_box), hbox, FALSE, FALSE, 0);
	// テクスチャ選択用のボタンを作成
	button = gtk_button_new_with_label(app->labels->tool_box.texture);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, FALSE, 0);
	(void)g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(TextureButtonCallBack), app);
	// 使用中のテクスチャを表示するラベルを作成
	app->texture_label = gtk_label_new(app->labels->tool_box.no_texture);
	gtk_box_pack_start(GTK_BOX(hbox), app->texture_label, FALSE, FALSE, 0);

	// 詳細な設定を行うUI部分のスクロールドウィンドウを作成
	window->detail_ui_scroll = gtk_scrolled_window_new(NULL, NULL);
	// ウィジェットの間隔をセット
	gtk_container_set_border_width(GTK_CONTAINER(window->detail_ui_scroll), 0);
	// スクロールバーを表示する条件をセット
	gtk_scrolled_window_set_policy(
		GTK_SCROLLED_WINDOW(window->detail_ui_scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start(GTK_BOX(detail_box), window->detail_ui_scroll, TRUE, TRUE, 0);

	// ペーンの位置を設定
	gtk_paned_set_position(GTK_PANED(window->pane), window->pane_position);

	// コントロールキー、シフトキーが押されたとき用のデータを作成
	SetColorPicker(&window->color_picker_core, &window->color_picker);
	SetControlPointTool(&window->vector_control_core, &window->vector_control);

	if((app->flags & APPLICATION_INITIALIZED) == 0)
	{
		file->delete_func(file);
		vector_file->delete_func(vector_file);
		common_tool->delete_func(common_tool);
	}

#if GTK_MAJOR_VERSION <= 2
	// 拡張デバイスを有効に
	gtk_widget_set_extension_events(widget, GDK_EXTENSION_EVENTS_CURSOR);
#endif

	return widget;
}

static void OnDeleteToolBoxWidget(GtkWidget* widget, APPLICATION* app)
{
	app->tool_window.ui = NULL;
}

static gboolean OnDeleteToolBoxWindow(GtkWidget* window, GdkEvent* event_info, APPLICATION* app)
{
	gint x, y;
	gint width, height;
	gtk_window_get_position(GTK_WINDOW(window), &x, &y);
	gtk_window_get_size(GTK_WINDOW(window), &width, &height);
	app->tool_window.window_x = x;
	app->tool_window.window_y = y;
	app->tool_window.window_width = width;
	app->tool_window.window_height = height;
	app->tool_window.window = NULL;
	app->flags |= APPLICATION_IN_DELETE_EVENT;
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(app->tool_window.menu_item), FALSE);
	app->flags &= ~(APPLICATION_IN_DELETE_EVENT);

	return FALSE;
}

GtkWidget *CreateToolBoxWindow(APPLICATION* app, GtkWidget *parent)
{
	GtkWidget *window = NULL;
	GtkWidget *box;

	if((app->tool_window.flags & TOOL_DOCKED) == 0)
	{
		// ウィンドウ作成
		window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
		// ウィンドウの位置とサイズを設定
		gtk_window_move(GTK_WINDOW(window), app->tool_window.window_x, app->tool_window.window_y);
		gtk_window_resize(GTK_WINDOW(window), app->tool_window.window_width, app->tool_window.window_height);
		// ウィンドウが閉じるときのコールバック関数をセット
		g_signal_connect(G_OBJECT(window), "delete_event", G_CALLBACK(OnDeleteToolBoxWindow), app);
		// 親ウィンドウを登録
		gtk_window_set_transient_for(GTK_WINDOW(window), GTK_WINDOW(parent));
		// タイトルをセット
		gtk_window_set_title(GTK_WINDOW(window), app->labels->tool_box.title);
		// 閉じるボタンのみのウィンドウへ
		gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_UTILITY);
		// タスクバーには表示しない
		gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
		// ショートカットキーを登録
		gtk_window_add_accel_group(GTK_WINDOW(window), app->hot_key);

		// ツールボックスの中身を作成して登録
		gtk_container_add(GTK_CONTAINER(window),
			(app->tool_window.ui =
				CreateToolBox(app, &app->tool_window, app->brush_file_path, app->vector_brush_file_path,
					app->common_tool_file_path, app->tool_window.brushes,
						app->tool_window.vector_brushes, app->tool_window.common_tools)));

		// ツールボックスの位置変更用に削除時のコールバック関数をセット
		g_signal_connect(G_OBJECT(app->tool_window.ui), "destroy",
			G_CALLBACK(OnDeleteToolBoxWidget), app);

		// キーボードのコールバック関数をセット
		g_signal_connect(G_OBJECT(window), "key-press-event",
			G_CALLBACK(KeyPressEvent), app);
		g_signal_connect(G_OBJECT(window), "key-release-event",
			G_CALLBACK(KeyPressEvent), app);
	}
	else
	{
		app->tool_window.ui =
			CreateToolBox(app, &app->tool_window, app->brush_file_path, app->vector_brush_file_path,
				app->common_tool_file_path, app->tool_window.brushes,
					app->tool_window.vector_brushes, app->tool_window.common_tools);
		// ツールボックスの位置変更用に削除時のコールバック関数をセット
		g_signal_connect(G_OBJECT(app->tool_window.ui), "destroy",
			G_CALLBACK(OnDeleteToolBoxWidget), app);

		if((app->tool_window.flags & TOOL_PLACE_RIGHT) == 0)
		{
			box = gtk_paned_get_child1(GTK_PANED(app->left_pane));
			if(box == NULL)
			{
				box = gtk_hbox_new(FALSE, 0);
				gtk_paned_pack1(GTK_PANED(app->left_pane), box, TRUE, FALSE);
				gtk_widget_show_all(box);
			}

			gtk_box_pack_start(GTK_BOX(box), app->tool_window.ui, TRUE, TRUE, 0);
			gtk_box_reorder_child(GTK_BOX(box), app->tool_window.ui, 0);
		}
		else
		{
			box = gtk_paned_get_child2(GTK_PANED(app->right_pane));
			if(box == NULL)
			{
				box = gtk_hbox_new(FALSE, 0);
				gtk_paned_pack2(GTK_PANED(app->right_pane), box, TRUE, FALSE);
				gtk_widget_show_all(box);
			}

			gtk_box_pack_end(GTK_BOX(box), app->tool_window.ui, TRUE, TRUE, 0);
		}
	}

	{
		int i, j;
		for(i=0; i<BRUSH_TABLE_HEIGHT; i++)
		{
			for(j=0; j<BRUSH_TABLE_WIDTH; j++)
			{
				app->tool_window.brushes[i][j].app = app;
				app->tool_window.brushes[i][j].color =
					&app->tool_window.color_chooser->rgb;
				app->tool_window.brushes[i][j].back_color =
					&app->tool_window.color_chooser->back_rgb;
				app->tool_window.brushes[i][j].brush_pattern_buff =
					&app->tool_window.brush_pattern_buff;
				app->tool_window.brushes[i][j].temp_pattern_buff =
					&app->tool_window.temp_pattern_buff;
			}
		}
	}

	// アクティブなブラシをセット
	if((app->flags & APPLICATION_INITIALIZED) == 0)
	{
		app->tool_window.brush_pattern_buff = (uint8*)MEM_ALLOC_FUNC(
			MAX_BRUSH_SIZE * MAX_BRUSH_STRIDE);
		(void)memset(app->tool_window.brush_pattern_buff, 0, MAX_BRUSH_SIZE * MAX_BRUSH_STRIDE);
		app->tool_window.temp_pattern_buff = (uint8*)MEM_ALLOC_FUNC(
			MAX_BRUSH_SIZE * MAX_BRUSH_STRIDE);
		(void)memset(app->tool_window.temp_pattern_buff, 0, MAX_BRUSH_SIZE * MAX_BRUSH_STRIDE);
		app->tool_window.active_brush[INPUT_PEN] = app->tool_window.active_brush[INPUT_ERASER] = app->tool_window.brushes[0];
		app->tool_window.active_vector_brush[INPUT_PEN] = app->tool_window.active_vector_brush[INPUT_ERASER] = app->tool_window.vector_brushes[0];
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->tool_window.active_brush[INPUT_PEN]->button), TRUE);
	}

	// タッチイベントの準備
#if MAJOR_VERSION > 1
	{
		size_t max_data_size = sizeof(TOUCH_POINT);
		int i, j;

		for(i=0; i<BRUSH_TABLE_HEIGHT; i++)
		{
			for(j=0; j<BRUSH_TABLE_WIDTH; j++)
			{
				if(max_data_size < app->tool_window.brushes[i][j].detail_data_size)
				{
					max_data_size = app->tool_window.brushes[i][j].detail_data_size;
				}
			}
		}

		for(i=0; i<MAX_TOUCH; i++)
		{
			app->tool_window.touch[i].brush_data = MEM_ALLOC_FUNC(max_data_size);
			(void)memcpy(app->tool_window.touch[i].brush_data,
				app->tool_window.active_brush[INPUT_PEN]->brush_data,
				app->tool_window.active_brush[INPUT_PEN]->detail_data_size
			);
		}
	}
#endif

	return window;
}

#ifdef __cplusplus
}
#endif
