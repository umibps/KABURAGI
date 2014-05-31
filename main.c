#include "configure.h"

#include <string.h>
#include <locale.h>
#include <gtk/gtk.h>
#include "application.h"
#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
# include "MikuMikuGtk+/tbb.h"
#endif

#if defined(_MSC_VER)

#ifdef _UNICODE
# if defined _M_IX86
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
# elif defined _M_IA64
#  pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='ia64' publicKeyToken='6595b64144ccf1df' language='*'\"")
# elif defined _M_X64
#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
# else
#  pragma comment(linker,"/manifestdependency:\"type='win32' 
name='Microsoft.Windows.Common-Controls' version='6.0.0.0' 
processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
# endif
#endif

# if !defined(_DEBUG)
#  pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
# endif

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
# pragma comment(lib, "OpenGL32.lib")
# pragma comment(lib, "GlU32.lib")
# ifdef TEST //_DEBUG
#  pragma comment(lib, "tbb_debug.lib")
#  pragma comment(lib, "tbb_preview_debug.lib")
#  pragma comment(lib, "tbbmalloc_debug.lib")
#  pragma comment(lib, "tbbproxy_debug.lib")
#  pragma comment(lib, "tbbmalloc_proxy_debug.lib")
#  pragma comment(lib, "BulletCollision_vs2008_debug.lib")
#  pragma comment(lib, "BulletDynamics_vs2008_debug.lib")
#  pragma comment(lib, "BulletSoftBody_vs2008_debug.lib")
#  pragma comment(lib, "ConvexDecomposition_vs2008_debug.lib")
#  pragma comment(lib, "LinearMath_vs2008_debug.lib")
#  pragma comment(lib, "OpenGLSupport_vs2008_debug.lib")
# else
#  pragma comment(lib, "tbb.lib")
#  pragma comment(lib, "tbb_preview.lib")
#  pragma comment(lib, "tbbmalloc.lib")
#  pragma comment(lib, "tbbproxy.lib")
#  pragma comment(lib, "tbbmalloc_proxy.lib")
#  pragma comment(lib, "BulletCollision_vs2008.lib")
#  pragma comment(lib, "BulletDynamics_vs2008.lib")
#  pragma comment(lib, "BulletSoftBody_vs2008.lib")
#  pragma comment(lib, "ConvexDecomposition_vs2008.lib")
#  pragma comment(lib, "LinearMath_vs2008.lib")
#  pragma comment(lib, "OpenGLSupport_vs2008.lib")
# endif
#endif

