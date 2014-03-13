#include <GL/glew.h>
#include <gtk/gtk.h>
#include <gtk/gtkgl.h>
#include "application.h"
#include "mikumikugtk.h"
#include "memory.h"

static void ToggleButtonSetValueCallback(GtkWidget* button, int* set_target)
{
	int value = (int)(g_object_get_data(G_OBJECT(button), "value"));
	void *data = g_object_get_data(G_OBJECT(button), "callback_data");
	void (*callback)(void*, int) = (void (*)(void*, int))g_object_get_data(G_OBJECT(button), "callback_function");
	int *disabled = (int*)(g_object_get_data(G_OBJECT(button), "disabled"));

	if(disabled == NULL || *disabled == 0)
	{
		if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) != FALSE)
		{
			*set_target = value;
			if(callback != NULL)
			{
				callback(data, value);
			}
		}
	}
}

static void ToggleButtonSetValue(
	GtkWidget* button,
	int value,
	int* target,
	void* data,
	void (*callback)(void*, int),
	int* disabled
)
{
	(void)g_signal_connect(G_OBJECT(button), "toggled", G_CALLBACK(ToggleButtonSetValueCallback), (void*)target);
	g_object_set_data(G_OBJECT(button), "value", GINT_TO_POINTER(value));
	g_object_set_data(G_OBJECT(button), "callback_data", data);
	g_object_set_data(G_OBJECT(button), "callback_function", (void*)callback);
	g_object_set_data(G_OBJECT(button), "disabled", disabled);
}

static void ToggleButtonSetFlagCallback(GtkWidget* button, int* set_target)
{
	unsigned int value = (unsigned int)(g_object_get_data(G_OBJECT(button), "value"));
	void *data = g_object_get_data(G_OBJECT(button), "callback_data");
	void (*callback)(void*, int) = (void (*)(void*, int))g_object_get_data(G_OBJECT(button), "callback_function");
	int *disabled = (int*)(g_object_get_data(G_OBJECT(button), "disabled"));
	int active;

	if(disabled == NULL || *disabled == 0)
	{
		if((active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button))) != FALSE)
		{
			*set_target |= value;
		}
		else
		{
			*set_target &= ~(value);
		}

		if(callback != NULL)
		{
			callback(data, active);
		}
	}
}

static void ToggleButtonSetFlag(
	GtkWidget* button,
	unsigned int value,
	unsigned int* target,
	void* data,
	void (*callback)(void*, int),
	int* disabled
)
{
	(void)g_signal_connect(G_OBJECT(button), "toggled",
		G_CALLBACK(ToggleButtonSetFlagCallback), target);
	g_object_set_data(G_OBJECT(button), "value", GUINT_TO_POINTER(value));
	g_object_set_data(G_OBJECT(button), "callback_data", data);
	g_object_set_data(G_OBJECT(button), "callback_function", (void*)callback);
	g_object_set_data(G_OBJECT(button), "disabled", disabled);
}

static void InitializeGL(PROJECT* project, int widget_width, int widget_height)
{
	SetRequiredOpengGLState();

	// グリッド
	InitializeGrid(&project->grid);
	LoadGrid(&project->grid, (void*)project->application_context);

	// 物理エンジンデータ
	InitializeWorld(&project->world);

	// シーン管理データ
	project->scene = (SCENE*)MEM_ALLOC_FUNC(sizeof(*project->scene));
	InitializeScene(project->scene, widget_width, widget_height, project->world.world, project);

	// シャドウマッピングの初期化
	project->shadow_map = (SHADOW_MAP*)MEM_ALLOC_FUNC(sizeof(*project->shadow_map));
	InitializeShadowMap(project->shadow_map, widget_width, widget_height, (void*)project->scene);

	// 操作中のボーン等を表示するデータを初期化
	InitializeDebugDrawer(&project->debug_drawer, (void*)project);
	DebugDrawerLoad(&project->debug_drawer);

	InitializeControl(&project->control, widget_width, widget_height, project);
	LoadControlHandle(&project->control.handle, project->application_context);

	project->world.debug_drawer = &project->debug_drawer;
}

