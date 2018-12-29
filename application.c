/*
* application.c
* �A�v���P�[�V�����̏�����
* ���j���[�̓�����`
*/

// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>
#include "application.h"
#include "menu.h"
#include "labels.h"
#include "tool_box.h"
#include "display.h"
#include "save.h"
#include "utils.h"
#include "memory.h"
#include "image_read_write.h"
#include "ini_file.h"
#include "brushes.h"
#include "tlg.h"

#include "./gui/GTK/input_gtk.h"
#include "./gui/GTK/utils_gtk.h"
#include "./gui/GTK/gtk_widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************
* GetActiveDrawWindow�֐�                                *
* �A�N�e�B�u�ȕ`��̈���擾����                         *
* ����                                                   *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                 *
*	�A�N�e�B�u�ȕ`��̈�                                 *
*********************************************************/
DRAW_WINDOW* GetActiveDrawWindow(APPLICATION* app)
{
	DRAW_WINDOW *ret = app->draw_window[app->active_window];
	if(ret == NULL)
	{
		return NULL;
	}

	if(ret->focal_window != NULL)
	{
		return ret->focal_window;
	}
	return ret;
}

/*****************************************************
* SetLayerBlendModeArray�֐�                         *
* �������[�h�̒萔�z���������                       *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
static void SetLayerBlendModeArray(APPLICATION* app)
{
	app->layer_blend_operators[0] = CAIRO_OPERATOR_OVER;
	app->layer_blend_operators[1] = CAIRO_OPERATOR_ADD;
	app->layer_blend_operators[2] = CAIRO_OPERATOR_MULTIPLY;
	app->layer_blend_operators[3] = CAIRO_OPERATOR_SCREEN;
	app->layer_blend_operators[4] = CAIRO_OPERATOR_OVERLAY;
	app->layer_blend_operators[5] = CAIRO_OPERATOR_LIGHTEN;
	app->layer_blend_operators[6] = CAIRO_OPERATOR_DARKEN;
	app->layer_blend_operators[7] = CAIRO_OPERATOR_COLOR_DODGE;
	app->layer_blend_operators[8] = CAIRO_OPERATOR_COLOR_BURN;
	app->layer_blend_operators[9] = CAIRO_OPERATOR_COLOR_BURN;
	app->layer_blend_operators[10] = CAIRO_OPERATOR_HARD_LIGHT;
	app->layer_blend_operators[11] = CAIRO_OPERATOR_SOFT_LIGHT;
	app->layer_blend_operators[12] = CAIRO_OPERATOR_DIFFERENCE;
	app->layer_blend_operators[13] = CAIRO_OPERATOR_EXCLUSION;
	app->layer_blend_operators[14] = CAIRO_OPERATOR_HSL_HUE;
	app->layer_blend_operators[15] = CAIRO_OPERATOR_HSL_SATURATION;
	app->layer_blend_operators[16] = CAIRO_OPERATOR_HSL_COLOR;
	app->layer_blend_operators[17] = CAIRO_OPERATOR_HSL_LUMINOSITY;
}

/*************************************************************************
* LoadMakeNewData�֐�                                                    *
* �V�K�쐬���̃f�[�^��ǂݍ���                                           *
* ����                                                                   *
* app					: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
* make_new_file_path	: �f�[�^�t�@�C���̃p�X                           *
*************************************************************************/
void LoadMakeNewData(APPLICATION* app, const char* make_new_file_path)
{
	INI_FILE *file;
	GFile *fp;
	GFileInputStream *stream;
	GFileInfo *file_info;
	size_t file_size;
	int i;

	if((fp = g_file_new_for_path(make_new_file_path)) == NULL)
	{
		return;
	}
	if((stream = g_file_read(fp, NULL, NULL)) == NULL)
	{
		g_object_unref(fp);
		return;
	}

	// �t�@�C���T�C�Y���擾
	file_info = g_file_input_stream_query_info(stream,
		G_FILE_ATTRIBUTE_STANDARD_SIZE, NULL, NULL);
	file_size = (size_t)g_file_info_get_size(file_info);

	file = CreateIniFile((void*)stream, (size_t (*)(void*, size_t, size_t, void*))FileRead,
		file_size, INI_READ);

	app->menu_data.make_new = (MAKE_NEW_DATA*)MEM_ALLOC_FUNC(
		sizeof(*app->menu_data.make_new) * (file->section_count+1));
	for(i=0; i<file->section_count; i++)
	{
		int index = app->menu_data.num_make_new_data+1;
		app->menu_data.make_new[index].name = IniFileStrdup(file, file->section[i].section_name, "NAME");
		if(app->menu_data.make_new[index].name != NULL)
		{
			app->menu_data.make_new[index].width = IniFileGetInteger(file, file->section[i].section_name, "WIDTH");
			app->menu_data.make_new[index].height = IniFileGetInteger(file, file->section[i].section_name, "HEIGHT");
			app->menu_data.make_new[index].resolution = IniFileGetInteger(file, file->section[i].section_name, "RESOLUTION");

			if(app->menu_data.make_new[index].width <= 0)
			{
				app->menu_data.make_new[index].width = 1;
			}
			if(app->menu_data.make_new[index].height <= 0)
			{
				app->menu_data.make_new[index].height = 1;
			}
			if(app->menu_data.make_new[index].resolution <= 0)
			{
				app->menu_data.make_new[index].resolution = 1;
			}

			app->menu_data.num_make_new_data++;
		}
	}

	file->delete_func(file);

	g_object_unref(file_info);
	g_object_unref(stream);
	g_object_unref(fp);
}

/*************************************************************************
* WriteMakeNewData�֐�                                                   *
* �V�K�쐬���̃f�[�^����������                                           *
* ����                                                                   *
* app					: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
* make_new_file_path	: �f�[�^�t�@�C���̃p�X                           *
* �Ԃ�l                                                                 *
*	����I��:TRUE	�ُ�I��:FALSE                                       *
*************************************************************************/
gboolean WriteMakeNewData(APPLICATION* app, const char* make_new_file_path)
{
	INI_FILE *file;
	GFile *fp;
	GFileOutputStream *stream;
	char section_name[256];
	int i;

	if((fp = g_file_new_for_path(make_new_file_path)) == NULL)
	{
		return FALSE;
	}
	if((stream = g_file_create(fp, G_FILE_CREATE_NONE, NULL, NULL)) == NULL)
	{
		if((stream = g_file_replace(fp, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL)) == NULL)
		{
			g_object_unref(fp);
			return FALSE;
		}
	}

	file = CreateIniFile((void*)stream, NULL, 0, INI_WRITE);

	for(i=0; i<app->menu_data.num_make_new_data; i++)
	{
		(void)sprintf(section_name, "DATA%d", i+1);
		IniFileAddString(file, section_name, "NAME", app->menu_data.make_new[i+1].name);
		IniFileAddInteger(file, section_name, "WIDTH", app->menu_data.make_new[i+1].width, 10);
		IniFileAddInteger(file, section_name, "HEIGHT", app->menu_data.make_new[i+1].height, 10);
		IniFileAddInteger(file, section_name, "RESOLUTION", app->menu_data.make_new[i+1].resolution, 10);
	}

	WriteIniFile(file, (size_t (*)(void*, size_t, size_t, void*))FileWrite);
	file->delete_func(file);

	g_object_unref(stream);
	g_object_unref(fp);

	return TRUE;
}

#define LAYER_GROUP_TEMPLATE_SECTION_NAME "LAYER_GROUP_TEMPLATE"
/*************************************************
* ReadLayerGroupTemplateData�֐�                 *
* ���C���[���܂Ƃ߂č쐬����ۂ̃f�[�^��ǂݍ��� *
* ����                                           *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����f�[�^   *
* file	: �ǂݍ��ޏ������t�@�C��                 *
*************************************************/
static void ReadLayerGroupTemplateData(APPLICATION* app, INI_FILE* file)
{
	LAYER_GROUP_TEMPLATE_NODE *target;
	char name[MAX_LAYER_NAME_LENGTH];
	char str[MAX_LAYER_NAME_LENGTH];
	char *text;
	int num_templates = 0;
	int counter;
	int i, j;

	counter = 1;
	while(1)
	{
		(void)memset(str, 0, sizeof(str));
		(void)sprintf(name, "GROUP_NAME%d", counter);
		(void)IniFileGetString(file, LAYER_GROUP_TEMPLATE_SECTION_NAME,
			name, str, sizeof(str));
		if(str[0] != '\0')
		{
			num_templates++;
		}
		else
		{
			break;
		}
		counter++;
	}

	app->layer_group_templates.num_templates = num_templates;
	app->layer_group_templates.templates
		= (LAYER_GROUP_TEMPLATE*)MEM_ALLOC_FUNC(sizeof(*app->layer_group_templates.templates)*num_templates);
	(void)memset(app->layer_group_templates.templates, 0, sizeof(*app->layer_group_templates.templates)*num_templates);

	for(i=0; i<num_templates; i++)
	{
		(void)sprintf(name, "GROUP_NAME%d", i+1);
		(void)IniFileGetString(file, LAYER_GROUP_TEMPLATE_SECTION_NAME, name, str, sizeof(str));
		text = g_convert(str, strlen(str), "UTF-8", app->system_code, NULL, NULL, NULL);
		app->layer_group_templates.templates[i].group_name = MEM_STRDUP_FUNC(text);
		g_free(text);
		(void)sprintf(name, "NUM_LAYERS%d", i+1);
		app->layer_group_templates.templates[i].num_layers =
			IniFileGetInteger(file, LAYER_GROUP_TEMPLATE_SECTION_NAME, name);
		(void)sprintf(name, "ADD_LAYER_SET%d", i+1);
		if(IniFileGetInteger(file, LAYER_GROUP_TEMPLATE_SECTION_NAME, name) != 0)
		{
			app->layer_group_templates.templates[i].flags |= LAYER_GROUP_TEMPLATE_FLAG_MAKE_LAYER_SET;
		}
		(void)sprintf(name, "ADD_UNDER_ACTIVE_LAYER%d", i+1);
		if(IniFileGetInteger(file, LAYER_GROUP_TEMPLATE_SECTION_NAME, name) != 0)
		{
			app->layer_group_templates.templates[i].flags |= LAYER_GROUP_TEMPLATE_FLAG_ADD_UNDER_ACTIVE_LAYER;
		}

		if(app->layer_group_templates.templates[i].num_layers > 0)
		{
			app->layer_group_templates.templates[i].names = target =
				(LAYER_GROUP_TEMPLATE_NODE*)MEM_ALLOC_FUNC(sizeof(*target) * app->layer_group_templates.templates[i].num_layers);
			(void)memset(target, 0, sizeof(*target) * app->layer_group_templates.templates[i].num_layers);
			for(j=0; j<app->layer_group_templates.templates[i].num_layers - 1; j++)
			{
				target->next = &app->layer_group_templates.templates[i].names[j+1];
				target = target->next;
			}
			target = app->layer_group_templates.templates[i].names;
			app->layer_group_templates.templates[i].add_flags =
				(uint8*)MEM_ALLOC_FUNC((app->layer_group_templates.templates[i].num_layers+7)/8);
			for(j=0; j<app->layer_group_templates.templates[i].num_layers; j++)
			{
				(void)sprintf(name, "LAYER_NAME%d_%d", i+1, j+1);
				(void)IniFileGetString(file, LAYER_GROUP_TEMPLATE_SECTION_NAME, name, str, sizeof(str));
				text = g_convert(str, strlen(str), "UTF-8", app->system_code, NULL, NULL, NULL);
				target->name = MEM_STRDUP_FUNC(text);
				g_free(text);
				(void)sprintf(name, "LAYER_TYPE%d_%d", i+1, j+1);
				(void)IniFileGetString(file, LAYER_GROUP_TEMPLATE_SECTION_NAME, name,
					str, sizeof(str));
				target->layer_type = GetLayerTypeFromString(str);
				(void)sprintf(name, "LAYER_ADD_FLAG%d_%d", i+1, j+1);
				if(IniFileGetInteger(file, LAYER_GROUP_TEMPLATE_SECTION_NAME, name) != 0)
				{
					FLAG_ON(app->layer_group_templates.templates[i].add_flags, j);
				}
				target = target->next;
			}
		}
	}
}

/*************************************************
* WriteLayerGroupTemplateData�֐�                *
* ���C���[���܂Ƃ߂č쐬����ۂ̃f�[�^���������� *
* ����                                           *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����f�[�^   *
* file	: �������ޏ������t�@�C��                 *
*************************************************/
static void WriteLayerGroupTemplateData(APPLICATION* app, INI_FILE* file)
{
	LAYER_GROUP_TEMPLATE_NODE *target;
	char name[MAX_LAYER_NAME_LENGTH];
	char *text;
	int num_templates;
	int i, j;

	num_templates = app->layer_group_templates.num_templates;

	for(i=0; i<num_templates; i++)
	{
		(void)sprintf(name, "GROUP_NAME%d", i+1);
		text = g_convert(app->layer_group_templates.templates[i].group_name,
			strlen(app->layer_group_templates.templates[i].group_name), app->system_code, "UTF-8", NULL, NULL, NULL);
		(void)IniFileAddString(file, LAYER_GROUP_TEMPLATE_SECTION_NAME, name, text);
		g_free(text);
		(void)sprintf(name, "NUM_LAYERS%d", i+1);
		(void)IniFileAddInteger(file, LAYER_GROUP_TEMPLATE_SECTION_NAME, name,
			app->layer_group_templates.templates[i].num_layers, 10);
		(void)sprintf(name, "ADD_LAYER_SET%d", i+1);
		(void)IniFileAddInteger(file, LAYER_GROUP_TEMPLATE_SECTION_NAME, name,
			(app->layer_group_templates.templates[i].flags & LAYER_GROUP_TEMPLATE_FLAG_MAKE_LAYER_SET) == 0 ? 0 : 1, 10);
		(void)sprintf(name, "ADD_UNDER_ACTIVE_LAYER%d", i+1);
		(void)IniFileAddInteger(file, LAYER_GROUP_TEMPLATE_SECTION_NAME, name,
			(app->layer_group_templates.templates[i].flags & LAYER_GROUP_TEMPLATE_FLAG_ADD_UNDER_ACTIVE_LAYER) == 0 ? 0 : 1, 10);

		for(target=app->layer_group_templates.templates[i].names, j=0;
			target != NULL; target = target->next, j++)
		{
			(void)sprintf(name, "LAYER_NAME%d_%d", i+1, j+1);
			text = g_convert(target->name, strlen(target->name), app->system_code, "UTF-8", NULL, NULL, NULL);
			(void)IniFileAddString(file, LAYER_GROUP_TEMPLATE_SECTION_NAME, name, text);
			g_free(text);
			(void)sprintf(name, "LAYER_TYPE%d_%d", i+1, j+1);
			(void)IniFileAddString(file, LAYER_GROUP_TEMPLATE_SECTION_NAME,
				name, GetLayerTypeString(target->layer_type));
			(void)sprintf(name, "LAYER_ADD_FLAG%d_%d", i+1, j+1);
			IniFileAddInteger(file, LAYER_GROUP_TEMPLATE_SECTION_NAME,
				name, (FLAG_CHECK(app->layer_group_templates.templates[i].add_flags, j)) == 0 ? 0 : 1, 10);
		}
	}
}

