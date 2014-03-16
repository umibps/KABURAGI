#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include "application.h"
#include "pmx_model.h"
#include "tbb.h"
#include "memory.h"
#include "ui.h"
#include "mikumikugtk.h"

// Visual Studioならライブラリの指定
#ifdef _MSC_VER
# pragma comment(lib, "OpenGL32.lib")
# pragma comment(lib, "GlU32.lib")

# ifdef _DEBUG
#  pragma comment(lib, "BulletCollision_vs2008_debug.lib")
#  pragma comment(lib, "BulletDynamics_vs2008_debug.lib")
#  pragma comment(lib, "BulletSoftBody_vs2008_debug.lib")
#  pragma comment(lib, "ConvexDecomposition_vs2008_debug.lib")
#  pragma comment(lib, "LinearMath_vs2008_debug.lib")
#  pragma comment(lib, "OpenGLSupport_vs2008_debug.lib")
# else
#  pragma comment(lib, "BulletCollision_vs2008.lib")
#  pragma comment(lib, "BulletDynamics_vs2008.lib")
#  pragma comment(lib, "BulletSoftBody_vs2008.lib")
#  pragma comment(lib, "ConvexDecomposition_vs2008.lib")
#  pragma comment(lib, "LinearMath_vs2008.lib")
#  pragma comment(lib, "OpenGLSupport_vs2008.lib")
# endif

# pragma comment(lib, "atk-1.0.lib")
# pragma comment(lib, "cairo.lib")
# pragma comment(lib, "expat.lib")
# pragma comment(lib, "fontconfig.lib")
# pragma comment(lib, "freetype.lib")
# pragma comment(lib, "gailutil.lib")
# pragma comment(lib, "gdk_pixbuf-2.0.lib")
# pragma comment(lib, "gdk-win32-2.0.lib")
# pragma comment(lib, "gio-2.0.lib")
# pragma comment(lib, "glib-2.0.lib")
# pragma comment(lib, "gmodule-2.0.lib")
# pragma comment(lib, "gobject-2.0.lib")
# pragma comment(lib, "gthread-2.0.lib")
# pragma comment(lib, "gtk-win32-2.0.lib")
# pragma comment(lib, "intl.lib")
# pragma comment(lib, "libpng.lib")
# pragma comment(lib, "pango-1.0.lib")
# pragma comment(lib, "pangocairo-1.0.lib")
# pragma comment(lib, "pangoft2-1.0.lib")
# pragma comment(lib, "pangowin32-1.0.lib")
#  if defined(_M_X64) || defined(_M_IA64)
#   pragma comment(lib, "zdll.lib")
#  else
#   pragma comment(lib, "zlib.lib")

#  ifdef _DEBUG
#   pragma comment(linker, "/NODEFAULTLIB:LIBCMT")
#  else
//#   pragma comment(linker, "/NODEFAULTLIB:LIBCMT")
#  endif

#  endif

// デバッグモードでなければコマンドプロンプトは表示しない
# ifndef _DEBUG
#  pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
# endif

#endif

#if 1

int Timer(GtkWidget* widget)
{
	gtk_widget_queue_draw(widget);
	return TRUE;
}

int main(int argc, char** argv)
{
	APPLICATION *mikumiku;
	GtkWidget *window;
	GtkWidget *main_box;
	GtkWidget *box;
	void *tbb;
#if defined(_DEBUG) && defined(CHECK_MEMORY_POOL) && CHECK_MEMORY_POOL != 0
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF | _CrtSetDbgFlag(_CRTDBG_REPORT_FLAG));
	//_CrtSetBreakAlloc(30354);
	//_CrtSetBreakAlloc(11170);