static void MakeContextShadowMap(PROJECT* project, int width, int height)
{
	ResizeShadowMap(project->shadow_map, width, height);
	MakeShadowMap(project->shadow_map);
}

gboolean ConfigureEvent(
	GtkWidget* widget,
	void* event_info,
	void* project_context
)
{
#define MIN_SIZE 32
	PROJECT *project = (PROJECT*)project_context;
	GdkEventConfigure *info = (GdkEventConfigure*)event_info;
	GtkAllocation allocation;
	GdkGLContext *context = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *drawable = gtk_widget_get_gl_drawable(widget);

	if(gdk_gl_drawable_gl_begin(drawable, context) == FALSE)
	{
		return FALSE;
	}

#if GTK_MAJOR_VERSION <= 2
	allocation = widget->allocation;
#else
	gtk_widget_get_allocation(widget, &allocation);
#endif

	if((project->flags & PROJECT_FLAG_GL_INTIALIZED) == 0)
	{
		if(allocation.width < MIN_SIZE || allocation.height < MIN_SIZE)
		{
			return FALSE;
		}

		if(glewInit() != GLEW_OK)
		{
			gdk_gl_drawable_gl_end(drawable);
			return FALSE;
		}

		InitializeGL(project, allocation.width, allocation.height);

		MakeContextShadowMap(project, 2048, 2048);
		project->flags |= PROJECT_FLAG_GL_INTIALIZED;
	}

	project->scene->width = allocation.width;
	project->scene->height = allocation.height;
	ResizeHandle(&project->control.handle, allocation.width, allocation.height);

	glViewport(0, 0, allocation.width, allocation.height);

	gdk_gl_drawable_gl_end(drawable);

	return TRUE;
}

void InitializeWidgets(WIDGETS* widgets, int widget_width, int widget_height, void* project)
{
	// OpenGLの設定
	GdkGLConfig *config;

	// キャンバスウィジェットを作成
	widgets->drawing_area = gtk_drawing_area_new();
	gtk_widget_set_size_request(widgets->drawing_area, widget_width, widget_height);
	// 発生するイベントを設定
	gtk_widget_set_events(widgets->drawing_area,
		GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK | GDK_POINTER_MOTION_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_SCROLL_MASK
				| GDK_ENTER_NOTIFY_MASK | GDK_LEAVE_NOTIFY_MASK | GDK_POINTER_MOTION_HINT_MASK | GDK_BUTTON_RELEASE_MASK
#if GTK_MAJOR_VERSION >= 3
				| GDK_TOUCH_MASK
#endif
	);
	// キャンバスにOpenGLを設定
	//config = gdk_gl_config_new(attrlibute);
	config = gdk_gl_config_new_by_mode(
		GDK_GL_MODE_RGBA | GDK_GL_MODE_DEPTH | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_STENCIL);
	if(config == NULL)
	{
		return;
	}
	if(gtk_widget_set_gl_capability(widgets->drawing_area, config,
		NULL, TRUE, GDK_GL_RGBA_TYPE) == FALSE)
	{
		return;
	}
}

gboolean MouseButtonPressEvent(GtkWidget* widget, GdkEventButton* event_info, void* project_context)
{
	MouseButtonPressCallback(project_context, event_info->x, event_info->y,
		(int)event_info->button, event_info->state);

	return TRUE;
}

gboolean MouseMotionEvent(GtkWidget* widget, GdkEventMotion* event_info, void* project_context)
{
	gdouble x0, y0;
	GdkModifierType state;

	if(event_info->is_hint != 0)
	{
		gint get_x, get_y;
		gdk_window_get_pointer(event_info->window, &get_x, &get_y, &state);
		x0 = get_x, y0 = get_y;
	}
	else
	{
		state = event_info->state;
		x0 = event_info->x;
		y0 = event_info->y;
	}

	MouseMotionCallback(project_context, event_info->x, event_info->y, (int)state);

	return TRUE;
}

