#ifndef _INCLUDED_SYSTEM_DEPENDS_H_
#define _INCLUDED_SYSTEM_DEPENDS_H_

// このファイルの定数を環境にあわせて変更して下さい。

#include <gtk/gtk.h>

typedef enum _eMOUSE_BUTTONS
{
	MOUSE_BUTTON_LEFT = 1,
	MOUSE_BUTTON_CENTER = 2,
	MOUSE_BUTTON_RITHGT = 3
} eMOUSE_BUTTONS;

typedef enum _eMOUSE_WHEEL_DIRECTION
{
	MOUSE_WHEEL_UP = GDK_SCROLL_UP,
	MOUSE_WHEEL_DOWN = GDK_SCROLL_DOWN,
	MOUSE_WHEEL_LEFT = GDK_SCROLL_LEFT,
	MOUSE_WHEEL_RIGHT = GDK_SCROLL_RIGHT
} eMOUSE_WHEEL_DIRECTION;

typedef enum _eMODIFIERS
{
	MODIFIER_SHIFT_MASK = GDK_SHIFT_MASK,
	MODIFIER_CONTROL_MASK = GDK_CONTROL_MASK
} eMODIFIERS;

extern char* LocaleFromUTF8(const char* utf8_code);
extern char* Locale2UTF8(const char* locale);
extern char* GetDirectoryName(const char* utf8_path);
extern char* NextCharUTF8(char* str);

#endif	// #ifndef _INCLUDED_SYSTEM_DEPENDS_H_
