// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <gtk/gtk.h>
#include "application.h"
#include "draw_window.h"
#include "display.h"
#include "image_read_write.h"
#include "utils.h"
#include "memory.h"
#include "display_filter.h"
#include "tlg6_encode.h"

#ifdef __cplusplus
extern "C" {
#endif

extern gchar* lcms_get_profile_desc(cmsHPROFILE profile);

void SaveAsPng(APPLICATION* app, DRAW_WINDOW* window, const gchar* file_name)
{
	GtkWidget* radio_button[2];
	GtkWidget* dialog = gtk_dialog_new_with_buttons(
		"",
		GTK_WINDOW(app->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL
	);
	GtkWidget* label, *spin, *hbox;
	GtkWidget* write_icc_profile = NULL;
	GtkAdjustment* adjust;
	LAYER* target;
	char str[128], compress[8];
	int stride;
	int x, y;
	int compress_value;
	uint8 *icc_profile_data = NULL;
	int32 icc_profile_size;

	if((app->flags & APPLICATION_DISPLAY_GRAY_SCALE) == 0)
	{
		radio_button[0] = gtk_radio_button_new_with_label(NULL, "24bit RGB");
		radio_button[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(
			GTK_RADIO_BUTTON(radio_button[0])), "32bit RGBA");

		if((app->flags & APPLICATION_DISPLAY_SOFT_PROOF) != 0)
		{
			write_icc_profile = gtk_check_button_new_with_label(app->labels->save.write_profile);
		}
	}
	else
	{
		radio_button[0] = gtk_radio_button_new_with_label(NULL, "8bit Gray Scale");
		radio_button[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(
			GTK_RADIO_BUTTON(radio_button[0])), "16bit Gray Scale & Alpha Channel");
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[0]), TRUE);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), radio_button[0],
		FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), radio_button[1],
		FALSE, TRUE, 0);
	hbox = gtk_hbox_new(FALSE, 0);
	(void)sprintf(str, "%s :", app->labels->save.compress);
	label = gtk_label_new(str);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, 9, 1, 1, 1));
	spin = gtk_spin_button_new(adjust, 1, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), hbox, FALSE, TRUE, 0);
	if(write_icc_profile != NULL)
	{
		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
			write_icc_profile, FALSE, TRUE, 0);
	}
	gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		(void)sprintf(compress, "%d", (int)gtk_spin_button_get_value(
			GTK_SPIN_BUTTON(spin)));
		compress_value = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));

		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_button[0])) == FALSE)
		{
			uint8* pixels;
			target = MixLayerForSave(window);

			if((app->flags & APPLICATION_DISPLAY_GRAY_SCALE) == 0)
			{
				GFile* fp = g_file_new_for_path(file_name);
				GFileOutputStream* stream =
					g_file_create(fp, G_FILE_CREATE_NONE, NULL, NULL);
				int free_data = 0;
				char *icc_name = NULL;

				if(stream == NULL)
				{
					stream = g_file_replace(fp, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);

					if(stream == NULL)
					{
						g_object_unref(fp);
						gtk_widget_destroy(dialog);
						return;
					}
				}

				(void)memset(window->temp_layer->pixels, 0xff, window->pixel_buf_size);
				cairo_set_source_surface(window->temp_layer->cairo_p, window->mixed_layer->surface_p, 0, 0);
				cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
				cairo_mask_surface(window->temp_layer->cairo_p, target->surface_p, 0, 0);
				for(x=0; x<window->width*window->height; x++)
				{
					window->temp_layer->pixels[x*4+3] = target->pixels[x*4+3];
				}

				stride = window->original_width * 4;
				pixels = window->mask_temp->pixels;

				if(write_icc_profile != NULL && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(write_icc_profile)) != FALSE)
				{
					if(window->icc_profile_data != NULL)
					{
						icc_profile_data = window->icc_profile_data;
						icc_profile_size = window->icc_profile_size;
						icc_name = lcms_get_profile_desc(window->input_icc);
					}
					else
					{
						FILE *fp = fopen(app->input_icc_path, "rb");
						size_t data_size;

						(void)fseek(fp, 0, SEEK_END);
						data_size = ftell(fp);
						rewind(fp);

						icc_profile_data = (uint8*)MEM_ALLOC_FUNC(data_size);
						(void)fread(icc_profile_data, 1, data_size, fp);

						(void)fclose(fp);

						icc_profile_size = (int32)data_size;
						free_data = 1;
						icc_name = lcms_get_profile_desc(app->input_icc);
					}

					app->display_filter.filter_func(window->temp_layer->pixels, window->temp_layer->pixels,
						window->width * window->height, app->display_filter.filter_data);
				}

				for(y=0; y<window->original_height; y++)
				{
					for(x=0; x<window->original_width; x++)
					{
						pixels[y*stride+x*window->channel] = window->temp_layer->pixels[y*target->stride+x*window->channel+2];
						pixels[y*stride+x*window->channel+1] = window->temp_layer->pixels[y*target->stride+x*window->channel+1];
						pixels[y*stride+x*window->channel+2] = window->temp_layer->pixels[y*target->stride+x*window->channel];
						pixels[y*stride+x*window->channel+3] = window->temp_layer->pixels[y*target->stride+x*window->channel+3];
					}
				}

				WritePNGDetailData((void*)stream, (stream_func)FileWrite, NULL, pixels,
					window->original_width, window->original_height, stride, 4, 0, compress_value,
						window->resolution, icc_name, icc_profile_data, icc_profile_size);

				g_object_unref(stream);
				g_object_unref(fp);
				g_free(icc_name);

				if(free_data != 0)
				{
					MEM_FREE_FUNC(icc_profile_data);
				}
			}
			else
			{
				int save_stride = window->original_width * 2;

				GFile* fp = g_file_new_for_path(file_name);
				GFileOutputStream* stream =
					g_file_create(fp, G_FILE_CREATE_NONE, NULL, NULL);

				if(stream == NULL)
				{
					stream = g_file_replace(fp, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);

					if(stream == NULL)
					{
						g_object_unref(fp);
						gtk_widget_destroy(dialog);
						return;
					}
				}

				pixels = (uint8*)MEM_ALLOC_FUNC(save_stride*window->original_height*2);
				app->display_filter.filter_func(target->pixels, target->pixels,
					target->width*target->height, NULL);

				for(y=0; y<window->original_height; y++)
				{
					for(x=0; x<window->original_width; x++)
					{
						pixels[save_stride*y+x*2] = target->pixels[target->stride*y+x*4];
						pixels[save_stride*y+x*2+1] = target->pixels[target->stride*y+x*4+3];
					}
				}

				WritePNGDetailData(stream, (stream_func)FileWrite, NULL, pixels, window->original_width,
					window->original_height, save_stride, 2, 0, compress_value, window->resolution,
						NULL, NULL, 0);

				MEM_FREE_FUNC(pixels);
				g_object_unref(fp);
				g_object_unref(stream);
			}
			DeleteLayer(&target);
		}
		else
		{
			uint8* pixels;
			target = window->mixed_layer;

			if((app->flags & APPLICATION_DISPLAY_GRAY_SCALE) == 0)
			{
				GFile* fp = g_file_new_for_path(file_name);
				GFileOutputStream* stream =
					g_file_create(fp, G_FILE_CREATE_NONE, NULL, NULL);
				int free_data = 0;
				char *icc_name = NULL;

				stride = window->original_width * 3;
				pixels = window->mask_temp->pixels;

				if(stream == NULL)
				{
					stream = g_file_replace(fp, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);

					if(stream == NULL)
					{
						g_object_unref(fp);
						gtk_widget_destroy(dialog);
						return;
					}
				}

				if(write_icc_profile != NULL && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(write_icc_profile)) != FALSE)
				{
					if(window->icc_profile_data != NULL)
					{
						icc_profile_data = window->icc_profile_data;
						icc_profile_size = window->icc_profile_size;
						icc_name = lcms_get_profile_desc(window->input_icc);
					}
					else
					{
						FILE *fp = fopen(app->input_icc_path, "rb");
						size_t data_size;

						(void)fseek(fp, 0, SEEK_END);
						data_size = ftell(fp);
						rewind(fp);

						icc_profile_data = (uint8*)MEM_ALLOC_FUNC(data_size);
						(void)fread(icc_profile_data, 1, data_size, fp);

						(void)fclose(fp);

						icc_profile_size = (int32)data_size;
						free_data = 1;
						icc_name = lcms_get_profile_desc(app->input_icc);
					}
					
					app->display_filter.filter_func(window->temp_layer->pixels, window->temp_layer->pixels,
						window->width * window->height, app->display_filter.filter_data);
				}

				(void)memset(window->mask_temp->pixels, 0, window->pixel_buf_size);

				for(y=0; y<window->original_height; y++)
				{
					for(x=0; x<window->original_width; x++)
					{
						pixels[y*stride+x*3] = target->pixels[y*target->stride+x*target->channel+2];
						pixels[y*stride+x*3+1] = target->pixels[y*target->stride+x*target->channel+1];
						pixels[y*stride+x*3+2] = target->pixels[y*target->stride+x*target->channel];
					}
				}

				WritePNGDetailData((void*)stream, (stream_func)FileWrite, NULL, pixels,
					window->original_width, window->original_height, stride, 3, 0, compress_value,
						window->resolution, icc_name, icc_profile_data, icc_profile_size);

				g_object_unref(fp);
				g_object_unref(stream);
				g_free(icc_name);
			}
			else
			{
				GFile* fp = g_file_new_for_path(file_name);
				GFileOutputStream* stream =
					g_file_create(fp, G_FILE_CREATE_NONE, NULL, NULL);

				if(stream == NULL)
				{
					stream = g_file_replace(fp, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);

					if(stream == NULL)
					{
						g_object_unref(fp);
						gtk_widget_destroy(dialog);
						return;
					}
				}

				pixels = (uint8*)MEM_ALLOC_FUNC(window->original_width*window->original_height);

				for(y=0; y<window->original_height; y++)
				{
					for(x=0; x<window->original_width; x++)
					{
						pixels[y*window->original_width+x] = target->pixels[y*target->stride+x*target->channel];
					}
				}

				WritePNGDetailData(stream, (stream_func)FileWrite, NULL, pixels, window->original_width,
					window->original_height, window->original_width, 1, 0, compress_value, window->resolution,
						NULL, NULL, 0);

				MEM_FREE_FUNC(pixels);
				g_object_unref(fp);
				g_object_unref(stream);
			}
		}
	}

	gtk_widget_destroy(dialog);
}