/*********************************************************
* ReadInitializeFile�֐�                                 *
* �������t�@�C����ǂݍ���                               *
* ����                                                   *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* file_path	: �������t�@�C���̃p�X                       *
* �Ԃ�l                                                 *
*	����I��:0	���s:���̒l                              *
*********************************************************/
int ReadInitializeFile(APPLICATION* app, const char* file_path, INITIALIZE_DATA* init_data)
{
	// �������t�@�C����͗p
	INI_FILE_PTR file;
	// �t�@�C���ǂݍ��݃X�g���[��
	GFile* fp = g_file_new_for_path(file_path);
	GFileInputStream* stream = g_file_read(fp, NULL, NULL);
	// �t�@�C���T�C�Y�擾�p
	GFileInfo *file_info;
	// �t�@�C���T�C�Y
	size_t data_size;
	// ������̈ꎞ�ۑ�
	char str[4096];
	// GLib�̃������̈ꎞ�ۑ�
	char *glib_str;
	// �F�f�[�^
	char color_string[128];
	uint32 color;

	if(stream == NULL)
	{
		return -1;
	}
	// �t�@�C���T�C�Y���擾
	file_info = g_file_input_stream_query_info(stream,
		G_FILE_ATTRIBUTE_STANDARD_SIZE, NULL, NULL);
	data_size = (size_t)g_file_info_get_size(file_info);
	// �t�@�C���ǂݍ��ݗp
	file = CreateIniFile(stream,
		(size_t (*)(void*, size_t, size_t, void*))FileRead, data_size, INI_READ);

	// �V�X�e���R�[�h�̓ǂݍ���
	app->system_code = IniFileStrdup(file, "SYSTEM_CODE", "CODE");
	// �f�[�^�t�@�C���E�f�B���N�g���p�X�̓ǂݍ���
		// �ʏ탌�C���[�̃u���V
	app->brush_file_path = IniFileStrdup(file, "BRUSH_DATA", "PATH");
	glib_str = g_convert(app->brush_file_path, -1, "UTF-8", app->system_code, NULL, NULL, NULL);
	MEM_FREE_FUNC(app->brush_file_path);
	app->brush_file_path = glib_str;
	// �x�N�g�����C���[�̃u���V
	app->vector_brush_file_path = IniFileStrdup(file, "VECTOR_BRUSH_DATA", "PATH");
	glib_str = g_convert(app->vector_brush_file_path, -1, "UTF-8", app->system_code, NULL, NULL, NULL);
	MEM_FREE_FUNC(app->vector_brush_file_path);
	app->vector_brush_file_path = glib_str;
	// �S��ނ̃��C���[���ʂŎg����c�[��
	app->common_tool_file_path = IniFileStrdup(file, "COMMON_TOOL_DATA", "PATH");
	glib_str = g_convert(app->common_tool_file_path, -1, "UTF-8", app->system_code, NULL, NULL, NULL);
	MEM_FREE_FUNC(app->common_tool_file_path);
	app->common_tool_file_path = glib_str;
	// �\���e�L�X�g�̃f�[�^
	app->language_file_path = IniFileStrdup(file, "LANGUAGE_FILE", "PATH");
	// �V�K�쐬���̃f�[�^
	app->make_new_file_path = IniFileStrdup(file, "MAKE_NEW_FILE", "PATH");
	// �e�f�[�^�f�B���N�g��
	app->pattern_path = IniFileStrdup(file, "PATTERN_DIRECTORY", "PATH");
	app->stamp_path = IniFileStrdup(file, "STAMP_DIRECTORY", "PATH");
	app->texture_path = IniFileStrdup(file, "TEXTURE_DIRECTORY", "PATH");
	app->script_path = IniFileStrdup(file, "SCRIPT_DIRECTORY", "PATH");

	// �E�B���h�E�̈ʒu�ƃT�C�Y��ǂݍ���
	app->window_x = IniFileGetInteger(file, "WINDOW", "X");
	app->window_y = IniFileGetInteger(file, "WINDOW", "Y");
	app->window_width = IniFileGetInteger(file, "WINDOW", "WIDTH");
	app->window_height = IniFileGetInteger(file, "WINDOW", "HEIGHT");
	app->left_pane_position = IniFileGetInteger(file, "WINDOW", "LEFT_PANE");
	app->right_pane_position = IniFileGetInteger(file, "WINDOW", "RIGHT_PANE");
	if(IniFileGetInteger(file, "WINDOW", "FULLSCREEN") != 0)
	{
		app->flags |= APPLICATION_FULL_SCREEN;
	}
	if(IniFileGetInteger(file, "WINDOW", "MAXIMIZE") != 0)
	{
		app->flags |= APPLICATION_WINDOW_MAXIMIZE;
	}
	app->gui_scale = IniFileGetInteger(file, "WINDOW", "GUI_SCALE") * (FLOAT_T)0.01;
	if(app->gui_scale < (FLOAT_T)0.5)
	{
		app->gui_scale = (FLOAT_T)1.0;
	}

	// RC�t�@�C���̃p�X��ǂݍ���
	app->preference.theme = IniFileStrdup(file, "WINDOW", "RC_FILE");
	if(app->preference.theme != NULL)
	{
		gchar* file_path;
		if(app->preference.theme[0] == '.'
			&& app->preference.theme[1] == '/')
		{
			file_path = g_build_filename(app->current_path,
				&app->preference.theme[2], NULL);
			gtk_rc_parse(file_path);
		}
		else
		{
			gtk_rc_parse(app->preference.theme);
		}
	}

	// GUI�̃X�P�[�����O��ǂݍ���
	if(app->gui_scale != 1.0)
	{
		GtkStyle *style;
		char rc_str[1024];
		const char *family;
		int size;

		style = gtk_widget_get_default_style();
		family = pango_font_description_get_family(style->font_desc);
		size = (pango_font_description_get_size_is_absolute(style->font_desc) == 0) ?
			pango_font_description_get_size(style->font_desc) : pango_font_description_get_size(style->font_desc);
		(void)sprintf(rc_str, "style \"font\" {\nfont_name = \"%s %d\"\n}\n\nclass \"*\" style \"font\"\n"
			"style \"checkbutton\" {\nGtkCheckButton::indicator-size = %d\n}\n\nclass \"GtkCheckButton\" style \"checkbutton\"\n",
				family, (int)(size * app->gui_scale / (10240 / 12.0)), (int)(13 * app->gui_scale));
		gtk_rc_parse_string(rc_str);
	}

	// �c�[���{�b�N�X�E�B���h�E�̈ʒu�ƃT�C�Y��ǂݍ���
	app->tool_window.window_x = IniFileGetInteger(file, "TOOL_BOX", "X");
	app->tool_window.window_y = IniFileGetInteger(file, "TOOL_BOX", "Y");
	app->tool_window.window_width = IniFileGetInteger(file, "TOOL_BOX", "WIDTH");
	app->tool_window.window_height = IniFileGetInteger(file, "TOOL_BOX", "HEIGHT");
	app->tool_window.place = IniFileGetInteger(file, "TOOL_BOX", "PLACE");
	app->tool_window.pane_position = IniFileGetInteger(file, "TOOL_BOX", "PANE");
	// �F�f�[�^��ǂݍ���
	{
		uint8 *color_buff = (uint8*)&color;

		if(IniFileGetString(file, "TOOL_BOX", "FORE_GROUND_COLOR", color_string, 128) != 0)
		{
			(void)sscanf(color_string, "%x", &color);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			init_data->fg_color[0] = color_buff[2];
			init_data->fg_color[1] = color_buff[1];
			init_data->fg_color[2] = color_buff[0];
#else
			init_data->fg_color[0] = color_buff[0];
			init_data->fg_color[1] = color_buff[1];
			init_data->fg_color[2] = color_buff[2];
#endif
		}
		else
		{
			init_data->fg_color[0] = init_data->fg_color[1]
				= init_data->fg_color[2] = 0xff;
		}

		if(IniFileGetString(file, "TOOL_BOX", "BACK_GROUND_COLOR", color_string, 128) != 0)
		{
			(void)sscanf(color_string, "%x", &color);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			init_data->bg_color[0] = color_buff[2];
			init_data->bg_color[1] = color_buff[1];
			init_data->bg_color[2] = color_buff[0];
#else
			init_data->bg_color[0] = color_buff[0];
			init_data->bg_color[1] = color_buff[1];
			init_data->bg_color[2] = color_buff[2];
#endif
		}
		else
		{
			init_data->bg_color[0] = init_data->bg_color[1]
				= init_data->bg_color[2] = 0xff;
		}
	}

	if(IniFileGetInteger(file, "TOOL_BOX", "SHOW_COLOR_CIRCLE") != 0)
	{
		app->tool_window.flags |= TOOL_SHOW_COLOR_CIRCLE;
	}
	if(IniFileGetInteger(file, "TOOL_BOX", "SHOW_COLOR_PALLETE") != 0)
	{
		app->tool_window.flags |= TOOL_SHOW_COLOR_PALLETE;
	}

	switch(app->tool_window.place)
	{
	case UTILITY_PLACE_WINDOW:
		break;
	case UTILITY_PLACE_LEFT:
		app->tool_window.flags |= TOOL_DOCKED;
		break;
	case UTILITY_PLACE_RIGHT:
		app->tool_window.flags |= TOOL_PLACE_RIGHT | TOOL_DOCKED;
		break;
	}

	// �ȈՃu���V�؂�ւ��̃f�[�^�̏������Ɠǂݍ���
	InitializeBrushChain(&app->tool_window.brush_chain);
	{
		int num_brush_chain = IniFileGetInteger(file, "BRUSH_SET", "NUM_BRUSH_SET");
		int num_item;
		char section_name[1024];
		char key;
		int i, j;

		if(IniFileGetString(file, "BRUSH_SET", "KEY", str, sizeof(str)) > 0)
		{
			(void)sscanf(str, "%c", &key);
		}
		else
		{
			key = '\0';
		}
		app->tool_window.brush_chain.key = key;
		if(IniFileGetString(file, "BRUSH_SET", "CHANGE_KEY", str, sizeof(str)) > 0)
		{
			(void)sscanf(str, "%c", &key);
		}
		else
		{
			key = '\0';
		}
		app->tool_window.brush_chain.change_key = key;

		for(i=1; i<=num_brush_chain; i++)
		{
			BRUSH_CHAIN_ITEM *item = (BRUSH_CHAIN_ITEM*)MEM_ALLOC_FUNC(sizeof(*item));
			InitializeBrushChainItem(item);

			PointerArrayAppend(app->tool_window.brush_chain.chains, item);
			(void)sprintf(section_name, "BRUSH_SET%d", i);
			num_item = IniFileGetInteger(file, section_name, "NUM_ITEM");
			for(j=1; j<=num_item; j++)
			{
				(void)sprintf(str, "BRUSH_NAME%d", j);
				PointerArrayAppend(item->names, IniFileStrdup(file, section_name, str));
			}
		}
	}

	// ���C���[�r���[�E�B���h�E�̈ʒu�ƃT�C�Y��ǂݍ���
	app->layer_window.window_x = IniFileGetInteger(file, "LAYER_VIEW", "X");
	app->layer_window.window_y = IniFileGetInteger(file, "LAYER_VIEW", "Y");
	app->layer_window.window_width = IniFileGetInteger(file, "LAYER_VIEW", "WIDTH");
	app->layer_window.window_height = IniFileGetInteger(file, "LAYER_VIEW", "HEIGHT");
	app->layer_window.place = IniFileGetInteger(file, "LAYER_VIEW", "PLACE");
	app->layer_window.pane_position = IniFileGetInteger(file, "LAYER_VIEW", "POSITION");
	(void)IniFileGetString(file, "LAYER_VIEW", "SCROLLBAR_PLACEMENT", str, sizeof(str));
	if(strcmp(str, "LEFT") == 0)
	{
		app->layer_window.flags |= LAYER_WINDOW_SCROLLBAR_PLACE_LEFT;
	}

	switch(app->layer_window.place)
	{
	case UTILITY_PLACE_WINDOW:
		break;
	case UTILITY_PLACE_LEFT:
		app->layer_window.flags |= LAYER_WINDOW_DOCKED;
		break;
	case UTILITY_PLACE_RIGHT:
		app->layer_window.flags |= LAYER_WINDOW_DOCKED | LAYER_WINDOW_PLACE_RIGHT;
		break;
	}

	// �i�r�Q�[�V�����E�B���h�E�̈ʒu�ƃT�C�Y��ǂݍ���
	app->navigation_window.window_x = IniFileGetInteger(file, "NAVIGATION", "X");
	app->navigation_window.window_y = IniFileGetInteger(file, "NAVIGATION", "Y");
	app->navigation_window.window_width = IniFileGetInteger(file, "NAVIGATION", "WIDTH");
	app->navigation_window.window_height = IniFileGetInteger(file, "NAVIGATION", "HEIGHT");

	// �v���r���[�E�B���h�E�̈ʒu�ƃT�C�Y��ǂݍ���
	app->preview_window.window_x = IniFileGetInteger(file, "PREVIEW", "X");
	app->preview_window.window_y = IniFileGetInteger(file, "PREVIEW", "Y");
	app->preview_window.window_width = IniFileGetInteger(file, "PREVIEW", "WIDTH");
	app->preview_window.window_height = IniFileGetInteger(file, "PREVIEW", "HEIGHT");

	// �Q�l�p�摜�\���E�B���h�E�̈ʒu�ƃT�C�Y��ǂݍ���
	app->reference_window.window_x = IniFileGetInteger(file, "REFERENCE", "X");
	app->reference_window.window_y = IniFileGetInteger(file, "REFERENCE", "Y");
	app->reference_window.window_width = IniFileGetInteger(file, "REFERENCE", "WIDTH");
	app->reference_window.window_height = IniFileGetInteger(file, "REFERENCE", "HEIGHT");

	// ���j���[�f�[�^��ǂݍ���
		// �V�K�쐬
	
	// �V�K�쐬���̃f�[�^�ǂݍ���
	if(app->make_new_file_path != NULL)
	{
		if(app->make_new_file_path[0] == '.'
			&& app->make_new_file_path[1] == '/')
		{
			file_path = g_build_filename(app->current_path, &app->make_new_file_path[2], NULL);
			LoadMakeNewData(app, file_path);
			g_free((gpointer)file_path);
		}
		else
		{
			LoadMakeNewData(app, app->make_new_file_path);
		}

		if(app->menu_data.make_new == NULL)
		{
			app->menu_data.make_new = (MAKE_NEW_DATA*)MEM_ALLOC_FUNC(sizeof(*app->menu_data.make_new));
		}
	}
	else
	{
		app->menu_data.make_new = (MAKE_NEW_DATA*)MEM_ALLOC_FUNC(sizeof(*app->menu_data.make_new));
		app->make_new_file_path = MEM_STRDUP_FUNC("./make_new_list.ini");
	}
	app->menu_data.make_new->width = IniFileGetInteger(file, "NEW", "WIDTH");
	app->menu_data.make_new->height = IniFileGetInteger(file, "NEW", "HEIGHT");
	app->menu_data.make_new->resolution = IniFileGetInteger(file, "NEW", "RESOLUTION");
	if(IniFileGetString(file, "NEW", "SECOND_BG_COLOR", color_string, sizeof(color_string)) != 0)
	{
		uint8 *color_buff = (uint8*)&color;
		(void)sscanf(color_string, "%x", &color);

#if 1 // defined(BIG_ENDIAN)
		app->menu_data.second_bg_color[0] = color_buff[2];
		app->menu_data.second_bg_color[1] = color_buff[1];
		app->menu_data.second_bg_color[2] = color_buff[0];
#else
		app->menu_data.second_bg_color[0] = color_buff[0];
		app->menu_data.second_bg_color[1] = color_buff[1];
		app->menu_data.second_bg_color[2] = color_buff[2];
#endif
	}

	// ��u���␳
	app->tool_window.smoother.num_use = IniFileGetInteger(file, "SMOOTH", "LEVEL");
	app->tool_window.smoother.rate = IniFileGetInteger(file, "SMOOTH", "RATE");
	app->tool_window.smoother.mode = IniFileGetInteger(file, "SMOOTH", "MODE");

	// �e�N�X�`���̐ݒ��ǂݍ���
	app->textures.strength = IniFileGetDouble(file, "TEXTURE", "STRENGTH");
	app->textures.scale = IniFileGetDouble(file, "TEXTURE", "SCALE");

	// ���ݒ��ǂݍ���
	ReadPreference(file, &app->preference);
	// �^�b�`�C�x���g�̏������@��ǂݍ���
	if(IniFileGetInteger(file, "PREFERENCE", "DRAW_WITH_TOUCH") != 0)
	{
		app->flags |= APPLICATION_DRAW_WITH_TOUCH;
	}
	// �w�i�F��ύX���邩��ǂݍ���
	if(IniFileGetInteger(file, "PREFERENCE", "CHANGE_BACK_GROUND") != 0)
	{
		app->flags |= APPLICATION_SET_BACK_GROUND_COLOR;
	}

	// �v���r���[�E�B���h�E���^�X�N�o�[�ɕ\�����邩��ǂݍ���
	if(IniFileGetInteger(file, "PREFERENCE", "SHOW_PREVIEW_WINDOW_ON_TASKBAR") != 0)
	{
		app->flags |= APPLICATION_SHOW_PREVIEW_ON_TASK_BAR;
	}

	// �g�p����ICC�v���t�@�C����ǂݍ���
	{
		char *temp_path;
		cmsHPROFILE monitor_icc;

		temp_path = IniFileStrdup(file, "ICC_PROFILE", "INPUT");
		app->input_icc_path = g_strdup(temp_path);
		if(app->input_icc_path != NULL)
		{
			if(*app->input_icc_path == '\0')
			{
				//app->input_icc = CreateDefaultSrgbProfile();
				app->input_icc = GetPrimaryMonitorProfile();
				g_free(app->input_icc_path);
				app->input_icc_path = NULL;
			}
			else
			{
				app->input_icc = cmsOpenProfileFromFile(app->input_icc_path, "r");
			}
		}
		else
		{
			//app->input_icc = CreateDefaultSrgbProfile();
			app->input_icc = GetPrimaryMonitorProfile();
		}
		MEM_FREE_FUNC(temp_path);

		temp_path = IniFileStrdup(file, "ICC_PROFILE", "OUTPUT");
		app->output_icc_path = g_strdup(temp_path);
		if(app->output_icc_path != NULL)
		{
			app->output_icc = cmsOpenProfileFromFile(app->output_icc_path, "r");
		}

		MEM_FREE_FUNC(temp_path);

		monitor_icc = GetPrimaryMonitorProfile();

		if(app->output_icc != NULL)
		{
			cmsBool bpc[] = { TRUE, TRUE, TRUE, TRUE };
			cmsHPROFILE hProfiles[] = { app->input_icc, app->output_icc, app->output_icc, monitor_icc };
			cmsUInt32Number intents[] = { INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC };
			cmsFloat64Number adaptationStates[] = { 0, 0, 0, 0 };

			app->icc_transform = cmsCreateExtendedTransform(cmsGetProfileContextID(hProfiles[1]), 4, hProfiles,
				bpc, intents, adaptationStates,
				NULL, 0, TYPE_BGRA_8, TYPE_BGRA_8, 0 /*cmsFLAGS_SOFTPROOFING*/);
		}
		else
		{
			app->icc_transform = cmsCreateTransform(app->input_icc, TYPE_BGRA_8,
				monitor_icc, TYPE_BGRA_8, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_BLACKPOINTCOMPENSATION);
		}

		cmsCloseProfile(monitor_icc);
	}

	// �o�b�N�A�b�v���쐬����f�B���N�g����ǂݍ���
	if(IniFileGetString(file, "BACKUP", "DIRECTORY", str, sizeof(str)) != 0)
	{
		app->backup_directory_path = g_convert(str, -1, "UTF-8", app->system_code, NULL, NULL, NULL);
	}
	else
	{
		app->backup_directory_path = g_strdup("./");
	}

	// ���C���[���܂Ƃ߂č쐬����f�[�^��ǂݍ���
	ReadLayerGroupTemplateData(app, file);

	// �t�@�C���ǂݍ��݃f�[�^��j��
	file->delete_func(file);

	// �ǂݍ��ݗp�̃I�u�W�F�N�g���폜
	g_object_unref(fp);
	g_object_unref(stream);
	g_object_unref(file_info);

	return 0;
}

