#include <gtk/gtk.h>
#include "load_image.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

uint8* LoadImage(const char* utf8_path, int* width, int* height, int* channel)
{
	GdkPixbuf *pixbuf;
	uint8 *ret;
	int mod4;
	int need_resize = FALSE;

	pixbuf = gdk_pixbuf_new_from_file(utf8_path, NULL);

	if(pixbuf == NULL)
	{
		return NULL;
	}

	*width = gdk_pixbuf_get_width(pixbuf);
	*height = gdk_pixbuf_get_height(pixbuf);
	*channel = gdk_pixbuf_get_rowstride(pixbuf) / *width;

	mod4 = *width % 4;
	if(mod4 != 0)
	{
		*width += 4 - mod4;
		need_resize = TRUE;
	}
	mod4 = *height % 4;
	if(mod4 != 0)
	{
		*height += 4 - mod4;
		need_resize = TRUE;
	}

	if(need_resize != FALSE)
	{
		GdkPixbuf *old_buf = pixbuf;
		pixbuf = gdk_pixbuf_scale_simple(pixbuf, *width, *height, GDK_INTERP_NEAREST);
		g_object_unref(old_buf);
	}

	ret = (uint8*)MEM_ALLOC_FUNC((*width) * (*height) * (*channel));
	(void)memcpy(ret, gdk_pixbuf_get_pixels(pixbuf), (*width) * (*height) * (*channel));

	g_object_unref(pixbuf);

	return ret;
}

#ifdef __cplusplus
}
#endif
