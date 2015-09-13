#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include "spin_scale.h"
#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

#define WIDGET_TYPE_SPIN_SCALE				(SpinScaleGetType())
#define WIDGET_SPIN_SCALE(OBJ)				(G_TYPE_CHECK_INSTANCE_CAST((OBJ), WIDGET_TYPE_SPIN_SCALE, SPIN_SCALE))
#define WIDGET_SPIN_SCALE_CLASS(CLASS)		(G_TYPE_CHECK_CLASS_CAST((CLASS), SPIN_SCALE_CLASS))
#define WIDGET_IS_SPIN_SCALE(CLASS)			(G_TYPE_CHECK_INSTANCE_TYPE((CLASS), WIDGET_TYPE_SPIN_SCALE))
#define WIDGET_SPIN_SCALE_GET_CLASS(OBJ)	(G_TYPE_INSTANCE_GET_CLASS((OBJ), WIDGET_TYPE_SPIN_SCALE, SpinScaleClass))
#define GET_PRIVATE(OBJ)					(G_TYPE_INSTANCE_GET_PRIVATE((OBJ), WIDGET_TYPE_SPIN_SCALE, SPIN_SCALE_PRIVATE))

typedef struct _SPIN_SCALE_CLASS
{
	GtkSpinButtonClass parent_class;
} SPIN_SCALE_CLASS;

typedef enum _ePROPS
{
	PROP_0,
	PROP_LABEL
} ePROPS;

typedef enum _eSPIN_SCALE_TARGET
{
	TARGET_NUMBER,
	TARGET_UPPER,
	TARGET_LOWER
} eSPIN_SCALE_TARGET;

typedef struct _SPIN_SCALE_PRIVATE
{
	gchar *label;

	gboolean scale_limits_set;
	gdouble scale_lower, scale_upper;

	PangoLayout *layout;
	gboolean changing_value;
	gboolean relative_change;
	gdouble start_x, start_value;
} SPIN_SCALE_PRIVATE;

// 関数のプロトタイプ宣言
static void SpinScaleInit(SPIN_SCALE* self);
static void SpinScaleClassInit(SPIN_SCALE_CLASS* data);

static gpointer SpinScaleParentClass = NULL;

static void SpinScaleClassInternInit(gpointer data)
{
	SpinScaleParentClass = g_type_class_peek_parent(data);
	SpinScaleClassInit((SPIN_SCALE_CLASS*)data);
}

GType SpinScaleGetType(void)
{
	static volatile gsize g_define_typeid__volatile = 0;

	if(g_once_init_enter(&g_define_typeid__volatile) != FALSE)
	{
		GType g_define_type_id =
			g_type_register_static_simple(
			GTK_TYPE_SPIN_BUTTON,
				g_intern_static_string("SPIN_SCALE"),
				sizeof(SPIN_SCALE_CLASS),
				(GClassInitFunc)SpinScaleClassInternInit,
				sizeof(SPIN_SCALE),
				(GInstanceInitFunc)SpinScaleInit,
				(GTypeFlags)0
			);

		{
#define _G_DEFINE_TYPE_EXTENDED()
		}

		g_once_init_leave(&g_define_typeid__volatile, g_define_type_id);
	}

	return g_define_typeid__volatile;
}

GType WidgetSpinScaleGetType (void)G_GNUC_CONST;

static void SpinScaleDispose(GObject *object)
{
	SPIN_SCALE_PRIVATE* data = GET_PRIVATE(object);

	if(data->layout != NULL)
	{
		g_object_unref(data->layout);
		data->layout = NULL;
	}

	G_OBJECT_CLASS(SpinScaleParentClass)->dispose(object);
}

static void SpinScaleFinalize(GObject *object)
{
	SPIN_SCALE_PRIVATE* data = GET_PRIVATE(object);

	if(data->label != NULL)
	{
		g_free(data->label);
		data->label = NULL;
	}

	G_OBJECT_CLASS(SpinScaleParentClass)->finalize(object);
}