# ifndef BUILD64BIT
#  if defined(USE_STATIC_LIB) && USE_STATIC_LIB != 0
#   ifdef _DEBUG
#    pragma comment(linker, "/NODEFAULTLIB:LIBCMTD.lib")
#    pragma comment(lib, "gtk_libsd.lib")
#    pragma comment(lib, "Dnsapi.lib")
#    pragma comment(lib, "ws2_32.lib")
#    pragma comment(lib, "Shlwapi.lib")
#    pragma comment(lib, "imm32.lib")
#    pragma comment(lib, "zlib.lib")
#    pragma comment(lib, "libpng.lib")
#    pragma comment(lib, "usp10.lib")
cairo_surface_t *
cairo_image_surface_create_from_png (const char	*filename)
{
	return 0;
}
/*
#    pragma comment(linker, "/NODEFAULTLIB:LIBCMTD.lib")
#    pragma comment(lib, "Dnsapi.lib")
#    pragma comment(lib, "ws2_32.lib")
#    pragma comment(lib, "pcre3d.lib")
#    pragma comment(lib, "Shlwapi.lib")
#    pragma comment(lib, "atk-1.0.lib")
#    pragma comment(lib, "cairo.lib")
#    pragma comment(lib, "expat.lib")
#    pragma comment(lib, "fontconfig.lib")
#    pragma comment(lib, "freetype.lib")
#    pragma comment(lib, "gailutil.lib")
#    pragma comment(lib, "gdk_pixbuf-2.0.lib")
#    pragma comment(lib, "gdk-win32-2.0.lib")
#    pragma comment(lib, "./STATIC_LIB/giod.lib")
#    pragma comment(lib, "./STATIC_LIB/glibd.lib")
#    pragma comment(lib, "./STATIC_LIB/gmoduled.lib")
#    pragma comment(lib, "./STATIC_LIB/gobjectd.lib")
#    pragma comment(lib, "./STATIC_LIB/gthreadd.lib")
#    pragma comment(lib, "gtk-win32-2.0.lib")
#    pragma comment(lib, "intl.lib")
#    pragma comment(lib, "libpng.lib")
#    pragma comment(lib, "pango-1.0.lib")
#    pragma comment(lib, "pangocairo-1.0.lib")
#    pragma comment(lib, "pangoft2-1.0.lib")
#    pragma comment(lib, "pangowin32-1.0.lib")
#    pragma comment(lib, "zlib.lib")
*/
#   else
#    pragma comment(lib, "./STATIC_LIB/libatk-1.0.a")
#    pragma comment(lib, "./STATIC_LIB/libcairo.a")
#    pragma comment(lib, "./STATIC_LIB/libcairo-gobject.a")
#    pragma comment(lib, "./STATIC_LIB/libcairo-script-interpreter.a")
#    pragma comment(lib, "./STATIC_LIB/libcharset.a")
#    pragma comment(lib, "./STATIC_LIB/libexpat.a")
#    pragma comment(lib, "./STATIC_LIB/libffi.a")
#    pragma comment(lib, "./STATIC_LIB/libfontconfig.a")
#    pragma comment(lib, "./STATIC_LIB/libfreetype.a")
#    pragma comment(lib, "./STATIC_LIB/libgailutil.a")
#    pragma comment(lib, "./STATIC_LIB/libgdk_pixbuf-2.0.a")
#    pragma comment(lib, "./STATIC_LIB/libgdk-win32-2.0.a")
#    pragma comment(lib, "./STATIC_LIB/libgio-2.0.a")
#    pragma comment(lib, "./STATIC_LIB/libglib-2.0.a")
#    pragma comment(lib, "./STATIC_LIB/libgmodule-2.0.a")
#    pragma comment(lib, "gobject-2.0.lib")
#    pragma comment(lib, "gthread-2.0.lib")
#    pragma comment(lib, "gtk-win32-2.0.lib")
#    pragma comment(lib, "intl.lib")
#    pragma comment(lib, "libpng.lib")
#    pragma comment(lib, "pango-1.0.lib")
#    pragma comment(lib, "pangocairo-1.0.lib")
#    pragma comment(lib, "pangoft2-1.0.lib")
#    pragma comment(lib, "pangowin32-1.0.lib")
#    pragma comment(lib, "zlib.lib")
#   endif
#  else
#  ifdef _DEBUG
#   pragma comment(linker, "/NODEFAULTLIB:LIBCMT")
#  else
#   if _MSC_VER >= 1600
#    pragma comment(linker, "/NODEFAULTLIB:LIBCMT")
#   endif
#  endif
#   if GTK_MAJOR_VERSION <= 2
#    pragma comment(lib, "atk-1.0.lib")
#    pragma comment(lib, "cairo.lib")
#    pragma comment(lib, "expat.lib")
#    pragma comment(lib, "fontconfig.lib")
#    pragma comment(lib, "freetype.lib")
#    pragma comment(lib, "gailutil.lib")
#    pragma comment(lib, "gdk_pixbuf-2.0.lib")
#    pragma comment(lib, "gdk-win32-2.0.lib")
#    pragma comment(lib, "gio-2.0.lib")
#    pragma comment(lib, "glib-2.0.lib")
#    pragma comment(lib, "gmodule-2.0.lib")
#    pragma comment(lib, "gobject-2.0.lib")
#    pragma comment(lib, "gthread-2.0.lib")
#    pragma comment(lib, "gtk-win32-2.0.lib")
#    pragma comment(lib, "intl.lib")
#    pragma comment(lib, "libpng.lib")
#    pragma comment(lib, "pango-1.0.lib")
#    pragma comment(lib, "pangocairo-1.0.lib")
#    pragma comment(lib, "pangoft2-1.0.lib")
#    pragma comment(lib, "pangowin32-1.0.lib")
#    if defined(_M_X64) || defined(_M_IA64)
#     pragma comment(lib, "zdll.lib")
#    else
#     pragma comment(lib, "zlib.lib")
#    endif
#   else
#    ifdef _DEBUG
//#     pragma comment(linker, "/NODEFAULTLIB:MSVCRTD")
#    else
#     pragma comment(linker, "/NODEFAULTLIB:MSVCRT")
#    endif

#    if defined(_M_X64) || defined(_M_IA64)
#     pragma comment(lib, "libpng15-15.lib")
#    else
#     pragma comment(lib, "libpng.lib")
#    endif
#    pragma comment(lib, "zlib.lib")
#    pragma comment(lib, "libz.dll.a")
#    pragma comment(lib, "cairo.lib")
#    pragma comment(lib, "atk-1.0.lib")
#    pragma comment(lib, "fontconfig.lib")
#    pragma comment(lib, "gailutil.lib")
#    pragma comment(lib, "gdk_pixbuf-2.0.lib")
#    pragma comment(lib, "gdk-win32-3.0.lib")
#    pragma comment(lib, "gio-2.0.lib")
#    pragma comment(lib, "glib-2.0.lib")
#    pragma comment(lib, "gmodule-2.0.lib")
#    pragma comment(lib, "gobject-2.0.lib")
#    pragma comment(lib, "gthread-2.0.lib")
#    pragma comment(lib, "gtk-win32-3.0.lib")
#    pragma comment(lib, "pango-1.0.lib")
#    pragma comment(lib, "pangocairo-1.0.lib")
#    pragma comment(lib, "pangoft2-1.0.lib")
#    pragma comment(lib, "pangowin32-1.0.lib")
#    pragma comment(lib, "intl.lib")

#    pragma comment(lib, "libatk-1.0.dll.a")
#    pragma comment(lib, "libcairo.dll.a")
#    pragma comment(lib, "libcairo-gobject.dll.a")
#    pragma comment(lib, "libcairo-script-interpreter.dll.a")
#    pragma comment(lib, "libcroco-0.6.dll.a")
#    pragma comment(lib, "libffi.dll.a")
#    pragma comment(lib, "libfontconfig.dll.a")
#    pragma comment(lib, "libfreetype.dll.a")
#    pragma comment(lib, "libgailutil-3.dll.a")
#    pragma comment(lib, "libgdk_pixbuf-2.0.dll.a")
#    pragma comment(lib, "libgdk-3.dll.a")
#    pragma comment(lib, "libgio-2.0.dll.a")
#    pragma comment(lib, "libglib-2.0.dll.a")
#    pragma comment(lib, "libgmodule-2.0.dll.a")
#    pragma comment(lib, "libgthread-2.0.dll.a")
#    pragma comment(lib, "libgtk-3.dll.a")
#    pragma comment(lib, "libjasper.dll.a")
#    pragma comment(lib, "libjpeg.dll.a")
#    pragma comment(lib, "liblzma.dll.a")
#    pragma comment(lib, "libpango-1.0.dll.a")
#    pragma comment(lib, "libpangocairo-1.0.dll.a")
#    pragma comment(lib, "libpangoft2-1.0.dll.a")
#    pragma comment(lib, "libpangowin32-1.0.dll.a")
#    pragma comment(lib, "libpixman-1.dll.a")
#    pragma comment(lib, "librsvg-2.dll.a")
#    pragma comment(lib, "libtiff.dll.a")
#    pragma comment(lib, "libtiffxx.dll.a")
#    pragma comment(lib, "libxml2.dll.a")

