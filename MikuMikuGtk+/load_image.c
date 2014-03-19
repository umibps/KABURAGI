#include <gtk/gtk.h>
#include "load_image.h"
#include "memory.h"

uint8* LoadImage(const char* utf8_path, int* width, int* height, int* channel)
{
	GdkPixbuf *pixbuf;
	uint8 *ret;

	pixbuf = gdk_pixbuf_new_from_file(utf8_path, NULL);

	if(pixbuf == NULL)
	{
		return NULL;
	}

	*width = gdk_pixbuf_get_width(pixbuf);
	*height = gdk_pixbuf_get_height(pixbuf);
	*channel = gdk_pixbuf_get_rowstride(pixbuf)/ *width;

	ret = (uint8*)MEM_ALLOC_FUNC((*width) * (*height) * (*channel));
	(void)memcpy(ret, gdk_pixbuf_get_pixels(pixbuf), (*width) * (*height) * (*channel));

	g_object_unref(pixbuf);

	return ret;
}
