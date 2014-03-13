#include <string.h>
#include "printer.h"
#include "memory.h"
#include "configure.h"

#ifdef __cplusplus
extern "C" {
#endif

static void BeginPrint(
	GtkPrintOperation* printer,
	GtkPrintContext* context,
	DRAW_WINDOW* window
)
{
	gtk_print_operation_set_n_pages(printer, 1);
}

static void DrawPage(
	GtkPrintOperation* operation,
	GtkPrintContext* context,
	gint page_nr,
	DRAW_WINDOW* window
)
{
	cairo_t *cairo_p;
	gdouble width, height;
	gdouble zoom_x, zoom_y, zoom;

	window->app->print_pixels = (uint8*)MEM_ALLOC_FUNC(window->pixel_buf_size);
	window->app->print_surface = cairo_image_surface_create_for_data(window->app->print_pixels,
		CAIRO_FORMAT_ARGB32, window->width, window->height, window->stride);
#if 0 //defined(DISPLAY_BGR) && DISPLAY_BGR != 0
	{
		int i;
		for(i=0; i<window->width*window->height; i++)
		{
			window->app->print_pixels[i*4] = window->mixed_layer->pixels[i*4+2];
			window->app->print_pixels[i*4+1] = window->mixed_layer->pixels[i*4+1];
			window->app->print_pixels[i*4+2] = window->mixed_layer->pixels[i*4];
			window->app->print_pixels[i*4+3] = window->mixed_layer->pixels[i*4+3];
		}
	}
#else
	(void)memcpy(window->app->print_pixels, window->mixed_layer->pixels, window->pixel_buf_size);
#endif

	cairo_p = gtk_print_context_get_cairo_context(context);
	width = gtk_print_context_get_width(context);
	height = gtk_print_context_get_height(context);

	zoom_x = width / window->original_width;
	zoom_y = height / window->original_height;

	if(zoom_x < zoom_y)
	{
		zoom =  zoom_x;
	}
	else
	{
		zoom = zoom_y;
	}

	cairo_scale(cairo_p, zoom, zoom);
	cairo_set_source_surface(cairo_p, window->app->print_surface, 0, 0);
	cairo_paint(cairo_p);
}

/*****************************************************
* ExecutePrint関数                                   *
* 印刷を実行                                         *
* 引数                                               *
* app	: アプリケーションを管理する構造体のアドレス *
*****************************************************/
void ExecutePrint(APPLICATION* app)
{
	GtkPrintOperation *printer;
	GtkPrintOperationResult result;

	printer = gtk_print_operation_new();

	if(app->print_settings != NULL)
	{
		gtk_print_operation_set_print_settings(
			printer, app->print_settings);
	}

	g_signal_connect(G_OBJECT(printer), "begin_print",
		G_CALLBACK(BeginPrint), app->draw_window[app->active_window]);
	g_signal_connect(G_OBJECT(printer), "draw_page",
		G_CALLBACK(DrawPage), app->draw_window[app->active_window]);

	result = gtk_print_operation_run(printer,
		GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG, GTK_WINDOW(app->window), NULL);

	if(result == GTK_PRINT_OPERATION_RESULT_APPLY)
	{
		if(app->print_pixels != NULL)
		{
			MEM_FREE_FUNC(app->print_pixels);
			cairo_surface_destroy(app->print_surface);
			app->print_surface = NULL;
			app->print_pixels = NULL;
		}

		if(app->print_settings != NULL)
		{
			g_object_unref(app->print_settings);
		}
		app->print_settings = g_object_ref(
			gtk_print_operation_get_print_settings(printer));
	}

	g_object_unref(printer);
}

#ifdef __cplusplus
}
#endif