#   endif
#  endif
# else
#  ifdef _DEBUG
#   pragma comment(linker, "/NODEFAULTLIB:LIBCMTD")
#  else
//#   pragma comment(linker, "/NODEFAULTLIB:LIBCMT")
#  endif
#  if MAJOR_VERSION == 1
#   pragma comment(lib, "atk-1.0.lib")
#   pragma comment(lib, "cairo.lib")
#   pragma comment(lib, "expat.lib")
#   pragma comment(lib, "fontconfig.lib")
#   pragma comment(lib, "freetype.lib")
#   pragma comment(lib, "gailutil.lib")
#   pragma comment(lib, "gdk_pixbuf-2.0.lib")
#   pragma comment(lib, "gdk-win32-2.0.lib")
#   pragma comment(lib, "gio-2.0.lib")
#   pragma comment(lib, "glib-2.0.lib")
#   pragma comment(lib, "gmodule-2.0.lib")
#   pragma comment(lib, "gobject-2.0.lib")
#   pragma comment(lib, "gthread-2.0.lib")
#   pragma comment(lib, "gtk-win32-2.0.lib")
#   pragma comment(lib, "intl.lib")
#   pragma comment(lib, "libpng.lib")
#   pragma comment(lib, "pango-1.0.lib")
#   pragma comment(lib, "pangocairo-1.0.lib")
#   pragma comment(lib, "pangoft2-1.0.lib")
#   pragma comment(lib, "pangowin32-1.0.lib")
#   if defined(_M_X64) || defined(_M_IA64)
#    pragma comment(lib, "zdll.lib")
#   else
#    pragma comment(lib, "zlib.lib")
#   endif
#  else
#    pragma comment(lib, "libpng15-15.lib")
#    pragma comment(lib, "zlib.lib")
#    pragma comment(lib, "libz.dll.a")
#    pragma comment(lib, "cairo.lib")
#    pragma comment(lib, "atk-1.0.lib")
#    pragma comment(lib, "fontconfig.lib")
#    pragma comment(lib, "gailutil.lib")
#    pragma comment(lib, "gdk_pixbuf-2.0.lib")
#    pragma comment(lib, "gdk-win32-3.0.lib")
#    pragma comment(lib, "gio-2.0.lib")
#    pragma comment(lib, "glib-2.0.lib")
#    pragma comment(lib, "gmodule-2.0.lib")
#    pragma comment(lib, "gobject-2.0.lib")
#    pragma comment(lib, "gthread-2.0.lib")
#    pragma comment(lib, "gtk-win32-3.0.lib")
#    pragma comment(lib, "pango-1.0.lib")
#    pragma comment(lib, "pangocairo-1.0.lib")
#    pragma comment(lib, "pangoft2-1.0.lib")
#    pragma comment(lib, "pangowin32-1.0.lib")
#    pragma comment(lib, "intl.lib")

#    pragma comment(lib, "libatk-1.0.dll.a")
#    pragma comment(lib, "libcairo.dll.a")
#    pragma comment(lib, "libcairo-gobject.dll.a")
#    pragma comment(lib, "libcairo-script-interpreter.dll.a")
#    pragma comment(lib, "libcroco-0.6.dll.a")
#    pragma comment(lib, "libffi.dll.a")
#    pragma comment(lib, "libfontconfig.dll.a")
#    pragma comment(lib, "libfreetype.dll.a")
#    pragma comment(lib, "libgailutil-3.dll.a")
#    pragma comment(lib, "libgdk_pixbuf-2.0.dll.a")
#    pragma comment(lib, "libgdk-3.dll.a")
#    pragma comment(lib, "libgio-2.0.dll.a")
#    pragma comment(lib, "libglib-2.0.dll.a")
#    pragma comment(lib, "libgmodule-2.0.dll.a")
#    pragma comment(lib, "libgthread-2.0.dll.a")
#    pragma comment(lib, "libgtk-3.dll.a")
#    pragma comment(lib, "libjasper.dll.a")
#    pragma comment(lib, "libjpeg.dll.a")
#    pragma comment(lib, "liblzma.dll.a")
#    pragma comment(lib, "libpango-1.0.dll.a")
#    pragma comment(lib, "libpangocairo-1.0.dll.a")
#    pragma comment(lib, "libpangoft2-1.0.dll.a")
#    pragma comment(lib, "libpangowin32-1.0.dll.a")
#    pragma comment(lib, "libpixman-1.dll.a")
#    pragma comment(lib, "librsvg-2.dll.a")
#    pragma comment(lib, "libtiff.dll.a")
#    pragma comment(lib, "libtiffxx.dll.a")
#    pragma comment(lib, "libxml2.dll.a")
#  endif
# endif