void SaveAsJpeg(APPLICATION* app, DRAW_WINDOW* window, const gchar* file_name)
{
	GtkWidget* dialog = gtk_dialog_new_with_buttons(
		"",
		GTK_WINDOW(app->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL
	);
	GtkWidget* label, *spin, *hbox;
	GtkWidget *optimize;
	GtkWidget *write_icc_profile = NULL;
	GtkAdjustment* adjust;
	LAYER* target;
	char str[128];
	uint8* pixels;
	uint8 *write_pixels;
	uint8 *icc_profile_data = NULL;
	int icc_profile_data_size;
	int stride;
	int x, y;

	hbox = gtk_hbox_new(FALSE, 0);
	(void)sprintf(str, "%s :", app->labels->save.quality);
	label = gtk_label_new(str);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(100, 0, 100, 1, 10, 10));
	spin = gtk_spin_button_new(adjust, 1, 0);
	gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), spin, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		hbox, FALSE, TRUE, 0);
	optimize = gtk_check_button_new_with_label("Optimize");
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		optimize, FALSE, TRUE, 0);

	if((app->flags & APPLICATION_DISPLAY_SOFT_PROOF) != 0)
	{
		write_icc_profile = gtk_check_button_new_with_label(app->labels->save.write_profile);
		gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
			write_icc_profile, FALSE, TRUE, 0);
	}

	gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		eIMAGE_DATA_TYPE data_type = IMAGE_DATA_RGB;
		int channel = 3;

		target = window->mixed_layer;
		stride = window->width * 3;
		write_pixels = pixels = window->mask_temp->pixels;

		for(y=0; y<window->original_height; y++)
		{
			for(x=0; x<window->original_width; x++)
			{
				pixels[y*stride+x*3] = target->pixels[y*target->stride+x*target->channel+2];
				pixels[y*stride+x*3+1] = target->pixels[y*target->stride+x*target->channel+1];
				pixels[y*stride+x*3+2] = target->pixels[y*target->stride+x*target->channel];
			}
		}

		if(write_icc_profile != NULL)
		{
			LAYER *src_layer = MixLayerForSaveWithBackGround(window);
			cmsHTRANSFORM *color_transform = NULL;
			cmsColorSpaceSignature color_space;

			color_space = cmsGetColorSpace(app->output_icc);
			switch(color_space)
			{
			case cmsSigCmykData:
				color_transform = cmsCreateTransform(app->input_icc, TYPE_BGRA_8,
					app->output_icc, TYPE_CMYK_8_REV, INTENT_RELATIVE_COLORIMETRIC, 0);
				if(color_transform != NULL)
				{
					write_pixels = window->temp_layer->pixels;
					cmsDoTransform(color_transform, src_layer->pixels,
						window->temp_layer->pixels, window->width * window->height);
					channel = 4;
					data_type = IMAGE_DATA_CMYK;
				}
				break;
			default:
				color_transform = cmsCreateTransform(app->input_icc, TYPE_BGRA_8,
					app->output_icc, TYPE_RGB_8, INTENT_RELATIVE_COLORIMETRIC, 0);
				if(color_transform != NULL)
				{
					write_pixels = window->temp_layer->pixels;
					cmsDoTransform(color_transform, src_layer->pixels,
						window->temp_layer->pixels, window->width * window->height);
				}
			}
			if(color_transform != NULL)
			{
				cmsDeleteTransform(color_transform);
			}

			DeleteLayer(&src_layer);

			if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(write_icc_profile)) != FALSE)
			{
				FILE *fp = fopen(app->output_icc_path, "rb");
				if(fp != NULL)
				{
					(void)fseek(fp, 0, SEEK_END);
					icc_profile_data_size = (int)ftell(fp);
					rewind(fp);
					icc_profile_data = (uint8*)MEM_ALLOC_FUNC(icc_profile_data_size);
					(void)fread(icc_profile_data, 1, icc_profile_data_size, fp);
					(void)fclose(fp);
				}
			}
		}

		WriteJpegFile(file_name, write_pixels, window->width, window->height, channel,
			data_type, (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin)),
				(int)gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(optimize)), icc_profile_data,
					icc_profile_data_size, window->resolution
		);

		MEM_FREE_FUNC(icc_profile_data);
	}

	gtk_widget_destroy(dialog);
}

