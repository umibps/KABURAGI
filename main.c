#include "configure.h"

#ifdef _DEBUG
//# define CHECK_MEMORY_POOL 1
#endif

#include <string.h>
#include <locale.h>
#include <gtk/gtk.h>
#include "application.h"
#include "MikuMikuGtk+/tbb.h"
#include "memory.h"
#include <gtk/gtkgl.h>

#if defined(_MSC_VER)

#if !defined(NO_KABURAGI_LIB) || NO_KABURAGI_LIB == 0
# pragma comment(lib, "KABURAGI.lib")
#endif

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

#if 1
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
//#    pragma comment(lib, "cairo.lib")
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
//#    pragma comment(lib, "./STATIC_LIB/libcairo.a")
//#    pragma comment(lib, "./STATIC_LIB/libcairo-gobject.a")
//#    pragma comment(lib, "./STATIC_LIB/libcairo-script-interpreter.a")
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
#     pragma comment(lib, "libpng.lib")
#    else
#     pragma comment(lib, "libpng.lib")
#    endif
#    pragma comment(lib, "zlib.lib")
//#    pragma comment(lib, "cairo.lib")
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
#    pragma comment(lib, "libgettextsrc-0-18-2.lib")
#    pragma comment(lib, "libgettextpo-0.lib")
#    pragma comment(lib, "libgettextlib-0-18-2.lib")
#    pragma comment(lib, "libintl-8.lib")

#    pragma comment(lib, "libatk-1.0-0.lib")
//#    pragma comment(lib, "libcairo-2.lib")
//#    pragma comment(lib, "libcairo-gobject-2.lib")
//#    pragma comment(lib, "libcairo-script-interpreter-2.lib")
#    pragma comment(lib, "libcroco-0.6-3.lib")
#    pragma comment(lib, "libffi-6.lib")
#    pragma comment(lib, "libfontconfig-1.lib")
#    pragma comment(lib, "libfreetype-6.lib")
#    pragma comment(lib, "libgailutil-3-0.lib")
#    pragma comment(lib, "libgdk_pixbuf-2.0-0.lib")
#    pragma comment(lib, "libgdk-3-0.lib")
#    pragma comment(lib, "libgio-2.0-0.lib")
#    pragma comment(lib, "libglib-2.0-0.lib")
#    pragma comment(lib, "libgmodule-2.0-0.lib")
#    pragma comment(lib, "libgthread-2.0-0.lib")
#    pragma comment(lib, "libgtk-3-0.lib")
#    pragma comment(lib, "libjasper-1.lib")
#    pragma comment(lib, "libjpeg-9.lib")
#    pragma comment(lib, "liblzma-5.lib")
#    pragma comment(lib, "libpango-1.0-0.lib")
#    pragma comment(lib, "libpangocairo-1.0-0.lib")
#    pragma comment(lib, "libpangoft2-1.0-0.lib")
#    pragma comment(lib, "libpangowin32-1.0-0.lib")
//#    pragma comment(lib, "libpixman-1-0.lib")
#    pragma comment(lib, "librsvg-2-2.lib")
#    pragma comment(lib, "libtiff-5.lib")
#    pragma comment(lib, "libxml2-2.lib")
#    pragma comment(lib, "zdll.lib")

#   endif
#  endif
# else
#  ifdef _DEBUG
#   pragma comment(linker, "/NODEFAULTLIB:LIBCMTD")
#  else
//#   pragma comment(linker, "/NODEFAULTLIB:LIBCMT")
#  endif
#  if GTK_MAJOR_VERSION <= 2
#   pragma comment(lib, "atk-1.0.lib")
//#   pragma comment(lib, "cairo.lib")
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
//#    pragma comment(lib, "cairo.lib")
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

#    pragma comment(lib, "libatk-1.0-0.lib")
//#    pragma comment(lib, "libcairo.dll.a")
//#    pragma comment(lib, "libcairo-gobject.dll.a")
//#    pragma comment(lib, "libcairo-script-interpreter.dll.a")
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

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _DEBUG
void AtExit(void)
{
	printf("Does have an error...?\n");
}

#endif

