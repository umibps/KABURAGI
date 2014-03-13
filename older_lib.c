#include "configure.h"

#if defined(BUILD64BIT) && BUILD64BIT != 0

#include <gtk/gtk.h>


/**
 * gdk_window_get_width:
 * @window: a #GdkWindow
 *
 * Returns the width of the given @window.
 *
 * On the X11 platform the returned size is the size reported in the
 * most-recently-processed configure event, rather than the current
 * size on the X server.
 *
 * Returns: The width of @window
 *
 * Since: 2.24
 */
int
gdk_window_get_width (GdkWindow *window)
{
  gint width, height;

  g_return_val_if_fail (GDK_IS_WINDOW (window), 0);

  gdk_drawable_get_size (GDK_DRAWABLE (window), &width, &height);

  return width;
}

/**
 * gdk_window_get_height:
 * @window: a #GdkWindow
 *
 * Returns the height of the given @window.
 *
 * On the X11 platform the returned size is the size reported in the
 * most-recently-processed configure event, rather than the current
 * size on the X server.
 *
 * Returns: The height of @window
 *
 * Since: 2.24
 */
int
gdk_window_get_height (GdkWindow *window)
{
  gint width, height;

  g_return_val_if_fail (GDK_IS_WINDOW (window), 0);

  gdk_drawable_get_size (GDK_DRAWABLE (window), &width, &height);

  return height;
}

#endif
