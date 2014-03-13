// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <ctype.h>
#include "application.h"
#include "text_layer.h"
#include "widgets.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************
* CreateTextLayer関数                        *
* テキストレイヤーのデータメモリ確保と初期化 *
* 引数                                       *
* x			: レイヤーのX座標                *
* y			: レイヤーのY座標                *
* width		: レイヤーの幅                   *
* height	: レイヤーの高さ                 *
* base_size	: 文字サイズの倍率               *
* font_size	: 文字サイズ                     *
* color		: 文字色                         *
* flags		: テキスト表示のフラグ           *
* 返り値                                     *
*	初期化された構造体のアドレス             *
*********************************************/
TEXT_LAYER* CreateTextLayer(
	gdouble x,
	gdouble y,
	gdouble width,
	gdouble height,
	int base_size,
	gdouble font_size,
	int32 font_id,
	uint8 color[3],
	uint32 flags
)
{
	TEXT_LAYER* ret = (TEXT_LAYER*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->x = x;
	ret->y = y;
	ret->width = width;
	ret->height = height;
	ret->base_size = base_size;
	ret->font_size = font_size;
	ret->font_id = font_id;
	(void)memcpy(ret->color, color, 3);
	ret->flags = flags;

	return ret;
}

/*****************************************************
* DeleteTextLayer関数                                *
* テキストレイヤーのデータを削除する                 *
* 引数                                               *
* layer	: テキストレイヤーのデータポインタのアドレス *
*****************************************************/
void DeleteTextLayer(TEXT_LAYER** layer)
{
	MEM_FREE_FUNC((*layer)->text);
	MEM_FREE_FUNC(*layer);

	*layer = NULL;
}

void TextLayerButtonPressCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	GdkEventButton* event
)
{
	if(event->button == 1)
	{
		TEXT_LAYER* layer = window->active_layer->layer_data.text_layer_p;
		layer->drag_start_x = x;
		layer->drag_start_y = y;
	}
}

void TextLayerMotionCallBack(
	DRAW_WINDOW* window,
	gdouble x,
	gdouble y,
	GdkModifierType state
)
{
#define TARGET_IS_START 0
#define TARGET_IS_WIDTH 1
#define TARGET_IS_HEIGHT 1
	if((state & GDK_BUTTON1_MASK) != 0)
	{
		TEXT_LAYER* layer = window->active_layer->layer_data.text_layer_p;
		int x_target, y_target;
		if(x < layer->x || x > layer->x + layer->width
			|| y < layer->y || y > layer->y + layer->height)
		{
			return;
		}

		x_target = (x > layer->x + layer->width/2) ? TARGET_IS_WIDTH : TARGET_IS_START;
		y_target = (y > layer->y + layer->height/2) ? TARGET_IS_HEIGHT : TARGET_IS_START;

		if(x_target == TARGET_IS_START)
		{
			layer->x += x - layer->drag_start_x;
			layer->width += layer->drag_start_x - x;
		}
		else
		{
			layer->width += x - layer->drag_start_x;
		}

		if(y_target == TARGET_IS_START)
		{
			layer->y += y - layer->drag_start_y;
			layer->height += layer->drag_start_y - y;
		}
		else
		{
			layer->height += y - layer->drag_start_y;
		}

		layer->drag_start_x = x;
		layer->drag_start_y = y;

		window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
	}
}