gboolean MouseButtonReleaseEvent(GtkWidget* widget, GdkEventButton* event_info, void* project_context)
{
	MouseButtonReleaseCallback(project_context, event_info->x, event_info->y,
		(int)event_info->button, event_info->state);

	return TRUE;
}

gboolean MouseWheelScrollEvent(GtkWidget* widget, GdkEventScroll* event_info, void* project_context)
{
	MouseScrollCallback(project_context, event_info->direction, event_info->state);

	return TRUE;
}

gboolean ProjectDisplayEvent(GtkWidget* widget, GdkEventExpose* event_info, void* project_context)
{
	GtkAllocation allocation;
	GdkGLContext *context = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *drawable = gtk_widget_get_gl_drawable(widget);

	if(gdk_gl_drawable_gl_begin(drawable, context) == FALSE)
	{
		return FALSE;
	}

#if GTK_MAJOR_VERSION >= 3
	gtk_widget_get_allocation(widget, &allocation);
#else
	allocation = widget->allocation;
#endif

	DisplayProject((PROJECT*)project_context, allocation.width, allocation.height);

	if(gdk_gl_drawable_is_double_buffered(drawable) != FALSE)
	{
		gdk_gl_drawable_swap_buffers(drawable);
	}
	else
	{
		glFlush();
	}

	//glPopMatrix ();

	gdk_gl_drawable_gl_end(drawable);

	return TRUE;
}

void ShowModelComment(MODEL_INTERFACE* model, PROJECT* project)
{
	GtkWidget *dialog;
	GtkWidget *note_book;
	GtkWidget *scrolled_window;
	GtkWidget *label;

	dialog = gtk_dialog_new_with_buttons("", GTK_WINDOW(project->widgets.main_window),
		GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT, GTK_STOCK_OK, GTK_RESPONSE_OK, NULL);
	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);
	note_book = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dialog))),
		note_book, TRUE, TRUE, 2);

	// 日本語タブ
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	label = gtk_label_new("\xE6\x97\xA5\xE6\x9C\xAC\xE8\xAA\x9E");
	(void)gtk_notebook_append_page(GTK_NOTEBOOK(note_book), scrolled_window, label);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	label = gtk_label_new(model->comment);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), label);
	// Englishタブ
	scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	label = gtk_label_new("English");
	(void)gtk_notebook_append_page(GTK_NOTEBOOK(note_book), scrolled_window, label);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	label = gtk_label_new(model->english_comment);
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(scrolled_window), label);

	gtk_widget_show_all(dialog);
	(void)gtk_dialog_run(GTK_DIALOG(dialog));
	gtk_widget_destroy(dialog);
}

void ExecuteLoadModel(APPLICATION* application)
{
	GtkWidget *chooser;
	GtkFileFilter *filter;
	GtkWindow *main_window = NULL;

	if(application->num_projects > 0)
	{
		main_window = GTK_WINDOW(
			application->projects[application->active_project]->widgets.main_window);
	}

	chooser = gtk_file_chooser_dialog_new(
		application->label.load_model,
		main_window,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_OK, GTK_RESPONSE_OK,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		NULL
	);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Model File");
	gtk_file_filter_add_pattern(filter, "*.pmx");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Accessory File");
	gtk_file_filter_add_pattern(filter, "*");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

	if(gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_OK)
	{
		MODEL_INTERFACE *model;
		gchar *system_path;
		gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
		gchar *file_type;
		const gchar *filter_name;

		filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(chooser));
		filter_name = gtk_file_filter_get_name(filter);
		if(strcmp(filter_name, "Model File") == 0)
		{
			file_type = ".pmx";
		}
		else
		{
			gchar *str = path;
			file_type = path;
			while(*str != '\0')
			{
				if((*str == '/' || *str == '\\')
					&& *(str+1) != '\0')
				{
					file_type = str+1;
				}
				else if(*str == '.')
				{
					file_type = str;
				}

				str = g_utf8_next_char(str);
			}
		}

		system_path = g_locale_from_utf8(path, -1, NULL, NULL, NULL);
		model = LoadModel(application, system_path, file_type);
		if(model->type == MODEL_TYPE_PMX_MODEL)
		{
			ShowModelComment(model, application->projects[application->active_project]);
		}
		g_free(system_path);
	}

	gtk_widget_destroy(chooser);
}