/*****************************************
* SaveAsOriginalFormat関数               *
* 独自形式でデータを書き出す             *
* 引数                                   *
* app		: アプリケーション全体の情報 *
* window	: 描画領域の情報             *
* file_name	: 書きだすファイルパス       *
*****************************************/
void SaveAsOriginalFormat(APPLICATION* app, DRAW_WINDOW* window, const char* file_name)
{
	FILE *fp = fopen(file_name, "wb");
	// ステータスバーのメッセージID
	guint context_id, message_id;

	if(fp == NULL)
	{
		return;
	}

	// メッセージを表示
	context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(app->status_bar), "Saving");
	message_id = gtk_statusbar_push(GTK_STATUSBAR(app->status_bar),
		context_id, app->labels->window.saving);
	gtk_widget_queue_draw(app->status_bar);

	WriteOriginalFormat((void*)fp, (stream_func)fwrite, window, 1, app->preference.compress);

	(void)fclose(fp);

	// メッセージ表示を終了
	gtk_statusbar_remove(GTK_STATUSBAR(app->status_bar), context_id, message_id);
}

/*****************************************
* SaveAsPhotoShopDocument関数            *
* PSD形式でデータを書き出す              *
* 引数                                   *
* app		: アプリケーション全体の情報 *
* window	: 描画領域の情報             *
* file_name	: 書きだすファイルパス       *
*****************************************/
void SaveAsPhotoShopDocument(APPLICATION* app, DRAW_WINDOW* window, const char* file_name)
{
	FILE *fp = fopen(file_name, "wb");

	if(fp == NULL)
	{
		return;
	}

	WritePhotoShopDocument((void*)fp, (stream_func)fwrite, (seek_func)fseek,
		(long (*)(void*))ftell, window);

	(void)fclose(fp);
}