#endif

	tbb = TbbObjectNew();

	// GTK+の初期化
	gtk_init(&argc, &argv);
	// OpenGLの初期化
	gtk_gl_init(&argc, &argv);

	mikumiku = MikuMikuGtkNew(410, 692, TRUE, NULL);
	(void)g_signal_connect(mikumiku->projects[mikumiku->active_project]->widgets.drawing_area,
		"expose-event", G_CALLBACK(ProjectDisplayEvent), mikumiku->projects[mikumiku->active_project]);
	(void)g_signal_connect(mikumiku->projects[mikumiku->active_project]->widgets.drawing_area,
		"button-press-event", G_CALLBACK(MouseButtonPressEvent), mikumiku->projects[mikumiku->active_project]);
	(void)g_signal_connect(mikumiku->projects[mikumiku->active_project]->widgets.drawing_area,
		"motion-notify-event", G_CALLBACK(MouseMotionEvent), mikumiku->projects[mikumiku->active_project]);
	(void)g_signal_connect(mikumiku->projects[mikumiku->active_project]->widgets.drawing_area,
		"button-release-event", G_CALLBACK(MouseButtonReleaseEvent), mikumiku->projects[mikumiku->active_project]);
	(void)g_signal_connect(mikumiku->projects[mikumiku->active_project]->widgets.drawing_area,
		"scroll-event", G_CALLBACK(MouseWheelScrollEvent), mikumiku->projects[mikumiku->active_project]);
	(void)g_signal_connect(mikumiku->projects[mikumiku->active_project]->widgets.drawing_area, "configure_event",
		G_CALLBACK(ConfigureEvent), mikumiku->projects[mikumiku->active_project]);
	//gtk_widget_set_size_request(mikumiku->widgets.drawing_area,
	//	720, 480);
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	mikumiku->widgets.main_window = window;
	main_box = gtk_vbox_new(FALSE, 0);
	gtk_container_add(GTK_CONTAINER(window), main_box);
	gtk_box_pack_start(GTK_BOX(main_box), MakeMenuBar(mikumiku, NULL), FALSE, FALSE, 0);
	box = gtk_hbox_new(FALSE, 0);
	gtk_box_pack_start(GTK_BOX(main_box), box, TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), (GtkWidget*)ModelControlWidgetNew(mikumiku), FALSE, FALSE, 0);
	gtk_box_pack_start(GTK_BOX(box), mikumiku->projects[mikumiku->active_project]->widgets.drawing_area,
		TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(box), (GtkWidget*)CameraLightControlWidgetNew(mikumiku), FALSE, FALSE, 0);
	(void)g_signal_connect(window, "destroy",
		G_CALLBACK(gtk_main_quit), NULL);

	(void)g_timeout_add(1000/60, (GSourceFunc)Timer,
		mikumiku->projects[mikumiku->active_project]->widgets.drawing_area);

	gtk_widget_show_all(window);

	gtk_main();

	DeleteTbbObject(tbb);

	return 0;
}

#elif 0

/*
 * An example of using GtkGLExt in C
 *
 * Written by Davyd Madeley <davyd@madeley.id.au> and made available under a
 * BSD license.
 *
 * This is purely an example, it may eat your cat and you can keep both pieces.
 *
 * Compile with:
 *    gcc -o gtkglext-example `pkg-config --cflags --libs gtk+-2.0 gtkglext-1.0 gtkglext-x11-1.0` gtkglext-example.c
 */

#include <math.h>
#include <stdlib.h>
#include <windows.h>
#include <GL/gl.h>

float boxv[][3] = {
	{ -0.5, -0.5, -0.5 },
	{  0.5, -0.5, -0.5 },
	{  0.5,  0.5, -0.5 },
	{ -0.5,  0.5, -0.5 },
	{ -0.5, -0.5,  0.5 },
	{  0.5, -0.5,  0.5 },
	{  0.5,  0.5,  0.5 },
	{ -0.5,  0.5,  0.5 }
};
#define ALPHA 0.5

static float ang = 30.;

static gboolean
expose (GtkWidget *da, GdkEventExpose *event, gpointer user_data)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context (da);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (da);

	// g_print (" :: expose\n");

	if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext))
	{
		g_assert_not_reached ();
	}

	/* draw in here */
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
	
	glRotatef (ang, 1, 0, 1);
	// glRotatef (ang, 0, 1, 0);
	// glRotatef (ang, 0, 0, 1);

	glShadeModel(GL_FLAT);

#if 0
	glBegin (GL_QUADS);
	glColor4f(0.0, 0.0, 1.0, ALPHA);
	glVertex3fv(boxv[0]);
	glVertex3fv(boxv[1]);
	glVertex3fv(boxv[2]);
	glVertex3fv(boxv[3]);

	glColor4f(1.0, 1.0, 0.0, ALPHA);
	glVertex3fv(boxv[0]);
	glVertex3fv(boxv[4]);
	glVertex3fv(boxv[5]);
	glVertex3fv(boxv[1]);
	
	glColor4f(0.0, 1.0, 1.0, ALPHA);
	glVertex3fv(boxv[2]);
	glVertex3fv(boxv[6]);
	glVertex3fv(boxv[7]);
	glVertex3fv(boxv[3]);
	
	glColor4f(1.0, 0.0, 0.0, ALPHA);
	glVertex3fv(boxv[4]);
	glVertex3fv(boxv[5]);
	glVertex3fv(boxv[6]);
	glVertex3fv(boxv[7]);
	
	glColor4f(1.0, 0.0, 1.0, ALPHA);
	glVertex3fv(boxv[0]);
	glVertex3fv(boxv[3]);
	glVertex3fv(boxv[7]);
	glVertex3fv(boxv[4]);
	
	glColor4f(0.0, 1.0, 0.0, ALPHA);
	glVertex3fv(boxv[1]);
	glVertex3fv(boxv[5]);
	glVertex3fv(boxv[6]);
	glVertex3fv(boxv[2]);

	glEnd ();