#endif

#if 1
# ifdef _MSC_VER
#  if defined(CHECK_MEMORY_POOL) && CHECK_MEMORY_POOL != 0
#   define _CRTDBG_MAP_ALLOC
#   include <stdlib.h>
#   include <crtdbg.h>
#  else
#   include <stdlib.h>
#  endif
# endif

#ifdef __cplusplus
extern "C" {
#endif

int main(int argc, char** argv)
{
	static APPLICATION application = {0};
	
	static APPLICATION_LABELS labels =
	{
		"English",
		{"OK", "Apply", "Cancel", "Close", "Normal", "Reverse", "Edit Selection", "Window",
			"Close Window", "Dock Left", "Dock Right", "Full Screen", "Reference Image Window",
			"Move Top Left", "Hot Key", "Loading...", "Saving..."},
		{"pixel", "Length", "Angle", "_BG", "Loop", "Preview", "Interval", "minute", "Detail", "Target",
			"Clip Board", "Name", "Type", "Resolution", "Center", "Straight", "Grow"},
		{"File", "New", "Open", "Open As Layer", "Save", "Save as", "Close", "Quit", "Edit", "Undo", "Redo",
			"Copy", "Copy Visible", "Cut", "Paste", "Clipboard", "Transform", "Projection", "Canvas",
			"Change Resolution", "Change Canvas Size", "Flip Canvas Horizontally", "Flip Canvas Vertically",
			"Switch _BG Color", "Change 2nd _BG Color", "Change ICC Profile", "Layer", "_New Layer(Color)",
			"_New Layer(Vector)", "_New Layer Set", "_New 3D Modeling Layer", "_Copy Layer", "_Delete Layer",
			"Fill with _FG Color", "Fill with Pattern", "Rasterize Layer", "Merge Down", "_Flatten Image",
			"_Visible to Layer", "Visible Copy", "Select", "None", "Invert", "All", "Grow", "Shrink", "View", "Zoom",
			"Zoom _In", "Zoom _Out", "Actual Size", "Reverse Horizontally", "Rotate", "Reset Roatate",
			"Display Filters", "Nothing", "Gray Scale", "Gray Scale (YIQ)", "Filters", "Blur", "Motion Blur",
			"Brightness & Contrast", "Hue & Saturation", "Luminosity to Opacity", "Color to Alpha",
			"Colorize with Under Layer", "Gradation Map", "Map with Detect Max Black Value", "Transparency as White",
			"Fill with Vector", "Script", "Help", "Version"
		},
		{"New", "New Canvas", "Width", "Height", "2nd BG Color", "Adopt ICC Profile?"},
		{"Tool Box", "Initialize", "New Brush", "Smooth", "Quality", "Rate", "Gaussian", "Average", "Magnification",
			"Brush Size", "Scale", "Flow", "Pressure", "Extend Range", "Blur", "OutLine Hardness",
			"Color Extends", "Start Distance of Drag and Move", "Anti Alias", "Change Text Color", "Horizonal",
			"Vertical", "Style", "Normal", "Bold", "Italic", "Oblique", "Reverse", "Reverse Horizontally",
			"Reverse Vertically", "Blend Mode", "Hue", "Saturation", "Brightness", "Contrast", "Distance",
			"Rotate Start", "Rotate Speed", "Random Rotate", "Rotate to Brush Direction", "Size Range",
			"Rotate Range", "Random Size", "Clockwise", "Counter Clockwise", "Both Direction", "Minimum Degree",
			"Minumum Distance", "Enter", "Out", "Mix", "Reverse FG BG", "Devide Stroke", "Delete Stroke", "Target",
			"Stroke", "Prior Angle", "Control Point", "Free", "Scale", "Free Shape", "Rotate", "Preference", "Name",
			"Copy Brush", "Change Brush", "Delete Brush", "Texture", "Strength", "No Texture", "Add Color",
			"Delete Color", "Load Pallete", "Add Pallete","Write Pallete", "Clear Pallete", "Pick Mode",
			"Single Pixel", "Average Color", "Open Path", "Close Path", "Start Editting 3D Model",
			"Finish Editting 3D Model",
			{"Pencil", "Hard Pen", "Air Brush", "Old Air Brush", "Water Brush", "Picker Brush", "Eraser", "Bucket",
				"Pattern Fill", "Blur Tool", "Smudge", "Mix Brush", "Gradation", "Text Tool", "Stamp Tool",
				"Image Brush", "Picker Image Brush", "Script Brush"},
			{"Detection Target", "Detect from ... ", "Pixels Color", "Pixel Color + Alpha",
				"Alpha", "Active Layer", "Under Layer", "Canvas", "Threshold", "Detection Area", "Normal", "Large"},
			{"Select/Release", "Move Control Point", "Change Pressure", "Delete Control Point",
				"Move Stroke", "Copy & Move Stroke", "Joint Stroke"}},
		{"Layer", "Layer", "Vector", "Layer Set", "Text", "3D Modeling", "Add Layer", "Add Vector Layer",
			"Add Layer Set", "Add 3D Modeling Layer", "Rename Layer", "Reorder Layer", "Opacity to Selection Area",
			"Opacity Add Selection Area", "Pasted Layer", "Under Layer", "Mixed Under Layer", "Blend Mode", "Opacity",
			"Masking with Under Layer", "Lock Opacity",
			{"Normal", "Add", "Multiply", "Screen", "Overlay", "Lighten", "Darken", "Dodge", "Burn",
				"Hard Light", "Soft Light", "Difference", "Exclusion", "Hue", "Saturation", "Color",
				"Luminosity", "Binalize"}},
		{"Random (Straight)", "Bidirection"},
		{"Compress", "Quality", "Write Opacity Data", "Write ICC Profile Data", "Save before close the image?",
			"There are some images with unsaved changes.", "There is Backup File.\nRecover it?"},
		{"Navigtion"},
		{"Preference", "Base Settings", "Auto Save", "Theme", "Default",
			"There is a conflict to set a hot key.", "Language", "Backup File Directory",
			"Show Preview Window on Taskbar", "Draw with Touch", "Scale Change and Move Canvas with Change",
			"Set Back Ground Color"},
		{"Execute Back Up..."}
	};

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
	void *tbb = TbbObjectNew();
#endif

	{
		gchar *raw_path;

		application.labels = &labels;

#if GTK_MAJOR_VERSION <= 2
		gtk_set_locale();
#endif
		gtk_init(&argc, &argv);

		raw_path = g_locale_to_utf8(argv[0], -1, NULL, NULL, NULL);
		application.current_path = g_path_get_dirname(raw_path);

#if defined(CHECK_MEMORY_POOL) && CHECK_MEMORY_POOL != 0
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF);
	//_CrtSetBreakAlloc(300);
#endif

		InitializeApplication(&application, INITIALIZE_FILE_NAME);
		g_free(raw_path);
	}

	if(argc > 1)
	{
		gchar *file_path = g_locale_to_utf8(argv[1], -1, NULL, NULL, NULL);

		OpenFile(file_path, &application);

		g_free(file_path);

#ifdef _PROFILING
		return 0;
#endif
	}

	gtk_main();

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
	DeleteTbbObject(tbb);
#endif

	return 0;
}

