#ifndef _INCLUDED_MIKU_MIKU_GTK_H_
#define _INCLUDED_MIKU_MIKU_GTK_H_

#ifdef __cplusplus
extern "C" {
#endif

extern void* ApplicationContextNew(int default_width, int default_height, const char* application_path);

extern void* ProjectContextNew(void* application_context, int width, int height, void** widget);

extern void LoadProjectContextData(
	void* project_context,
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int)
);

extern void SaveProjectContextData(
	void* project_context,
	void* dst,
	size_t (*write_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	long (*tell_func)(void*)
);

extern void* ModelControlWidgetNew(void* application_context);

extern void* CameraLightControlWidgetNew(void* application_context);

extern void MouseButtonPressCallback(
	void* project_context,
	double x,
	double y,
	int button,
	int state
);

extern void MouseMotionCallback(
	void* project_context,
	double x,
	double y,
	int state
);

extern void MouseButtonReleaseCallback(
	void* project_context,
	double x,
	double y,
	int button,
	int state
);

extern void MouseScrollCallback(
	void* project_context,
	int direction,
	int state
);

extern void ResizeCallback(
	void* project_context,
	int width,
	int height
);

extern void ResizeModelControlWidget(void* application_context, int new_width, int new_height);

extern void RenderForPixelData(
	void* project_context,
	int width,
	int height,
	unsigned char* pixels,
	void (*after_render)(void*, unsigned char*),
	void* user_data
);

extern void ProjectSetOriginalSize(void* project_context, int width, int height);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_MIKU_MIKU_GTK_H_