GtkWidget* BoneTreeViewNew(void)
{
	GtkTreeViewColumn *column;
	GtkCellRenderer *renderer;
	GtkWidget *view;

	view = gtk_tree_view_new();
	column = gtk_tree_view_column_new();
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_add_attribute(column, renderer, "text", 0);

	return view;
}

void OnChangedSelectedBone(GtkWidget* widget, PROJECT* project)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	BONE_INTERFACE *bone;

	if(gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter) != FALSE)
	{
		gtk_tree_model_get(model, &iter, 1, &bone, -1);
		ChangeSelectedBones(project, &bone, 1);
	}
}

typedef struct _SET_LABEL_DATA
{
	char *label;
	POINTER_ARRAY *child_names;
} SET_LABEL_DATA;

void BoneTreeViewSetBones(GtkWidget *tree_view, void* model_interface, void* project_context)
{
	PROJECT *project = (PROJECT*)project_context;
	MODEL_INTERFACE *model = (MODEL_INTERFACE*)model_interface;
	STRUCT_ARRAY *set_label_data = StructArrayNew(sizeof(SET_LABEL_DATA), DEFAULT_BUFFER_SIZE);
	SET_LABEL_DATA *parent;
	SET_LABEL_DATA *data;
	LABEL_INTERFACE **labels;
	LABEL_INTERFACE *label;
	BONE_INTERFACE *bone;
	POINTER_ARRAY *added_bones;
	GtkTreeStore *tree_store;
	GtkTreeStore *old_store = NULL;
	GtkTreeIter toplevel;
	GtkTreeIter child;
	int num_labels;
	int num_children;
	int num_added_bones = 0;
	int i, j;

	if(model == NULL)
	{
		return;
	}

	tree_store = GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(tree_view)));
	if(tree_store == NULL)
	{
		tree_store = old_store = gtk_tree_store_new(2, G_TYPE_STRING, G_TYPE_POINTER);
		gtk_tree_view_set_model(GTK_TREE_VIEW(tree_view), GTK_TREE_MODEL(tree_store));
	}
	else
	{
		gtk_tree_store_clear(tree_store);
	}

	labels = (LABEL_INTERFACE**)model->get_labels(model, &num_labels);

	parent = (SET_LABEL_DATA*)StructArrayReserve(set_label_data);
	parent->child_names = PointerArrayNew(DEFAULT_BUFFER_SIZE);
	added_bones = PointerArrayNew(DEFAULT_BUFFER_SIZE);
	for(i=0; i<num_labels; i++)
	{
		label = labels[i];
		num_children = label->get_count(label);
		if(label->special != FALSE)
		{
			if(num_children > 0 && strcmp(label->name, "Root") == 0)
			{
				bone = label->get_bone(label, 0);
				if(bone != NULL)
				{
					parent->label = bone->name;
					PointerArrayAppend(added_bones, bone);
				}
			}
			if(parent->label == NULL)
			{
				continue;
			}
		}
		else
		{
			parent->label = label->name;
			PointerArrayAppend(added_bones, bone);
		}

		for(j=0; j<num_children; j++)
		{
			bone = label->get_bone(label, j);
			if(bone != NULL)
			{
				PointerArrayAppend(parent->child_names, bone->name);
				PointerArrayAppend(added_bones, bone);
			}
		}

		if(parent->child_names->num_data > 0)
		{
			parent = (SET_LABEL_DATA*)StructArrayReserve(set_label_data);
			parent->child_names = PointerArrayNew(DEFAULT_BUFFER_SIZE);
		}
	}

	data = (SET_LABEL_DATA*)set_label_data->buffer;
	for(i=0; i<(int)set_label_data->num_data; i++)
	{
		parent = &data[i];
		bone = (BONE_INTERFACE*)added_bones->buffer[num_added_bones];
		num_added_bones++;
		gtk_tree_store_append(tree_store, &toplevel, NULL);
		gtk_tree_store_set(tree_store, &toplevel, 0, parent->label, 1, bone, -1);

		for(j=0; j<(int)parent->child_names->num_data; j++)
		{
			bone = (BONE_INTERFACE*)added_bones->buffer[num_added_bones];
			num_added_bones++;
			gtk_tree_store_append(tree_store, &child, &toplevel);
			gtk_tree_store_set(tree_store, &child, 0,
				parent->child_names->buffer[j], 1, bone, -1);
		}
	}

	for(i=0; i<(int)set_label_data->num_data; i++)
	{
		parent = &data[i];
		PointerArrayDestroy(&parent->child_names, NULL);
	}
	StructArrayDestroy(&set_label_data, NULL);


	MEM_FREE_FUNC(labels);

	if(old_store != NULL)
	{
		g_object_unref(old_store);
	}
}