#ifdef __cplusplus
}
#endif

#elif 0

#pragma comment(lib, "atk-1.0.lib")
#pragma comment(lib, "cairo.lib")
#pragma comment(lib, "expat.lib")
#pragma comment(lib, "fontconfig.lib")
#pragma comment(lib, "freetype.lib")
#pragma comment(lib, "gailutil.lib")
#pragma comment(lib, "gdk_pixbuf-2.0.lib")
#pragma comment(lib, "gdk-win32-2.0.lib")
#pragma comment(lib, "gio-2.0.lib")
#pragma comment(lib, "glib-2.0.lib")
#pragma comment(lib, "gmodule-2.0.lib")
#pragma comment(lib, "gobject-2.0.lib")
#pragma comment(lib, "gthread-2.0.lib")
#pragma comment(lib, "gtk-win32-2.0.lib")
#pragma comment(lib, "intl.lib")
#pragma comment(lib, "libpng.lib")
#pragma comment(lib, "pango-1.0.lib")
#pragma comment(lib, "pangocairo-1.0.lib")
#pragma comment(lib, "pangoft2-1.0.lib")
#pragma comment(lib, "pangowin32-1.0.lib")
#pragma comment(lib, "zlib.lib")

#include <gtk/gtk.h>
#include "application.h"
#include "script.h"

int main(int argc, char** argv)
{
	static APPLICATION application;
	
	static APPLICATION_LABELS labels =
	{
		{"OK", "Apply", "Cancel", "Close", "Normal", "Reverse", "Edit Selection", "Window", "Dock Left",
			"Dock Right", "Full Screen"},
		{"pixel", "Loop", "Preview"},
		{"File", "New", "Open", "Open As Layer", "Save", "Save as", "Close", "Quit", "Edit", "Undo", "Redo",
			"Copy", "Cut", "Paste", "Clipboard", "Transform", "Canvas", "Change Resolution", "Change Canvas Size",
			"Flip Canvas Horizontally", "Flip Canvas Vertically", "Layer", "_New Layer(Color)", "_New Layer(Vector)",
			"_New Layer Set", "_Copy Layer", "_Delete Layer", "Fill with _FG Color", "Fill with Pattern",
			"Rasterize Layer", "Merge Down", "_Flatten Image", "_Visible to Layer", "Visible Copy", "Select", "None",
			"Invert", "All", "Grow", "Shrink", "View", "Zoom", "Zoom _In", "Zoom _Out", "Actual Size", 
			"Reverse Horizontally", "Rotate", "Reset Roatate", "Display Filters", "Nothing", "Gray Scale",
			"Gray Scale (YIQ)", "Filters", "Blur", "Brightness & Contrast", "Hue & Saturation", 
			"Luminosity to Opacity", "Help", "Version"
		},
		{"New", "New Canvas", "Width", "Height"},
		{"Tool Box", "Smooth", "Quality", "Rate", "Gaussian", "Average", "Magnification", "Brush Size", "Scale",
			"Flow", "Pressure", "Extend Range", "Blur", "OutLine Hardness",  "Color Extends",
			"Start Distance of Drag and Move", "Anti Alias", "Change Text Color", "Horizonal",
			"Vertical", "Style", "Normal", "Bold", "Italic", "Oblique", "Reverse", "Reverse Horizontally",
			"Reverse Vertically", "Blend Mode", "Hue", "Saturation", "Brightness", "Contrast", "Distance",
			"Rotate Start", "Rotate Speed", "Rotate to Brush Direction", "Clockwise", "Counter Clockwise",
			"Enter", "Out", "Mix", "Reverse FG BG", "Devide Stroke", "Delete Stroke", "Target", "Stroke",
			"Control Point", "Free", "Scale", "Free Shape", "Rotate", "Preference", "Name", "Copy Brush",
			"Delete Brush", "Texture", "Strength", "No Texture",
			{"Detection Target", "Detect from ... ", "Pixels Color", "Pixel Color + Alpha",
				"Alpha", "Active Layer", "Canvas", "Threshold", "Detection Area", "Normal", "Large",},
			{"Select/Release", "Move Control Point", "Change Pressure", "Delete Control Point",
				"Move Stroke", "Copy & Move Stroke", "Joint Stroke"}},
		{"Layer", "Layer", "Vector", "Layer Set", "Text", "Add Layer", "Add Vector Layer", "Add Layer Set",
			"Rename Layer", "Reorder Layer", "Opacity to Selection Area", "Pasted Layer",
			"Blend Mode", "Opacity", "Masking with Under Layer", "Lock Opacity",
			{"Normal", "Add", "Multiply", "Screen", "Overlay", "Lighten", "Darken", "Dodge", "Burn",
				"Hard Light", "Soft Light", "Difference", "Exclusion", "Hue", "Saturation", "Color",
				"Luminosity"}},
		{"Compress", "Quality", "Save before close the image?", "There are some images with unsaved changes."},
		{"Navigtion"},
		{"Preference", "Base Settings", "Theme", "Default"}
	};

	(void)memset(&application, 0, sizeof(application));
	application.labels = &labels;
	gtk_init(&argc, &argv);
	return 0;
}