/**************************************************************
* WriteInitializeFile�֐�                                     *
* �������t�@�C������������                                    *
* ����                                                        *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X      *
* write_app	: �A�v���P�[�V������GUI�f�[�^�������o���ׂ̃f�[�^ *
* file_path	: �������t�@�C���̃p�X                            *
* �Ԃ�l                                                      *
*	����I��:0	���s:���̒l                                   *
**************************************************************/
int WriteInitializeFile(APPLICATION* app, WRITE_APPLICATIOIN_DATA* write_app, const char* file_path)
{
	GFile* fp = g_file_new_for_path(file_path);
	GFileOutputStream* stream =
		g_file_create(fp, G_FILE_CREATE_NONE, NULL, NULL);
	INI_FILE_PTR file;
	int32 color;
	char str[128];
	char *write_path;
	
	// �t�@�C���I�[�v���Ɏ��s������㏑��������
	if(stream == NULL)
	{
		stream = g_file_replace(fp, NULL, FALSE, G_FILE_CREATE_NONE, NULL, NULL);

		if(stream == NULL)
		{
			g_object_unref(fp);
			return -1;
		}
	}

	file = CreateIniFile(stream, NULL,0, INI_WRITE);

	// �V�X�e���R�[�h����������
	(void)IniFileAddString(file, "SYSTEM_CODE", "CODE", app->system_code);

	// �f�[�^�t�@�C���E�f�B���N�g���p�X����������
		// �ʏ탌�C���[�̃u���V
	write_path = g_convert(app->brush_file_path, -1, app->system_code, "UTF-8", NULL, NULL, NULL);
	(void)IniFileAddString(file, "BRUSH_DATA", "PATH", write_path);
	g_free(write_path);
	// �ȈՃu���V�؂�ւ��̃f�[�^����������
	{
		char section_name[1024];
		char key_name[1024];
		int i, j;

		(void)IniFileAddInteger(file, "BRUSH_SET", "NUM_BRUSH_SET", (int)app->tool_window.brush_chain.chains->num_data, 10);
		(void)sprintf(str, "%c", (char)app->tool_window.brush_chain.key);
		(void)IniFileAddString(file, "BRUSH_SET", "KEY", str);
		(void)sprintf(str, "%c", (char)app->tool_window.brush_chain.change_key);
		(void)IniFileAddString(file, "BRUSH_SET", "CHANGE_KEY", str);

		for(i=0; i<(int)app->tool_window.brush_chain.chains->num_data; i++)
		{
			BRUSH_CHAIN_ITEM *item = (BRUSH_CHAIN_ITEM*)app->tool_window.brush_chain.chains->buffer[i];

			(void)sprintf(section_name, "BRUSH_SET%d", i+1);
			(void)IniFileAddInteger(file, section_name, "NUM_ITEM", (int)item->names->num_data, 10);

			for(j=0; j<(int)item->names->num_data; j++)
			{
				(void)sprintf(key_name, "BRUSH_NAME%d", j+1);
				(void)IniFileAddString(file, section_name, key_name, (const char*)item->names->buffer[j]);
			}
		}
	}
	// �x�N�g�����C���[�̃u���V
	write_path = g_convert(app->vector_brush_file_path, -1, app->system_code, "UTF-8", NULL, NULL, NULL);
	(void)IniFileAddString(file, "VECTOR_BRUSH_DATA", "PATH", write_path);
	g_free(write_path);
	// �S��ނ̃��C���[�Ŏg����c�[��
	write_path = g_convert(app->common_tool_file_path, -1, app->system_code, "UTF-8", NULL, NULL, NULL);
	(void)IniFileAddString(file, "COMMON_TOOL_DATA", "PATH", write_path);
	g_free(write_path);
	// �\�������̃f�[�^
	(void)IniFileAddString(file, "LANGUAGE_FILE", "PATH", app->language_file_path);
	// �V�K�쐬���̃f�[�^
	(void)IniFileAddString(file, "MAKE_NEW_FILE", "PATH", app->make_new_file_path);
	// �e��f�[�^�f�B���N�g��
	(void)IniFileAddString(file, "PATTERN_DIRECTORY", "PATH", app->pattern_path);
	(void)IniFileAddString(file, "STAMP_DIRECTORY", "PATH", app->stamp_path);
	(void)IniFileAddString(file, "TEXTURE_DIRECTORY", "PATH", app->texture_path);
	(void)IniFileAddString(file, "SCRIPT_DIRECTORY", "PATH", app->script_path);

	// �E�B���h�E�̈ʒu�ƃT�C�Y����������
	{
		int x, y, width, height;
		x = write_app->main_window_x,	y = write_app->main_window_y;
		width = write_app->main_window_width,	height = write_app->main_window_height;
		(void)IniFileAddInteger(file, "WINDOW", "X", x, 10);
		(void)IniFileAddInteger(file, "WINDOW", "Y", y, 10);
		(void)IniFileAddInteger(file, "WINDOW", "WIDTH", width, 10);
		(void)IniFileAddInteger(file, "WINDOW", "HEIGHT", height, 10);
		(void)IniFileAddInteger(file, "WINDOW", "FULLSCREEN",
			((app->flags & APPLICATION_FULL_SCREEN) != 0) ? 1 : 0, 10);
		(void)IniFileAddInteger(file, "WINDOW", "MAXIMIZE",
			((app->flags & APPLICATION_WINDOW_MAXIMIZE) != 0) ? 1 : 0, 10);
		(void)IniFileAddInteger(file, "WINDOW", "GUI_SCALE", (int)(app->gui_scale * 100), 10);
		if(app->preference.theme != NULL)
		{
			(void)IniFileAddString(file, "WINDOW", "RC_FILE", app->preference.theme);
		}

		(void)IniFileAddInteger(file, "WINDOW", "LEFT_PANE", app->left_pane_position, 10);
		(void)IniFileAddInteger(file, "WINDOW", "RIGHT_PANE", app->right_pane_position, 10);

		{
			if(app->tool_window.window != NULL)
			{
				gtk_window_get_position(GTK_WINDOW(app->tool_window.window), &x, &y);
				gtk_window_get_size(GTK_WINDOW(app->tool_window.window), &width, &height);
			}
			else
			{
				x = app->tool_window.window_x, y = app->tool_window.window_y;
				width = app->tool_window.window_width, height = app->tool_window.window_height;
			}

			if(app->tool_window.ui != NULL)
			{
				app->tool_window.pane_position =
					gtk_paned_get_position(GTK_PANED(app->tool_window.pane));
			}

			(void)IniFileAddInteger(file, "TOOL_BOX", "X", x, 10);
			(void)IniFileAddInteger(file, "TOOL_BOX", "Y", y, 10);
			(void)IniFileAddInteger(file, "TOOL_BOX", "WIDTH", width, 10);
			(void)IniFileAddInteger(file, "TOOL_BOX", "HEIGHT", height, 10);
			(void)IniFileAddInteger(file, "TOOL_BOX", "PLACE", app->tool_window.place, 10);
			(void)IniFileAddInteger(file, "TOOL_BOX", "PANE", app->tool_window.pane_position, 10);

#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
			color = app->tool_window.color_chooser->rgb[0] | (app->tool_window.color_chooser->rgb[1] << 8)
				| (app->tool_window.color_chooser->rgb[2] << 16);
#else
			color = app->tool_window.color_chooser->rgb[2] | (app->tool_window.color_chooser->rgb[1] << 8)
				| (app->tool_window.color_chooser->rgb[0] << 16);
#endif
			(void)sprintf(str, "%06x", color);
			(void)IniFileAddString(file, "TOOL_BOX", "FORE_GROUND_COLOR", str);

#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
			color = app->tool_window.color_chooser->back_rgb[0] | (app->tool_window.color_chooser->back_rgb[1] << 8)
				| (app->tool_window.color_chooser->back_rgb[2] << 16);
#else
			color = app->tool_window.color_chooser->back_rgb[2] | (app->tool_window.color_chooser->back_rgb[1] << 8)
				| (app->tool_window.color_chooser->back_rgb[0] << 16);
#endif
			(void)sprintf(str, "%06x", color);
			(void)IniFileAddString(file, "TOOL_BOX", "BACK_GROUND_COLOR", str);

			(void)IniFileAddInteger(file, "TOOL_BOX", "SHOW_COLOR_CIRCLE",
				((app->tool_window.color_chooser->flags & COLOR_CHOOSER_SHOW_CIRCLE) != 0) ? 1 : 0, 10);
			(void)IniFileAddInteger(file, "TOOL_BOX", "SHOW_COLOR_PALLETE",
				((app->tool_window.color_chooser->flags & COLOR_CHOOSER_SHOW_PALLETE) != 0) ? 1 : 0, 10);
		}

		if(app->layer_window.window != NULL)
		{
			gtk_window_get_position(GTK_WINDOW(app->layer_window.window), &x, &y);
			gtk_window_get_size(GTK_WINDOW(app->layer_window.window), &width, &height);
		}
		else
		{
			x = app->layer_window.window_x, y = app->layer_window.window_y;
			width = app->layer_window.window_width, height = app->layer_window.window_height;
		}

		(void)IniFileAddInteger(file, "LAYER_VIEW", "X", x, 10);
		(void)IniFileAddInteger(file, "LAYER_VIEW", "Y", y, 10);
		(void)IniFileAddInteger(file, "LAYER_VIEW", "WIDTH", width, 10);
		(void)IniFileAddInteger(file, "LAYER_VIEW", "HEIGHT", height, 10);
		(void)IniFileAddInteger(file, "LAYER_VIEW", "PLACE", app->layer_window.place, 10);
		(void)IniFileAddInteger(file, "LAYER_VIEW", "POSITION",
			app->layer_window.pane_position, 10);

		if(app->navigation_window.window != NULL)
		{
			gtk_window_get_position(GTK_WINDOW(app->navigation_window.window), &x, &y);
			gtk_window_get_size(GTK_WINDOW(app->navigation_window.window), &width, &height);
		}
		else
		{
			x = app->navigation_window.window_x, y = app->navigation_window.window_y;
			width = app->navigation_window.window_width, y = app->navigation_window.window_height;
		}

		(void)IniFileAddString(file, "LAYER_VIEW", "SCROLLBAR_PLACEMENT",
			((app->layer_window.flags & LAYER_WINDOW_SCROLLBAR_PLACE_LEFT) == 0) ? "RIGHT" : "LEFT");

		(void)IniFileAddInteger(file, "NAVIGATION", "X", x, 10);
		(void)IniFileAddInteger(file, "NAVIGATION", "Y", y, 10);
		(void)IniFileAddInteger(file, "NAVIGATION", "WIDTH", width, 10);
		(void)IniFileAddInteger(file, "NAVIGATION", "HEIGHT", height, 10);

		if(app->preview_window.window != NULL)
		{
			gtk_window_get_position(GTK_WINDOW(app->preview_window.window), &x, &y);
			gtk_window_get_size(GTK_WINDOW(app->preview_window.window), &width, &height);
			(void)IniFileAddInteger(file, "PREVIEW", "X", x, 10);
			(void)IniFileAddInteger(file, "PREVIEW", "Y", y, 10);
			(void)IniFileAddInteger(file, "PREVIEW", "WIDTH", width, 10);
			(void)IniFileAddInteger(file, "PREVIEW", "HEIGHT", height, 10);
		}
	}

	if(app->reference_window.data != NULL)
	{
		gtk_window_get_position(GTK_WINDOW(app->reference_window.data->window),
			&app->reference_window.window_x, &app->reference_window.window_y);
		gtk_window_get_size(GTK_WINDOW(app->reference_window.data->window),
			&app->reference_window.window_width, &app->reference_window.window_height);
	}
	(void)IniFileAddInteger(file, "REFERENCE", "X", app->reference_window.window_x, 10);
	(void)IniFileAddInteger(file, "REFERENCE", "Y", app->reference_window.window_y, 10);
	(void)IniFileAddInteger(file, "REFERENCE", "WIDTH", app->reference_window.window_width, 10);
	(void)IniFileAddInteger(file, "REFERENCE", "HEIGHT", app->reference_window.window_height, 10);

	// ���j���[�f�[�^����������
		// �V�K�쐬
	(void)IniFileAddInteger(file, "NEW", "WIDTH", app->menu_data.make_new->width, 10);
	(void)IniFileAddInteger(file, "NEW", "HEIGHT", app->menu_data.make_new->height, 10);
	(void)IniFileAddInteger(file, "NEW", "RESOLUTION", app->menu_data.make_new->resolution, 10);
	color = app->menu_data.second_bg_color[2] | (app->menu_data.second_bg_color[1] << 8)
		| (app->menu_data.second_bg_color[0] << 16);
	(void)sprintf(str, "%06x", color);
	(void)IniFileAddString(file, "NEW", "SECOND_BG_COLOR", str);
	// ��u���␳
	(void)IniFileAddInteger(file, "SMOOTH", "LEVEL", app->tool_window.smoother.num_use, 10);
	(void)IniFileAddInteger(file, "SMOOTH", "RATE", (int)app->tool_window.smoother.rate, 10);
	(void)IniFileAddInteger(file, "SMOOTH", "MODE", app->tool_window.smoother.mode, 10);
	// �e�N�X�`��
	(void)IniFileAddDouble(file, "TEXTURE", "STRENGTH", app->textures.strength, 2);
	(void)IniFileAddDouble(file, "TEXTURE", "SCALE", app->textures.scale, 2);

	// ���ݒ����������
	WritePreference(file, &app->preference);
	// �^�b�`�C�x���g�̏������@����������
	(void)IniFileAddInteger(file, "PREFERENCE", "DRAW_WITH_TOUCH",
		(app->flags & APPLICATION_DRAW_WITH_TOUCH) != 0 ? 1 : 0, 10);
	// �w�i�F��ύX���邩����������
	(void)IniFileAddInteger(file, "PREFERENCE", "CHANGE_BACK_GROUND",
		(app->flags & APPLICATION_SET_BACK_GROUND_COLOR) != 0 ? 1 : 0, 10);
	// �v���r���[�E�B���h�E���^�X�N�o�[�ɕ\�����邩����������
	(void)IniFileAddInteger(file, "PREFERENCE", "SHOW_PREVIEW_WINDOW_ON_TASKBAR",
		(app->flags & APPLICATION_SHOW_PREVIEW_ON_TASK_BAR) != 0 ? 1 : 0, 10);

	// �g�p����ICC�v���t�@�C������������
	(void)IniFileAddString(file, "ICC_PROFILE", "INPUT", app->input_icc_path);
	(void)IniFileAddString(file, "ICC_PROFILE", "OUTPUT", app->output_icc_path);

	// �o�b�N�A�b�v���쐬����f�B���N�g������������
	{
		gchar *path;
		path = g_convert(app->backup_directory_path, -1, app->system_code, "UTF-8", NULL, NULL, NULL);
		(void)IniFileAddString(file, "BACKUP", "DIRECTORY", path);
		g_free(path);
	}

	// ���C���[���܂Ƃ߂č쐬����f�[�^����������
	WriteLayerGroupTemplateData(app, file);

	// �쐬�����f�[�^���t�@�C���ɏ�������
	(void)WriteIniFile(file, (size_t (*)(void*, size_t, size_t, void*))FileWrite);

	// �f�[�^���J��
	file->delete_func(file);

	// �����o���p�̃I�u�W�F�N�g���폜
	g_object_unref(fp);
	g_object_unref(stream);

	return 0;
}

/*******************************************************
* OnQuitApplication�֐�                                *
* �A�v���P�[�V�����I���O�ɌĂяo�����R�[���o�b�N�֐� *
* ����                                                 *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X   *
* �Ԃ�l                                               *
*	�I�����~:TRUE	�I�����s:FALSE                     *
*******************************************************/
gboolean OnQuitApplication(APPLICATION* app)
{
	// GUI�̃f�[�^�����o���p
	WRITE_APPLICATIOIN_DATA write_data;
	// �����ۑ��t�@�C���폜�p
	GDir *dir;
	// �t�H���_�ւ̃p�X(AppData�쐬�p)
	gchar *dir_path;
	// �t�@�C���ւ̃p�X(Mac�΍�)
	gchar *file_path;
	// OS�̕����R�[�h�̃p�X
	char *system_path;
	// �A�v���P�[�V�����f�[�^�f�B���N�g���Ƀf�[�^�쐬����t���O
	gboolean make_app_dir = FALSE;
	// for���p�̃J�E���^
	int i;

	for(i=0; i<app->window_num; i++)
	{
		if((app->draw_window[i]->history.flags & HISTORY_UPDATED) != 0)
		{	// �ۑ����邩�ǂ����̃_�C�A���O��\��
			GtkWidget* dialog = gtk_dialog_new_with_buttons(
				NULL,
				GTK_WINDOW(app->widgets->window),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_STOCK_SAVE,
				GTK_RESPONSE_ACCEPT,
				GTK_STOCK_NO,
				GTK_RESPONSE_NO,
				GTK_STOCK_CANCEL,
				GTK_RESPONSE_REJECT,
				NULL
			);
			// �_�C�A���O�ɓ���郉�x��
			GtkWidget* label = gtk_label_new(app->labels->save.quit_with_unsaved_change);
			// �_�C�A���O�̑I������
			gint result;

			// �_�C�A���O�Ƀ��x��������
			gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(
				GTK_DIALOG(dialog))), label, FALSE, FALSE, 0);
			gtk_widget_show(label);

			// �_�C�A���O�����s
			result = gtk_dialog_run(GTK_DIALOG(dialog));

			// �_�C�A���O�����
			gtk_widget_destroy(dialog);

			// �_�C�A���O�̑I���ɂ���ď�����؂�ւ�
			switch(result)
			{
			case GTK_RESPONSE_ACCEPT:	// �ۑ����s
				{
					int16 before_id = app->active_window;
					app->active_window = (int16)i;
					ExecuteSave(app);
					app->active_window = before_id;
				}
				break;
			case GTK_RESPONSE_REJECT:	// �L�����Z��
				// ���鑀����~�߂�
				return TRUE;
			}
		}
	}

	// �u���V�t�@�C�����X�V����
	if(app->common_tool_file_path[0] == '.' && app->common_tool_file_path[1] == '/')
	{
		file_path = g_build_filename(app->current_path, &app->common_tool_file_path[2], NULL);
		system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
		if(WriteCommonToolData(&app->tool_window, system_path, app) < 0)
		{
			app->flags |= APPLICATION_WRITE_PROGRAM_DATA_DIRECTORY;

			g_free(file_path);
			dir_path = g_build_filename(g_get_user_config_dir(), "KABURAGI", NULL);
			make_app_dir = TRUE;
			if((dir = g_dir_open(dir_path, 0, NULL)) == NULL)
			{
				(void)g_mkdir(dir_path,
#ifdef __MAC_OS__
					S_IRUSR | S_IWUSR | S_IXUSR |
					S_IRGRP | S_IWGRP | S_IXGRP |
					S_IROTH | S_IXOTH | S_IXOTH
#elif defined(_WIN32)
					0
#else
					S_IRUSR | S_IWUSR | S_IXUSR |
					S_IRGRP | S_IWGRP | S_IXGRP |
					S_IROTH | S_IXOTH | S_IXOTH
#endif
				);
			}
			else
			{
				g_dir_close(dir);
			}

			file_path = g_build_filename(dir_path, "common_tools.ini", NULL);
			if(WriteCommonToolData(&app->tool_window, file_path, app) == 0)
			{
				g_free(app->common_tool_file_path);
				app->common_tool_file_path = file_path;
			}
			else
			{
				g_free(file_path);
				file_path = g_build_filename(app->current_path, &app->common_tool_file_path[2], NULL);
				(void)WriteCommonToolData(&app->tool_window, file_path, app);
				g_free(file_path);
			}
			g_free(dir_path);

			app->flags &= ~(APPLICATION_WRITE_PROGRAM_DATA_DIRECTORY);
		}
		else
		{
			g_free(file_path);
		}

		g_free(system_path);
	}
	else
	{
		(void)WriteCommonToolData(&app->tool_window, app->common_tool_file_path, app);
	}

	if(app->brush_file_path[0] == '.' && app->brush_file_path[1] == '/')
	{
		file_path = g_build_filename(app->current_path, &app->brush_file_path[2], NULL);
		if(WriteBrushDetailData(&app->tool_window, file_path, app) < 0)
		{
			app->flags |= APPLICATION_WRITE_PROGRAM_DATA_DIRECTORY;
			g_free(file_path);
			dir_path = g_build_filename(g_get_user_config_dir(), "KABURAGI", NULL);
			make_app_dir = TRUE;
			if((dir = g_dir_open(dir_path, 0, NULL)) == NULL)
			{
				(void)g_mkdir(dir_path,
#ifdef __MAC_OS__
					S_IRUSR | S_IWUSR | S_IXUSR |
					S_IRGRP | S_IWGRP | S_IXGRP |
					S_IROTH | S_IXOTH | S_IXOTH
#elif defined(_WIN32)
					0
#else
					S_IRUSR | S_IWUSR | S_IXUSR |
					S_IRGRP | S_IWGRP | S_IXGRP |
					S_IROTH | S_IXOTH | S_IXOTH
#endif
				);
			}
			else
			{
				g_dir_close(dir);
			}

			file_path = g_build_filename(dir_path, "brushes.ini", NULL);
			if(WriteBrushDetailData(&app->tool_window, file_path, app) == 0)
			{
				g_free(app->brush_file_path);
				app->brush_file_path = file_path;
			}
			else
			{
				g_free(file_path);
				file_path = g_build_filename(app->current_path, &app->brush_file_path[2], NULL);
				(void)WriteBrushDetailData(&app->tool_window, file_path, app);
				g_free(file_path);
			}
			g_free(dir_path);

			app->flags &= ~(APPLICATION_WRITE_PROGRAM_DATA_DIRECTORY);
		}
		else
		{
			g_free(file_path);
		}
	}
	else
	{
		(void)WriteBrushDetailData(&app->tool_window, app->brush_file_path, app);
	}

	if(app->vector_brush_file_path[0] == '.' && app->vector_brush_file_path[1] == '/')
	{
		file_path = g_build_filename(app->current_path, &app->vector_brush_file_path[2], NULL);
		system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
		if(WriteVectorBrushData(&app->tool_window, system_path, app) < 0)
		{
			app->flags |= APPLICATION_WRITE_PROGRAM_DATA_DIRECTORY;

			g_free(file_path);
			dir_path = g_build_filename(g_get_user_config_dir(), "KABURAGI", NULL);
			make_app_dir = TRUE;
			if((dir = g_dir_open(dir_path, 0, NULL)) == NULL)
			{
				(void)g_mkdir(dir_path,
#ifdef __MAC_OS__
					S_IRUSR | S_IWUSR | S_IXUSR |
					S_IRGRP | S_IWGRP | S_IXGRP |
					S_IROTH | S_IXOTH | S_IXOTH
#elif defined(_WIN32)
					0
#else
					S_IRUSR | S_IWUSR | S_IXUSR |
					S_IRGRP | S_IWGRP | S_IXGRP |
					S_IROTH | S_IXOTH | S_IXOTH
#endif
				);
			}
			else
			{
				g_dir_close(dir);
			}

			file_path = g_build_filename(dir_path, "vector_brushes.ini", NULL);
			if(WriteVectorBrushData(&app->tool_window, file_path, app) == 0)
			{
				g_free(app->vector_brush_file_path);
				app->vector_brush_file_path = file_path;
			}
			else
			{
				g_free(file_path);
				file_path = g_build_filename(app->current_path, &app->vector_brush_file_path[2], NULL);
				(void)WriteVectorBrushData(&app->tool_window, file_path, app);
				g_free(file_path);
			}
			g_free(dir_path);

			app->flags &= ~(APPLICATION_WRITE_PROGRAM_DATA_DIRECTORY);
		}
		else
		{
			g_free(file_path);
		}
	}
	else
	{
		(void)WriteVectorBrushData(&app->tool_window, app->vector_brush_file_path, app);
	}

	// �V�K�쐬���̃v���Z�b�g�f�[�^���X�V����
	if(app->make_new_file_path[0] == '.' && app->make_new_file_path[1] == '/')
	{
		file_path = g_build_filename(app->current_path, &app->make_new_file_path[2], NULL);
		if(WriteMakeNewData(app, file_path) == FALSE)
		{
			app->flags |= APPLICATION_WRITE_PROGRAM_DATA_DIRECTORY;
			g_free(file_path);
			dir_path = g_build_filename(g_get_user_config_dir(), "KABURAGI", NULL);
			make_app_dir = TRUE;
			if((dir = g_dir_open(dir_path, 0, NULL)) == NULL)
			{
				(void)g_mkdir(dir_path,
#ifdef __MAC_OS__
					S_IRUSR | S_IWUSR | S_IXUSR |
					S_IRGRP | S_IWGRP | S_IXGRP |
					S_IROTH | S_IXOTH | S_IXOTH
#elif defined(_WIN32)
					0
#else
					S_IRUSR | S_IWUSR | S_IXUSR |
					S_IRGRP | S_IWGRP | S_IXGRP |
					S_IROTH | S_IXOTH | S_IXOTH
#endif
				);
			}
			else
			{
				g_dir_close(dir);
			}

			file_path = g_build_filename(dir_path, "make_new_list.ini", NULL);
			if(WriteMakeNewData(app, file_path) != FALSE)
			{
				g_free(app->brush_file_path);
				app->brush_file_path = file_path;
			}
			else
			{
				g_free(file_path);
				file_path = g_build_filename(app->current_path, &app->make_new_file_path[2], NULL);
				(void)WriteMakeNewData(app, file_path);
				g_free(file_path);
			}
			g_free(dir_path);

			app->flags &= ~(APPLICATION_WRITE_PROGRAM_DATA_DIRECTORY);
		}
		else
		{
			g_free(file_path);
		}
	}
	else
	{
		(void)WriteMakeNewData(app, app->make_new_file_path);
	}

	// �������t�@�C�����X�V����
	if(make_app_dir == FALSE)
	{
		dir_path = g_strdup(app->current_path);
	}
	else
	{
		dir_path = g_build_filename(g_get_user_config_dir(), "KABURAGI", NULL);

		if((dir = g_dir_open(dir_path, 0, NULL)) == NULL)
		{
			(void)g_mkdir(dir_path,
#ifdef __MAC_OS__
				S_IRUSR | S_IWUSR | S_IXUSR |
				S_IRGRP | S_IWGRP | S_IXGRP |
				S_IROTH | S_IXOTH | S_IXOTH
#elif defined(_WIN32)
				0
#else
				S_IRUSR | S_IWUSR | S_IXUSR |
				S_IRGRP | S_IWGRP | S_IXGRP |
				S_IROTH | S_IXOTH | S_IXOTH
#endif
			);
		}
		else
		{
			g_dir_close(dir);
		}
	}

	file_path = g_build_filename(dir_path, INITIALIZE_FILE_NAME, NULL);
	GetWriteMainWindowData(&write_data, app);
	if(WriteInitializeFile(app, &write_data, file_path) != 0)
	{
		g_free(file_path);
		file_path = g_build_filename(app->current_path, INITIALIZE_FILE_NAME, NULL);
		(void)WriteInitializeFile(app, &write_data, file_path);
	}
	g_free(file_path);

	// �p���b�g�t�@�C�����X�V����
	file_path = g_build_filename(dir_path, PALLETE_FILE_NAME, NULL);
	if(WritePalleteFile(app->tool_window.color_chooser, file_path) != 0)
	{
		g_free(file_path);
		file_path = g_build_filename(app->current_path, PALLETE_FILE_NAME, NULL);
		WritePalleteFile(app->tool_window.color_chooser, file_path);
	}
	g_free(file_path);

	g_free(dir_path);

	// �����ۑ��t�@�C�����폜
	dir = g_dir_open(app->backup_directory_path, 0, NULL);
	if(dir != NULL)
	{
		gchar *file_name;
		size_t name_length;

		while((file_name = (gchar*)g_dir_read_name(dir)) != NULL)
		{
			name_length = strlen(file_name);
			if(name_length >= 4)
			{
				if(StringCompareIgnoreCase(&file_name[name_length-4], ".kbt") == 0)
				{
					char *system_path;
					file_path = g_build_filename(app->current_path, file_name, NULL);
					system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
					(void)remove(system_path);
					g_free(file_path);
					g_free(system_path);
				}
			}
		}

		g_dir_close(dir);
	}

	// �A�v���P�[�V�����I��
	gtk_main_quit();

	return FALSE;
}