static void SpinScaleSetProperty(
	GObject *object,
	guint property_id,
	const GValue *value,
	GParamSpec *pspec
)
{
	SPIN_SCALE_PRIVATE* data = GET_PRIVATE(object);

	switch(property_id)
	{
	case PROP_LABEL:
		g_free(data->label);
		data->label = g_value_dup_string(value);
		if(data->layout != NULL)
		{
			g_object_unref(data->layout);
			data->layout = NULL;
		}
		gtk_widget_queue_resize(GTK_WIDGET(object));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

static void SpinScaleGetProperty(
	GObject *object,
	guint property_id,
	GValue *value,
	GParamSpec *pspec
)
{
	SPIN_SCALE_PRIVATE* data = GET_PRIVATE(object);

	switch(property_id)
	{
	case PROP_LABEL:
		g_value_set_string(value, data->label);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
		break;
	}
}

#if GTK_MAJOR_VERSION <= 2
static void SpinScaleSizeRequest(GtkWidget* widget, GtkRequisition* requisition)
{
	SPIN_SCALE_PRIVATE* data = GET_PRIVATE(widget);
	GtkStyle *style = gtk_widget_get_style(widget);
	PangoContext *context = gtk_widget_get_pango_context(widget);
	PangoFontMetrics *metrics;
	gint height;

	GTK_WIDGET_CLASS(SpinScaleParentClass)->size_request(widget, requisition);

	metrics = pango_context_get_metrics(context, style->font_desc,
		pango_context_get_language(context));

	height = PANGO_PIXELS(pango_font_metrics_get_ascent(metrics) +
		pango_font_metrics_get_descent(metrics));

	requisition->height += height;

	if(data->label != NULL)
	{
		gint char_width, digit_width, char_pixels;

		char_width = pango_font_metrics_get_approximate_char_width(metrics);
		digit_width = pango_font_metrics_get_approximate_digit_width(metrics);
		char_pixels = PANGO_PIXELS(MAX(char_width, digit_width));

		requisition->width += char_pixels * 3;
	}

	pango_font_metrics_unref(metrics);
}
#else
static void SpinScaleGetPreferredWidth(
	GtkWidget* widget,
	gint* minimal_width,
	gint* natural_width
)
{
	SPIN_SCALE_PRIVATE* data = GET_PRIVATE(widget);
	GtkStyle *style = gtk_widget_get_style(widget);
	PangoContext *context = gtk_widget_get_pango_context(widget);
	PangoFontMetrics *metrics;

	GTK_WIDGET_CLASS(SpinScaleParentClass)->get_preferred_width(
		widget, minimal_width, natural_width);

	metrics = pango_context_get_metrics(context, style->font_desc,
		pango_context_get_language(context));

	if(data->label != NULL)
	{
		gint char_width, digit_width, char_pixels;

		char_width = pango_font_metrics_get_approximate_char_width(metrics);
		digit_width = pango_font_metrics_get_approximate_digit_width(metrics);
		char_pixels = PANGO_PIXELS(MAX(char_width, digit_width));

		*natural_width += char_pixels * 3;
	}

	*minimal_width = *natural_width;

	pango_font_metrics_unref(metrics);
}

static void SpinScaleGetPreferredHeight(
	GtkWidget* widget,
	gint* minimal_height,
	gint* natural_height
)
{
	SPIN_SCALE_PRIVATE* data = GET_PRIVATE(widget);
	GtkStyle *style = gtk_widget_get_style(widget);
	PangoContext *context = gtk_widget_get_pango_context(widget);
	PangoFontMetrics *metrics;
	gint height;

	GTK_WIDGET_CLASS(SpinScaleParentClass)->get_preferred_height(
		widget, minimal_height, natural_height);

	metrics = pango_context_get_metrics(context, style->font_desc,
		pango_context_get_language(context));

	height = PANGO_PIXELS(pango_font_metrics_get_ascent(metrics) +
		pango_font_metrics_get_descent(metrics));

	*minimal_height += height;
	*natural_height = *minimal_height;

	pango_font_metrics_unref(metrics);
}
#endif

static void SpinScaleStyleSet(GtkWidget* widget, GtkStyle *prev_style)
{
	SPIN_SCALE_PRIVATE* data = GET_PRIVATE(widget);

	GTK_WIDGET_CLASS(SpinScaleParentClass)->style_set(widget, prev_style);

	if(data->layout != NULL)
	{
		g_object_unref(data->layout);
		data->layout = NULL;
	}
}

#if GTK_MAJOR_VERSION <= 2
static gboolean SpinScaleExpose(GtkWidget *widget, GdkEventExpose *pevent)
#else
static gboolean SpinScaleExpose(GtkWidget *widget, cairo_t* cairo_p)
#endif
{
	SPIN_SCALE_PRIVATE *data = GET_PRIVATE(widget);
	GtkStyle *style = gtk_widget_get_style(widget);
#if GTK_MAJOR_VERSION <= 2
	cairo_t *cairo_p;
#else
	GdkRectangle text_area;
	double left, right;
	double top, bottom;
	gboolean is_entry;
#endif
	gboolean rtl;
	gint width, height;

#if GTK_MAJOR_VERSION <= 2
	GTK_WIDGET_CLASS(SpinScaleParentClass)->expose_event(widget, pevent);

	cairo_p = gdk_cairo_create(pevent->window);
	gdk_cairo_region(cairo_p, pevent->region);
	cairo_clip(cairo_p);
#else
	GTK_WIDGET_CLASS(SpinScaleParentClass)->draw(widget, cairo_p);
	cairo_clip_extents(cairo_p, &left, &top, &right, &bottom);
#endif

	rtl = (gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL);

#if GTK_MAJOR_VERSION <= 2
	width = gdk_window_get_width(pevent->window);
	height = gdk_window_get_height(pevent->window);
#else
	width = (gint)(right - left);
	height = (gint)(bottom - top);
	gtk_entry_get_text_area(GTK_ENTRY(widget), &text_area);

	is_entry = (width == text_area.width && height == text_area.height)
		? TRUE : FALSE;
#endif

#if GTK_MAJOR_VERSION <= 2
	if(pevent->window == gtk_entry_get_text_window(GTK_ENTRY(widget)))
#else
	//if(is_entry != FALSE)
	if(1)
#endif
	{
		// スピンボタン横を非表示
		if(rtl != FALSE)
		{
			cairo_rectangle(cairo_p, -0.5, 0.5, width, height - 1);
		}
		else
		{
			cairo_rectangle(cairo_p, 0.5, 0.5, width, height - 1);
		}

		gdk_cairo_set_source_color(cairo_p, &style->text[gtk_widget_get_state(widget)]);

		cairo_stroke(cairo_p);
	}
	else
	{
		// テキストボックス横を非表示
		if(rtl != FALSE)
		{
			cairo_rectangle(cairo_p, 0.5, 0.5, width, height - 1);
		}
		else
		{
			cairo_rectangle(cairo_p, -0.5, 0.5, width, height - 1);
		}

		gdk_cairo_set_source_color(cairo_p, &style->text[gtk_widget_get_state(widget)]);

		cairo_stroke(cairo_p);

		if(rtl != FALSE)
		{
			cairo_rectangle(cairo_p, 1.5, 1.5, width - 2, height - 3);
		}
		else
		{
			cairo_rectangle(cairo_p, 0.5, 1.5, width - 2, height - 3);
		}

		gdk_cairo_set_source_color(cairo_p, &style->base[gtk_widget_get_state(widget)]);

		cairo_stroke(cairo_p);
	}

	if(data->label != NULL && gtk_widget_is_drawable(widget)
#if GTK_MAJOR_VERSION <= 2
		&& pevent->window == gtk_entry_get_text_window(GTK_ENTRY(widget)))
#else
		)//&& is_entry != FALSE)
#endif
	{
		GtkRequisition requisition;
		GtkAllocation allocation;
		PangoRectangle logical;
		gint layout_offset_x, layout_offset_y;

#if GTK_MAJOR_VERSION <= 2
		GTK_WIDGET_CLASS(SpinScaleParentClass)->size_request(widget, &requisition);
#else
		gtk_widget_size_request(widget, &requisition);
#endif
		gtk_widget_get_allocation(widget, &allocation);

		if(data->layout == NULL)
		{
			data->layout = gtk_widget_create_pango_layout(widget, data->label);
			pango_layout_set_ellipsize(data->layout, PANGO_ELLIPSIZE_END);
		}

		pango_layout_set_width(data->layout, PANGO_SCALE * (allocation.width - requisition.width));
		pango_layout_get_pixel_extents(data->layout, NULL, &logical);

		gtk_entry_get_layout_offsets(GTK_ENTRY(widget), NULL, &layout_offset_y);

		if(gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL)
		{
			layout_offset_x = width - logical.width - 2;
		}
		else
		{
			layout_offset_x = 2;
		}

		layout_offset_x -= logical.x;

		cairo_move_to(cairo_p, layout_offset_x, layout_offset_y);

		gdk_cairo_set_source_color(cairo_p, &style->text[gtk_widget_get_state(widget)]);

		pango_cairo_show_layout(cairo_p, data->layout);
	}

#if GTK_MAJOR_VERSION <= 2
	cairo_destroy(cairo_p);
#endif

	return FALSE;
}

static eSPIN_SCALE_TARGET SpinScaleGetTarget(
	GtkWidget *widget,
	gdouble x,
	gdouble y
)
{
	GtkAllocation allocation;
	PangoRectangle logical;
	gint layout_x, layout_y;

	gtk_widget_get_allocation(widget, &allocation);
	gtk_entry_get_layout_offsets(GTK_ENTRY(widget), &layout_x, &layout_y);
	pango_layout_get_pixel_extents(gtk_entry_get_layout(GTK_ENTRY(widget)),
		NULL, &logical);

	if(x > layout_x && x < layout_x + logical.width
		&& y > layout_y && y < layout_y + logical.height)
	{
		return TARGET_NUMBER;
	}
	else if(y > allocation.height / 2)
	{
		return TARGET_LOWER;
	}

	return TARGET_UPPER;
}

static void SpinScaleGetLimits(
	SPIN_SCALE *scale,
	gdouble *lower,
	gdouble *upper
)
{
	SPIN_SCALE_PRIVATE *data = GET_PRIVATE(scale);

	if(data->scale_limits_set != FALSE)
	{
		*lower = data->scale_lower;
		*upper = data->scale_upper;
	}
	else
	{
		GtkSpinButton *spin_button = GTK_SPIN_BUTTON(scale);
		GtkAdjustment *adjustment = gtk_spin_button_get_adjustment(spin_button);

		*lower = gtk_adjustment_get_lower(adjustment);
		*upper = gtk_adjustment_get_upper(adjustment);
	}
}

static void SpinScaleChangeValue(GtkWidget *widget, gdouble x)
{
	SPIN_SCALE_PRIVATE *data = GET_PRIVATE(widget);
	GtkSpinButton *spin_button = GTK_SPIN_BUTTON(widget);
	GtkAdjustment *adjustment = gtk_spin_button_get_adjustment(spin_button);
#if GTK_MAJOR_VERSION <= 2
	GdkWindow *text_window = gtk_entry_get_text_window(GTK_ENTRY(widget));
#else
	GdkRectangle text_area;
#endif
	gdouble lower, upper;
	gint width;
	gdouble value;

	SpinScaleGetLimits(WIDGET_SPIN_SCALE(widget), &lower, &upper);

#if GTK_MAJOR_VERSION <= 2
	width = gdk_window_get_width(text_window);
#else
	gtk_entry_get_text_area(GTK_ENTRY(widget), &text_area);
	width = text_area.width;
#endif

	if(gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL)
	{
		x = width - x;
	}

	if(data->relative_change != FALSE)
	{
		gdouble diff, step;

		step = (upper - lower) / width / 10.0;

		if(gtk_widget_get_direction(widget) == GTK_TEXT_DIR_RTL)
		{
			diff = x - (width - data->start_x);
		}
		else
		{
			diff = x - data->start_x;
		}

		value = (data->start_value + diff * step);
	}
	else
	{
		gdouble fraction = x / (gdouble)width;

		value = fraction * (upper - lower) + lower;
	}

	gtk_adjustment_set_value(adjustment, value);
}

static gboolean SpinScaleButtonPress(GtkWidget* widget, GdkEventButton *pevent)
{
	SPIN_SCALE_PRIVATE *data = GET_PRIVATE(widget);
#if GTK_MAJOR_VERSION >= 3
	GdkRectangle rect;
	gint x, y;

	gtk_entry_get_text_area(GTK_ENTRY(widget), &rect);
	gtk_widget_get_pointer(widget, &x, &y);
#endif

	data->changing_value = FALSE;
	data->relative_change = FALSE;

#if GTK_MAJOR_VERSION <= 2
	if(pevent->window == gtk_entry_get_text_window(GTK_ENTRY(widget)))
#else
	if(x >= rect.x && y >= rect.y
		&& x <= rect.x + rect.width && y <= rect.y + rect.height)
#endif
	{
		switch(SpinScaleGetTarget(widget, pevent->x, pevent->y))
		{
		case TARGET_UPPER:
			data->changing_value = TRUE;

			gtk_widget_grab_focus(widget);

			SpinScaleChangeValue(widget, pevent->x);

			return TRUE;
		case TARGET_LOWER:
			data->changing_value = TRUE;

			gtk_widget_grab_focus(widget);

			data->relative_change = TRUE;
			data->start_x = pevent->x;
			data->start_value = gtk_adjustment_get_value(gtk_spin_button_get_adjustment(GTK_SPIN_BUTTON(widget)));

			return TRUE;
		}
	}	// if(pevent->window == gtk_entry_get_text_window(GTY_ENTRY(widget)))

	return GTK_WIDGET_CLASS(SpinScaleParentClass)->button_press_event(widget, pevent);
}

static gboolean SpinScaleButtonRelease(GtkWidget *widget, GdkEventButton *pevent)
{
	SPIN_SCALE_PRIVATE *data = GET_PRIVATE(widget);

	if(data->changing_value != FALSE)
	{
		data->changing_value = FALSE;

		SpinScaleChangeValue(widget, pevent->x);

		return TRUE;
	}

	return GTK_WIDGET_CLASS(SpinScaleParentClass)->button_release_event(widget, pevent);
}

static gboolean SpinScaleMotionNotify(GtkWidget *widget, GdkEventMotion *pevent)
{
	SPIN_SCALE_PRIVATE *data = GET_PRIVATE(widget);

	if(data->changing_value != FALSE)
	{
		SpinScaleChangeValue(widget, pevent->x);

		return TRUE;
	}

	GTK_WIDGET_CLASS(SpinScaleParentClass)->motion_notify_event(widget, pevent);

	if((pevent->state & (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)) == 0
#if GTK_MAJOR_VERSION <= 2
		&& pevent->window == gtk_entry_get_text_window(GTK_ENTRY(widget)))
#else
		)
#endif
	{
		GdkDisplay *display = gtk_widget_get_display(widget);
		GdkCursor *cursor = NULL;

		switch(SpinScaleGetTarget(widget, pevent->x, pevent->y))
		{
		case TARGET_NUMBER:
			cursor = gdk_cursor_new_for_display(display, GDK_XTERM);
			break;
		case TARGET_UPPER:
			cursor = gdk_cursor_new_for_display(display, GDK_SB_UP_ARROW);
			break;
		case TARGET_LOWER:
			cursor = gdk_cursor_new_for_display(display, GDK_SB_H_DOUBLE_ARROW);
			break;
		}

		gdk_window_set_cursor(pevent->window, cursor);
		gdk_cursor_unref(cursor);
	}

	return FALSE;
}