/*****************************************
* SaveAsTiff関数                         *
* TIFF形式でデータを書き出す             *
* 引数                                   *
* app		: アプリケーション全体の情報 *
* window	: 描画領域の情報             *
* file_name	: 書きだすファイルパス       *
*****************************************/
void SaveAsTiff(APPLICATION* app, DRAW_WINDOW* window, const char* file_name)
{
	GtkWidget *dialog;
	GtkWidget *vbox;
	GtkWidget *write_alpha;
	GtkWidget *compress;
	GtkWidget *write_profile = NULL;
	uint8 *profile_data = NULL;
	size_t file_size = 0;

	dialog = gtk_dialog_new_with_buttons(
		"",
		GTK_WINDOW(app->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL
	);

	vbox = gtk_vbox_new(FALSE, 0);
	write_alpha = gtk_check_button_new_with_label(app->labels->save.write_alpha);
	gtk_box_pack_start(GTK_BOX(vbox), write_alpha, FALSE, FALSE, 3);

	compress = gtk_check_button_new_with_label(app->labels->save.compress);
	gtk_box_pack_start(GTK_BOX(vbox), compress, FALSE, FALSE, 3);

	if((app->flags & APPLICATION_DISPLAY_SOFT_PROOF) != 0)
	{
		write_profile = gtk_check_button_new_with_label(app->labels->save.write_profile);
		gtk_box_pack_start(GTK_BOX(vbox), write_profile, FALSE, FALSE, 3);
	}

	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		vbox, FALSE, FALSE, 0);
	gtk_widget_show_all(dialog);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		if(write_profile != NULL && app->output_icc_path != NULL)
		{
			FILE *fp = fopen(app->output_icc_path, "rb");
			if(fp != NULL)
			{
				(void)fseek(fp, 0, SEEK_END);
				file_size = ftell(fp);
				rewind(fp);

				profile_data = (uint8*)MEM_ALLOC_FUNC(file_size);
				(void)fread(profile_data, 1, file_size, fp);

				(void)fclose(fp);
			}
		}

		WriteTiff(file_name, window, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(write_alpha)),
			gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(compress)), profile_data, (guint32)file_size);

		MEM_FREE_FUNC(profile_data);
	}

	gtk_widget_destroy(dialog);
}