#if 1 //!defined(_MSC_VER) || !defined(_DEBUG)
int main(int argc, char** argv)
{
	static APPLICATION application = {0};
	
	static APPLICATION_LABELS labels =
	{
		"English",
		{"OK", "Apply", "Cancel", "Normal", "Reverse", "Edit Selection", "Window",
			"Close Window", "Dock Left", "Dock Right", "Full Screen", "Reference Image Window",
			"Move Top Left", "Hot Key", "Loading...", "Saving..."},
		{"pixel", "Length", "Angle", "_BG", "Loop", "Preview", "Interval", "minute", "Detail", "Target",
			"Clip Board", "Name", "Type", "Resolution", "Center", "Straight", "Grow", "Mode", "Red", "Green", "Blue",
			"Cyan", "Magenta", "Yellow", "Key Plate", "Add", "Delete"},
		{"File", "New", "Open", "Open as Layer", "Save", "Save as", "Close", "Quit", "Edit", "Undo", "Redo",
			"Copy", "Copy Visible", "Cut", "Paste", "Clipboard", "Transform", "Projection", "Canvas",
			"Change Resolution", "Change Canvas Size", "Flip Canvas Horizontally", "Flip Canvas Vertically",
			"Switch _BG Color", "Change 2nd _BG Color", "Change ICC Profile", "Layer", "_New Layer(Color)",
			"_New Layer(Vector)", "_New Layer Set", "_New 3D Modeling Layer", "_New Adjustment Lyaer", "_Copy Layer",
			"_Delete Layer", "Fill with _FG Color", "Fill with Pattern", "Rasterize Layer", "Merge Down",
			"_Flatten Image", "_Visible to Layer", "Visible Copy", "Select", "None", "Invert", "All", "Grow",
			"Shrink", "View", "Zoom", "Zoom _In", "Zoom _Out", "Actual Size", "Reverse Horizontally", "Rotate",
			"Reset Roatate", "Display Filters", "Nothing", "Gray Scale", "Gray Scale (YIQ)", "Filters", "Blur",
			"Motion Blur", "Gaussian Blur", "Brightness & Contrast", "Hue & Saturation", "Levels", "Tone Curve",
			"Luminosity to Opacity", "Color to Alpha", "Colorize with Under Layer", "Gradation Map",
			"Map with Detect Max Black Value", "Transparency as White", "Fill with Vector", "Render", "Cloud",
			"Fractal", "Trace Pixels", "Plug-in", "Script", "Help", "Version"
		},
		{"New", "New Canvas", "Width", "Height", "2nd BG Color", "Adopt ICC Profile?", "Preset", "Add Preset",
			"Swap Height and Width"},
		{"Tool Box", "Initialize", "New Brush", "Smooth", "Quality", "Rate", "Gaussian", "Average", "Magnification",
			"Brush Size", "Scale", "Flow", "Pressure", "Extend Range", "Blur", "OutLine Hardness",
			"Color Extends", "Start Distance of Drag and Move", "Anti Alias", "Change Text Color", "Horizonal",
			"Vertical", "Style", "Normal", "Bold", "Italic", "Oblique", "Balloon", "Balloon Has Edge", "Line Color",
			"Fill Color", "Line Width", "Change Line Width", "Manually Set", "Aspect Ratio", "Centering Horizontally",
			"Centering Vertically", "Adjust Range to Text", "Number of Edge,", "Edge Size", "Edge Size Random",
			"Edge Distance Random", "Number of Children", "Start Child Size", "End Child Size", "Reverse",
			"Reverse Horizontally", "Reverse Vertically", "Blend Mode", "Hue", "Saturation", "Brightness", "Contrast",
			"Distance", "Rotate Start", "Rotate Speed", "Random Rotate", "Rotate to Brush Direction", "Size Range",
			"Rotate Range", "Random Size", "Clockwise", "Counter Clockwise", "Both Direction", "Minimum Degree",
			"Minumum Distance", "Minimum Pressure", "Enter", "Out", "Enter & Out", "Mix", "Reverse FG BG",
			"Devide Stroke", "Delete Stroke", "Target", "Stroke", "Prior Angle", "Control Point", "Free", "Scale",
			"Free Shape", "Rotate", "Preference", "Name", "Copy Brush", "Change Brush", "Delete Brush", "Texture",
			"Strength", "No Texture", "Add Color", "Delete Color", "Load Pallete", "Add Pallete", "Write Pallete",
			"Clear Pallete", "Pick Mode", "Single Pixel", "Average Color", "Open Path", "Close Path", "Update",
			"Frequency", "Cloud Color", "Persistence", "Random Seed", "Use Random", "Update Immediately",
			"Number of Octaves", "Linear", "Cosine", "Cubic", "Colorize", "Start Editting 3D Model",
			"Finish Editting 3D Model", "Scatter", "Scatter Size", "Scatter Range", "Random Size Scatter",
			"Random Flow Scatter", "Bevel", "Round", "Mitter", "Normal Brush",
			{"Pencil", "Hard Pen", "Air Brush", "Old Air Brush", "Water Color Brush", "Picker Brush", "Eraser", "Bucket",
				"Pattern Fill", "Blur Tool", "Smudge", "Mix Brush", "Gradation", "Text Tool", "Stamp Tool",
				"Image Brush", "Image Blend\nBrush", "Picker Image Brush", "Script Brush", "Custom Brush", "PLUG_IN"},
			{"Detection Target", "Detect from ... ", "Pixels Color", "Pixel Color + Alpha",
				"Alpha", "Active Layer", "Under Layer", "Canvas", "Threshold", "Detection Area", "Normal", "Large"},
			{"Select/Release", "Move Control Point", "Change Pressure", "Delete Control Point",
				"Move Stroke", "Copy & Move Stroke", "Joint Stroke"},
			{"Circle", "Eclipse", "Triangle", "Square", "Rhombus", "Hexagon", "Star", "Pattern", "Image"}
		},
		{"Layer", "Layer", "Vector", "Layer Set", "Text", "Adjustment", "3D Modeling", "Add Layer", "Add Vector Layer",
			"Add Layer Set", "Add 3D Modeling Layer", "Add Adjustment Layer", "Rename Layer", "Reorder Layer",
			"Opacity to Selection Area", "Opacity Add Selection Area", "Pasted Layer", "Under Layer",
			"Mixed Under Layer", "Blend Mode", "Opacity", "Masking with Under Layer", "Lock Opacity",
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

	static FRACTAL_LABEL fractal_labels =
	{
		"Triangle", "Transform", "Variations", "Colors", "Random",
		"Weight", "Symmetry", "Linear", "Sinusoidal", "Spherical",
		"Swirl", "Shorseshoe", "Polar", "Handkerchief", "Heart", "Disc",
		"Spiral", "Hyperbolic", "Diamond", "Ex", "Julia", "Bent", "Waves",
		"Fish Eye", "Pop Corn", "Preserve Weights", "Update", "Update Immediately",
		"Adjust", "Rendering", "Gamma", "Brightness", "Vibrancy", "Camera", "Zoom",
		"Option", "Oversample", "Filter Radius", "Forced Symmetry", "Type",
		"Order", "None", "Bilateral", "Rotational", "Dihedral", "Mutation",
		"Directions", "Controls", "Speed", "Trend", "Auto Zoom", "Add", "Delete",
		"Flip Horizontal", "Flip Vertical", "Flip Horizontal All",
		"Flip Vertical All", "Use Random", "Rest", "Create ID"
	};

	void *tbb = NULL;

#if defined(CHECK_MEMORY_POOL) && CHECK_MEMORY_POOL != 0
	_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF);
#endif

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
	application.flags |= APPLICATION_HAS_3D_LAYER;
	tbb = TbbObjectNew();
#endif

#ifdef _DEBUG
	atexit(AtExit);
#endif

	{
		gchar *raw_path;

		application.labels = &labels;
		application.fractal_labels = &fractal_labels;

#if GTK_MAJOR_VERSION <= 2
		gtk_set_locale();
#endif
		gtk_init(&argc, &argv);

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
		gtk_gl_init(&argc, &argv);
#endif

		raw_path = g_locale_to_utf8(argv[0], -1, NULL, NULL, NULL);
		application.current_path = g_path_get_dirname(raw_path);

		InitializeApplication(&application, INITIALIZE_FILE_NAME);
		g_free(raw_path);
	}

	/*
	{
		cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 500, 500);
		cairo_t *cairo_p = cairo_create(surface);

		cairo_scale(cairo_p, 1, 2);
		cairo_arc(cairo_p, 100, 100, 50, 0, 2*G_PI);
		cairo_fill(cairo_p);

		cairo_surface_write_to_png(surface, "test.png");

		cairo_destroy(cairo_p);
	}
	*/

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

#else

#include <windows.h>
#define INCLUDE_WIN_DEFAULT_API

int test(char** argv)
{
	int argc = 1;
	static APPLICATION application = {0};
	
	static APPLICATION_LABELS labels =
	{
		"English",
		{"OK", "Apply", "Cancel", "Normal", "Reverse", "Edit Selection", "Window",
			"Close Window", "Dock Left", "Dock Right", "Full Screen", "Reference Image Window",
			"Move Top Left", "Hot Key", "Loading...", "Saving..."},
		{"pixel", "Length", "Angle", "_BG", "Loop", "Preview", "Interval", "minute", "Detail", "Target",
			"Clip Board", "Name", "Type", "Resolution", "Center", "Straight", "Grow", "Mode", "Red", "Green", "Blue",
			"Cyan", "Magenta", "Yellow", "Key Plate", "Add", "Delete"},
		{"File", "New", "Open", "Open as Layer", "Save", "Save as", "Close", "Quit", "Edit", "Undo", "Redo",
			"Copy", "Copy Visible", "Cut", "Paste", "Clipboard", "Transform", "Projection", "Canvas",
			"Change Resolution", "Change Canvas Size", "Flip Canvas Horizontally", "Flip Canvas Vertically",
			"Switch _BG Color", "Change 2nd _BG Color", "Change ICC Profile", "Layer", "_New Layer(Color)",
			"_New Layer(Vector)", "_New Layer Set", "_New 3D Modeling Layer", "_New Adjustment Lyaer", "_Copy Layer",
			"_Delete Layer", "Fill with _FG Color", "Fill with Pattern", "Rasterize Layer", "Merge Down",
			"_Flatten Image", "_Visible to Layer", "Visible Copy", "Select", "None", "Invert", "All", "Grow",
			"Shrink", "View", "Zoom", "Zoom _In", "Zoom _Out", "Actual Size", "Reverse Horizontally", "Rotate",
			"Reset Roatate", "Display Filters", "Nothing", "Gray Scale", "Gray Scale (YIQ)", "Filters", "Blur",
			"Motion Blur", "Gaussian Blur", "Brightness & Contrast", "Hue & Saturation", "Levels", "Tone Curve",
			"Luminosity to Opacity", "Color to Alpha", "Colorize with Under Layer", "Gradation Map",
			"Map with Detect Max Black Value", "Transparency as White", "Fill with Vector", "Render", "Cloud",
			"Fractal", "Trace Pixels", "Plug-in", "Script", "Help", "Version"
		},
		{"New", "New Canvas", "Width", "Height", "2nd BG Color", "Adopt ICC Profile?", "Preset", "Add Preset",
			"Swap Height and Width"},
		{"Tool Box", "Initialize", "New Brush", "Smooth", "Quality", "Rate", "Gaussian", "Average", "Magnification",
			"Brush Size", "Scale", "Flow", "Pressure", "Extend Range", "Blur", "OutLine Hardness",
			"Color Extends", "Start Distance of Drag and Move", "Anti Alias", "Change Text Color", "Horizonal",
			"Vertical", "Style", "Normal", "Bold", "Italic", "Oblique", "Balloon", "Balloon Has Edge", "Line Color",
			"Fill Color", "Line Width", "Change Line Width", "Manually Set", "Aspect Ratio", "Centering Horizontally",
			"Centering Vertically", "Adjust Range to Text", "Number of Edge,", "Edge Size", "Edge Size Random",
			"Edge Distance Random", "Number of Children", "Start Child Size", "End Child Size", "Reverse",
			"Reverse Horizontally", "Reverse Vertically", "Blend Mode", "Hue", "Saturation", "Brightness", "Contrast",
			"Distance", "Rotate Start", "Rotate Speed", "Random Rotate", "Rotate to Brush Direction", "Size Range",
			"Rotate Range", "Random Size", "Clockwise", "Counter Clockwise", "Both Direction", "Minimum Degree",
			"Minumum Distance", "Minimum Pressure", "Enter", "Out", "Enter & Out", "Mix", "Reverse FG BG",
			"Devide Stroke", "Delete Stroke", "Target", "Stroke", "Prior Angle", "Control Point", "Free", "Scale",
			"Free Shape", "Rotate", "Preference", "Name", "Copy Brush", "Change Brush", "Delete Brush", "Texture",
			"Strength", "No Texture", "Add Color", "Delete Color", "Load Pallete", "Add Pallete", "Write Pallete",
			"Clear Pallete", "Pick Mode", "Single Pixel", "Average Color", "Open Path", "Close Path", "Update",
			"Frequency", "Cloud Color", "Persistence", "Random Seed", "Use Random", "Update Immediately",
			"Number of Octaves", "Linear", "Cosine", "Cubic", "Colorize", "Start Editting 3D Model",
			"Finish Editting 3D Model", "Scatter", "Scatter Size", "Scatter Range", "Random Size Scatter",
			"Random Flow Scatter", "Bevel", "Round", "Mitter", "Normal Brush",
			{"Pencil", "Hard Pen", "Air Brush", "Old Air Brush", "Water Color Brush", "Picker Brush", "Eraser", "Bucket",
				"Pattern Fill", "Blur Tool", "Smudge", "Mix Brush", "Gradation", "Text Tool", "Stamp Tool",
				"Image Brush", "Image Blend\nBrush", "Picker Image Brush", "Script Brush", "Custom Brush", "PLUG_IN"},
			{"Detection Target", "Detect from ... ", "Pixels Color", "Pixel Color + Alpha",
				"Alpha", "Active Layer", "Under Layer", "Canvas", "Threshold", "Detection Area", "Normal", "Large"},
			{"Select/Release", "Move Control Point", "Change Pressure", "Delete Control Point",
				"Move Stroke", "Copy & Move Stroke", "Joint Stroke"},
			{"Circle", "Eclipse", "Triangle", "Square", "Rhombus", "Hexagon", "Star", "Pattern", "Image"}
		},
		{"Layer", "Layer", "Vector", "Layer Set", "Text", "Adjustment", "3D Modeling", "Add Layer", "Add Vector Layer",
			"Add Layer Set", "Add 3D Modeling Layer", "Add Adjustment Layer", "Rename Layer", "Reorder Layer",
			"Opacity to Selection Area", "Opacity Add Selection Area", "Pasted Layer", "Under Layer",
			"Mixed Under Layer", "Blend Mode", "Opacity", "Masking with Under Layer", "Lock Opacity",
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

	static FRACTAL_LABEL fractal_labels =
	{
		"Triangle", "Transform", "Variations", "Colors", "Random",
		"Weight", "Symmetry", "Linear", "Sinusoidal", "Spherical",
		"Swirl", "Shorseshoe", "Polar", "Handkerchief", "Heart", "Disc",
		"Spiral", "Hyperbolic", "Diamond", "Ex", "Julia", "Bent", "Waves",
		"Fish Eye", "Pop Corn", "Preserve Weights", "Update", "Update Immediately",
		"Adjust", "Rendering", "Gamma", "Brightness", "Vibrancy", "Camera", "Zoom",
		"Option", "Oversample", "Filter Radius", "Forced Symmetry", "Type",
		"Order", "None", "Bilateral", "Rotational", "Dihedral", "Mutation",
		"Directions", "Controls", "Speed", "Trend", "Auto Zoom", "Add", "Delete",
		"Flip Horizontal", "Flip Vertical", "Flip Horizontal All",
		"Flip Vertical All", "Use Random", "Rest", "Create ID"
	};

	void *tbb = NULL;

#if defined(CHECK_MEMORY_POOL) && CHECK_MEMORY_POOL != 0
	_CrtSetDbgFlag(_CRTDBG_CHECK_ALWAYS_DF);
#endif

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
	application.flags |= APPLICATION_HAS_3D_LAYER;
	tbb = TbbObjectNew();
#endif

#ifdef _DEBUG
	atexit(AtExit);
#endif

	{
		gchar *raw_path;

		application.labels = &labels;
		application.fractal_labels = &fractal_labels;

#if GTK_MAJOR_VERSION <= 2
		gtk_set_locale();
#endif
		gtk_init(&argc, &argv);

#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
		gtk_gl_init(&argc, &argv);
#endif

		raw_path = g_locale_to_utf8(argv[0], -1, NULL, NULL, NULL);
		application.current_path = g_path_get_dirname(raw_path);

		InitializeApplication(&application, INITIALIZE_FILE_NAME);
		g_free(raw_path);
	}

	/*
	{
		cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 500, 500);
		cairo_t *cairo_p = cairo_create(surface);

		cairo_scale(cairo_p, 1, 2);
		cairo_arc(cairo_p, 100, 100, 50, 0, 2*G_PI);
		cairo_fill(cairo_p);

		cairo_surface_write_to_png(surface, "test.png");

		cairo_destroy(cairo_p);
	}
	*/

	if(argc > 1)
	{
		//gchar *file_path = g_locale_to_utf8(argv[1], -1, NULL, NULL, NULL);

		//OpenFile(file_path, &application);

		//g_free(file_path);

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

int WINAPI main(int argc, char** argv)
{
	DWORD dwD;
	MSG msg;
	HANDLE id;
	
	id = CreateThread(NULL,0,test,(LPVOID)argv,0,&dwD);

	while(1){// (GetMessage (&msg,NULL,0,0)) { /* メッセージループ */
		DWORD code;
		if(GetExitCodeThread(id, &code), code != STILL_ACTIVE)
		{
			(void)printf("Did have an error...?\n");
			return 0;
		}
		TranslateMessage(&msg);
		DispatchMessage(&msg);

	}
	

	return 0;

}

#endif

#ifdef __cplusplus
}
#endif