static gboolean SpinScaleLeaveNotify(GtkWidget *widget, GdkEventCrossing *pevent)
{
	gdk_window_set_cursor(pevent->window, NULL);

	return GTK_WIDGET_CLASS(SpinScaleParentClass)->leave_notify_event(widget, pevent);
}

static void SpinScaleValueChanged(GtkSpinButton *spin_button)
{
	GtkAdjustment *adjustment = gtk_spin_button_get_adjustment(spin_button);
	gdouble lower, upper, value;

	SpinScaleGetLimits(WIDGET_SPIN_SCALE(spin_button), &lower, &upper);

	value = CLAMP(gtk_adjustment_get_value(adjustment), lower, upper);

	gtk_entry_set_progress_fraction(GTK_ENTRY(spin_button),
		(value - lower) / (upper - lower));
}

static void SpinScaleClassInit(SPIN_SCALE_CLASS* data)
{
	GObjectClass *object_class = G_OBJECT_CLASS(data);
	GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(data);
	GtkSpinButtonClass *spin_button_class = GTK_SPIN_BUTTON_CLASS(data);

	object_class->dispose = SpinScaleDispose;
	object_class->finalize = SpinScaleFinalize;
	object_class->set_property = SpinScaleSetProperty;
	object_class->get_property = SpinScaleGetProperty;

#if GTK_MAJOR_VERSION <= 2
	widget_class->size_request = SpinScaleSizeRequest;
#else
	widget_class->get_preferred_width = SpinScaleGetPreferredWidth;
	widget_class->get_preferred_height = SpinScaleGetPreferredHeight;
#endif
	widget_class->style_set = SpinScaleStyleSet;
#if GTK_MAJOR_VERSION <= 2
	widget_class->expose_event = SpinScaleExpose;
#else
	widget_class->draw = SpinScaleExpose;
#endif
	widget_class->button_press_event = SpinScaleButtonPress;
	widget_class->button_release_event = SpinScaleButtonRelease;
	widget_class->motion_notify_event = SpinScaleMotionNotify;
	widget_class->leave_notify_event = SpinScaleLeaveNotify;

	spin_button_class->value_changed = SpinScaleValueChanged;

	g_object_class_install_property(object_class, PROP_LABEL,
		g_param_spec_string(
			"label", NULL, NULL, NULL, G_PARAM_READABLE | G_PARAM_WRITABLE
		)
	);

	g_type_class_add_private(data, sizeof(SPIN_SCALE_PRIVATE));
}

