#include <stdio.h>
#include <gtk/gtk.h>
#include "cell_renderer_widget.h"

#ifdef __cplusplus
extern "C" {
#endif

// ウィジェット定義用マクロ
#define GTK_TYPE_CELL_RENDERER_WIDGET		(gtk_cell_renderer_widget_get_type())
#define GTK_CELL_RENDERER_WIDGET(o)			(G_TYPE_CHECK_INSTANCE_CAST((o),GTK_TYPE_CELL_RENDERER_WIDGET,GtkCellRendererWidget))
#define GTK_CELL_RENDERER_WIDGET_CLASS(o)    

typedef struct _GtkCellRendererWidget
{
	GtkCellRenderer cell;
	GtkWidget       *widget;
} GtkCellRendererWidget;

typedef struct _GtkCellRendererWidgetClass
{
	struct _GtkCellRendererClass cell_class;
	void (*clicked)(GtkCellRendererWidget *crw,const gchar *path);
} GtkCellRendererWidgetClass;

GType gtk_cell_renderer_widget_get_type(void);


static void gtk_cell_renderer_widget_finalize(GObject *object);

static void gtk_cell_renderer_widget_set_property(
	GObject *object,
	guint property_id,
	GValue *value,
	GParamSpec *pspec
);

static void gtk_cell_renderer_widget_get_property(
	GObject *object,
	guint property_id,
	GValue *value,
	GParamSpec *pspec
);

static void gtk_cell_renderer_widget_get_size(
	GtkCellRenderer *cell,
	GtkWidget *widget,
	GdkRectangle *cell_area,
	gint *x_offset,
	gint *y_offset,
	gint *width,
	gint *height
);

static void gtk_cell_renderer_widget_render(
	GtkCellRenderer *cell,
	GdkWindow *window,
	GtkWidget *widget,
	GdkRectangle *background_area,
	GdkRectangle *cell_area,
	GdkRectangle *expose_area,
	GtkCellRendererState flags
);

enum
{
	PROP_0,
	PROP_WIDGET,
};

G_DEFINE_TYPE(GtkCellRendererWidget, gtk_cell_renderer_widget, GTK_TYPE_CELL_RENDERER)

static void gtk_cell_renderer_widget_init(GtkCellRendererWidget *crw)
{
	crw->widget = NULL;
}

// クラスの初期化処理
static void gtk_cell_renderer_widget_class_init(GtkCellRendererWidgetClass *klass)
{
	GObjectClass *obj_class;
	GtkCellRendererClass *class;

	obj_class = G_OBJECT_CLASS(klass);
	class = GTK_CELL_RENDERER_CLASS(klass);

	obj_class->finalize = gtk_cell_renderer_widget_finalize;

	obj_class->set_property =
		(void (*)(GObject*, guint, const GValue*, GParamSpec*))gtk_cell_renderer_widget_set_property;
	obj_class->get_property = gtk_cell_renderer_widget_get_property;

	class->get_size = gtk_cell_renderer_widget_get_size;
	class->render = gtk_cell_renderer_widget_render;

	g_object_class_install_property(
		obj_class,
		PROP_WIDGET,
		g_param_spec_object("widget", "widget of gtk", "widget to display in treeview",
			GTK_TYPE_WIDGET, G_PARAM_READWRITE)
	);
}

static void gtk_cell_renderer_widget_finalize(GObject *object)
{
	GtkCellRendererWidget *crw = GTK_CELL_RENDERER_WIDGET(object);
	if(crw->widget != NULL)
	{
		gtk_widget_destroy(crw->widget);
	}
	G_OBJECT_CLASS(gtk_cell_renderer_widget_parent_class)->finalize(object);
}

static void gtk_cell_renderer_widget_set_property(
	GObject *object,
	guint property_id,
	GValue *value,
	GParamSpec *pspec
)
{
	GtkCellRendererWidget *crw = GTK_CELL_RENDERER_WIDGET(object);

	switch(property_id)
	{
	case PROP_WIDGET:
		crw->widget = GTK_WIDGET(g_value_get_object(value));
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	break;
	}
}

static void gtk_cell_renderer_widget_get_property(
	GObject *object,
	guint property_id,
	GValue *value,
	GParamSpec *pspec
)
{
	GtkCellRendererWidget *crw = GTK_CELL_RENDERER_WIDGET(object);

	switch(property_id)
	{
	case PROP_WIDGET:
		g_value_set_object(value,crw->widget);
		break;
	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object, property_id, pspec);
	break;
	}
}

//
static void gtk_cell_renderer_widget_get_size(
	GtkCellRenderer *cell,
	GtkWidget *widget,
	GdkRectangle *cell_area,
	gint *x_offset,
	gint *y_offset,
	gint *width,
	gint *height
)
{
	GtkCellRendererWidget *crw = GTK_CELL_RENDERER_WIDGET(cell);

	if(crw->widget && GTK_IS_WIDGET(crw->widget))
	{
		GtkRequisition req={0};
		gtk_widget_size_request(crw->widget,&req);
		if(width != NULL)
		{
			*width = req.width+2*cell->xpad;
		}
		if(height != NULL)
		{
			*height = req.height+2*cell->ypad;
		}
	}

	if(x_offset != NULL)
	{
		*x_offset = 0;
	}

	if(y_offset != NULL)
	{
		*y_offset = 0;
	}
}

static void gtk_cell_renderer_widget_render(
	GtkCellRenderer *cell,
	GdkWindow *window,
	GtkWidget *widget,
	GdkRectangle *background_area,
	GdkRectangle *cell_area,
	GdkRectangle *expose_area,
	GtkCellRendererState flags
)
{
	GtkCellRendererWidget *crw = GTK_CELL_RENDERER_WIDGET(cell);
	GtkAllocation ac={cell_area->x,cell_area->y,cell_area->width,cell_area->height};
	GdkEvent *event;

	if(!gtk_widget_get_visible(crw->widget))
	{
		gtk_widget_set_visible(crw->widget,TRUE);
	}

	gtk_widget_set_parent_window(crw->widget,window);

	if(!gtk_widget_get_parent(crw->widget))
	{
		gtk_widget_set_parent(crw->widget,widget);
	}


	if(!gtk_widget_get_realized(crw->widget))
	{
		gtk_widget_realize(crw->widget);
	}

	if(!gtk_widget_get_mapped(crw->widget))
	{
		gtk_widget_map(crw->widget);
	}

	gtk_widget_size_allocate(crw->widget, &ac);
	gtk_widget_show(crw->widget);

	event = gtk_get_current_event();
	gtk_container_propagate_expose(GTK_CONTAINER(widget), crw->widget, (GdkEventExpose*)event);
	gdk_event_free(event);
}

GtkCellRenderer* gtk_cell_renderer_widget_new(void)
{
	return GTK_CELL_RENDERER(g_object_new(GTK_TYPE_CELL_RENDERER_WIDGET,NULL));
}

#ifdef __cplusplus
}
#endif