static void ChangeTextLayerFontFamily(GtkComboBox *combo_box, LAYER* layer)
{
	layer->layer_data.text_layer_p->font_id = gtk_combo_box_get_active(combo_box);
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

static void ChangeTextLayerFontSize(GtkAdjustment* slider, LAYER* layer)
{
	layer->layer_data.text_layer_p->font_size = gtk_adjustment_get_value(slider);
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

static void ChangeTextColorButtonCallBack(GtkWidget* button, LAYER* layer)
{
	(void)memcpy(layer->layer_data.text_layer_p->color,
		layer->window->app->tool_window.color_chooser->rgb, 3);
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

static void ChangeTextDirectionCallBack(GtkWidget* widget, LAYER* layer)
{
	if(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "radio_id")) == 0)
	{
		layer->layer_data.text_layer_p->flags &= ~(TEXT_LAYER_VERTICAL);
	}
	else
	{
		layer->layer_data.text_layer_p->flags |= TEXT_LAYER_VERTICAL;
	}
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

static void ChangeTextLayerBold(GtkWidget* widget, LAYER* layer)
{
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
	{
		layer->layer_data.text_layer_p->flags &= ~(TEXT_LAYER_BOLD);
	}
	else
	{
		layer->layer_data.text_layer_p->flags |= TEXT_LAYER_BOLD;
	}
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

static void ChangeTextLayerStyle(GtkWidget* widget, LAYER* layer)
{
	int mode = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(widget), "text-style"));
	switch(mode)
	{
	case TEXT_STYLE_NORMAL:
		layer->layer_data.text_layer_p->flags &= ~(TEXT_LAYER_ITALIC | TEXT_LAYER_OBLIQUE);
		break;
	case TEXT_STYLE_ITALIC:
		layer->layer_data.text_layer_p->flags &= ~(TEXT_LAYER_OBLIQUE);
		layer->layer_data.text_layer_p->flags |= TEXT_LAYER_ITALIC;
		break;
	case TEXT_STYLE_OBLIQUE:
		layer->layer_data.text_layer_p->flags &= ~(TEXT_LAYER_ITALIC);
		layer->layer_data.text_layer_p->flags |= TEXT_LAYER_OBLIQUE;
		break;
	}
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

GtkWidget* CreateTextLayerDetailUI(APPLICATION* app, LAYER* target, TEXT_LAYER* layer)
{
#define MAX_STR_LENGTH 256
#define UI_FONT_SIZE 8.0
	GtkWidget* vbox = gtk_vbox_new(FALSE, 0), *hbox = gtk_hbox_new(FALSE, 0);
#if MAJOR_VERSION == 1
	GtkWidget* font_selection = gtk_combo_box_new_text();
#else
	GtkWidget* font_selection = gtk_combo_box_text_new();
#endif
	GtkWidget* font_scale, *label, *button;
	GtkWidget* base_scale;
	GtkWidget* radio_buttons[3];
	GtkAdjustment* adjust;
	GList* list;
	const gchar* font_name;
	char item_str[MAX_STR_LENGTH];
	int i;

	for(i=0; i<app->num_font; i++)
	{
		font_name = pango_font_family_get_name(app->font_list[i]);
		(void)sprintf(item_str, "<span font_family=\"%s\">%s</span>",
			font_name, font_name);
#if MAJOR_VERSION == 1
		gtk_combo_box_append_text(GTK_COMBO_BOX(font_selection), item_str);
#else
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(font_selection), item_str);
#endif
	}
	list = gtk_cell_layout_get_cells(GTK_CELL_LAYOUT(font_selection));
	gtk_cell_layout_set_attributes(GTK_CELL_LAYOUT(font_selection), list->data, "markup", 0, NULL);
	gtk_widget_set_size_request(font_selection, 128, 32);
	gtk_combo_box_set_active(GTK_COMBO_BOX(font_selection), layer->font_id);
	g_signal_connect(G_OBJECT(font_selection), "changed", G_CALLBACK(ChangeTextLayerFontFamily), target);

	gtk_box_pack_start(GTK_BOX(vbox), font_selection, FALSE, TRUE, 0);

	switch(layer->base_size)
	{
	case 0:
		adjust = GTK_ADJUSTMENT(gtk_adjustment_new(layer->font_size, 0.1, 10, 0.1, 0.1, 0));
		break;
	case 1:
		adjust = GTK_ADJUSTMENT(gtk_adjustment_new(layer->font_size, 1, 100, 1, 10, 0));
		break;
	default:
		adjust = GTK_ADJUSTMENT(gtk_adjustment_new(layer->font_size, 5, 500, 1, 10, 0));
	}

	font_scale = SpinScaleNew(adjust,
		app->labels->tool_box.scale, 1);
	g_signal_connect(G_OBJECT(adjust), "value_changed",
		G_CALLBACK(ChangeTextLayerFontSize), target);

	{
		const char *mag_str[] = {"x 0.1", "x 1", "x 10"};
		label = gtk_label_new("");
		(void)sprintf(item_str, "<span font_desc=\"%.2f\">%s : </span>",
			UI_FONT_SIZE, app->labels->tool_box.base_scale);
		gtk_label_set_markup(GTK_LABEL(label), item_str);
		gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
#if MAJOR_VERSION == 1
		base_scale = gtk_combo_box_new_text();
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[0]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[1]);
		gtk_combo_box_append_text(GTK_COMBO_BOX(base_scale), mag_str[2]);
#else
		base_scale = gtk_combo_box_text_new();
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[0]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[1]);
		gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(base_scale), mag_str[2]);