static void SpinScaleInit(SPIN_SCALE *scale)
{
	gtk_entry_set_alignment(GTK_ENTRY(scale), 1);
	gtk_entry_set_has_frame(GTK_ENTRY(scale), FALSE);
	gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(scale), TRUE);
}

/*****************************************************
* SpinScaleNew関数                                   *
* スピンボタンとスケールバーの混合ウィジェットを生成 *
* 引数                                               *
* adjustment	: スケールに使用するアジャスタ       *
* label			: スケールのラベル                   *
* digits		: 表示する桁数                       *
* 返り値                                             *
*	生成したウィジェット                             *
*****************************************************/
GtkWidget* SpinScaleNew(
	GtkAdjustment *adjustment,
	const gchar *label,
	gint digits
)
{
	g_return_val_if_fail(GTK_IS_ADJUSTMENT(adjustment), NULL);

	return g_object_new(WIDGET_TYPE_SPIN_SCALE,
		"adjustment",	adjustment,
		"label",		label,
		"digits",		digits,
		NULL
	);
}

/***************************************************
* SpinScaleSetScaleLimits関数                      *
* スケールの上下限値を設定する                     *
* 引数                                             *
* scale	: スピンボタンとスケールの混合ウィジェット *
* lower	: 下限値                                   *
* upper	: 上限値                                   *
***************************************************/
void SpinScaleSetScaleLimits(
	SPIN_SCALE *scale,
	gdouble lower,
	gdouble upper
)
{
	SPIN_SCALE_PRIVATE *data;
	GtkSpinButton *spin_button;
	GtkAdjustment *adjustment;
	gdouble value;

	g_return_if_fail(WIDGET_IS_SPIN_SCALE(scale));
	g_return_if_fail(lower <= upper);

	data = GET_PRIVATE(scale);
	spin_button = GTK_SPIN_BUTTON(scale);
	adjustment = gtk_spin_button_get_adjustment(spin_button);

	gtk_adjustment_set_lower(adjustment, lower);
	gtk_adjustment_set_upper(adjustment, upper);

	value = gtk_adjustment_get_value(adjustment);
	if(value < lower)
	{
		gtk_adjustment_set_value(adjustment, lower);
	}
	else if(value > upper)
	{
		gtk_adjustment_set_value(adjustment, upper);
	}

	data->scale_limits_set = TRUE;
	data->scale_lower = lower;
	data->scale_upper = upper;

	SpinScaleValueChanged(spin_button);
}