/*****************************************
* SaveAsTlg関数                          *
* TLG形式で画像を書き出す                *
* 引数                                   *
* app		: アプリケーション全体の情報 *
* window	: 描画領域の情報             *
* file_name	: 書きだすファイルパス       *
*****************************************/
void SaveAsTlg(APPLICATION* app, DRAW_WINDOW* window, const gchar* file_name)
{
	GtkWidget *radio_button[2];
	GtkWidget *rgb_button;
	GtkWidget *dialog = gtk_dialog_new_with_buttons(
		"",
		GTK_WINDOW(app->window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
		GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
		GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL
	);
	LAYER *target;
	int stride;
	int x, y;

	if((app->flags & APPLICATION_DISPLAY_GRAY_SCALE) == 0)
	{
		radio_button[0] = gtk_radio_button_new_with_label(NULL, "24bit RGB");
		radio_button[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(
			GTK_RADIO_BUTTON(radio_button[0])), "32bit RGBA");
	}
	else
	{
		radio_button[0] = gtk_radio_button_new_with_label(NULL, "8bit Gray Scale");
		radio_button[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(
			GTK_RADIO_BUTTON(radio_button[0])), "16bit Gray Scale & Alpha Channel");
	}
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(radio_button[0]), TRUE);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), radio_button[0],
		FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), radio_button[1],
		FALSE, TRUE, 0);

	rgb_button = gtk_check_button_new_with_label("Save as RGB (not BGR)");
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), rgb_button,
		FALSE, TRUE, 0);

	gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
	{
		FILE *fp = fopen(file_name, "wb");

		if(fp == NULL)
		{
			return;
		}

		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(radio_button[0])) == FALSE)
		{
			uint8* pixels;
			target = MixLayerForSave(window);

			if((app->flags & APPLICATION_DISPLAY_GRAY_SCALE) == 0)
			{
				(void)memset(window->temp_layer->pixels, 0xff, window->pixel_buf_size);
				cairo_set_source_surface(window->temp_layer->cairo_p, window->mixed_layer->surface_p, 0, 0);
				cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
				cairo_mask_surface(window->temp_layer->cairo_p, target->surface_p, 0, 0);
				for(x=0; x<window->width*window->height; x++)
				{
					window->temp_layer->pixels[x*4+3] = target->pixels[x*4+3];
				}

				stride = window->original_width * 4;
				pixels = (uint8*)MEM_ALLOC_FUNC(stride * window->original_height);

				for(y=0; y<window->original_height; y++)
				{
					for(x=0; x<window->original_width; x++)
					{

#if 1 //defined(DISPLAY_BGR) && DISPLAY_BGR != 0
						if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rgb_button)) != FALSE)
						{
							pixels[y*stride+x*window->channel] = window->temp_layer->pixels[y*target->stride+x*window->channel+2];
							pixels[y*stride+x*window->channel+1] = window->temp_layer->pixels[y*target->stride+x*window->channel+1];
							pixels[y*stride+x*window->channel+2] = window->temp_layer->pixels[y*target->stride+x*window->channel];
						}
						else
						{
							pixels[y*stride+x*window->channel] = window->temp_layer->pixels[y*target->stride+x*window->channel];
							pixels[y*stride+x*window->channel+1] = window->temp_layer->pixels[y*target->stride+x*window->channel+1];
							pixels[y*stride+x*window->channel+2] = window->temp_layer->pixels[y*target->stride+x*window->channel+2];
						}
#else
						if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rgb_button)) != FALSE)
						{
							pixels[y*stride+x*window->channel] = window->temp_layer->pixels[y*target->stride+x*window->channel];
							pixels[y*stride+x*window->channel+1] = window->temp_layer->pixels[y*target->stride+x*window->channel+1];
							pixels[y*stride+x*window->channel+2] = window->temp_layer->pixels[y*target->stride+x*window->channel+2];
						}
						else
						{
							pixels[y*stride+x*window->channel] = window->temp_layer->pixels[y*target->stride+x*window->channel+2];
							pixels[y*stride+x*window->channel+1] = window->temp_layer->pixels[y*target->stride+x*window->channel+1];
							pixels[y*stride+x*window->channel+2] = window->temp_layer->pixels[y*target->stride+x*window->channel];
						}