#endif
		gtk_combo_box_set_active(GTK_COMBO_BOX(base_scale), layer->base_size);
		gtk_box_pack_start(GTK_BOX(hbox), base_scale, TRUE, TRUE, 0);
		gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
	}

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), font_scale, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	g_object_set_data(G_OBJECT(base_scale), "scale", font_scale);
	g_signal_connect(G_OBJECT(base_scale), "changed", G_CALLBACK(SetBrushBaseScale), &layer->base_size);


	button = gtk_button_new_with_label(app->labels->tool_box.change_text_color);
	g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(ChangeTextColorButtonCallBack), target);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(TRUE, 0);
	radio_buttons[0] = gtk_radio_button_new_with_label(
		NULL, app->labels->tool_box.text_horizonal);
	radio_buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.text_vertical
	);

	for(i=0; i<2; i++)
	{
		g_object_set_data(G_OBJECT(radio_buttons[i]), "radio_id", GINT_TO_POINTER(i));
		g_signal_connect(G_OBJECT(radio_buttons[i]), "toggled",
			G_CALLBACK(ChangeTextDirectionCallBack), target);
		gtk_box_pack_start(GTK_BOX(hbox), radio_buttons[i], FALSE, TRUE, 0);
	}

	if((layer->flags & TEXT_LAYER_VERTICAL) == 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[0]), TRUE);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[1]), TRUE);
	}

	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(FALSE, 2);
	label = gtk_label_new("");
	(void)sprintf(item_str, "<span font_desc=\"%.2f\">%s</span>",
		UI_FONT_SIZE, app->labels->tool_box.text_style);
	gtk_label_set_markup(GTK_LABEL(label), item_str);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);

	button = gtk_check_button_new_with_label(app->labels->tool_box.text_bold);
	g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(ChangeTextLayerBold), target);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), layer->flags & TEXT_LAYER_BOLD);
	gtk_box_pack_start(GTK_BOX(hbox), button, FALSE, TRUE, 0);

	hbox = gtk_hbox_new(TRUE, 0);
	radio_buttons[0] = gtk_radio_button_new_with_label(
		NULL, app->labels->tool_box.text_normal);
	radio_buttons[1] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.text_italic
	);
	radio_buttons[2] = gtk_radio_button_new_with_label(
		gtk_radio_button_get_group(GTK_RADIO_BUTTON(radio_buttons[0])),
		app->labels->tool_box.text_oblique
	);

	for(i=0; i<3; i++)
	{
		g_object_set_data(G_OBJECT(radio_buttons[i]), "text-style",
			GINT_TO_POINTER(i));
		g_signal_connect(G_OBJECT(radio_buttons[i]), "toggled",
			G_CALLBACK(ChangeTextLayerStyle), target);
	}
	gtk_box_pack_start(GTK_BOX(hbox), radio_buttons[0], FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), radio_buttons[1], FALSE, FALSE, 0);

	if((layer->flags & TEXT_LAYER_ITALIC) != 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[1]), TRUE);
	}
	else if((layer->flags & TEXT_LAYER_OBLIQUE) != 0)
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[2]), TRUE);
	}
	else
	{
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_buttons[0]), TRUE);
	}
	gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), radio_buttons[2], FALSE, TRUE, 0);

	return vbox;
}