/*********************************************************************
* UpdateWindowTitle�֐�                                              *
* �E�C���h�E�^�C�g���̍X�V                                           *
* ����                                                               *
* app				: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************************/
void UpdateWindowTitle(APPLICATION* app)
{
	gchar *window_title;

	if(app->window_num)
	{
		window_title = GetWindowTitle(app->draw_window[app->active_window], app);
	}
	else
	{
		window_title = g_strdup_printf("%s %d.%d.%d.%d", g_get_application_name(),
			MAJOR_VERSION, MINOR_VERSION, RELEASE_VERSION, BUILD_VERSION);
	}

	gtk_window_set_title(GTK_WINDOW(app->widgets->window), window_title);

	g_free(window_title);
}

/*****************************************************
* RecoverBackUp�֐�                                  *
* �o�b�N�A�b�v�t�@�C�����������ꍇ�ɕ�������         *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void RecoverBackUp(APPLICATION* app)
{
	// ���s�t�@�C���̃f�B���N�g�����`�F�b�N
	GDir *dir = g_dir_open(app->backup_directory_path, 0, NULL);
	// �t�@�C���p�X
	gchar *file_path;
	// �t�@�C�����̒���
	size_t name_length;
	// �t�@�C����
	gchar *file_name;

	// �f�B���N�g���I�[�v�����s
	if(dir == NULL)
	{
		return;
	}

	// �f�B���N�g���̃t�@�C����ǂݍ���
	while((file_name = (gchar*)g_dir_read_name(dir)) != NULL)
	{
		name_length = strlen(file_name);
		if(name_length >= 4)
		{
			file_path = g_build_filename(app->backup_directory_path, file_name, NULL);
			if(StringCompareIgnoreCase(&file_name[name_length-4], ".kbt") == 0)
			{	// �ꎞ�ۑ��t�@�C������
					// �������邩�q�˂�
				GtkWidget *dialog = gtk_message_dialog_new(
					GTK_WINDOW(app->widgets->window), GTK_DIALOG_MODAL, GTK_MESSAGE_QUESTION,
						GTK_BUTTONS_YES_NO, app->labels->save.recover_backup);
				gchar *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
				if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_YES)
				{
					// �t�@�C���ǂݍ��݃X�g���[��
					FILE *fp = fopen(system_path, "rb");
					// �t�@�C���T�C�Y
					size_t data_size;
					// �t�@�C�����쐬�p
					char *str;
					int i;	// for���p�̃J�E���^

					(void)fseek(fp, 0, SEEK_END);
					data_size = (size_t)ftell(fp);
					rewind(fp);

					// �t�@�C���p�X����t�@�C�������������o��
					file_name = str = file_path;
					while(*str != '\0')
					{
						if(*str == '/' || *str == '\\')
						{
							file_name = str+1;
						}

						str = g_utf8_next_char(str);
					}

					// �t�@�C���I�[�v�����̃t���O�𗧂Ă邱�ƂŃE�B�W�F�b�g�̃R�[���o�b�N�ɂ��
						// �Z�O�����e�[�V�����t�H�[���g�����
					app->flags |= APPLICATION_IN_OPEN_OPERATION;
					//app->active_window = app->window_num;
					app->draw_window[app->window_num] =
						ReadOriginalFormat(fp, (stream_func_t)fread, data_size, app, file_name);
					app->flags &= ~(APPLICATION_IN_OPEN_OPERATION);

					if(app->draw_window[app->window_num] != NULL)
					{
						app->active_window = app->window_num;
						// �t�@�C���p�X��ݒ�
						app->draw_window[app->active_window]->file_path = MEM_STRDUP_FUNC(file_path);

						// �i�r�Q�[�V�����̕\�����X�V
						ChangeNavigationDrawWindow(&app->navigation_window, app->draw_window[app->active_window]);

						// �����ɂ��Ă����ꕔ�̃��j���[��L����
						for(i=0; i<app->menus.num_disable_if_no_open; i++)
						{
							gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
						}
						gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
						gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);

						// �`��̈�̃J�E���^���X�V
						app->window_num++;

						// �E�B���h�E�̃^�C�g�����X�V
						UpdateWindowTitle(app);
					}

					(void)fclose(fp);
				}

				(void)remove(system_path);
				g_free(system_path);

				gtk_widget_destroy(dialog);
			}

			g_free(file_path);
		}
	}
}

/*********************************************************
* OpenFile�֐�                                           *
* �󂯂��Ƃ��p�X�̃t�@�C�����J��                         *
* ����                                                   *
* file_path	: �t�@�C���p�X                               *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
void OpenFile(char *file_path, APPLICATION* app)
{
	// �s�N�Z���o�b�t�@�ɕϊ�
	GdkPixbuf* pixbuf;
	// �g���q����p
	gchar* str;
	// �g���q
	gchar *extention;
	// �t�@�C����
	gchar* file_name;
	// for���p�̃J�E���^
	int i, j;
	// �`��̈搔�i�����E���s�̔���p�j
	int last_window_num = app->window_num;

	// �g���q�擾
	str = file_path + strlen(file_path) - 1;
	while(*str != '.' && str > file_path)
	{
		str--;
	}
	extention = str;

	if(StringCompareIgnoreCase(str, ".kab") == 0)
	{
		// �t�@�C���ǂݍ��݃X�g���[��
		gchar *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
		FILE *fp = fopen(system_path, "rb");
		// �t�@�C���T�C�Y
		size_t data_size;
		// �Ǎ������b�Z�[�W��ID
		guint context_id, message_id;
		// �\���C�x���g�����p
		GdkEvent *queued_event;

		// �Ǎ����̃��b�Z�[�W��\��
		context_id = gtk_statusbar_get_context_id(GTK_STATUSBAR(app->widgets->status_bar), "Loading");
		message_id = gtk_statusbar_push(GTK_STATUSBAR(app->widgets->status_bar),
			context_id, app->labels->window.loading);
		// �C�x���g���񂵂ă��b�Z�[�W��\��
#if GTK_MAJOR_VERSION <= 2
		gdk_window_process_updates(app->widgets->status_bar->window, TRUE);
#else
		gdk_window_process_updates(gtk_widget_get_window(app->widgets->status_bar), TRUE);
#endif

		while(gdk_events_pending() != FALSE)
		{
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event != NULL)
			{
#if GTK_MAJOR_VERSION <= 2
				if(queued_event->any.window == app->widgets->status_bar->window
#else
				if(queued_event->any.window == gtk_widget_get_window(app->widgets->status_bar)
#endif
					&& queued_event->any.type == GDK_EXPOSE)
				{
					gdk_event_free(queued_event);
					break;
				}
				else
				{
					gdk_event_free(queued_event);
				}
			}
		}

		(void)fseek(fp, 0, SEEK_END);
		data_size = (size_t)ftell(fp);
		rewind(fp);

		g_free(system_path);
		
		// �t�@�C���p�X����t�@�C�������������o��
		file_name = str = file_path;
		while(*str != '\0')
		{
			if(*str == '/' || *str == '\\')
			{
				file_name = str+1;
			}

			str = g_utf8_next_char(str);
		}

		// �t�@�C���I�[�v�����̃t���O�𗧂Ă邱�ƂŃE�B�W�F�b�g�̃R�[���o�b�N�ɂ��
			// �Z�O�����e�[�V�����t�H�[���g�����
		app->flags |= APPLICATION_IN_OPEN_OPERATION;
		//app->active_window = app->window_num;
		app->draw_window[app->window_num] =
			ReadOriginalFormat(fp, (stream_func_t)fread, data_size, app, file_name);
		app->flags &= ~(APPLICATION_IN_OPEN_OPERATION);

		if(app->draw_window[app->window_num] == NULL)
		{
			return;
		}

		app->active_window = app->window_num;
		// �t�@�C���p�X��ݒ�
		app->draw_window[app->active_window]->file_path = MEM_STRDUP_FUNC(file_path);

		// �i�r�Q�[�V�����̕\�����X�V
		ChangeNavigationDrawWindow(&app->navigation_window, app->draw_window[app->active_window]);

		// �ǂݍ��݃X�g���[���폜
		(void)fclose(fp);

		// �����ɂ��Ă����ꕔ�̃��j���[��L����
		for(i=0; i<app->menus.num_disable_if_no_open; i++)
		{
			gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
		}
		gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
		gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);

		// �A�N�e�B�u�ȃ��C���[�̏����R���g���[���ɐݒ�
		gtk_combo_box_set_active(GTK_COMBO_BOX(app->layer_window.layer_control.mode), app->draw_window[app->active_window]->active_layer->layer_mode);
		gtk_adjustment_set_value(app->layer_window.layer_control.opacity, app->draw_window[app->active_window]->active_layer->alpha);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->layer_window.layer_control.mask_width_under),
			app->draw_window[app->active_window]->active_layer->flags & LAYER_MASKING_WITH_UNDER_LAYER);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->layer_window.layer_control.lock_opacity),
			app->draw_window[app->active_window]->active_layer->flags & LAYER_LOCK_OPACITY);

		// �`��̈�̃J�E���^���X�V
		app->window_num++;

		// ���b�Z�[�W�\�����I��
		gtk_statusbar_remove(GTK_STATUSBAR(app->widgets->status_bar), context_id, message_id);
	}
	else if(StringCompareIgnoreCase(str, ".tlg") == 0)
	{
		// �t�@�C���ǂݍ��݃X�g���[��
		gchar *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
		FILE *fp = fopen(system_path, "rb");

		if(fp != NULL)
		{
			uint8 *pixels;
			int width, height;
			int channel;

			pixels = ReadTlgStream((void*)fp, (size_t (*)(void*, size_t, size_t, void*))fread,
				(int (*)(void*, long, int))fseek, (long (*)(void*))ftell, &width, &height, &channel);

			if(pixels != NULL)
			{
				// �s�N�Z���f�[�^���ʂ����C���[
				LAYER *target;
				// 1�s���̃o�C�g��
				int stride = width * 4;

				// �t�@�C���p�X����t�@�C�������������o��
				file_name = str = file_path;
				while(*str != '\0')
				{
					if(*str == '/' || *str == '\\')
					{
						file_name = str+1;
					}

					str = g_utf8_next_char(str);
				}

				if(channel == 1)
				{
					uint8 *old_pixels = pixels;
					pixels = (uint8*)MEM_ALLOC_FUNC(width*height*4);
					for(i=0; i<width*height; i++)
					{
						pixels[i*4] = old_pixels[i];
						pixels[i*4+1] = old_pixels[i];
						pixels[i*4+2] = old_pixels[i];
						pixels[i*4+3] = 0xff;
					}

					MEM_FREE_FUNC(old_pixels);
				}
				else if(channel == 2)
				{
					uint8 *old_pixels = pixels;
					pixels = (uint8*)MEM_ALLOC_FUNC(width*height*4);
					for(i=0; i<width*height; i++)
					{
						pixels[i*4] = old_pixels[i*2];
						pixels[i*4+1] = old_pixels[i*2];
						pixels[i*4+2] = old_pixels[i*2];
						pixels[i*4+3] = old_pixels[i*2+1];
					}

					MEM_FREE_FUNC(old_pixels);
				}
				else if(channel == 3)
				{
					uint8 *old_pixels = pixels;
					pixels = (uint8*)MEM_ALLOC_FUNC(width*height*4);
					for(i=0; i<width*height; i++)
					{
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
						pixels[i*4] = old_pixels[i*3+2];
						pixels[i*4+1] = old_pixels[i*3+1];
						pixels[i*4+2] = old_pixels[i*3];
#else
						pixels[i*4] = old_pixels[i*3];
						pixels[i*4+1] = old_pixels[i*3+1];
						pixels[i*4+2] = old_pixels[i*3+2];
#endif
						pixels[i*4+3] = 0xff;
					}

					MEM_FREE_FUNC(old_pixels);
				}
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
				else
				{
					uint8 b;
					for(i=0; i<width*height; i++)
					{
						b = pixels[i*4];
						pixels[i*4] = pixels[i*4+2];
						pixels[i*4+2] = b;
					}
				}
#endif

				// �`��̈��V���ɒǉ�
				app->draw_window[app->window_num] = CreateDrawWindow(
					width, height, 4, file_name, app->widgets->note_book, app->window_num, app);

				app->active_window = app->window_num;
				app->window_num++;

				// �t�@�C���p�X��ݒ�
				app->draw_window[app->active_window]->file_path = MEM_STRDUP_FUNC(file_path);

				// �i�r�Q�[�V�����̕\�����X�V
				ChangeNavigationDrawWindow(&app->navigation_window, app->draw_window[app->active_window]);

				// �摜�f�[�^����ԉ��̃��C���[�ɃR�s�[
				target = app->draw_window[app->active_window]->active_layer;

				for(i=0; i<height; i++)
				{
					for(j=0; j<width; j++)
					{
						target->pixels[target->stride*i+target->channel*j] =
							pixels[(height-i-1)*stride+j*target->channel+2];
						target->pixels[target->stride*i+target->channel*j+1] =
							pixels[(height-i-1)*stride+j*target->channel+1];
						target->pixels[target->stride*i+target->channel*j+2] =
							pixels[(height-i-1)*stride+j*target->channel];
						target->pixels[target->stride*i+target->channel*j+3] =
							pixels[(height-i-1)*stride+j*target->channel+3];
					}
				}

				// ��ԉ��̃��C���[���A�N�e�B�u�\����
				LayerViewSetActiveLayer(
					app->draw_window[app->active_window]->active_layer, app->layer_window.view
				);

				// �����ɂ��Ă����ꕔ�̃��j���[��L����
				for(i=0; i<app->menus.num_disable_if_no_open; i++)
				{
					gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
				}
				gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
				gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);
			}
		}

		g_free(system_path);
	}
	else if(StringCompareIgnoreCase(str, ".psd") == 0)
	{
		// �t�@�C���ǂݍ��݃X�g���[��
		gchar *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
		FILE *fp = fopen(system_path, "rb");
		// �L�����o�X�f�[�^
		DRAW_WINDOW *window;
		// PSD�f�[�^�̃��C���[
		LAYER *layer;

		layer = ReadPhotoShopDocument((void*)fp, (stream_func_t)fread,
			(seek_func_t)fseek, (long (*)(void*))ftell);
		if(layer != NULL)
		{
			// KABURAGI�͑S�Ẵ��C���[�̈ʒu������
				// ���A�����T�C�Y�Ȃ̂Œ������s��
			LAYER *target;
			LAYER *canvas_layer;
			int32 min_x = layer->x, min_y = layer->y;
			int32 max_x = layer->x + layer->width, max_y = layer->y + layer->height;
			int32 width, height;
			uint8 alpha;
			target = layer->next;
			while(target != NULL)
			{
				if(min_x > target->x)
				{
					min_x = target->x;
				}
				if(min_y > target->y)
				{
					min_y = target->y;
				}
				if(max_x < target->x + target->width)
				{
					max_x = target->x + target->width;
				}
				if(max_y < target->y + target->height)
				{
					max_y = target->y + target->height;
				}
				target = target->next;
			}
			width = max_x - min_x;
			height = max_y - min_y;

			// �t�@�C���p�X����t�@�C�������������o��
			file_name = str = file_path;
			while(*str != '\0')
			{
				if(*str == '/' || *str == '\\')
				{
					file_name = str+1;
				}

				str = g_utf8_next_char(str);
			}
			app->draw_window[app->window_num] = window =
				CreateDrawWindow(width, height, 4, file_name,
					app->widgets->note_book, app->window_num, app);
			DeleteLayer(&window->layer);
			window->layer = CreateLayer(0, 0, window->width, window->height,
				4, TYPE_NORMAL_LAYER, NULL, NULL,
				layer->name, window);
			window->layer->flags = layer->flags;
			target = layer->next;
			canvas_layer = window->layer;
			while(target != NULL)
			{
				canvas_layer->next = CreateLayer(0, 0, window->width, window->height, 4, TYPE_NORMAL_LAYER,
					canvas_layer, NULL, target->name, window);
				canvas_layer = canvas_layer->next;
				canvas_layer->flags = target->flags;
				canvas_layer->alpha = target->alpha;
				target = target->next;
			}
			target = layer;
			window->num_layer = 0;
			// ���C���[���̐����グ�Ɠ����F�̒���
			while(target != NULL)
			{
				window->num_layer++;
				for(i=0; i<target->width * target->height; i++)
				{
					alpha = target->pixels[i*4+3];
					target->pixels[i*4+0] = MINIMUM(alpha, target->pixels[i*4+0]);
					target->pixels[i*4+1] = MINIMUM(alpha, target->pixels[i*4+1]);
					target->pixels[i*4+2] = MINIMUM(alpha, target->pixels[i*4+2]);
				}
				target = target->next;
			}
			target = layer;
			canvas_layer = window->layer;
			while(target != NULL)
			{
				int copy_start_x, copy_start_y;
				copy_start_x = target->x - min_x;
				copy_start_y = target->y - min_y;
				for(i=0; i<target->height; i++)
				{
					(void)memcpy(&canvas_layer->pixels[(copy_start_y+i)*canvas_layer->stride + copy_start_x*4],
						&target->pixels[i*target->stride], target->stride);
				}
				canvas_layer = canvas_layer->next;
				target = target->next;
			}

			// �ǂݍ��݂Ɏg�������C���[�����폜
			target = layer;
			while(target != NULL)
			{
				layer = target->next;
				DeleteTempLayer(&target);
				target = layer;
			}
			(void)fclose(fp);

			app->active_window = app->window_num;
			app->window_num++;

			layer = window->layer;
			for(i=0; i<window->num_layer; i++)
			{
				if(layer == NULL)
				{
					window->num_layer = i;
					break;
				}
				LayerViewAddLayer(layer, window->layer, app->layer_window.view, i+1);
				layer = layer->next;
			}

			// �t�@�C���p�X��ݒ�
			window->file_path = MEM_STRDUP_FUNC(file_path);

			// �i�r�Q�[�V�����̕\�����X�V
			ChangeNavigationDrawWindow(&app->navigation_window, window);

			// ��ԉ��̃��C���[���A�N�e�B�u�\����
			LayerViewSetActiveLayer(
				window->active_layer, app->layer_window.view
			);

			// �����ɂ��Ă����ꕔ�̃��j���[��L����
			for(i=0; i<app->menus.num_disable_if_no_open; i++)
			{
				gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
			}
			gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
			gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);
		}
	}
	else if(StringCompareIgnoreCase(str, ".dds") == 0)
	{
		// �t�@�C���ǂݍ��݃X�g���[��
		gchar *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
		FILE *fp = fopen(system_path, "rb");

		if(fp != NULL)
		{
			uint8 *pixels;
			int width, height;
			int channel;
			size_t file_size;

			(void)fseek(fp, 0, SEEK_END);
			file_size = ftell(fp);
			rewind(fp);

			pixels = ReadDdsStream((void*)fp, (stream_func_t)fread,
				(seek_func_t)fseek, file_size, &width, &height, &channel);

			if(pixels != NULL)
			{
				// �s�N�Z���f�[�^���ʂ����C���[
				LAYER *target;
				// 1�s���̃o�C�g��
				int stride = width * 4;
				// �㉺���]�p
				uint8 *trans = (uint8*)MEM_ALLOC_FUNC(width*height*4);

				// �t�@�C���p�X����t�@�C�������������o��
				file_name = str = file_path;
				while(*str != '\0')
				{
					if(*str == '/' || *str == '\\')
					{
						file_name = str+1;
					}

					str = g_utf8_next_char(str);
				}

				if(channel == 1)
				{
					uint8 *old_pixels = pixels;
					pixels = (uint8*)MEM_ALLOC_FUNC(width*height*4);
					for(i=0; i<width*height; i++)
					{
						pixels[i*4] = old_pixels[i];
						pixels[i*4+1] = old_pixels[i];
						pixels[i*4+2] = old_pixels[i];
						pixels[i*4+3] = 0xff;
					}

					MEM_FREE_FUNC(old_pixels);
				}
				else if(channel == 2)
				{
					uint8 *old_pixels = pixels;
					pixels = (uint8*)MEM_ALLOC_FUNC(width*height*4);
					for(i=0; i<width*height; i++)
					{
						pixels[i*4] = old_pixels[i*2];
						pixels[i*4+1] = old_pixels[i*2];
						pixels[i*4+2] = old_pixels[i*2];
						pixels[i*4+3] = old_pixels[i*2+1];
					}

					MEM_FREE_FUNC(old_pixels);
				}
				else if(channel == 3)
				{
					uint8 *old_pixels = pixels;
					pixels = (uint8*)MEM_ALLOC_FUNC(width*height*4);
					for(i=0; i<width*height; i++)
					{
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
						pixels[i*4] = old_pixels[i*3];
						pixels[i*4+1] = old_pixels[i*3+1];
						pixels[i*4+2] = old_pixels[i*3+2];
#else
						pixels[i*4] = old_pixels[i*3+2];
						pixels[i*4+1] = old_pixels[i*3+1];
						pixels[i*4+2] = old_pixels[i*3];
#endif
						pixels[i*4+3] = 0xff;
					}

					MEM_FREE_FUNC(old_pixels);
				}
#if !defined(USE_BGR_COLOR_SPACE) || USE_BGR_COLOR_SPACE == 0
				else
				{
					uint8 b;
					for(i=0; i<width*height; i++)
					{
						b = pixels[i*4];
						pixels[i*4] = pixels[i*4+2];
						pixels[i*4+2] = b;
					}
				}
#endif
				for(i=0; i<height; i++)
				{
					(void)memcpy(&trans[stride*i], &pixels[(height-i-1)*stride], stride);
				}
				MEM_FREE_FUNC(pixels);
				pixels = trans;

				// �`��̈��V���ɒǉ�
				app->draw_window[app->window_num] = CreateDrawWindow(
					width, height, 4, file_name, app->widgets->note_book, app->window_num, app);

				app->active_window = app->window_num;
				app->window_num++;

				// �t�@�C���p�X��ݒ�
				app->draw_window[app->active_window]->file_path = MEM_STRDUP_FUNC(file_path);

				// �i�r�Q�[�V�����̕\�����X�V
				ChangeNavigationDrawWindow(&app->navigation_window, app->draw_window[app->active_window]);

				// �摜�f�[�^����ԉ��̃��C���[�ɃR�s�[
				target = app->draw_window[app->active_window]->active_layer;

				for(i=0; i<height; i++)
				{
					for(j=0; j<width; j++)
					{
						target->pixels[target->stride*i+target->channel*j] =
							pixels[(height-i-1)*stride+j*target->channel+2];
						target->pixels[target->stride*i+target->channel*j+1] =
							pixels[(height-i-1)*stride+j*target->channel+1];
						target->pixels[target->stride*i+target->channel*j+2] =
							pixels[(height-i-1)*stride+j*target->channel];
						target->pixels[target->stride*i+target->channel*j+3] =
							pixels[(height-i-1)*stride+j*target->channel+3];
					}
				}

				// ��ԉ��̃��C���[���A�N�e�B�u�\����
				LayerViewSetActiveLayer(
					app->draw_window[app->active_window]->active_layer, app->layer_window.view
				);

				// �����ɂ��Ă����ꕔ�̃��j���[��L����
				for(i=0; i<app->menus.num_disable_if_no_open; i++)
				{
					gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
				}
				gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
				gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);
			}
		}

		g_free(system_path);
	}
	else
	{
		GError *error = NULL;
		LAYER *target;
		pixbuf = gdk_pixbuf_new_from_file(file_path, &error);

		// �ϊ�����
		if(pixbuf != NULL)
		{	// �摜�̕��A�����A�s�N�Z���̃f�[�^���擾
			int32 width = gdk_pixbuf_get_width(pixbuf),
				height = gdk_pixbuf_get_height(pixbuf);
			uint8* pixels;
			int stride;
			const int channel = 4;

			// �t�@�C���p�X����t�@�C�������������o��
			file_name = str = file_path;
			while(*str != '\0')
			{
				if(*str == '/' || *str == '\\')
				{
					file_name = str+1;
				}

				str = g_utf8_next_char(str);
			}

			// ���`�����l�����Ȃ���Βǉ�
			if(gdk_pixbuf_get_has_alpha(pixbuf) == FALSE)
			{
				GdkPixbuf *old_buf = pixbuf;
				pixbuf = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
				g_object_unref(old_buf);
			}

			// �s�N�Z���Ɖ摜��񕪂̃o�C�g�����擾
			pixels = gdk_pixbuf_get_pixels(pixbuf);
			stride = gdk_pixbuf_get_rowstride(pixbuf);

			// �`��̈��V���ɒǉ�
			app->draw_window[app->window_num] = CreateDrawWindow(
				width, height, 4, file_name, app->widgets->note_book, app->window_num, app);
			// ICC�v���t�@�C���p�̃_�C�A���O���\��������expose�C�x���g���Ă΂�Ă��܂��̂�
				// �ꎞ�I�ɃV�O�i�����~����
#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
			g_signal_handlers_disconnect_by_func(app->draw_window[app->window_num]->gl_area,
				G_CALLBACK(DisplayDrawWindow), app->draw_window[app->window_num]);
#endif

			app->active_window = app->window_num;
			// �t�@�C���p�X��ݒ�
			app->draw_window[app->active_window]->file_path = MEM_STRDUP_FUNC(file_path);

			// �i�r�Q�[�V�����̕\�����X�V
			ChangeNavigationDrawWindow(&app->navigation_window, app->draw_window[app->active_window]);

			// �摜�f�[�^����ԉ��̃��C���[�ɃR�s�[
			target = app->draw_window[app->active_window]->active_layer;
			for(i=0; i<height; i++)
			{
				for(j=0; j<width; j++)
				{
					target->pixels[target->stride*i+target->channel*j] =
						pixels[i*stride+j*target->channel+2];
					target->pixels[target->stride*i+target->channel*j+1] =
						pixels[i*stride+j*target->channel+1];
					target->pixels[target->stride*i+target->channel*j+2] =
						pixels[i*stride+j*target->channel];
					target->pixels[target->stride*i+target->channel*j+3] =
						pixels[i*stride+j*target->channel+3];
				}
			}

			// ��ԉ��̃��C���[���A�N�e�B�u�\����
			LayerViewSetActiveLayer(
				app->draw_window[app->active_window]->active_layer, app->layer_window.view
			);

			// �s�N�Z���o�b�t�@�͕s�v
			g_object_unref(pixbuf);

			// �����ɂ��Ă����ꕔ�̃��j���[��L����
			for(i=0; i<app->menus.num_disable_if_no_open; i++)
			{
				gtk_widget_set_sensitive(app->menus.disable_if_no_open[i], TRUE);
			}
			gtk_widget_set_sensitive(app->layer_window.layer_control.mode, TRUE);
			gtk_widget_set_sensitive(app->layer_window.layer_control.lock_opacity, TRUE);

			// �𑜓x�f�[�^�����Őݒ�
			app->draw_window[app->active_window]->resolution = DEFALUT_RESOLUTION;

			// �𑜓x�f�[�^�AICC�v���t�@�C���f�[�^���\�Ȃ�ǂݍ���
			if(StringCompareIgnoreCase(extention, ".jpg") == 0
				|| StringCompareIgnoreCase(extention, ".jpeg") == 0)
			{
				char *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
				int resolution = 0;
				uint8 *icc_profile_data;
				int32 icc_profile_size;

				ReadJpegHeader(system_path, NULL, NULL, &resolution,
					&icc_profile_data, &icc_profile_size);
				if(resolution != 0)
				{
					app->draw_window[app->active_window]->resolution = resolution;
				}

				if(icc_profile_data != NULL)
				{
					app->draw_window[app->active_window]->icc_profile_data = icc_profile_data;
					DrawWindowSetIccProfile(app->draw_window[app->active_window], icc_profile_size, TRUE);
				}

				g_free(system_path);
			}
			else if(StringCompareIgnoreCase(extention, ".png") == 0)
			{
				char *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
				FILE *fp = fopen(system_path, "rb");
				int32 resolution = 0;
				uint8 *icc_profile_data;
				int32 icc_profile_size;

				ReadPNGHeader((void*)fp, (stream_func_t)fread, NULL, NULL, NULL,
					&resolution, NULL, &icc_profile_data, &icc_profile_size);

				if(resolution != 0)
				{
					app->draw_window[app->active_window]->resolution = (int16)resolution;
				}

				if(icc_profile_data != NULL)
				{
					app->draw_window[app->active_window]->icc_profile_data = icc_profile_data;
					DrawWindowSetIccProfile(app->draw_window[app->active_window], icc_profile_size, TRUE);
				}

				g_free(system_path);
				(void)fclose(fp);
			}
			else if(StringCompareIgnoreCase(extention, ".tif") == 0
				|| StringCompareIgnoreCase(extention, ".tiff") == 0)
			{
				char *system_path = g_locale_from_utf8(file_path, -1, NULL, NULL, NULL);
				FILE *fp = fopen(system_path, "rb");
				int resolution = 0;
				uint8 *icc_profile_data = NULL;
				int32 icc_profile_size = 0;

				ReadTiffTagData(system_path, &resolution, &icc_profile_data, &icc_profile_size);

				if(resolution != 0)
				{
					app->draw_window[app->active_window]->resolution = (int16)resolution;
				}

				if(icc_profile_data != NULL)
				{
					app->draw_window[app->active_window]->icc_profile_data = icc_profile_data;
					DrawWindowSetIccProfile(app->draw_window[app->active_window], icc_profile_size, TRUE);
				}

				g_free(system_path);
			}

			// �ꎞ��~���Ă����\���p�̃V�O�i�����Đڑ�
#if GTK_MAJOR_VERSION >= 3
			(void)g_signal_connect(G_OBJECT(app->draw_window[app->active_window]->window),
				"draw", G_CALLBACK(DisplayDrawWindow), app->draw_window[app->active_window]
			);
#else
			(void)g_signal_connect(G_OBJECT(app->draw_window[app->active_window]->window),
				"expose_event", G_CALLBACK(DisplayDrawWindow), app->draw_window[app->active_window]
			);
#endif

			// �`��̈�̃J�E���^���X�V
			app->window_num++;
		}
		else
		{
			(void)g_printerr("File Open Error : %s\n", error->message);
		}

		g_error_free(error);
	}

	// �E�C���h�E�^�C�g�����X�V
	if(app->window_num != last_window_num)
	{
		UpdateWindowTitle(app);
	}
}

/*********************************************************
* ExecuteOpenFile�֐�                                    *
* �t�@�C�����J��                                         *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void ExecuteOpenFile(APPLICATION* app)
{	// �t�@�C�����J���_�C�A���O
	GtkWidget *chooser = gtk_file_chooser_dialog_new(
		app->labels->menu.open,
		GTK_WINDOW(app->widgets->window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL
	);

	// �v���r���[���Z�b�g
	SetFileChooserPreview(chooser);

	if(gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT)
	{	// �t�@�C�����J���ꂽ
		gchar* file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
		
		OpenFile(file_path, app);

		g_free(file_path);
	}

	// �t�@�C���I���_�C�A���O�����
	gtk_widget_destroy(chooser);
}

/*********************************************************
* ExecuteOpenFileAsLayer�֐�                             *
* ���C���[�Ƃ��ăt�@�C�����J��                           *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void ExecuteOpenFileAsLayer(APPLICATION* app)
{	// �t�@�C�����J���_�C�A���O
	GtkWidget* chooser = gtk_file_chooser_dialog_new(
		app->labels->menu.open,
		GTK_WINDOW(app->widgets->window),
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL
	);

	// �v���r���[���Z�b�g
	SetFileChooserPreview(chooser);

	if(gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT)
	{	// �t�@�C�����J���ꂽ
		gchar *file_path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
		// �s�N�Z���o�b�t�@�ɕϊ�
		GdkPixbuf *pixbuf;
		// ���C���[��ǉ�����L�����o�X
		DRAW_WINDOW *window = GetActiveDrawWindow(app);
		// �ǉ����郌�C���[
		LAYER *layer;
		// �t�@�C�����擾�p
		gchar *str;
		// �t�@�C����
		gchar *file_name;
		// ���C���[�̖��O
		char layer_name[4096];
		// for���p�̃J�E���^
		int i;

		pixbuf = gdk_pixbuf_new_from_file(file_path, NULL);

		// �ϊ�����
		if(pixbuf != NULL)
		{	// �摜�̕��A�����A�s�N�Z���̃f�[�^���擾
			int32 width = gdk_pixbuf_get_width(pixbuf),
				height = gdk_pixbuf_get_height(pixbuf);
			uint8* pixels;
			int stride, original_stride;
			int copy_height, copy_stride;
			const int channel = 4;

			// �t�@�C���p�X����t�@�C�������������o��
			file_name = file_path;
			str = file_path;
			while(*str != '\0')
			{
				if(*str == '/' || *str == '\\')
				{
					file_name = str+1;
				}

				str = g_utf8_next_char(str);
			}

			// ���`�����l�����Ȃ���Βǉ�
			if(gdk_pixbuf_get_has_alpha(pixbuf) == FALSE)
			{
				pixbuf = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
			}

			// �s�N�Z���Ɖ摜��񕪂̃o�C�g�����擾
			pixels = gdk_pixbuf_get_pixels(pixbuf);
			original_stride = stride = gdk_pixbuf_get_rowstride(pixbuf);

			// ���C���[�̖��O���쐬
			i = 1;
			(void)strcpy(layer_name, file_name);
			while(CorrectLayerName(app->draw_window[app->active_window]->layer, layer_name) == 0)
			{
				(void)sprintf(layer_name, "%s (%d)", file_name, i);
			}

			// �A�N�e�B�u�ȕ`��̈�Ƀ��C���[��ǉ�
			layer = CreateLayer(0, 0, app->draw_window[app->active_window]->width,
				window->height, 4, TYPE_NORMAL_LAYER,
				window->active_layer,
				window->active_layer->next,
				layer_name,
				window
			);
			window->num_layer++;

#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			{
				uint8 r;
				for(i=0; i<width*height; i++)
				{
					r = pixels[i*4];
					pixels[i*4] = pixels[i*4+2];
					pixels[i*4+2] = r;
				}
			}
#endif

			// �摜�f�[�^��ǉ��������C���[�ɃR�s�[
			if(width > layer->width)
			{
				width = layer->width;
				copy_stride = stride = layer->width * 4;
			}
			else if(width < layer->width)
			{
				copy_stride = width * 4;
			}

			if(height > layer->height)
			{
				copy_height = height = layer->height;
			}
			else if(height < layer->height)
			{
				copy_height = height;
			}

			for(i=0; i<copy_height; i++)
			{
				(void)memcpy(&layer->pixels[layer->stride*i],
						&pixels[i*original_stride], copy_stride
				);
			}

			// �����f�[�^��ǉ�
			AddNewLayerWithImageHistory(layer, layer->pixels, layer->width, layer->height,
				layer->stride, 4, app->labels->menu.open_as_layer);

			// �ǉ��������C���[���A�N�e�B�u��
			LayerViewAddLayer(layer, window->layer,
				app->layer_window.view, window->num_layer);
			ChangeActiveLayer(window, layer);
			LayerViewSetActiveLayer(
				window->active_layer, app->layer_window.view
			);

			// �s�N�Z���o�b�t�@�͕s�v
			g_object_unref(pixbuf);
		}

		g_free(file_path);
	}

	// �t�@�C���I���_�C�A���O�����
	gtk_widget_destroy(chooser);
}

/*********************************************************
* ExecuteSave�֐�                                        *
* �㏑���ۑ�                                             *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void ExecuteSave(APPLICATION* app)
{
	// �t�@�C���p�X���A�N�e�B�u�ȕ`��̈�ɐݒ肳��Ă�
		// ���邢�͏����o���T�|�[�g�O�̊g���q�Ȃ疼�O��t���ĕۑ�
	// �g���q
	char* extention;
	// ���O��t���ĕۑ�����t���O
	int save_as = 0;
	
	if(app->draw_window[app->active_window]->file_path == NULL)
	{
		save_as++;
	}
	else
	{
		// ��Ɨp�|�C���^
		char* str;

		extention = str = app->draw_window[app->active_window]->file_path;
		while(*str != '\0')
		{
			if(*str == '.')
			{
				extention = str+1;
			}

			str = g_utf8_next_char(str);
		}

		if(IsSupportFileType(extention) == 0)
		{
			save_as++;
		}
	}

	// ���O��t���ĕۑ�
	if(save_as != 0)
	{
		ExecuteSaveAs(app);
	}
	else
	{	// �㏑���ۑ�
		Save(app, app->draw_window[app->active_window],
			app->draw_window[app->active_window]->file_path, extention);
	}
}

/*********************************************************
* ExecuteSaveAs�֐�                                      *
* ���O��t���ĕۑ�                                       *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void ExecuteSaveAs(APPLICATION* app)
{	// �t�@�C���I���_�C�A���O
	GtkWidget *chooser;
	// �t�@�C���^�C�v�̃t�B���^�[
	GtkFileFilter *filter;

	if(app->window_num < 1)
	{
		return;
	}

	chooser = gtk_file_chooser_dialog_new(
		app->labels->menu.save_as,
		GTK_WINDOW(app->widgets->window),
		GTK_FILE_CHOOSER_ACTION_SAVE,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
		NULL
	);

	// �㏑������O�Ɍx�����o��
	gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(chooser), TRUE);
	// �t�@�C���^�C�v�̃t�B���^�[���Z�b�g
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Original Format");
	gtk_file_filter_add_pattern(filter, "*.kab");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Portable Network Graphic (PNG)");
	gtk_file_filter_add_pattern(filter, "*.png");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Joint Photographic Experts Group (JPEG)");
	gtk_file_filter_add_pattern(filter, "*.jpg");
	gtk_file_filter_add_pattern(filter, "*.jpeg");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "PhotoShop Document (PSD)");
	gtk_file_filter_add_pattern(filter, "*.psd");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "Tagged Image File Format (TIFF)");
	gtk_file_filter_add_pattern(filter, "*.tif");
	gtk_file_filter_add_pattern(filter, "*.tiff");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);
	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, "TLG");
	gtk_file_filter_add_pattern(filter, "*.tlg");
	gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(chooser), filter);

	// �v���r���[��ݒ�
	SetFileChooserPreview(chooser);

	if(app->draw_window[app->active_window]->file_path != NULL)
	{
		char *directly = g_path_get_dirname(app->draw_window[app->active_window]->file_path);
		(void)gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(chooser), directly);
		gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(chooser),
			app->draw_window[app->active_window]->file_name);
		g_free(directly);
	}

	if(gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT)
	{	// �ۑ��t�@�C���������肳�ꂽ
			// �t�@�C���p�X
		const gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
		// �t�@�C���t�B���^�̏��
		const gchar *filter_name;
		// �t�@�C����
		const char *file_name;
		// �t�@�C���`��
		const char *file_type;
		// ��Ɨp�|�C���^
		const char *str;

		// �I������Ă���t�@�C���̃t�B���^�[���擾
		filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(chooser));
		filter_name = gtk_file_filter_get_name(filter);
		if(strcmp(filter_name, "Original Format") == 0)
		{	// �Ǝ��`��
			file_type = "kab";
		}
		else if(strcmp(filter_name, "Portable Network Graphic (PNG)") == 0)
		{	// PNG
			file_type = "png";
		}
		else if(strcmp(filter_name, "Joint Photographic Experts Group (JPEG)") == 0)
		{	// JPEG
			file_type = "jpg";
		}
		else if(strcmp(filter_name, "PhotoShop Document (PSD)") == 0)
		{	// PSD
			file_type = "psd";
		}
		else if(strcmp(filter_name, "Tagged Image File Format (TIFF)") == 0)
		{	// TIFF
			file_type = "tif";
		}
		else if(strcmp(filter_name, "TLG") == 0)
		{	// TLG
			file_type = "tlg";
		}

		// �����o�����s
		str = path;
		path = Save(app, app->draw_window[app->active_window], path, file_type);
		g_free((char*)str);

		// �t�@�C�������擾
		str = path;
		while(*str != '\0')
		{
			if(*str == '/' || *str == '\\')
			{
				file_name = str+1;
			}

			str = g_utf8_next_char(str);
		}

		// �t�@�C�����A�t�@�C���p�X�����X�V
		MEM_FREE_FUNC(app->draw_window[app->active_window]->file_name);
		MEM_FREE_FUNC(app->draw_window[app->active_window]->file_path);

		app->draw_window[app->active_window]->file_name = MEM_STRDUP_FUNC(file_name);
		app->draw_window[app->active_window]->file_path = (char*)path;

		// �E�B���h�E�̃^�C�g�����X�V
		UpdateWindowTitle(app);
	}

	gtk_widget_destroy(chooser);
}

/*********************************************************
* ExecuteClose�֐�                                       *
* �A�N�e�B�u�ȕ`��̈�����                           *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void ExecuteClose(APPLICATION* app)
{
	// ����^�u��ID
	int close_page = app->active_window;

	if(OnCloseDrawWindow((void*)app->draw_window[app->active_window],
		app->active_window) == FALSE)
	{
		gtk_notebook_remove_page(GTK_NOTEBOOK(app->widgets->note_book), close_page);
	}
}

/*********************************************************
* DeleteActiveLayer�֐�                                  *
* ���݂̃A�N�e�B�u���C���[���폜����                     *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void DeleteActiveLayer(APPLICATION* app)
{
	// ���C���[�폜�����s����`��̈�
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// �A�N�e�B�u���C���[�̎��̃��C���[�A�폜���郌�C���[
	LAYER* next_active, *delete_layer = window->active_layer;
	// for���p�̃J�E���^
	int i;

	//AUTO_SAVE(app->draw_window[app->active_window]);
	// ���C���[��0�ɂȂ�Ȃ��悤�Ƀ`�F�b�N
	if(window->num_layer <= 1)
	{
		return;
	}

	// �폜�������c��
	AddDeleteLayerHistory(window, delete_layer);

	// �Ǐ��L�����o�X���[�h�Ȃ���������C���[�̖��O���o����
	if((window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		LAYER *parent_delete = SearchLayer(app->draw_window[app->active_window]->layer, delete_layer->name);
		if(parent_delete != NULL)
		{
			DeleteLayer(&parent_delete);
		}
		app->draw_window[app->active_window]->num_layer--;
	}

	if(window->layer == window->active_layer)
	{	// ��ԉ��̃��C���[���폜����ꍇ�`��̈�̈�ԉ��̃��C���[�����ւ�
			// �폜��̈�ԉ��̃��C���[���A�N�e�B�u�ɂ���
		next_active = window->layer->next;
		ChangeActiveLayer(window, next_active);
		DeleteLayer(&delete_layer);
		window->layer = next_active;
	}
	else
	{	// �A�N�e�B�u���C���[���폜���č폜���ꂽ���C���[�̉��̃��C���[���A�N�e�B�u��
		next_active = window->active_layer->prev;
		ChangeActiveLayer(window, next_active);
		DeleteLayer(&delete_layer);
	}
	window->num_layer--;

	// �A�N�e�B�u���C���[�̕\�����X�V
	LayerViewSetActiveLayer(next_active, app->layer_window.view);

	// ���C���[����݂̂ɂȂ�����E�B�W�F�b�g�̖���������
	if(window->num_layer <= 1)
	{
		for(i=0; i<app->menus.num_disable_if_single_layer; i++)
		{
			gtk_widget_set_sensitive(app->menus.disable_if_single_layer[i], FALSE);
		}
	}
}

/***************************************
* FILL_FORE_GROUND_COLOR_HISTORY�\���� *
* �`��F�œh��ׂ������f�[�^           *
***************************************/
typedef struct _FILL_FORE_GROUND_COLOR_HISTORY
{
	uint8 color[3];		// �h��ׂ��F
	size_t data_size;	// �s�N�Z���f�[�^�̃T�C�Y
	int16 name_length;	// �h��ׂ����C���[�̖��O�̒���
	char *layer_name;	// �h��ׂ����C���[�̖��O
	uint8 *pixel_data;	// �h��ׂ��O�̃s�N�Z���f�[�^
} FILL_FORE_GROUND_COLOR_HISTORY;

