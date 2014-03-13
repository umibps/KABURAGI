#ifndef _INCLUDED_MIKU_MIKU_GTK_H_
#define _INCLUDED_MIKU_MIKU_GTK_H_

extern void* ApplicationContextNew(int default_width, int default_height);

extern void* ProjectContextNew(void* application_context, int width, int height, void** widget);

extern void* ModelControlWidgetNew(void* application_context);

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

extern void RenderForPixelData(
	void* project_context,
	int width,
	int height,
	unsigned char* pixels,
	void (*after_render)(void*, unsigned char*),
	void* user_data
);

#endif	// #ifndef _INCLUDED_MIKU_MIKU_GTK_H_
