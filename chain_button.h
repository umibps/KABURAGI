#ifndef _INCLUDED_CHAIN_BUTTON_H_
#define _INCLUDED_CHAIN_BUTTON_H_

#include <gtk/gtk.h>

#ifdef _MSC_VER
# ifdef __cplusplus
#  define EXTERN extern "C" __declspec(dllexport)
# else
#  define EXTERN extern __declspec(dllexport)
# endif
#else
# define EXTERN extern
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _eCHAIN_POSITION
{
	CHAIN_POSITION_TOP,
	CHAIN_POSITION_LEFT,
	CHAIN_POSITION_BOTTOM,
	CHAIN_POSITION_RIGHT
} eCHAIN_POSITION;
typedef struct _CHAIN_BUTTON
{
	GtkTable parent_instance;
	eCHAIN_POSITION position;
	gboolean active;
	GtkWidget *button;
	GtkWidget *line1;
	GtkWidget *line2;
	GtkWidget *image;
} CHAIN_BUTTON;

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_CHAIN_BUTTON_H_