void FillForeGroundColorUndo(DRAW_WINDOW* window, void* p)
{
	FILL_FORE_GROUND_COLOR_HISTORY* history = (FILL_FORE_GROUND_COLOR_HISTORY*)p;
	MEMORY_STREAM stream;
	LAYER *target = window->layer;
	int32 width, height, stride;
	uint8 *buff = (uint8*)p;
	uint8 *image = &buff[offsetof(FILL_FORE_GROUND_COLOR_HISTORY, layer_name) + history->name_length+1];
	uint8 *pixels;

	while(strcmp(target->name, (const char*)&buff[offsetof(FILL_FORE_GROUND_COLOR_HISTORY, layer_name)]) != 0)
	{
		target = target->next;
	}

	stream.buff_ptr = image;
	stream.data_size = history->data_size;
	stream.data_point = 0;

	pixels = ReadPNGStream(&stream, (stream_func_t)MemRead, &width, &height, &stride);
	(void)memcpy(target->pixels, pixels, target->stride*target->height);

	MEM_FREE_FUNC(pixels);
}

void FillForeGroundColorRedo(DRAW_WINDOW* window, void* p)
{
	FILL_FORE_GROUND_COLOR_HISTORY* history = (FILL_FORE_GROUND_COLOR_HISTORY*)p;
	LAYER *target = window->layer;
	uint8 *buff = (uint8*)p;

	while(strcmp(target->name, (const char*)&buff[offsetof(FILL_FORE_GROUND_COLOR_HISTORY, layer_name)]) != 0)
	{
		target = target->next;
	}

	FillLayerColor(target, history->color);
}