void* ModelControlWidgetNew(void* application_context)
{
	APPLICATION *application = (APPLICATION*)application_context;
	PROJECT *project = application->projects[application->active_project];
	SCENE *scene = project->scene;
	GtkWidget *vbox;
	GtkWidget *note_book_box;
	GtkWidget *layout_box;
	GtkWidget *note_book;
	GtkWidget *label;
	GtkWidget *widget;
	GtkWidget *control[4];
	GtkTreeSelection *selection;
	gchar *path;
	int i;

	vbox = gtk_vbox_new(FALSE, 0);

	layout_box = gtk_vbox_new(FALSE, 0);
	path = g_build_filename(application->paths.image_directory_path, "add_model.png", NULL);
	widget = gtk_image_new_from_file(path);
	g_free(path);
	gtk_box_pack_start(GTK_BOX(layout_box), widget, FALSE, FALSE, 0);
	label = gtk_label_new(application->label.load_model);
	gtk_box_pack_start(GTK_BOX(layout_box), label, FALSE, FALSE, 0);
	control[0] = gtk_button_new();
	gtk_container_add(GTK_CONTAINER(control[0]), layout_box);
	(void)g_signal_connect_swapped(G_OBJECT(control[0]), "clicked", G_CALLBACK(ExecuteLoadModel), application);
	gtk_box_pack_start(GTK_BOX(vbox), control[0], FALSE, FALSE, 0);

	note_book = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(vbox), note_book, FALSE, FALSE, 0);
	note_book_box = gtk_vbox_new(FALSE, 0);
	label = gtk_label_new(application->label.bone);
	gtk_notebook_append_page(GTK_NOTEBOOK(note_book), note_book_box, label);
	control[0] = gtk_radio_button_new_with_label(NULL, application->label.edit_mode.select);
	control[1] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(control[0])),
		application->label.edit_mode.move);
	control[2] = gtk_radio_button_new_with_label(gtk_radio_button_get_group(GTK_RADIO_BUTTON(control[0])),
		application->label.edit_mode.rotate);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(control[project->control.edit_mode]), TRUE);
	for(i=0; i<NUM_EDIT_MODE; i++)
	{
		ToggleButtonSetValue(control[i], i, (int*)&project->control.edit_mode,
			&project->control, (void (*)(void*, int))ControlSetEditMode, &project->widgets.ui_disabled);
		gtk_box_pack_start(GTK_BOX(note_book_box), control[i], FALSE, FALSE, 0);
	}

	control[0] = gtk_scrolled_window_new(NULL, NULL);
	project->widgets.bone_tree_view = BoneTreeViewNew();
	gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(control[0]), project->widgets.bone_tree_view);
	gtk_widget_set_size_request(control[0], 128, 360);
	gtk_box_pack_start(GTK_BOX(note_book_box), control[0], FALSE, TRUE, 0);
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(project->widgets.bone_tree_view));
	(void)g_signal_connect(G_OBJECT(selection), "changed",
		G_CALLBACK(OnChangedSelectedBone), project);

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	label = gtk_label_new(application->label.environment);
	note_book_box = gtk_vbox_new(FALSE, 0);
	gtk_notebook_append_page(GTK_NOTEBOOK(note_book), note_book_box, label);

	control[0] = gtk_check_button_new_with_label(application->label.enable_physics);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(control[0]), project->flags & PROJECT_FLAG_ALWAYS_PHYSICS);
	ToggleButtonSetFlag(control[0], PROJECT_FLAG_ALWAYS_PHYSICS, &project->flags,
		project, (void (*)(void*, int))ProjectSetEnableAlwaysPhysics, &project->widgets.ui_disabled);
	gtk_box_pack_start(GTK_BOX(note_book_box), control[0], FALSE, FALSE, 0);

	control[0] = gtk_check_button_new_with_label(application->label.display_grid);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(control[0]),
		project->flags & PROJECT_FLAG_DRAW_GRID);
	ToggleButtonSetFlag(control[0], PROJECT_FLAG_DRAW_GRID, &project->flags,
		NULL, NULL, &project->widgets.ui_disabled);
	gtk_box_pack_start(GTK_BOX(note_book_box), control[0], FALSE, FALSE, 0);

	return (void*)vbox;
}