#endif
						pixels[y*stride+x*window->channel+3] = window->temp_layer->pixels[y*target->stride+x*window->channel+3];
					}
				}

				TLG6Encode(pixels, window->original_width, window->original_height, 4, (void*)fp,
					(size_t (*)(const void*, size_t, size_t, void*))fwrite);

				MEM_FREE_FUNC(pixels);
			}
			else
			{
				int save_stride = window->original_width * 2;

				pixels = (uint8*)MEM_ALLOC_FUNC(save_stride*window->original_height*2);
				app->display_filter.filter_func(target->pixels, target->pixels,
					target->width*target->height, NULL);

				for(y=0; y<window->original_height; y++)
				{
					for(x=0; x<window->original_width; x++)
					{
						pixels[save_stride*y+x*2] = target->pixels[target->stride*y+x*4];
						pixels[save_stride*y+x*2+1] = target->pixels[target->stride*y+x*4+3];
					}
				}

				TLG6Encode(pixels, window->original_width, window->original_height, 2, (void*)fp,
					(size_t (*)(const void*, size_t, size_t, void*))fwrite);

				MEM_FREE_FUNC(pixels);
			}
			DeleteLayer(&target);
		}
		else
		{
			uint8* pixels;
			target = window->mixed_layer;

			if((app->flags & APPLICATION_DISPLAY_GRAY_SCALE) == 0)
			{
				stride = window->original_width * 4;
				pixels = (uint8*)MEM_ALLOC_FUNC(stride * window->original_height);

				for(y=0; y<window->original_height; y++)
				{
					for(x=0; x<window->original_width; x++)
					{
#if 1 //defined(DISPLAY_BGR) && DISPLAY_BGR != 0
						if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rgb_button)) != FALSE)
						{
							pixels[y*stride+x*3] = target->pixels[y*target->stride+x*target->channel+2];
							pixels[y*stride+x*3+1] = target->pixels[y*target->stride+x*target->channel+1];
							pixels[y*stride+x*3+2] = target->pixels[y*target->stride+x*target->channel];
						}
						else
						{
							pixels[y*stride+x*3] = target->pixels[y*target->stride+x*target->channel];
							pixels[y*stride+x*3+1] = target->pixels[y*target->stride+x*target->channel+1];
							pixels[y*stride+x*3+2] = target->pixels[y*target->stride+x*target->channel+2];
						}
#else
						if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(rgb_button)) != FALSE)
						{
							pixels[y*stride+x*3] = target->pixels[y*target->stride+x*target->channel];
							pixels[y*stride+x*3+1] = target->pixels[y*target->stride+x*target->channel+1];
							pixels[y*stride+x*3+2] = target->pixels[y*target->stride+x*target->channel+2];
						}
						else
						{
							pixels[y*stride+x*3] = target->pixels[y*target->stride+x*target->channel+2];
							pixels[y*stride+x*3+1] = target->pixels[y*target->stride+x*target->channel+1];
							pixels[y*stride+x*3+2] = target->pixels[y*target->stride+x*target->channel];
						}