void AddFillForeGourndColorHistory(LAYER* target, uint8 color[3])
{
	MEMORY_STREAM_PTR image = CreateMemoryStream(target->stride*target->height);
	MEMORY_STREAM_PTR data = CreateMemoryStream(target->stride*target->height);
	FILL_FORE_GROUND_COLOR_HISTORY history;

	// �s�N�Z���f�[�^��PNG���k
	WritePNGStream(image, (stream_func_t)MemWrite, NULL, target->pixels,
		target->width, target->height, target->stride, 4, 0, target->window->app->preference.compress);

	(void)memcpy(history.color, color, 3);
	history.data_size = image->data_point;
	history.name_length = (int16)strlen(target->name)+1;
	(void)MemWrite(&history, 1, offsetof(FILL_FORE_GROUND_COLOR_HISTORY, layer_name), data);
	(void)MemWrite(target->name, 1, history.name_length+1, data);

	(void)MemWrite(image->buff_ptr, 1, image->data_point, data);

	AddHistory(&target->window->history, target->window->app->labels->menu.fill_layer_fg_color,
		data->buff_ptr, (uint32)data->data_point, FillForeGroundColorUndo, FillForeGroundColorRedo);

	(void)DeleteMemoryStream(image);
	(void)DeleteMemoryStream(data);
}