typedef struct _RENDER_FOR_PIXEL_DATA
{
	uint8 *pixels;
	int width;
	int height;
	PROJECT *project;
	void *user_data;
	void (*finished)(void*, unsigned char*);
} RENDER_FOR_PIXEL_DATA;

gboolean RenderForPixelDataDrawing(
	GtkWidget* widget,
	GdkEventExpose* event_info,
	RENDER_FOR_PIXEL_DATA* data
)
{
	GtkAllocation allocation;
	GdkGLContext *context = gtk_widget_get_gl_context(widget);
	GdkGLDrawable *drawable = gtk_widget_get_gl_drawable(widget);

	if(gdk_gl_drawable_gl_begin(drawable, context) == FALSE)
	{
		return TRUE;
	}

#if GTK_MAJOR_VERSION >= 3
	gtk_widget_get_allocation(widget, &allocation);
#else
	allocation = widget->allocation;
#endif

	if(allocation.width != data->width || allocation.height != data->height)
	{
		return TRUE;
	}

	RenderEngines(data->project, allocation.width, allocation.height);

	if(gdk_gl_drawable_is_double_buffered(drawable) != FALSE)
	{
		gdk_gl_drawable_swap_buffers(drawable);
	}
	else
	{
		glFlush();
	}

	gdk_gl_drawable_gl_end(drawable);

	glReadBuffer(GL_FRONT);
	glReadPixels(0, 0, data->width, data->height, GL_RGBA,
		GL_UNSIGNED_BYTE, data->pixels);

	(void)g_signal_handlers_disconnect_by_func(G_OBJECT(widget),
		G_CALLBACK(RenderForPixelDataDrawing), data);
	data->finished(data->user_data, data->pixels);

	MEM_FREE_FUNC(data);

	return FALSE;
}

void RenderForPixelData(
	void* project_context,
	int width,
	int height,
	unsigned char* pixels,
	void (*after_render)(void*, unsigned char*),
	void* user_data
)
{
	RENDER_FOR_PIXEL_DATA *data;
	PROJECT *project = (PROJECT*)project_context;

	data = (RENDER_FOR_PIXEL_DATA*)MEM_ALLOC_FUNC(sizeof(*data));
	data->pixels = pixels;
	data->width = width;
	data->height = height;
	data->project = project;
	data->user_data = user_data;
	data->finished = after_render;

	gtk_widget_set_size_request(project->widgets.drawing_area, width, height);
	(void)g_signal_handlers_disconnect_by_func(
		G_OBJECT(project->widgets.drawing_area), G_CALLBACK(ProjectDisplayEvent), project);
#if GTK_MAJOR_VERSION >= 3
	(void)g_signal_connect(G_OBJECT(project->widgets.drawing_area),
		"draw", G_CALLBACK(RenderForPixelDataDrawing), data);
#else
	(void)g_signal_connect(G_OBJECT(project->widgets.drawing_area),
		"expose-event", G_CALLBACK(RenderForPixelDataDrawing), data);
#endif
}