#endif
					}
				}

				TLG6Encode(pixels, window->original_width, window->original_height, 3, (void*)fp,
					(size_t (*)(const void*, size_t, size_t, void*))fwrite);

				MEM_FREE_FUNC(pixels);
			}
			else
			{
				pixels = (uint8*)MEM_ALLOC_FUNC(window->original_width*window->original_height);

				for(y=0; y<window->original_height; y++)
				{
					for(x=0; x<window->original_width; x++)
					{
						pixels[y*window->original_width+x] = target->pixels[y*target->stride+x*target->channel];
					}
				}

				TLG6Encode(pixels, window->original_width, window->original_height, 1, (void*)fp,
					(size_t (*)(const void*, size_t, size_t, void*))fwrite);

				MEM_FREE_FUNC(pixels);
			}
		}

		(void)fclose(fp);
	}

	gtk_widget_destroy(dialog);
}

/***************************************************
* IsSupportFileType関数                            *
* 書き出しをサポートしているファイルタイプかを判定 *
* 引数                                             *
* extention	: 拡張子の文字列                       *
* 返り値                                           *
*	サポートしている:1	サポートしていない:0       *
***************************************************/
int IsSupportFileType(const char* extention)
{
	if(
		StringCompareIgnoreCase(extention, "kab") != 0
		&& StringCompareIgnoreCase(extention, "png") != 0
		&& StringCompareIgnoreCase(extention, "jpg") != 0
		&& StringCompareIgnoreCase(extention, "tif") != 0
		&& StringCompareIgnoreCase(extention, "tiff") != 0
		&& StringCompareIgnoreCase(extention, "tlg") != 0
	)
	{
		return 0;
	}

	return 1;
}

/*********************************************************
* Save関数                                               *
* 書き出し実行                                           *
* 引数                                                   *
* app		: アプリケーションを管理する構造体のアドレス *
* window	: 描画領域の情報                             *
* file_name	: 保存するファイルパス                       *
* file_type	: 保存するファイル形式の拡張子               *
* 返り値                                                 *
*	書き出したファイルのパス                             *
*********************************************************/
const char* Save(
	APPLICATION* app,
	DRAW_WINDOW* window,
	const gchar* file_name,
	const char* file_type
)
{
	// ファイルパス作成用
	char file_path[4096];
	// ファイル拡張子
	const gchar* extention;
	// 作業用ポインタ
	const gchar* str = file_name;
	// OS側の文字コードのパス
	gchar *system_path;

	// 拡張子を取得
	extention = str;
	while(*str != '\0')
	{
		if(*str == '.')
		{
			extention = str;
		}

		str = g_utf8_next_char(str);
	}

	// ファイルパスを作成
	if(StringCompareIgnoreCase(extention+1, file_type) != 0)
	{
		(void)sprintf(file_path, "%s.%s", file_name, file_type);
	}
	else
	{
		(void)strcpy(file_path, file_name);
	}

	system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);

	// 拡張子で書き出し方法を切り替え
	if(StringCompareIgnoreCase(file_type, "kab") == 0)
	{	// 独自形式
		SaveAsOriginalFormat(app, window, system_path);
	}
	else if(StringCompareIgnoreCase(file_type, "png") == 0)
	{	// PNG
		SaveAsPng(app, window, file_path);
	}
	else if(StringCompareIgnoreCase(file_type, "jpg") == 0)
	{	// JPEG
		SaveAsJpeg(app, window, system_path);
	}
	else if(StringCompareIgnoreCase(file_type, "psd") == 0)
	{	// PSD
		SaveAsPhotoShopDocument(app, window, system_path);
	}
	else if(StringCompareIgnoreCase(file_type, "tif") == 0)
	{	// TIFF
		SaveAsTiff(app, window, system_path);
	}
	else if(StringCompareIgnoreCase(file_type, "tlg") == 0)
	{	// TLG
		SaveAsTlg(app, window, system_path);
	}

	// 保存したのでフラグを降ろす
	window->history.flags &= ~(HISTORY_UPDATED);

	// バックアップのタイマーをリセット
	if(window->auto_save_timer != NULL)
	{
		g_timer_start(window->auto_save_timer);
	}

	g_free(system_path);

	return MEM_STRDUP_FUNC(file_path);
}

#ifdef __cplusplus
}
#endif