void RenderTextLayer(DRAW_WINDOW* window, LAYER* target, TEXT_LAYER* layer)
{
#define RIGHT_MOVE_AMOUNT 0.3f
#define UPPER_MOVE_AMOUNT 0.3f
	gdouble draw_y;
	char* show_text, *str;
	size_t length, i;
	cairo_font_slant_t font_slant;
	cairo_font_weight_t font_weight;

	if(layer->text == NULL)
	{
		return;
	}

	if((layer->flags & TEXT_LAYER_ITALIC) != 0)
	{
		font_slant = CAIRO_FONT_SLANT_ITALIC;
	}
	else if((layer->flags & TEXT_LAYER_OBLIQUE) != 0)
	{
		font_slant = CAIRO_FONT_SLANT_OBLIQUE;
	}
	else
	{
		font_slant = CAIRO_FONT_SLANT_NORMAL;
	}

	if((layer->flags & TEXT_LAYER_BOLD) != 0)
	{
		font_weight = CAIRO_FONT_WEIGHT_BOLD;
	}
	else
	{
		font_weight = CAIRO_FONT_WEIGHT_NORMAL;
	}

	length = strlen(layer->text);

	(void)memset(window->mask_temp->pixels, 0, window->pixel_buf_size);
	(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
	(void)memset(target->pixels, 0, target->stride*target->height);

	cairo_save(window->mask_temp->cairo_p);

	cairo_set_source_rgb(window->mask_temp->cairo_p, layer->color[0]*DIV_PIXEL,
		layer->color[1]*DIV_PIXEL, layer->color[2]*DIV_PIXEL);
	cairo_select_font_face(
		window->mask_temp->cairo_p,
		pango_font_family_get_name(window->app->font_list[layer->font_id]),
		font_slant, font_weight
	);
	cairo_set_font_size(window->mask_temp->cairo_p, layer->font_size);

	if((layer->flags & TEXT_LAYER_VERTICAL) == 0)
	{
		draw_y = layer->y + layer->font_size;
		show_text = str = MEM_STRDUP_FUNC(layer->text);

		for(i=0; i<length; i++)
		{
			if(str[i] == '\n')
			{
				str[i] = '\0';
				cairo_move_to(window->mask_temp->cairo_p, layer->x, draw_y);
				cairo_show_text(window->mask_temp->cairo_p, show_text);
				draw_y += layer->font_size;
				show_text = &str[i+1];
			}
		}
		cairo_move_to(window->mask_temp->cairo_p, layer->x, draw_y);
		cairo_show_text(window->mask_temp->cairo_p, show_text);

		MEM_FREE_FUNC(str);
	}
	else
	{
		gdouble draw_x = layer->x + layer->width - layer->font_size;
		gchar character_buffer[8];
		guint character_size, j;
		char* next_char;
		uint32 utf8_code;
		gdouble right_move, upper_move, rotate;

		show_text = str = layer->text;
		draw_y = layer->y;

		for(i=0; i<length; i++)
		{
			right_move = upper_move = rotate = 0;

			if(show_text[i] == '\n')
			{
				draw_y = layer->y;
				draw_x -= layer->font_size;
			}
			else if(show_text[i] >= 0x20 && show_text[i] <= 0x7E)
			{
				character_buffer[0] = show_text[i];
				character_buffer[1] = '\0';

				cairo_move_to(window->mask_temp->cairo_p,
					draw_x, draw_y);
				cairo_rotate(window->mask_temp->cairo_p, (FLOAT_T)G_PI*0.5);
				cairo_show_text(window->mask_temp->cairo_p, character_buffer);
				cairo_rotate(window->mask_temp->cairo_p, - (FLOAT_T)G_PI*0.5);
				draw_y += layer->font_size * 0.7f;
			}
			else
			{
				str = (char*)(&utf8_code);
				next_char = g_utf8_next_char(&show_text[i]);
				character_size = (guint)(next_char - &show_text[i]);
				for(j=0; j<sizeof(character_buffer)/sizeof(character_buffer[0]); j++)
				{
					character_buffer[j] = 0;
				}
				for(j=0; j<character_size; j++)
				{
					character_buffer[j] = show_text[i+j];
				}
				character_buffer[character_size] = '\0';
				i += character_size - 1;

				utf8_code = 0;
#ifndef __BIG_ENDIAN
				for(j=0; j<character_size; j++)
				{
					str[character_size-j-1] = character_buffer[j];
				}
#else
				for(j=0; j<character_size; j++)
				{
					str[sizeof(guint)-character_size+j] = character_buffer[j];
				}
#endif
				if(
					// 「ぁ」「ぃ」「ぅ」「ぇ」「ぉ」
						// 「っ」「ゃ」「ゅ」「ょ」「ゎ」
					utf8_code == 0xE38181
					|| utf8_code == 0xE38183
					|| utf8_code == 0xE38185
					|| utf8_code == 0xE38187
					|| utf8_code == 0xE38189
					|| utf8_code == 0xE381A3
					|| utf8_code == 0xE38283
					|| utf8_code == 0xE38285
					|| utf8_code == 0xE38287
					|| utf8_code == 0xE3828E
					// 「ァ」「ィ」「ゥ」「ェ」「ォ」
						// 「ッ」「ャ」「ュ」「ョ」「ヮ」
					|| utf8_code == 0xE382A1
					|| utf8_code == 0xE382A3
					|| utf8_code == 0xE382A5
					|| utf8_code == 0xE382A7
					|| utf8_code == 0xE382A9
					|| utf8_code == 0xE38383
					|| utf8_code == 0xE383A3
					|| utf8_code == 0xE383A5
					|| utf8_code == 0xE383A7
					|| utf8_code == 0xE383AE
					// 「ヵ」「ヶ」
					|| utf8_code == 0xE383B5
					|| utf8_code == 0xE383B6
				)
				{
					right_move = layer->font_size * RIGHT_MOVE_AMOUNT;
					upper_move = layer->font_size * UPPER_MOVE_AMOUNT;
					draw_x += right_move;
					draw_y -= upper_move;
					upper_move *= 0.5;
				}
				else if(
					// 「、」「。」
					utf8_code == 0xE38081 || utf8_code == 0xE38082
				)
				{
					right_move = layer->font_size * RIGHT_MOVE_AMOUNT * 2;
					upper_move = layer->font_size * UPPER_MOVE_AMOUNT * 2;
					draw_x += right_move;
					draw_y -= upper_move;
					upper_move *= 0.25;
				}
				else if(
					// 「：」
					utf8_code == 0xEFBC9A
					// 「‥」
					|| utf8_code == 0xE280A5
					// 「…」
					|| utf8_code == 0xE280A6
					// 「ー」
					|| utf8_code == 0xE383BC
					// 「―」
					|| utf8_code == 0xE28095
					// 「～」
					|| utf8_code == 0xE3809C
					// 「（）」、「〔〕」、「［］」、「〈〉」、「《》」、
						// 「「」」、「『』」、「【】」
					|| utf8_code == 0xEFBC88
					|| utf8_code == 0xEFBC89
					|| utf8_code == 0xE38094
					|| utf8_code == 0xE38095
					|| utf8_code == 0xEFBCBB
					|| utf8_code == 0xEFBCBD
					|| utf8_code == 0xEFBD9B
					|| utf8_code == 0xEFBD9D
					|| utf8_code == 0xE38088
					|| utf8_code == 0xE38089
					|| utf8_code == 0xE3808A
					|| utf8_code == 0xE3808B
					|| utf8_code == 0xE3808C
					|| utf8_code == 0xE3808D
					|| utf8_code == 0xE3808E
					|| utf8_code == 0xE3808F
					|| utf8_code == 0xE38090
					|| utf8_code == 0xE38091
				)
				{
					right_move = layer->font_size * RIGHT_MOVE_AMOUNT * 0.5;
					upper_move = layer->font_size * UPPER_MOVE_AMOUNT * 3;
					rotate = (FLOAT_T)G_PI * 0.5;
					draw_x += right_move;
					draw_y -= upper_move;
					upper_move *= - 0.75;
				}
				else
				{
					// 文字サイズを取得してX座標を修正
					cairo_text_extents_t character_info;
					cairo_text_extents(window->mask_temp->cairo_p, character_buffer, &character_info);
					if(layer->font_size > character_info.width + character_info.x_bearing)
					{
						right_move = (layer->font_size - (character_info.width + character_info.x_bearing)) * 0.5;
					}
					else
					{
						right_move = ((character_info.width + character_info.x_bearing) - layer->font_size) * 0.5;
					}

					right_move -= character_info.x_bearing;
					draw_x += right_move;
				}

				cairo_move_to(window->mask_temp->cairo_p,
					draw_x - layer->font_size * 0.2f, draw_y + layer->font_size);
				cairo_rotate(window->mask_temp->cairo_p, rotate);
				cairo_show_text(window->mask_temp->cairo_p, character_buffer);
				cairo_rotate(window->mask_temp->cairo_p, - rotate);

				draw_x -= right_move;

				draw_y += layer->font_size * 1.2f - upper_move;
			}
		}
	}

	cairo_set_source_rgb(window->temp_layer->cairo_p, layer->color[0]*DIV_PIXEL,
		layer->color[1]*DIV_PIXEL, layer->color[2]*DIV_PIXEL);
	cairo_rectangle(window->temp_layer->cairo_p, layer->x, layer->y, layer->width, layer->height);
	cairo_fill(window->temp_layer->cairo_p);
	cairo_set_source_surface(target->cairo_p, window->mask_temp->surface_p, 0, 0);
	cairo_mask_surface(target->cairo_p, window->temp_layer->surface_p, 0, 0);

	cairo_restore(window->mask_temp->cairo_p);
}

void DisplayTextLayerRange(DRAW_WINDOW* window, TEXT_LAYER* layer)
{
	cairo_set_line_width(window->disp_temp->cairo_p, 1);
	cairo_set_source_rgb(window->disp_temp->cairo_p, 1, 1, 1);
	cairo_rectangle(window->disp_temp->cairo_p, layer->x * window->zoom*0.01f,
		layer->y * window->zoom*0.01f, layer->width * window->zoom*0.01f,
		layer->height * window->zoom*0.01f);
	cairo_stroke(window->disp_temp->cairo_p);
}

void OnChangeTextCallBack(GtkTextBuffer* buffer, LAYER* layer)
{
	GtkTextIter start, end;
	MEM_FREE_FUNC(layer->layer_data.text_layer_p->text);
	gtk_text_buffer_get_start_iter(buffer, &start);
	gtk_text_buffer_get_end_iter(buffer, &end);
	layer->layer_data.text_layer_p->text = MEM_STRDUP_FUNC(
		gtk_text_buffer_get_text(buffer, &start, &end, FALSE));
	layer->window->flags |= DRAW_WINDOW_UPDATE_ACTIVE_OVER;
}

/*************************************************************
* TextFieldFocusIn関数                                       *
* テキストレイヤーの編集ウィジェットがフォーカスされた時に   *
*   呼び出されるコールバック関数                             *
* 引数                                                       *
* text_field	: テキストレイヤーの編集ウィジェット         *
* focus			: イベントの情報                             *
* app			: アプリケーションを管理する構造体のアドレス *
* 返り値                                                     *
*	常にFALSE                                                *
*************************************************************/
gboolean TextFieldFocusIn(GtkWidget* text_field, GdkEventFocus* focus, APPLICATION* app)
{
	if(app->tool_window.window == NULL)
	{
		app->flags |= APPLICATION_REMOVE_ACCELARATOR;
		gtk_window_remove_accel_group(GTK_WINDOW(app->window), app->hot_key);
	}

	return FALSE;
}

/*******************************************************************
* TextFieldFocusOut関数                                            *
* テキストレイヤーの編集ウィジェットからフォーカスが移動したた時に *
*   呼び出されるコールバック関数                                   *
* 引数                                                             *
* text_field	: テキストレイヤーの編集ウィジェット               *
* focus			: イベントの情報                                   *
* app			: アプリケーションを管理する構造体のアドレス       *
* 返り値                                                           *
*	常にFALSE                                                      *
*******************************************************************/
gboolean TextFieldFocusOut(GtkWidget* text_field, GdkEventFocus* focus, APPLICATION* app)
{
	if(app->tool_window.window == NULL
		&& (app->flags & APPLICATION_REMOVE_ACCELARATOR) != 0)
	{
		app->flags &= ~(APPLICATION_REMOVE_ACCELARATOR);
		gtk_window_add_accel_group(GTK_WINDOW(app->window), app->hot_key);
	}

	return FALSE;
}

/*************************************************************
* OnDestroyTextField関数                                     *
* テキストレイヤーの編集ウィジェットが削除される時に         *
*   呼び出されるコールバック関数                             *
* 引数                                                       *
* text_field	: テキストレイヤーの編集ウィジェット         *
* app			: アプリケーションを管理する構造体のアドレス *
*************************************************************/
void OnDestroyTextField(GtkWidget* text_field, APPLICATION* app)
{
	if((app->flags & APPLICATION_REMOVE_ACCELARATOR) != 0)
	{
		app->flags &= ~(APPLICATION_REMOVE_ACCELARATOR);
		gtk_window_add_accel_group(GTK_WINDOW(app->window), app->hot_key);
	}
}

#ifdef __cplusplus
}
#endif