#else
/*
#include <gtk/gtk.h>

void ButtonClicked(GtkWidget *button, GtkWidget *widget)
{
	gtk_widget_destroy(widget);
}

int main(int argc, char** argv)
{
	GtkWidget *pane, *label, *button, *window;
	GtkWidget *dock;

	gtk_init(&argc, &argv);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

	pane = gtk_hpaned_new();
	label = gtk_label_new("test");
	button = gtk_button_new_with_label("destroy");
	g_signal_connect(G_OBJECT(button), "clicked", G_CALLBACK(ButtonClicked), label);
	gtk_paned_add1(GTK_PANED(pane), label);
	gtk_paned_add2(GTK_PANED(pane), button);

	dock = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_type_hint(GTK_WINDOW(dock), GDK_WINDOW_TYPE_HINT_DOCK);

	label = gtk_label_new("dock?");
	gtk_container_add(GTK_CONTAINER(dock), label);
	//gtk_container_add(GTK_CONTAINER(window), dock);

	//gtk_container_add(GTK_CONTAINER(window), pane);

	gtk_widget_show_all(window);
	gtk_widget_show_all(dock);

	gtk_main();

	return 0;
}
*/
/*
#include <gtk/gtk.h>  
  
guint animation_hidden = 0;  
guint animation_show = 0;  
gboolean hidden = FALSE;  
gboolean processing = FALSE;  
int y_pos = 0;  
  
gboolean  
panel_toplevel_animation_show (GtkWidget *toplevel)  
{  
    y_pos += 1;  
    if (y_pos <= 0) {  
      processing = TRUE;  
      gdk_window_move (toplevel->window, toplevel->allocation.x, y_pos);  
    } else {  
      y_pos = 0;  
      processing = FALSE;  
      hidden = FALSE;  
      return FALSE;  
    }  
    //g_message ("show %d", y_pos);  
    return TRUE;  
}  
  
gboolean  
panel_toplevel_animation_hidden (GtkWidget *toplevel)  
{  
    y_pos -= 1;  
    if (toplevel->allocation.height + y_pos > 1) {  
      processing = TRUE;  
      gdk_window_move (toplevel->window, toplevel->allocation.x, y_pos);  
    } else {  
      processing = FALSE;  
      hidden = TRUE;  
      return FALSE;  
    }  
    //g_message ("hidden %d", y_pos);  
    return TRUE;  
}  
  
void panel_hidden (GtkWidget *widget)  
{  
    animation_hidden = g_timeout_add (20, (GSourceFunc) panel_toplevel_animation_hidden, widget);  
}  
  
void panel_show (GtkWidget *widget)  
{  
    animation_show = g_timeout_add (20, (GSourceFunc) panel_toplevel_animation_show, widget);  
}  
  
gboolean enter_event (GtkWidget         *widget,  
                      GdkEventCrossing  *event,  
                      gpointer           data)  
{  
    if (!processing && hidden && event->detail != GDK_NOTIFY_INFERIOR) {  
        g_message ("show");  
        panel_show (widget);  
    }  
    return FALSE;  
}  
  
gboolean leave_event (GtkWidget         *widget,  
                      GdkEventCrossing  *event,  
                      gpointer           data)  
{  
    if (!processing && !hidden && event->detail != GDK_NOTIFY_INFERIOR){  
        g_message ("hidden");  
        panel_hidden (widget);  
    }  
  
    return FALSE;  
}  
  
void size_allocate (GtkWidget     *widget,  
                    GtkAllocation *allocation,  
                    gpointer       user_data)  
{  
  g_message ("size_allocate");  
}  
  
void size_request (GtkWidget      *widget,  
                   GtkRequisition *requisition,  
                   gpointer        user_data)     
{  
  g_message ("size_request");  
}  
  
gboolean expose_event (GtkWidget      *widget,  
                       GdkEventExpose *event,  
                       gpointer        user_data)  
{  
  g_message ("expose_event");  
  return FALSE;  
}  
  
int main( int   argc,  
          char *argv[] )  
{  
    GtkWidget *window;  
    GtkWidget *box;
	GtkWidget *image;
  
    gtk_init (&argc, &argv);  
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);  
    gtk_window_set_type_hint (GTK_WINDOW (window), GDK_WINDOW_TYPE_HINT_DOCK);  
    gtk_window_set_default_size (GTK_WINDOW (window), gdk_screen_get_width (gdk_screen_get_default ()), 100);  
  
    box = gtk_vbox_new (FALSE, 0);  
    gtk_container_add (GTK_CONTAINER (window), box);  
    image = gtk_image_new_from_file ("image/pencil.png");  
    gtk_box_pack_start (GTK_BOX(box), image, TRUE, TRUE, 0);  
    
    gtk_widget_set_double_buffered (window, FALSE);  
    
    g_signal_connect (G_OBJECT (window), "size_allocate",  
                      G_CALLBACK (size_allocate), NULL);  
    g_signal_connect (G_OBJECT (window), "size-request",  
                      G_CALLBACK (size_request), NULL);  
    g_signal_connect (G_OBJECT (window), "expose-event",  
                      G_CALLBACK (expose_event), NULL);  
    g_signal_connect (G_OBJECT (window), "enter_notify_event",  
                      G_CALLBACK (enter_event), NULL);  
    g_signal_connect (G_OBJECT (window), "leave_notify_event",  
                      G_CALLBACK (leave_event), NULL);  
  
    gtk_widget_show_all (window);  
    gdk_window_set_back_pixmap(window->window,NULL,FALSE);  
    gtk_main ();  
  
    return 0;  
}  
*/
#if 1
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "types.h"

