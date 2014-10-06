#include <gtk/gtk.h>
#include "load_image.h"
#include "image_read_write.h"
#include "utils.h"
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
	size_t length = strlen(utf8_path);

	if(StringCompareIgnoreCase(&utf8_path[length-4], ".dds") == 0)
	{
		char *system_path = LocaleFromUTF8(utf8_path);
		FILE *fp = fopen(system_path, "rb");
		uint8 *pixels;
		size_t data_size;

		if(fp == NULL)
		{
			MEM_FREE_FUNC(system_path);
			return NULL;
		}

		(void)fseek(fp, 0, SEEK_END);
		data_size = ftell(fp);
		rewind(fp);

		pixels = ReadDdsStream((void*)fp, (stream_func_t)fread,
			(seek_func_t)fseek, data_size, width, height, channel);

		MEM_FREE_FUNC(system_path);
		(void)fclose(fp);

		return pixels;
	}

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