/*****************************************************
* SpinScaleGetScaleLimits関数                        *
* 上下限値を取得する                                 *
* 引数                                               *
* scale	: スピンボタンとスケールの混合ウィジェット   *
* lower	: 下限値を格納する倍精度浮動小数点のアドレス *
* upper	: 上限値を格納する倍精度浮動小数点のアドレス *
* 返り値                                             *
*	取得成功:TRUE	取得失敗:FALSE                   *
*****************************************************/
gboolean SpinScaleGetScaleLimits(
	SPIN_SCALE *scale,
	gdouble *lower,
	gdouble *upper
)
{
	SPIN_SCALE_PRIVATE *data;

	g_return_val_if_fail(WIDGET_IS_SPIN_SCALE(scale), FALSE);

	data = GET_PRIVATE(scale);

	if(lower != NULL)
	{
		*lower = data->scale_lower;
	}

	if(upper != NULL)
	{
		*upper = data->scale_upper;
	}

	return data->scale_limits_set;
}

/***************************************************
* SpinScaleSetStep関数                             *
* 矢印キーでの増減の値を設定する                   *
* 引数                                             *
* scale	: スピンボタンとスケールの混合ウィジェット *
* step	: 増減の値                                 *
***************************************************/
void SpinScaleSetStep(SPIN_SCALE *scale, gdouble step)
{
	SPIN_SCALE_PRIVATE *data;
	GtkSpinButton *spin_button;
	GtkAdjustment *adjustment;

	g_return_if_fail(WIDGET_IS_SPIN_SCALE(scale));

	data = GET_PRIVATE(scale);
	spin_button = GTK_SPIN_BUTTON(scale);
	adjustment = gtk_spin_button_get_adjustment(spin_button);

	gtk_adjustment_set_step_increment(adjustment, step);
}

/***************************************************
* SpinScaleSetPase関数                             *
* Pase Up/Downキーでの増減の値を設定する           *
* 引数                                             *
* scale	: スピンボタンとスケールの混合ウィジェット *
* page	: 増減の値                                 *
***************************************************/
void SpinScaleSetPage(SPIN_SCALE *scale, gdouble page)
{
	SPIN_SCALE_PRIVATE *data;
	GtkSpinButton *spin_button;
	GtkAdjustment *adjustment;

	g_return_if_fail(WIDGET_IS_SPIN_SCALE(scale));

	data = GET_PRIVATE(scale);
	spin_button = GTK_SPIN_BUTTON(scale);
	adjustment = gtk_spin_button_get_adjustment(spin_button);

	gtk_adjustment_set_page_increment(adjustment, page);
}

#ifdef __cplusplus
}
#endif