#endif

	glBegin (GL_LINES);
	glColor3f (1., 0., 0.);
	glVertex3f (0., 0., 0.);
	glVertex3f (1., 0., 0.);
	glEnd ();
	
	glBegin (GL_LINES);
	glColor3f (0., 1., 0.);
	glVertex3f (0., 0., 0.);
	glVertex3f (0., 1., 0.);
	glEnd ();
	
	glBegin (GL_LINES);
	glColor3f (0., 0., 1.);
	glVertex3f (0., 0., 0.);
	glVertex3f (0., 0., 1.);
	glEnd ();

	glBegin(GL_LINES);
	glColor3f (1., 1., 1.);
	glVertex3fv(boxv[0]);
	glVertex3fv(boxv[1]);
	
	glVertex3fv(boxv[1]);
	glVertex3fv(boxv[2]);
	
	glVertex3fv(boxv[2]);
	glVertex3fv(boxv[3]);
	
	glVertex3fv(boxv[3]);
	glVertex3fv(boxv[0]);
	
	glVertex3fv(boxv[4]);
	glVertex3fv(boxv[5]);
	
	glVertex3fv(boxv[5]);
	glVertex3fv(boxv[6]);
	
	glVertex3fv(boxv[6]);
	glVertex3fv(boxv[7]);
	
	glVertex3fv(boxv[7]);
	glVertex3fv(boxv[4]);
	
	glVertex3fv(boxv[0]);
	glVertex3fv(boxv[4]);
	
	glVertex3fv(boxv[1]);
	glVertex3fv(boxv[5]);
	
	glVertex3fv(boxv[2]);
	glVertex3fv(boxv[6]);
	
	glVertex3fv(boxv[3]);
	glVertex3fv(boxv[7]);
	glEnd();

	glPopMatrix ();

	if (gdk_gl_drawable_is_double_buffered (gldrawable))
		gdk_gl_drawable_swap_buffers (gldrawable);

	else
		glFlush ();

	gdk_gl_drawable_gl_end (gldrawable);

	return TRUE;
}

static gboolean
configure (GtkWidget *da, GdkEventConfigure *event, gpointer user_data)
{
	GdkGLContext *glcontext = gtk_widget_get_gl_context (da);
	GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (da);

	if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext))
	{
		g_assert_not_reached ();
	}

	glLoadIdentity();
	glViewport (0, 0, da->allocation.width, da->allocation.height);
	glOrtho (-10,10,-10,10,-20050,10000);
	glEnable (GL_BLEND);
	glBlendFunc (GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glScalef (10., 10., 10.);
	
	gdk_gl_drawable_gl_end (gldrawable);

	return TRUE;
}

static gboolean
rotate (gpointer user_data)
{
	GtkWidget *da = GTK_WIDGET (user_data);

	ang++;

	gdk_window_invalidate_rect (da->window, &da->allocation, FALSE);
	gdk_window_process_updates (da->window, FALSE);

	return TRUE;
}

int
main (int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *da;
	GdkGLConfig *glconfig;

	gtk_init (&argc, &argv);
	gtk_gl_init (&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_default_size (GTK_WINDOW (window), 800, 600);
	da = gtk_drawing_area_new ();

	gtk_container_add (GTK_CONTAINER (window), da);
	g_signal_connect_swapped (window, "destroy",
			G_CALLBACK (gtk_main_quit), NULL);
	gtk_widget_set_events (da, GDK_EXPOSURE_MASK);

	gtk_widget_show (window);

	/* prepare GL */
	glconfig = gdk_gl_config_new_by_mode (
			GDK_GL_MODE_RGB |
			GDK_GL_MODE_DEPTH |
			GDK_GL_MODE_DOUBLE);

	if (!glconfig)
	{
		g_assert_not_reached ();
	}

	if (!gtk_widget_set_gl_capability (da, glconfig, NULL, TRUE,
				GDK_GL_RGBA_TYPE))
	{
		g_assert_not_reached ();
	}

	g_signal_connect (da, "configure-event",
			G_CALLBACK (configure), NULL);
	g_signal_connect (da, "expose-event",
			G_CALLBACK (expose), NULL);

	gtk_widget_show_all (window);

	g_timeout_add (1000 / 60, rotate, da);

	gtk_main ();
}

#else

int main(void)
{
	float inv[] = {1,2,0,-1,-1,1,2,0,2,0,1,1,1,-2,-1,1};
	int i;

	InvertMatrix4x4(inv, inv);

	for(i=0; i<16; i++)
	{
		printf("%f\t", inv[i]);
		if(i%4 == 3)
		{
			putchar('\n');
		}
	}

	system("pause");

	return 0;
}

#endif