/*********************************************************
* FillForeGroundColor�֐�                                *
* �`��F�ŃA�N�e�B�u���C���[��h��Ԃ�                 *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void FillForeGroundColor(APPLICATION* app)
{
	if(app->draw_window[app->active_window]->active_layer->layer_type != TYPE_NORMAL_LAYER)
	{
		return;
	}

	AddFillForeGourndColorHistory(app->draw_window[app->active_window]->active_layer,
		app->tool_window.color_chooser->rgb);

	FillLayerColor(
		app->draw_window[app->active_window]->active_layer,
		app->tool_window.color_chooser->rgb
	);
}

typedef struct _FILL_PATTERN_HISTORY_DATA
{
	size_t before_data_size;	// �h��ׂ��O�̃s�N�Z���f�[�^�̃T�C�Y
	size_t after_data_size;		// �h��ׂ���s�N�Z���f�[�^�̃T�C�Y
	int16 name_length;			// �h��ׂ����C���[�̖��O�̒���
	char *layer_name;			// �h��ׂ����C���[�̖��O
	uint8 *pixel_data;			// �h��ׂ��O�̃s�N�Z���f�[�^
} FILL_PATTERN_HISTORY_DATA;

void FillPatternUndo(DRAW_WINDOW* window, void* p)
{
	FILL_PATTERN_HISTORY_DATA* history = (FILL_PATTERN_HISTORY_DATA*)p;
	MEMORY_STREAM stream;
	LAYER *target = window->layer;
	int32 width, height, stride;
	uint8 *buff = (uint8*)p;
	uint8 *image = &buff[offsetof(FILL_PATTERN_HISTORY_DATA, layer_name) + history->name_length+1];
	uint8 *pixels;

	while(strcmp(target->name, (const char*)&buff[offsetof(FILL_PATTERN_HISTORY_DATA, layer_name)]) != 0)
	{
		target = target->next;
	}

	stream.buff_ptr = image;
	stream.data_size = history->before_data_size;
	stream.data_point = 0;

	pixels = ReadPNGStream(&stream, (stream_func_t)MemRead, &width, &height, &stride);
	(void)memcpy(target->pixels, pixels, target->stride*target->height);

	MEM_FREE_FUNC(pixels);
}

void FillPatternRedo(DRAW_WINDOW* window, void* p)
{
	FILL_PATTERN_HISTORY_DATA* history = (FILL_PATTERN_HISTORY_DATA*)p;
	MEMORY_STREAM stream;
	LAYER *target = window->layer;
	int32 width, height, stride;
	uint8 *buff = (uint8*)p;
	uint8 *image = &buff[offsetof(FILL_PATTERN_HISTORY_DATA, layer_name)
		+ history->name_length+1 + history->before_data_size];
	uint8 *pixels;

	while(strcmp(target->name, (const char*)&buff[offsetof(FILL_PATTERN_HISTORY_DATA, layer_name)]) != 0)
	{
		target = target->next;
	}

	stream.buff_ptr = image;
	stream.data_size = history->after_data_size;
	stream.data_point = 0;

	pixels = ReadPNGStream(&stream, (stream_func_t)MemRead, &width, &height, &stride);
	(void)memcpy(target->pixels, pixels, target->stride*target->height);

	MEM_FREE_FUNC(pixels);
}

void AddFillPatternHistory(LAYER* target, LAYER* after)
{
	MEMORY_STREAM_PTR before_image = CreateMemoryStream(target->stride*target->height);
	MEMORY_STREAM_PTR after_image = CreateMemoryStream(target->stride*target->height);
	MEMORY_STREAM_PTR data = CreateMemoryStream(target->stride*target->height);
	FILL_PATTERN_HISTORY_DATA history = {0};

	// �h��ׂ��O�̃s�N�Z���f�[�^��PNG���k
	WritePNGStream(before_image, (stream_func_t)MemWrite, NULL, target->pixels,
		target->width, target->height, target->stride, 4, 0, target->window->app->preference.compress);
	
	// �h��ׂ���̃s�N�Z���f�[�^��PNG���k
	WritePNGStream(after_image, (stream_func_t)MemWrite, NULL, after->pixels,
		target->width, target->height, target->stride, 4, 0, target->window->app->preference.compress);

	history.before_data_size = before_image->data_point;
	history.after_data_size = after_image->data_point;
	history.name_length = (int16)strlen(target->name)+1;
	(void)MemWrite(&history, 1, offsetof(FILL_PATTERN_HISTORY_DATA, layer_name), data);
	(void)MemWrite(target->name, 1, history.name_length+1, data);
	(void)MemWrite(before_image->buff_ptr, 1, before_image->data_point, data);
	(void)MemWrite(after_image->buff_ptr, 1, after_image->data_point, data);

	AddHistory(&target->window->history, target->window->app->labels->menu.fill_layer_fg_color,
		data->buff_ptr, (uint32)data->data_point, FillPatternUndo, FillPatternRedo);

	(void)DeleteMemoryStream(before_image);
	(void)DeleteMemoryStream(after_image);
	(void)DeleteMemoryStream(data);
}

/*********************************************************
* FillPattern�֐�                                        *
* �A�N�e�B�u�ȃp�^�[���ŃA�N�e�B�u�ȃ��C���[��h��ׂ�   *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void FillPattern(APPLICATION* app)
{
	DRAW_WINDOW *window = app->draw_window[app->active_window];

	if(window->active_layer->layer_type != TYPE_NORMAL_LAYER)
	{
		return;
	}

	(void)memcpy(window->work_layer->pixels, window->active_layer->pixels,
		window->pixel_buf_size);

	FillLayerPattern(
		window->work_layer,
		&app->patterns,
		app,
		app->tool_window.color_chooser->rgb
	);

	AddFillPatternHistory(window->active_layer, window->work_layer);

	(void)memcpy(window->active_layer->pixels, window->work_layer->pixels,
		window->pixel_buf_size);
	(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
}

/*********************************************************
* FlipImageHorizontally�֐�                              *
* �`��̈�����E���]����                                 *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void FlipImageHorizontally(APPLICATION* app)
{
	FlipDrawWindowHorizontally(app->draw_window[app->active_window]);

	AddHistory(&app->draw_window[app->active_window]->history, app->labels->menu.flip_canvas_horizontally,
		NULL, 0, (history_func)FlipDrawWindowHorizontally, (history_func)FlipDrawWindowHorizontally);
}

/*********************************************************
* FlipImageVertically�֐�                                *
* �`��̈���㉺���]����                                 *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void FlipImageVertically(APPLICATION* app)
{
	FlipDrawWindowVertically(app->draw_window[app->active_window]);

	AddHistory(&app->draw_window[app->active_window]->history, app->labels->menu.flip_canvas_vertically,
		NULL, 0, (history_func)FlipDrawWindowVertically, (history_func)FlipDrawWindowVertically);
}

/*****************************************************
* SwitchSecondBackColor�֐�                          *
* �w�i�F��2�߂̂��̂Ɠ���ւ���                    *
* ����                                               *
* menu	: ���j���[�E�B�W�F�b�g                       *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void SwitchSecondBackColor(GtkWidget* menu, APPLICATION* app)
{
	if((app->flags & APPLICATION_IN_SWITCH_DRAW_WINDOW) == 0)
	{
		if(gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu)) == FALSE)
		{
			(void)memset(app->draw_window[app->active_window]->back_ground,
				0xff, app->draw_window[app->active_window]->pixel_buf_size);
			app->draw_window[app->active_window]->flags &= ~(DRAW_WINDOW_SECOND_BG);
		}
		else
		{
			int i;

			for(i=0; i<app->draw_window[app->active_window]->width * app->draw_window[app->active_window]->height; i++)
			{
				app->draw_window[app->active_window]->back_ground[i*4+0] = app->draw_window[app->active_window]->second_back_ground[0];
				app->draw_window[app->active_window]->back_ground[i*4+1] = app->draw_window[app->active_window]->second_back_ground[1];
				app->draw_window[app->active_window]->back_ground[i*4+2] = app->draw_window[app->active_window]->second_back_ground[2];
				app->draw_window[app->active_window]->back_ground[i*4+3] = 0xff;
			}

			app->draw_window[app->active_window]->flags |= DRAW_WINDOW_SECOND_BG;
		}

		app->draw_window[app->active_window]->flags |= DRAW_WINDOW_UPDATE_ACTIVE_UNDER;
		gtk_widget_queue_draw(app->draw_window[app->active_window]->window);

		if(app->layer_window.change_bg_button != NULL)
		{
			app->flags |= APPLICATION_IN_SWITCH_DRAW_WINDOW;
			gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(app->layer_window.change_bg_button),
				gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(menu)));
			app->flags &= ~(APPLICATION_IN_SWITCH_DRAW_WINDOW);
		}
	}
}

/*****************************************************
* Change2ndBackColor�֐�                             *
* 2�߂̔w�i�F��ύX����                            *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void Change2ndBackColor(APPLICATION* app)
{
	DRAW_WINDOW *window = app->draw_window[app->active_window];
	GtkWidget *dialog = gtk_color_selection_dialog_new(app->labels->menu.change_2nd_bg_color);
	GtkWidget *selection = gtk_color_selection_dialog_get_color_selection(
		GTK_COLOR_SELECTION_DIALOG(dialog));
	GdkColor color;

	gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_MOUSE);

	color.red = (app->menu_data.second_bg_color[0] << 8) | app->menu_data.second_bg_color[0];
	color.green = (app->menu_data.second_bg_color[1] << 8) | app->menu_data.second_bg_color[1];
	color.blue = (app->menu_data.second_bg_color[2] << 8) | app->menu_data.second_bg_color[2];

	gtk_color_selection_set_current_color(GTK_COLOR_SELECTION(selection), &color);

	if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_OK)
	{
		gtk_color_selection_get_current_color(GTK_COLOR_SELECTION(selection), &color);
		app->menu_data.second_bg_color[0] = color.red / 256;
		app->menu_data.second_bg_color[1] = color.green / 256;
		app->menu_data.second_bg_color[2] = color.blue / 256;

#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
		window->second_back_ground[2] = color.red / 256;
		window->second_back_ground[1] = color.green / 256;
		window->second_back_ground[0] = color.blue / 256;
#else
		window->second_back_ground[0] = color.red / 256;
		window->second_back_ground[1] = color.green / 256;
		window->second_back_ground[2] = color.blue / 256;
#endif
	}

	gtk_widget_destroy(dialog);
}

/*********************************************************
* MergeDownActiveLayer�֐�                               *
* �A�N�e�B�u�ȃ��C���[�����̃��C���[�ƌ�������           *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void MergeDownActiveLayer(APPLICATION* app)
{
	// �A�N�e�B�u�ȕ`��̈�
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// ������ɃA�N�e�B�u�ɂȂ郌�C���[
	LAYER* next_active = window->active_layer->prev;

	// �Ǐ��L�����o�X���[�h�Ȃ�e�L�����o�X���ɏ���
	if((window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		DRAW_WINDOW *parent = window->focal_window;
		LAYER *parent_target = SearchLayer(parent->layer, window->active_layer->name);
		LAYER *parent_next_active = parent->active_layer;
		if(parent_target != NULL)
		{
			if(parent_next_active == parent_target)
			{
				parent_next_active = parent_target->prev;
			}
			if(parent_next_active == NULL)
			{
				parent_next_active = parent->layer;
			}
			// �����f�[�^�̍쐬
			AddLayerMergeDownHistory(
				parent,
				parent_target
			);

			// ���C���[���������s����
			LayerMergeDown(parent_target);

			// �A�N�e�B�u�ȃ��C���[��ύX����
			parent->active_layer = parent_next_active;
		}
	}

	// �����f�[�^�̍쐬
	AddLayerMergeDownHistory(
		window,
		window->active_layer
	);

	// ���C���[���������s����
	LayerMergeDown(window->active_layer);

	// �A�N�e�B�u���C���[��ύX����
	ChangeActiveLayer(window, next_active);
	LayerViewSetActiveLayer(next_active, app->layer_window.view);
}

/*********************************************************
* FlattenImage�֐�                                       *
* �摜�̓��������s                                       *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void FlattenImage(APPLICATION* app)
{
	AUTO_SAVE(app->draw_window[app->active_window]);

	MergeAllLayer(app->draw_window[app->active_window]);
}

/*********************************************************
* ActiveLayerAlpha2SelectionArea�֐�                     *
* �A�N�e�B�u���C���[�̕s����������I��͈͂ɂ���         *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void ActiveLayerAlpha2SelectionArea(APPLICATION* app)
{
	LayerAlpha2SelectionArea(GetActiveDrawWindow(app));
}

/*********************************************************
* ActiveLayerAlphaAddSelectionArea�֐�                   *
* �A�N�e�B�u���C���[�̕s����������I��͈͂ɉ�����       *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void ActiveLayerAlphaAddSelectionArea(APPLICATION* app)
{
	LayerAlphaAddSelectionArea(GetActiveDrawWindow(app));
}

/*********************************************************
* ExecuteCopyLayer�֐�                                   *
* ���C���[�̕��������s����                               *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void ExecuteCopyLayer(APPLICATION* app)
{
	// �A�N�e�B�u���C���[�̃R�s�[�쐬
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	LAYER *layer = CreateLayerCopy(window->active_layer);
	window->num_layer++;

	// �Ǐ��L�����o�X���[�h�Ȃ�߂鎞�ɔ����ăt���O�𗧂Ă�
	if((window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		DRAW_WINDOW *parent = app->draw_window[app->active_window];
		LAYER *parent_prev = SearchLayer(parent->layer, window->active_layer->name);
		LAYER *parent_next = (parent_prev == NULL) ? parent->layer : parent_prev->next;
		LAYER *parent_new = CreateLayer(0, 0, parent->width, parent->height, parent->channel,
			(eLAYER_TYPE)layer->layer_type, parent_prev, parent_next, layer->name, parent);
		parent->num_layer++;
		AddNewLayerHistory(parent_new, parent_new->layer_type);
	}

	LayerViewAddLayer(layer, window->layer,
		app->layer_window.view, window->num_layer);
	ChangeActiveLayer(window, layer);
	LayerViewSetActiveLayer(layer, app->layer_window.view);
}

/*********************************************************
* ExecuteVisible2Layer�֐�                               *
* ���������C���[�ɂ���                                 *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void ExecuteVisible2Layer(APPLICATION* app)
{
	// ���C���[��ǉ�����`��̈�
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	char layer_name[256];		// �쐬���郌�C���[�̖��O
	LAYER *visible, *new_layer;	// �����̃R�s�[�ƐV�K�쐬�̃��C���[
	int layer_id = 1;			// ���C���[�̖��O�ɕt�����鐔�l

	// ���C���[�̖��O���d�����Ȃ��悤�ɐݒ�
	do
	{
		(void)sprintf(layer_name, "%s%d", app->labels->menu.visible_copy, layer_id);
		layer_id++;
	} while(CorrectLayerName(window->layer, layer_name) == 0);

	// �����̍������s��
	visible = MixLayerForSave(window);
	// ���C���[�쐬
	new_layer = CreateLayer(visible->x, visible->y, visible->width,
		visible->height, visible->channel, TYPE_NORMAL_LAYER,
		window->active_layer, window->active_layer->next,
		layer_name, window
	);
	// ���C���[�̃s�N�Z���f�[�^�ɍ��������f�[�^���R�s�[
	(void)memcpy(new_layer->pixels, visible->pixels, visible->height*visible->stride);
	window->num_layer++;

	// �Ǐ��L�����o�X���[�h�Ȃ�e�L�����o�X�ł����C���[�쐬
	if((window->flags & DRAW_WINDOW_IS_FOCAL_WINDOW) != 0)
	{
		DRAW_WINDOW *parent = app->draw_window[app->active_window];
		LAYER *parent_prev = SearchLayer(parent->layer, window->active_layer->name);
		LAYER *parent_next = (parent_prev == NULL) ? parent->layer : parent_prev->next;
		LAYER *parent_new = CreateLayer(0, 0, parent->width, parent->height, parent->channel,
			(eLAYER_TYPE)new_layer->layer_type, parent_prev, parent_next, new_layer->name, parent);
		parent->num_layer++;
		AddNewLayerHistory(parent_new, parent_new->layer_type);
	}

	// ���C���[�E�B���h�E�ɒǉ����ăA�N�e�B�u��
	LayerViewAddLayer(new_layer, window->layer,
		app->layer_window.view, window->num_layer);

	// �����p�ɍ�������C���[���폜
	DeleteLayer(&visible);
}

/*********************************************************
* RasterizeActiveLayer�֐�                               *
* �A�N�e�B�u���C���[�����X�^���C�Y����                   *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void RasterizeActiveLayer(APPLICATION* app)
{
	RasterizeLayer(GetActiveDrawWindow(app)->active_layer);
}

/*********************************************************
* ExecuteSelectAll�֐�                                   *
* �S�đI�������s                                         *
* ����                                                   *
* app	: �A�v���P�[�V�����S�̂��Ǘ�����\���̂̃A�h���X *
*********************************************************/
void ExecuteSelectAll(APPLICATION* app)
{
	// �`��̈�̏��
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	// ���[�y���[�h�΍�
	DRAW_WINDOW *canvas = (window->focal_window == NULL) ? window : window->focal_window;

	// �ꎞ�ۑ����C���[�Ɍ��݂̑I��͈͂��ʂ�
	(void)memcpy(canvas->temp_layer->pixels,canvas->selection->pixels,
		 canvas->width * canvas->height);

	// �I��͈͂�255�Ŗ��߂�
	(void)memset(canvas->selection->pixels, 0xff,
		 canvas->width * canvas->height);

	// �I��͈͕ω��̗����f�[�^�쐬
	AddSelectionAreaChangeHistory(
		canvas, app->labels->menu.select_all, 0, 0, canvas->width, canvas->height);

	// �I��͈̗͂̈���X�V
	(void)UpdateSelectionArea(&canvas->selection_area, canvas->selection, canvas->temp_layer);

	canvas->flags |= DRAW_WINDOW_HAS_SELECTION_AREA;
}