GdkPixbuf *pixbuf;
cairo_surface_t *surface;
GTimer *timer;
cairo_pattern_t *pattern;
double sin_value, cos_value;
gint width, height, disp_size;
double half_size;
guint signal_id;
int src_stride, dst_stride;
cairo_matrix_t g_mat;

void RotateImage(uint8* src, uint8* dst)
{
	FLOAT_T start_x, start_y;
	FLOAT_T x, y;
	guint32 color_value, *color_p;
	int src_x, src_y;
	int int_sin = (int)(sin_value * 1024);
	int int_cos = (int)(cos_value * 1024);
	int half = (int)half_size;
	int i, j;

	start_x = cos_value * (-width/2) - sin_value * (-height/2) + width/2;
	start_y = sin_value * (-width/2) + cos_value * (-height/2) + height/2;

	for(i=0; i<height; i++)
	{
		x = start_x;
		y = start_y;
		for(j=0; j<width; j++)
		{
			src_x = ((((int)j - half)*int_cos - ((int)i - half)*int_sin) >> 10) + width/2;
			src_y = ((((int)j - half)*int_sin + ((int)i - half)*int_cos) >> 10) + height/2;
			if(src_x >= 0 && src_x < width && src_y >= 0 && src_y < height)
			{
				color_value = *((guint32*)(&src[src_y*src_stride+src_x*4]));
				color_p = (guint32*)(&dst[i*dst_stride+j*4]);
				*color_p = color_value;
			}
			x += cos_value;
			y += sin_value;
		}

		start_x += sin_value;
		start_y += cos_value;
	}
}

void expose(GtkWidget *widget, GdkEventExpose *expose_event, void* data)
{
	static int count = 0;
	cairo_t *cr = gdk_cairo_create(widget->window);
	gdk_cairo_region(cr, expose_event->region);
	cairo_clip(cr);
	/*
	{
		cairo_surface_t *target = cairo_get_target(cr);
		uint8 *src = cairo_image_surface_get_data(surface);
		uint8 *dst = cairo_image_surface_get_data(target);

		RotateImage(src, dst);

		cairo_surface_mark_dirty(target);
	}
	*/

	//cairo_set_source(cr, pattern);
	cairo_set_matrix(cr, &g_mat);
	cairo_set_source_surface(cr, surface, 0, 0);
	cairo_paint(cr);

	cairo_destroy(cr);

	if(count == 0)
	{
		timer = g_timer_new();
		g_timer_start(timer);
	}
	count++;

	if(count == 1000)
	{
		GtkWidget *dialog;
		int pass = (int)(g_timer_elapsed(timer, NULL) * 1000);

		dialog = gtk_message_dialog_new(NULL, GTK_DIALOG_MODAL,
			GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "%d ms", pass);
		gtk_dialog_run(GTK_DIALOG(dialog));

		exit(0);
	}
}

void idle(GtkWidget *widget)
{
	gtk_widget_queue_draw(widget);
}

void size_allocate(GtkWidget *widget)
{
	GtkAdjustment *hadjust, *vadjust;
	hadjust = gtk_scrolled_window_get_hadjustment(GTK_SCROLLED_WINDOW(widget));
	vadjust = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(widget));
	gtk_adjustment_set_value(hadjust, width/2);
	gtk_adjustment_set_value(vadjust, height/2);
	g_signal_handler_disconnect(G_OBJECT(widget), signal_id);
}