/*****************************************************
* ExecuteChangeResolution�֐�                        *
* �L�����o�X�̉𑜓x�ύX�����s                       *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteChangeResolution(APPLICATION* app)
{
	// �摜�̕��ƍ��������߂�_�C�A���O�{�b�N�X���쐬
	GtkWidget *dialog =
		gtk_dialog_new_with_buttons(
			app->labels->menu.change_resolution,
			GTK_WINDOW(app->widgets->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			NULL
		);
	// �E�B�W�F�b�g�̔z�u�p
	GtkWidget *table, *spin1, *spin2, *label;
	GtkWidget *dpi;
	GtkAdjustment *adjust;
	// OK, �L�����Z���̌��ʂ��󂯂�
	gint ret;
	// �쐬����摜�̕��ƍ���
	int32 width, height;

	table = gtk_table_new(3, 3, FALSE);

	// �_�C�A���O�{�b�N�X�ɕ��̃��x���ƒl�ݒ�p�̃E�B�W�F�b�g��o�^
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table);
	label = gtk_label_new(app->labels->make_new.width);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->draw_window[app->active_window]->original_width,
		1, LONG_MAX, 1, 10, 0));
	spin1 = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), spin1, 1, 2, 0, 1);

	// �_�C�A���O�{�b�N�X�ɍ����̃��x���ƒl�ݒ�p�̃E�B�W�F�b�g��o�^
	label = gtk_label_new(app->labels->make_new.height);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->draw_window[app->active_window]->original_height,
		1, LONG_MAX, 1, 10, 0));
	spin2 = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), spin2, 1, 2, 1, 2);

	// �𑜓x�ݒ�p�̃E�B�W�F�b�g��o�^
	label = gtk_label_new(app->labels->unit.resolution);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 2, 3);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->draw_window[app->active_window]->resolution,
		1, 1200, 1, 1, 0));
	dpi = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), dpi, 1, 2, 2, 3);
	label = gtk_label_new("dpi");
	gtk_table_attach_defaults(GTK_TABLE(table), label, 2, 3, 2, 3);

	// �_�C�A���O�{�b�N�X��\��
	gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));

	// ���[�U�[���uOK�v�A�u�L�����Z���v���N���b�N�܂ő҂�
	ret = gtk_dialog_run(GTK_DIALOG(dialog));

	// �uOK�v�������ꂽ��
	if(ret == GTK_RESPONSE_ACCEPT)
	{	// ���͂��ꂽ���ƍ����̒l���擾
		width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin1));
		height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin2));

		// ���͒l���L��
		app->draw_window[app->active_window]->original_width = width;
		app->draw_window[app->active_window]->original_height = height;

		// 4�̔{���ɑ�����
		width += (4 - (width % 4)) % 4;
		height += (4 - (height % 4)) % 4;

		// ���݂̕��A�����Ɠ��͂��ꂽ�l���قȂ�ΕύX�����s
		if(width != app->draw_window[app->active_window]->width
			|| height != app->draw_window[app->active_window]->height)
		{
			// ��ɗ������Ƃ��Ă���
			AddChangeDrawWindowResolutionHistory(
				app->draw_window[app->active_window], width, height);

			// �𑜓x�ύX�̎��s
			ChangeDrawWindowResolution(app->draw_window[app->active_window],
				width, height);
		}

		app->draw_window[app->active_window]->resolution =
			(int16)gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(dpi));
	}

	// �_�C�A���O�{�b�N�X�����
	gtk_widget_destroy(dialog);
}

/*****************************************************
* ExecuteChangeCanvasSize�֐�                        *
* �L�����o�X�̃T�C�Y�ύX�����s                       *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteChangeCanvasSize(APPLICATION* app)
{
	// �摜�̕��ƍ��������߂�_�C�A���O�{�b�N�X���쐬
	GtkWidget *dialog =
		gtk_dialog_new_with_buttons(
			app->labels->menu.change_canvas_size,
			GTK_WINDOW(app->widgets->window),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_STOCK_OK,
			GTK_RESPONSE_ACCEPT,
			GTK_STOCK_CANCEL,
			GTK_RESPONSE_REJECT,
			NULL
		);
	// �E�B�W�F�b�g�̔z�u�p
	GtkWidget *table, *spin1, *spin2, *label;
	GtkAdjustment *adjust;
	// OK, �L�����Z���̌��ʂ��󂯂�
	gint ret;
	// �쐬����摜�̕��ƍ���
	int32 width, height;

	table = gtk_table_new(3, 3, FALSE);

	// �_�C�A���O�{�b�N�X�ɕ��̃��x���ƒl�ݒ�p�̃E�B�W�F�b�g��o�^
	gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), table);
	label = gtk_label_new(app->labels->make_new.width);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 0, 1);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->draw_window[app->active_window]->original_width,
		1, LONG_MAX, 1, 10, 100));
	spin1 = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), spin1, 1, 2, 0, 1);

	// �_�C�A���O�{�b�N�X�ɍ����̃��x���ƒl�ݒ�p�̃E�B�W�F�b�g��o�^
	label = gtk_label_new(app->labels->make_new.height);
	gtk_table_attach_defaults(GTK_TABLE(table), label, 0, 1, 1, 2);
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(app->draw_window[app->active_window]->original_height, 1, LONG_MAX, 1, 10, 100));
	spin2 = gtk_spin_button_new(adjust, 1, 0);
	gtk_table_attach_defaults(GTK_TABLE(table), spin2, 1, 2, 1, 2);

	// �_�C�A���O�{�b�N�X��\��
	gtk_widget_show_all(gtk_dialog_get_content_area(GTK_DIALOG(dialog)));

	// ���[�U�[���uOK�v�A�u�L�����Z���v���N���b�N�܂ő҂�
	ret = gtk_dialog_run(GTK_DIALOG(dialog));

	// �uOK�v�������ꂽ��
	if(ret == GTK_RESPONSE_ACCEPT)
	{	// ���͂��ꂽ���ƍ����̒l���擾
		width = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin1));
		height = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(spin2));

		// ���͒l���L��
		app->draw_window[app->active_window]->original_width = width;
		app->draw_window[app->active_window]->original_height = height;

		// 4�̔{���ɑ�����
		width += (4 - (width % 4)) % 4;
		height += (4 - (height % 4)) % 4;

		// ���݂̕��A�����Ɠ��͂��ꂽ�l���قȂ�ΕύX�����s
		if(width != app->draw_window[app->active_window]->width
			|| height != app->draw_window[app->active_window]->height)
		{
			// ��ɗ������Ƃ��Ă���
			AddChangeDrawWindowSizeHistory(
				app->draw_window[app->active_window], width, height);

			// �𑜓x�ύX�̎��s
			ChangeDrawWindowSize(app->draw_window[app->active_window],
				width, height);
		}
	}

	// �_�C�A���O�{�b�N�X�����
	gtk_widget_destroy(dialog);
}

/*****************************************************
* ExecuteChangeCanvasIccProfile�֐�                  *
* ����                                               *
* menu	: ���j���[�E�B�W�F�b�g                       *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void ExecuteChangeCanvasIccProfile(GtkWidget* menu, APPLICATION* app)
{
	DRAW_WINDOW *window = app->draw_window[app->active_window];
	cmsHPROFILE *monitor_profile;
	GtkWidget *dialog;
	gint response;
	
	gchar *profile_path = NULL;
	gint32 profile_size = 0;
	gpointer profile_data = NULL;
	cmsHPROFILE h_profile = NULL;
	cmsHTRANSFORM h_transform = NULL;

	if(!window)
	{
		return;
	}

	dialog = IccProfileChangerDialogNew(GTK_WINDOW(app->widgets->window), app->input_icc_path);
	response = gtk_dialog_run(GTK_DIALOG(dialog));
	profile_path = (gchar *)g_object_get_data(G_OBJECT(dialog), "file");
	gtk_widget_destroy(dialog);

	if(response == GTK_RESPONSE_OK)
	{
		if(profile_path != NULL)
		{
			FILE *fp;

			fp = fopen(profile_path, "rb");

			if(fp != NULL)
			{
				(void)fseek(fp, 0, SEEK_END);
				profile_size = (int32)ftell(fp);
				rewind(fp);

				profile_data = g_malloc(profile_size);
				(void)fread(profile_data, 1, profile_size, fp);

				(void)fclose(fp);

				h_profile = cmsOpenProfileFromMem(profile_data, profile_size);

				if(h_profile)
				{
					// TODO : �}���`���j�^�Ή�
					monitor_profile = GetPrimaryMonitorProfile();

					if(app->output_icc != NULL)
					{
						cmsBool bpc[] = {TRUE, TRUE, TRUE, TRUE};
						cmsHPROFILE h_profiles[] = {h_profile, app->output_icc, app->output_icc, monitor_profile};
						cmsUInt32Number intents[] = { INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC, INTENT_RELATIVE_COLORIMETRIC };
						cmsFloat64Number adaptation_states[] = {0, 0, 0, 0};

						h_transform = cmsCreateExtendedTransform(cmsGetProfileContextID(h_profiles[1]), 4, h_profiles,
							bpc, intents, adaptation_states, NULL, 0, TYPE_BGRA_8, TYPE_BGRA_8, 0);
					}
					else
					{
						h_transform = cmsCreateTransform(h_profile, TYPE_BGRA_8,
							monitor_profile, TYPE_BGRA_8, INTENT_RELATIVE_COLORIMETRIC, cmsFLAGS_BLACKPOINTCOMPENSATION);
					}

					cmsCloseProfile(monitor_profile);

					if(h_transform)
					{
						g_free(window->icc_profile_path);
						g_free(window->icc_profile_data);

						if(window->input_icc)
						{
							cmsCloseProfile(window->input_icc);
						}

						if(window->icc_transform)
						{
							cmsDeleteTransform(window->icc_transform);
						}

						window->icc_profile_path = profile_path;
						window->icc_profile_size = profile_size;
						window->icc_profile_data = profile_data;
						window->input_icc = h_profile;
						window->icc_transform = h_transform;

						// �i�f�B�X�v���C�t�B���^��؂�ւ��āj�\�����X�V
						{
							GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(app->menus.display_filter_menus[DISPLAY_FUNC_TYPE_ICC_PROFILE]);
							gtk_check_menu_item_set_active(item, FALSE);
							gtk_check_menu_item_set_active(item, TRUE);
						}

						UpdateWindowTitle(app);

						return;
					}
				}
			}
		}
		else
		{
			// �v���t�@�C���j��
			// TODO : Transform����������̏����Ɣ��̂��Ȃ�Ƃ��ł��Ȃ���
			g_free(window->icc_profile_path);
			g_free(window->icc_profile_data);

			if(window->input_icc)
			{
				cmsCloseProfile(window->input_icc);
			}

			if(window->icc_transform)
			{
				cmsDeleteTransform(window->icc_transform);
			}

			window->icc_profile_path = NULL;
			window->icc_profile_data = NULL;
			window->icc_profile_size = 0;
			window->input_icc = NULL;
			window->icc_transform = NULL;

			// �i�f�B�X�v���C�t�B���^��؂�ւ��āj�\�����X�V
			{
				GtkCheckMenuItem *item = GTK_CHECK_MENU_ITEM(app->menus.display_filter_menus[DISPLAY_FUNC_TYPE_ICC_PROFILE]);
				gtk_check_menu_item_set_active(item, FALSE);
				gtk_check_menu_item_set_active(item, TRUE);
			}

			UpdateWindowTitle(app);

			return;
		}
	}

	// �ȉ��ATransform�̐����܂Ŋ������Ȃ������ꍇ�̌�n��
	if(h_profile)
	{
		cmsCloseProfile(h_profile);
	}

	g_free(profile_data);
	g_free(profile_path);
}

/*****************************************************
* DisplayVersion�֐�                                 *
* �o�[�W��������\������                           *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*****************************************************/
void DisplayVersion(APPLICATION* app)
{
	// �o�[�W��������\������_�C�A���O
	GtkWidget *dialog =
		gtk_about_dialog_new();
	// �N�̕�����擾�p
	char *year;
	char *date = __DATE__;
	// ��ҏ��
	const char *authors[] = {"umi", NULL};
	// �A�C�R���\��
	GdkPixbuf *pixbuf;
	gchar *logo_file_path;
	// �\�����镶����
	char str[4096];

	gtk_window_set_title(GTK_WINDOW(dialog), app->labels->menu.version);
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(app->widgets->window));

	// ���쌠�\��
	year = &date[strlen(date)];
	while(year > date)
	{
		if(*year == ' ')
		{
			break;
		}
		year--;
	}
	(void)sprintf(str, "Copyright \xC2\xA9 %s", year);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(dialog), str);

	// ��ҕ\��
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(dialog), authors);

	// �E�F�u�T�C�g
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(dialog),
		"http://gameprogrammingbyumi.blogspot.jp/");

	// �E�F�u�T�C�g�̃��x��
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(dialog),
		"\xE8\xB6\xA3\xE5\x91\xB3\xE3\x83\xBB\xE3\x82\xB2\xE3\x83\xBC\xE3\x83\xA0\xE3\x83\x97\xE3\x83\xAD\xE3\x82\xB0\xE3\x83\xA9\xE3\x83\x9F\xE3\x83\xB3\xE3\x82\xB0\xE3\x81\xAE\xE3\x81\xBE\xE3\x81\xA3\xE3\x81\x9F\xE3\x82\x8A\xE3\x83\x96\xE3\x83\xAD\xE3\x82\xB0");

	// ���C�Z���X
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(dialog),
		"GNU General Public License v3\nhttp://www.gnu.org/licenses/gpl-3.0.html");

	// ���S
	logo_file_path = g_build_filename(app->current_path, "image/icon.png", NULL);
	pixbuf = gdk_pixbuf_new_from_file(logo_file_path, NULL);
	gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(dialog), pixbuf);
	g_object_unref(pixbuf);
	g_free(logo_file_path);

	// �o�[�W����
	(void)sprintf(str, "%d.%d.%d.%d",
		MAJOR_VERSION, MINOR_VERSION, RELEASE_VERSION, BUILD_VERSION);
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog), str);

	// �R�����g
	(void)sprintf(str, "Build : %s", __DATE__);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(dialog), str);

	gtk_widget_show_all(dialog);

	(void)gtk_dialog_run(GTK_DIALOG(dialog));

	gtk_widget_destroy(dialog);
}

static void TextureChange(GtkIconView* icon_view, APPLICATION* app)
{
	DRAW_WINDOW *window = GetActiveDrawWindow(app);
	GList *list = gtk_icon_view_get_selected_items(icon_view);
	GtkTreePath *path;
	gint *indices;

	if(list == NULL)
	{
		return;
	}

	path = (GtkTreePath*)list->data;
	indices = gtk_tree_path_get_indices(path);
	if(indices == NULL)
	{
		app->textures.active_texture = 0;
		gtk_label_set_text(GTK_LABEL(app->widgets->texture_label), app->labels->tool_box.no_texture);
	}
	else
	{
		app->textures.active_texture = indices[0];
		if(app->textures.active_texture == 0)
		{
			gtk_label_set_text(GTK_LABEL(app->widgets->texture_label), app->labels->tool_box.no_texture);
		}
		else
		{
			gtk_label_set_text(GTK_LABEL(app->widgets->texture_label),
				app->textures.texture[app->textures.active_texture-1].name);
		}
	}

	if(app->window_num > 0)
	{
		FillTextureLayer(window->texture, &app->textures);
	}

	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);
}

static void ChangeTextureStrength(GtkAdjustment* scale, APPLICATION* app)
{
	app->textures.strength = gtk_adjustment_get_value(scale);

	if(app->window_num > 0)
	{
		FillTextureLayer(GetActiveDrawWindow(app)->texture, &app->textures);
	}
}

static void ChangeTextureScale(GtkAdjustment* scale, APPLICATION* app)
{
	app->textures.scale = gtk_adjustment_get_value(scale);

	if(app->window_num > 0)
	{
		FillTextureLayer(GetActiveDrawWindow(app)->texture, &app->textures);
	}
}

static void ChangeTextureRotate(GtkAdjustment* scale, APPLICATION* app)
{
	app->textures.angle = gtk_adjustment_get_value(scale);

	if(app->window_num > 0)
	{
		FillTextureLayer(GetActiveDrawWindow(app)->texture, &app->textures);
	}
}

static void ClickedCloseButton(GtkWidget* button, GtkWidget* window)
{
	gtk_widget_destroy(window);
}

/*********************************************************
* CreateTextureChooser�֐�                               *
* �e�N�X�`����I������E�B�W�F�b�g���쐬����             *
* ����                                                   *
* texture	: �e�N�X�`�����Ǘ�����\���̂̃A�h���X       *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                 *
*	�e�N�X�`����I������E�B���h�E�E�B�W�F�b�g           *
*********************************************************/
GtkWidget* CreateTextureChooser(TEXTURES* textures, APPLICATION* app)
{
	GdkPixbuf *no_texture;
	GtkWidget *ret = gtk_window_new(GTK_WINDOW_POPUP);
	GtkWidget *vbox = gtk_vbox_new(FALSE, 0);
	GtkWidget *scroll;
	GtkWidget *icon_view;
	GtkWidget *scale;
	GtkWidget *button;
	GtkAdjustment *adjust;
	GtkListStore *list_store;
	GtkTreeIter iter, active;
	GtkTreePath *path;
	uint8 *pixels;
	int data_size;
	gint window_x, window_y;
	int i;

	// �e�N�X�`���I�𒆂͕`�悳���Ȃ�
	gtk_window_set_modal(GTK_WINDOW(ret), TRUE);

	// �X�N���[���E�B���h�E���쐬
	scroll = gtk_scrolled_window_new(NULL, NULL);
	// �X�N���[�����E�B���h�E�ɓo�^
	gtk_container_add(GTK_CONTAINER(ret), vbox);
	gtk_box_pack_start(GTK_BOX(vbox), scroll, FALSE, FALSE, 0);
	// �X�N���[���o�[�͕K�v�ɉ����ĕ\��
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll),
		GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

	// �A�C�R���r���[�ɕ\�����郊�X�g���쐬
	list_store = gtk_list_store_new(
		2, G_TYPE_STRING, GDK_TYPE_PIXBUF);
	// ���X�g�ɃA�C�R��(�e�N�X�`��)�ƃt�@�C������o�^
		// �ŏ��Ƀe�N�X�`��������o�^
	no_texture = gdk_pixbuf_new(GDK_COLORSPACE_RGB, FALSE,
		8, TEXTURE_ICON_SIZE, TEXTURE_ICON_SIZE);
	pixels = gdk_pixbuf_get_pixels(no_texture);
	data_size = gdk_pixbuf_get_rowstride(no_texture) * TEXTURE_ICON_SIZE;
	(void)memset(pixels, 0xff, data_size);
	gtk_list_store_append(list_store, &iter);
	active = iter;
	gtk_list_store_set(list_store, &iter,
		0, app->labels->tool_box.no_texture, 1, no_texture, -1);

	// ���������ɓǂݍ��񂾃e�N�X�`���̃A�C�R����o�^
	for(i=0; i<textures->num_texture; i++)
	{
		gtk_list_store_append(list_store, &iter);
		gtk_list_store_set(list_store, &iter,
			0, textures->texture[i].name,
			1, textures->texture[i].thumbnail,
			-1
		);
		if(i+1 == textures->active_texture)
		{
			active = iter;
		}
	}

	// �A�C�R���r���[���쐬
		// ��قǍ쐬�������X�g���g�p
	icon_view = gtk_icon_view_new_with_model(
		GTK_TREE_MODEL(list_store));
	// �A�C�R���r���[���X�N���[���E�B���h�E�ɓo�^
	gtk_container_add(GTK_CONTAINER(scroll), icon_view);

	// �A�C�R���r���[�̃A�C�R����ƃe�L�X�g����w��
	gtk_icon_view_set_text_column(GTK_ICON_VIEW(icon_view), 0);
	gtk_icon_view_set_pixbuf_column(GTK_ICON_VIEW(icon_view), 1);
	// ��x�Ɉ�̃A�C�R�������I���o���Ȃ��悤�ɂ���
	gtk_icon_view_set_selection_mode(GTK_ICON_VIEW(icon_view),
		GTK_SELECTION_SINGLE);

	// �|�b�v�A�b�v�E�B���h�E�̍��W�����C���E�B���h�E�̍����
	gtk_window_get_position(GTK_WINDOW(app->widgets->window),
		&window_x, &window_y);
	gtk_window_move(GTK_WINDOW(ret), window_x, window_y);

	// �A�N�e�B�u�ȃe�N�X�`����ݒ�
	path = gtk_tree_model_get_path(GTK_TREE_MODEL(list_store), &active);
	gtk_icon_view_select_path(GTK_ICON_VIEW(icon_view), path);
	// �A�C�R���I�����̃R�[���o�b�N�֐����Z�b�g
	(void)g_signal_connect(G_OBJECT(icon_view), "selection_changed",
		G_CALLBACK(TextureChange), app);

	// �e�N�X�`���̋�����ݒ肷��E�B�W�F�b�g���쐬
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(textures->strength, 0, 100, 1, 1, 0));
	scale = SpinScaleNew(adjust, app->labels->tool_box.texture_strength, 1);
	(void)g_signal_connect(G_OBJECT(adjust), "value_changed",
		G_CALLBACK(ChangeTextureStrength), app);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	// �e�N�X�`���̊g�嗦��ݒ肷��E�B�W�F�b�g���쐬
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(textures->scale, 0.1, 10, 0.1, 1, 0));
	scale = SpinScaleNew(adjust, app->labels->menu.zoom, 1);
	(void)g_signal_connect(G_OBJECT(adjust), "value_changed",
		G_CALLBACK(ChangeTextureScale), app);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	// �e�N�X�`���̉�]�p�x��ݒ肷��E�B�W�F�b�g���쐬
	adjust = GTK_ADJUSTMENT(gtk_adjustment_new(textures->angle, -180, 180, 1, 1, 0));
	scale = SpinScaleNew(adjust, app->labels->menu.rotate, 1);
	(void)g_signal_connect(G_OBJECT(adjust), "value_changed",
		G_CALLBACK(ChangeTextureRotate), app);
	gtk_box_pack_start(GTK_BOX(vbox), scale, FALSE, FALSE, 0);

	// �u����v�{�^�����쐬
	button = gtk_button_new_with_label(app->labels->menu.close);
	(void)g_signal_connect(G_OBJECT(button), "clicked",
		G_CALLBACK(ClickedCloseButton), ret);
	gtk_box_pack_start(GTK_BOX(vbox), button, FALSE, FALSE, 0);

	gtk_widget_set_size_request(scroll, 320, 320);

	return ret;
}

/*********************************************************
* Change2LoupeMode�֐�                                   *
* ���[�y���[�h�ֈڍs����                                 *
* ����                                                   *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
void Change2LoupeMode(APPLICATION* app)
{
	Change2FocalMode(app->draw_window[app->active_window]);
}

/*********************************************************
* ReturnFromLoupeMode�֐�                                *
* ���[�y���[�h����߂�                                   *
* ����                                                   *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
*********************************************************/
void ReturnFromLoupeMode(APPLICATION* app)
{
	ReturnFromFocalMode(app->draw_window[app->active_window]);
}

/**********************************************************
* MemoryAllocate�֐�                                      *
* KABURAGI / MIKADO�Ŏg�p���郁�����A���P�[�^�Ń������m�� *
* ����                                                    *
* size	: �m�ۂ���o�C�g��                                *
* �Ԃ�l                                                  *
*	�m�ۂ����������̃A�h���X                              *
**********************************************************/
void* MemoryAllocate(size_t size)
{
	return MEM_ALLOC_FUNC(size);
}

/******************************************************************
* MemoryReallocate�֐�                                            *
* KABURAGI / MIKADO�Ŏg�p���郁�����A���P�[�^�Ńo�b�t�@�T�C�Y�ύX *
* ����                                                            *
* address	: �T�C�Y�ύX����o�b�t�@                              *
* size		: �V�����o�b�t�@�T�C�Y                                *
* �Ԃ�l                                                          *
*	�T�C�Y�ύX��̃o�b�t�@�̃A�h���X                              *
******************************************************************/
void* MemoryReallocate(void* address, size_t size)
{
	return MEM_REALLOC_FUNC(address, size);
}

/**************************************************************
* MemoryFree�֐�                                              *
* KABURAGI / MIKADO�Ŋm�ۂ��ꂽ���������J������(�v���O�C���p) *
* ����                                                        *
* memory	: �J�����郁�����̃|�C���^                        *
**************************************************************/
void MemoryFree(void* memory)
{
	MEM_FREE_FUNC(memory);
}

/*********************************************************
* SetHas3DLayer�֐�                                      *
* 3D���f�����O�̗L��/������ݒ肷��                      *
* ����                                                   *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* enable	: �L��:TRUE	����:FALSE                       *
*********************************************************/
void SetHas3DLayer(APPLICATION* app, int enable)
{
	if(enable == FALSE)
	{
		app->flags &= ~(APPLICATION_HAS_3D_LAYER);
	}
	else
	{
		app->flags |= APPLICATION_HAS_3D_LAYER;
	}
}

/*****************************************************
* GetHas3DLayer�֐�                                  *
* 3D���f�����O�̗L��/������Ԃ�                      *
* ����                                               *
* app	: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                             *
*	3D���f�����O�L��:TRUE	����:FALSE               *
*****************************************************/
int GetHas3DLayer(APPLICATION* app)
{
#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
	if((app->flags & APPLICATION_HAS_3D_LAYER) != 0)
	{
		return TRUE;
	}
#endif
	return FALSE;
}

#ifdef _MSC_VER

#pragma comment(lib, "Msimg32.lib")

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
#   if _MSC_VER >= 1600 && _MSC_VER < 1900
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
#    pragma comment(lib, "libcairo-2.lib")
#    pragma comment(lib, "libcairo-gobject-2.lib")
#    pragma comment(lib, "libcairo-script-interpreter-2.lib")
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
#    pragma comment(lib, "libpixman-1-0.lib")
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

#ifdef __cplusplus
}
#endif