int main(int argc, char** argv)
{
	GtkWidget *window, *draw, *scroll;
	double trans_x, trans_y;
	cairo_matrix_t matrix;

	gtk_init(&argc, &argv);

	pixbuf = gdk_pixbuf_new_from_file("test.png", NULL);
	surface = cairo_image_surface_create_from_png("test.png");

	width = cairo_image_surface_get_width(surface);
	height = cairo_image_surface_get_height(surface);
	disp_size = (gint)(2 * sqrt((width/2)*(width/2)+(height/2)*(height/2)) + 1);
	half_size = disp_size * 0.5;
	src_stride = width*4;
	dst_stride = disp_size*4;

	sin_value = sin(G_PI/6);
	cos_value = cos(G_PI/6);

	trans_x = - half_size + ((width / 2) * cos_value + (height / 2) * sin_value);
	trans_y = - half_size + ((width / 2) * sin_value - (height / 2) * cos_value);

	pattern = cairo_pattern_create_for_surface(surface);
	cairo_matrix_init_rotate(&matrix, G_PI/6);
	cairo_matrix_translate(&matrix, trans_x, trans_y);
	cairo_pattern_set_filter(pattern, CAIRO_FILTER_FAST);
	cairo_pattern_set_matrix(pattern, &matrix);

	cairo_matrix_init_rotate(&g_mat, -G_PI/6);
	cairo_matrix_translate(&g_mat, -trans_x, -trans_y);

	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_size_request(window, 1500, 1000);
	draw = gtk_drawing_area_new();
	gtk_widget_set_size_request(draw, disp_size, disp_size);
	scroll = gtk_scrolled_window_new(NULL, NULL);
	signal_id = g_signal_connect(G_OBJECT(scroll), "size-allocate", G_CALLBACK(size_allocate), NULL);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scroll), draw);
	gtk_container_add(GTK_CONTAINER(window), scroll);

	gtk_widget_add_events(draw, GDK_EXPOSURE_MASK);
	g_signal_connect(G_OBJECT(draw), "expose_event", G_CALLBACK(expose), NULL);
	g_idle_add((GSourceFunc)idle, draw);

	gtk_widget_show_all(window);

	gtk_main();

	return 0;
}
#else


#include <stdio.h>
#include <gtk/gtk.h>

static gboolean
event_cb (GtkWidget * widget, GdkEvent * ev)
{
  gboolean handled;

  handled = FALSE;

  switch (ev->type)
    {
    case GDK_BUTTON_PRESS:
      printf ("button %d press\n", ev->button.button);
      handled = TRUE;
      break;

    case GDK_BUTTON_RELEASE:
      printf ("button %d release\n", ev->button.button);
      handled = TRUE;
      break;

    case GDK_MOTION_NOTIFY:
      /* A hint? Read the position to get the latest value.
       */
      if (ev->motion.is_hint)
        {
          GdkDisplay *display = gtk_widget_get_display (widget);
          GdkScreen *screen;
          int x_root, y_root;

          printf ("seen a hint!\n");

          gdk_display_get_pointer (display, &screen, &x_root, &y_root, NULL);
          ev->motion.x_root = x_root;
          ev->motion.y_root = y_root;
        }

      printf ("motion at %g x %g\n", ev->motion.x_root, ev->motion.y_root);

      if (ev->motion.state & GDK_BUTTON1_MASK)
        printf ("(and btn1 held down)\n");
      if (ev->motion.state & GDK_BUTTON2_MASK)
        printf ("(and btn2 held down)\n");

      handled = TRUE;

      break;

    default:
      break;
    }

  return (handled);
}

int
main (int argc, char **argv)
{
  GtkWidget *win;
  GtkWidget *area;

  gtk_init (&argc, &argv);
  win = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  g_signal_connect (win, "destroy", G_CALLBACK (gtk_main_quit), NULL);

  area = gtk_drawing_area_new ();
  gtk_widget_add_events (GTK_WIDGET (area),
                         GDK_POINTER_MOTION_MASK |
                         GDK_POINTER_MOTION_HINT_MASK |
                         GDK_BUTTON_PRESS_MASK |
                         GDK_BUTTON_RELEASE_MASK);

  g_signal_connect_after (G_OBJECT (area), "event",
                            GTK_SIGNAL_FUNC (event_cb), NULL);

  gtk_container_add (GTK_CONTAINER (win), area);

  gtk_window_set_default_size (GTK_WINDOW (win), 250, 250);
  gtk_widget_show_all (win);

  gtk_main ();

  return (0);
}
#endif
/*
#include <gtk/gtk.h>

#define DND_INFO_TEXT_URI_LIST 10

static GtkTargetEntry target[] = {
  { "text/uri-list",  0, DND_INFO_TEXT_URI_LIST }
};

void drag_data_received(GtkWidget *widget, 
                        GdkDragContext *context,
                        gint x,
                        gint y,
                        GtkSelectionData *data,
                        guint info,
                        guint time_,
                        gpointer user_data)
{
  gchar *fname = g_filename_from_uri(*g_uri_list_extract_uris(data->data), NULL, NULL);
  gtk_label_set_text (user_data, fname);
}

int main(int argc, char ** argv)
{
  GtkWidget *window;
  GtkWidget *label;

  gtk_init(&argc, &argv);

  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(window), "DnD");

  gtk_window_set_default_size(GTK_WINDOW(window), 150, 150);

  g_signal_connect(G_OBJECT(window), "destroy",
           G_CALLBACK(gtk_main_quit), NULL);

  label = gtk_label_new("ここにドロップ");

  gtk_widget_set_size_request(label, 150, 150);
  gtk_container_add(GTK_CONTAINER(window), label);

  g_signal_connect(G_OBJECT(label), "drag-data-received",
                   G_CALLBACK(drag_data_received), label);

  gtk_drag_dest_set(label, GTK_DEST_DEFAULT_ALL, target, 1, GDK_ACTION_COPY);

  gtk_widget_show_all(window);

  gtk_main();
  return 0;
}
*/
#endif
