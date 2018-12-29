#ifndef _INCLUDED_BRUSH_CORE_H_
#define _INCLUDED_BRUSH_CORE_H_

#include <gtk/gtk.h>
#include "layer.h"
#include "draw_window.h"
#include "types.h"
#include "configure.h"
#include "utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BRUSH_STEP (FLOAT_T)(0.0750)
#define MIN_BRUSH_STEP (FLOAT_T)(BRUSH_STEP * MINIMUM_PRESSURE)
#define BRUSH_UPDATE_MARGIN 7

typedef enum _eBRUSH_SHAPE
{
	BRUSH_SHAPE_CIRCLE,
	BRUSH_SHAPE_IMAGE
} eBRUSH_SHAPE;

typedef enum _eBRUSH_TYPE
{
	BRUSH_TYPE_PENCIL,
	BRUSH_TYPE_HARD_PEN,
	BRUSH_TYPE_AIR_BRUSH,
	BRUSH_TYPE_BLEND_BRUSH,
	BRUSH_TYPE_OLD_AIR_BRUSH,
	BRUSH_TYPE_WATER_COLOR_BRUSH,
	BRUSH_TYPE_PICKER_BRUSH,
	BRUSH_TYPE_ERASER,
	BRUSH_TYPE_BUCKET,
	BRUSH_TYPE_PATTERN_FILL,
	BRUSH_TYPE_BLUR,
	BRUSH_TYPE_SMUDGE,
	BRUSH_TYPE_MIX_BRUSH,
	BRUSH_TYPE_GRADATION,
	BRUSH_TYPE_TEXT,
	BRUSH_TYPE_STAMP_TOOL,
	BRUSH_TYPE_IMAGE_BRUSH,
	BRUSH_TYPE_BLEND_IMAGE_BRUSH,
	BRUSH_TYPE_PICKER_IMAGE_BRUSH,
	BRUSH_TYPE_SCRIPT_BRUSH,
	BRUSH_TYPE_CUSTOM_BRUSH,
	BRUSH_TYPE_PLUG_IN,
	NUM_BRUSH_TYPE
} eBRUSH_TYPE;

typedef void (*brush_core_func)(DRAW_WINDOW* window, FLOAT_T x, FLOAT_T y,
	FLOAT_T pressure, struct _BRUSH_CORE* core, void* state);

typedef void (*brush_update_func)(DRAW_WINDOW* window, FLOAT_T x, FLOAT_T y, void* data);

typedef struct _BRUSH_CORE
{
	struct _APPLICATION *app;
	FLOAT_T max_x, max_y, min_x, min_y;
	uint32 flags;
	size_t detail_data_size;

	cairo_surface_t *brush_surface;
	cairo_surface_t *temp_surface;
	cairo_pattern_t *brush_pattern;
	cairo_pattern_t *temp_pattern;
	cairo_t *temp_cairo;
	uint8 **brush_pattern_buff, **temp_pattern_buff;
	int stride;

	gchar *name;
	char *image_file_path;
	uint8 (*color)[3];
	uint8 (*back_color)[3];
	uint8 brush_type;
	gchar hot_key;

	void* brush_data;
	brush_core_func press_func, motion_func, release_func;
	void (*key_press_func)(DRAW_WINDOW* window, GdkEventKey* key, void* data);
	void (*draw_cursor)(DRAW_WINDOW* window, FLOAT_T x, FLOAT_T y, void* data);
	brush_update_func button_update, motion_update;
	GtkWidget* (*create_detail_ui)(struct _APPLICATION *app, struct _BRUSH_CORE* core);
	void (*color_change)(const uint8 color[3], void* data);
	void (*change_editting_selection)(void* data, int is_editting);
	GtkWidget *button;
} BRUSH_CORE;

typedef struct _BRUSH_UPDATE_INFO
{
	// �X�V���s���͈�
	FLOAT_T min_x, min_y, max_x, max_y;
	// �X�V�͈͂̍���̍��W
	int start_x, start_y;
	// �X�V�͈͂̕��A����
	FLOAT_T width, height;
	// �`������s���邩�ۂ�
	int update;
} BRUSH_UPDATE_INFO;

typedef struct _BRUSH_UPDATE_AREA
{
	// �X�V���s���͈�
	FLOAT_T min_x, min_y, max_x, max_y;
	// �������ς݃t���O
	int initialized;
} BRUSH_UPDATE_AREA;

typedef struct _BRUSH_CHAIN_ITEM
{
	POINTER_ARRAY *names;
} BRUSH_CHAIN_ITEM;

typedef struct _BRUSH_CHAIN
{
	int key;
	int current;
	int change_key;
	unsigned int timer_id;
	int bursh_button_stop;
	POINTER_ARRAY *chains;
} BRUSH_CHAIN;

EXTERN void ChangeBrush(
	BRUSH_CORE* core,
	void* brush_data,
	brush_core_func press_func,
	brush_core_func motion_func,
	brush_core_func release_func
);

EXTERN void BrushCorePointReset(BRUSH_CORE* core);

EXTERN void AddBrushHistory(
	BRUSH_CORE* core,
	LAYER* active
);

EXTERN void AddSelectionEditHistory(BRUSH_CORE* core, LAYER* selection);

/*****************************************************
* SetBrushBaseScale�֐�                              *
* �u���V�T�C�Y�̔{����ݒ肷��                       *
* ����                                               *
* widget	: �{���ݒ�p�̃R���{�{�b�N�X�E�B�W�F�b�g *
* index		: �{���̃C���f�b�N�X��ێ�����A�h���X   *
*****************************************************/
EXTERN void SetBrushBaseScale(GtkWidget* widget, int* index);

/***************************************
* BrushCoreSetCirclePattern�֐�        *
* �u���V�̉~�`�摜�p�^�[�����쐬       *
* ����                                 *
* core				: �u���V�̊�{��� *
* r					: ���a             *
* outline_hardness	: �֊s�̍d��       *
* blur				: �{�P��           *
* alpha				: �s�����x         *
* color				: �F               *
***************************************/
EXTERN void BrushCoreSetCirclePattern(
	BRUSH_CORE* core,
	FLOAT_T r,
	FLOAT_T outline_hardness,
	FLOAT_T blur,
	FLOAT_T alpha,
	const uint8 color[3]
);

/***********************************************
* BrushCoreSetGrayCirclePattern�֐�            *
* �u���V�̃O���[�X�P�[���~�`�摜�p�^�[�����쐬 *
* ����                                         *
* core				: �u���V�̊�{���         *
* r					: ���a                     *
* outline_hardness	: �֊s�̍d��               *
* blur				: �{�P��                   *
* alpha				: �s�����x                 *
***********************************************/
EXTERN void BrushCoreSetGrayCirclePattern(
	BRUSH_CORE* core,
	FLOAT_T r,
	FLOAT_T outline_hardness,
	FLOAT_T blur,
	FLOAT_T alpha
);

EXTERN void BrushCoreUndoRedo(DRAW_WINDOW* window, void* p);

/*****************************************************
* DrawCircleBrush�֐�                                *
* �u���V���}�X�N���C���[�ɕ`�悷��                   *
* ����                                               *
* window		: �L�����o�X                         *
* core			: �u���V�̊�{���                   *
* x				: �`��͈͂̍����X���W              *
* y				: �`��͈͂̍����Y���W              *
* width			: �`��͈͂̕�                       *
* height		: �`��͈͂̍���                     *
* mask			: ��ƃ��C���[�ɃR�s�[����ۂ̃}�X�N *
* zoom			: �g��E�k����                       *
* alpha			: �s�����x                           *
* blend_mode	: �������[�h                         *
*****************************************************/
EXTERN void DrawCircleBrush(
	DRAW_WINDOW* window,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T width,
	FLOAT_T height,
	uint8** mask,
	FLOAT_T zoom,
	FLOAT_T alpha,
	uint16 blend_mode
);

/*************************************************
* DrawCircleBrushWorkLayer�֐�                   *
* �u���V����ƃ��C���[�ɕ`�悷��                 *
* ����                                           *
* window	: �L�����o�X                         *
* core		: �u���V�̊�{���                   *
* x			: �`��͈͂̍����X���W              *
* y			: �`��͈͂̍����Y���W              *
* width		: �`��͈͂̕�                       *
* height	: �`��͈͂̍���                     *
* mask		: ��ƃ��C���[�ɃR�s�[����ۂ̃}�X�N *
* zoom		: �g��E�k����                       *
* alpha		: �s�����x                           *
*************************************************/
EXTERN void DrawCircleBrushWorkLayer(
	DRAW_WINDOW* window,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T width,
	FLOAT_T height,
	uint8** mask,
	FLOAT_T zoom,
	FLOAT_T alpha
);

/*************************************************
* DrawImageBrush�֐�                             *
* �摜�u���V���}�X�N���C���[�ɕ`�悷��           *
* ����                                           *
* window		: �L�����o�X�̏��               *
* core			: �u���V�̊�{���               *
* x				: �`��͈͂̒��SX���W            *
* y				: �`��͈͂̒��SY���W            *
* width			: �`��͈͂̕�                   *
* height		: �`��͈͂̍���                 *
* scale			: �`�悷��g�嗦                 *
* size			: �摜�̒��ӂ̒���               *
* angle			: �摜�̊p�x                     *
* image_width	: �摜�̕�                       *
* image_height	: �摜�̍���                     *
* mask			: �������̃}�X�N���󂯂�|�C���^ *
* alpha			: �Z�x                           *
* blend_mode	: �������[�h                     *
*************************************************/
EXTERN void DrawImageBrush(
	DRAW_WINDOW* window,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T width,
	FLOAT_T height,
	FLOAT_T scale,
	FLOAT_T size,
	FLOAT_T angle,
	FLOAT_T image_width,
	FLOAT_T image_height,
	uint8** mask,
	FLOAT_T alpha,
	uint16 blend_mode
);

/*************************************************
* DrawImageBrushWorkLayer�֐�                    *
* �摜�u���V����ƃ��C���[�ɕ`�悷��             *
* ����                                           *
* window		: �L�����o�X�̏��               *
* core			: �u���V�̊�{���               *
* x				: �`��͈͂̒��SX���W            *
* y				: �`��͈͂̒��SY���W            *
* width			: �`��͈͂̕�                   *
* height		: �`��͈͂̍���                 *
* scale			: �`�悷��g�嗦                 *
* size			: �摜�̒��ӂ̒���               *
* angle			: �摜�̊p�x                     *
* image_width	: �摜�̕�                       *
* image_height	: �摜�̍���                     *
* mask			: �������̃}�X�N���󂯂�|�C���^ *
* alpha			: �Z�x                           *
*************************************************/
EXTERN void DrawImageBrushWorkLayer(
	DRAW_WINDOW* window,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T width,
	FLOAT_T height,
	FLOAT_T scale,
	FLOAT_T size,
	FLOAT_T angle,
	FLOAT_T image_width,
	FLOAT_T image_height,
	uint8** mask,
	FLOAT_T alpha
);

/*************************************************
* AdaptNormalBrush�֐�                           *
* �ʏ�̃u���V�̕`�挋�ʂ���ƃ��C���[�ɔ��f���� *
* ����                                           *
* window		: �L�����o�X�̏��               *
* draw_pixel	: �`�挋�ʂ̓������s�N�Z���f�[�^ *
* width			: �`��͈͂̕�                   *
* height		: �`��͈͂̍���                 *
* start_x		: �`��͈͂̍����X���W          *
* start_y		: �`��͈͂̍����Y���W          *
* anti_alias	: �A���`�G�C���A�X���s�����ۂ�   *
*************************************************/
EXTERN void AdaptNormalBrush(
	DRAW_WINDOW* window,
	uint8* draw_pixel,
	int width,
	int height,
	int start_x,
	int start_y,
	int anti_alias
);

/*************************************************
* AdaptBlendBrush�֐�                            *
* �����u���V�̕`�挋�ʂ���ƃ��C���[�ɔ��f����   *
* ����                                           *
* window		: �L�����o�X�̏��               *
* draw_pixel	: �`�挋�ʂ̓������s�N�Z���f�[�^ *
* width			: �`��͈͂̕�                   *
* height		: �`��͈͂̍���                 *
* start_x		: �`��͈͂̍����X���W          *
* start_y		: �`��͈͂̍����Y���W          *
* anti_alias	: �A���`�G�C���A�X���s�����ۂ�   *
* blend_mode	: �������[�h                     *
*************************************************/
EXTERN void AdaptBlendBrush(
	DRAW_WINDOW* window,
	uint8* draw_pixel,
	int width,
	int height,
	int start_x,
	int start_y,
	int anti_alias,
	int blend_mode
);

/******************************************************
* UpdateBrushButtonPressDrawArea�֐�                  *
* �u���V�̃N���b�N�ɑ΂���X�V����͈͂�ݒ肷��      *
* ����                                                *
* window		: �L�����o�X�̏��                    *
* area			: �X�V�͈͂��L������A�h���X          *
* core			: �u���V�̊�{���                    *
* x				: �`��͈͂̒��S��X���W               *
* y				: �`��͈͂̒��S��Y���W               *
* size			: �u���V�̒��ӂ̒���                  *
* brush_area	: �u���V�X�g���[�N�I�����̍X�V�͈�    *
******************************************************/
EXTERN void UpdateBrushButtonPressDrawArea(
	DRAW_WINDOW* window,
	BRUSH_UPDATE_INFO* area,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T size,
	BRUSH_UPDATE_AREA* brush_area
);

/******************************************************
* UpdateBrushMotionDrawArea�֐�                       *
* �u���V�̃h���b�O�ɑ΂���X�V����͈͂�ݒ肷��      *
* ����                                                *
* window		: �L�����o�X�̏��                    *
* area			: �X�V�͈͂��L������A�h���X          *
* core			: �u���V�̊�{���                    *
* x				: �`��͈͂̒��S��X���W               *
* y				: �`��͈͂̒��S��Y���W               *
* before_x		: �O��`�掞�̃}�E�X��X���W           *
* before_y		: �O��`�掞�̃}�E�X��Y���W           *
* size			: �u���V�̒��ӂ̒���                  *
* brush_area	: �u���V�X�g���[�N�I�����̍X�V�͈�    *
******************************************************/
EXTERN void UpdateBrushMotionDrawArea(
	DRAW_WINDOW* window,
	BRUSH_UPDATE_INFO* area,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T before_x,
	FLOAT_T before_y,
	FLOAT_T size,
	BRUSH_UPDATE_AREA* brush_area
);

/*************************************
* UpdateBrushMotionDrawAreaSize�֐�  *
* �u���V�X�V�͈͂̃T�C�Y���X�V����   *
* ����                               *
* window	: �L�����o�X�̏��       *
* area	: �X�V�͈͂��L������A�h���X *
*************************************/
EXTERN void UpdateBrushMotionDrawAreaSize(
	DRAW_WINDOW* window,
	BRUSH_UPDATE_INFO* area
);

/***************************************************
* UpdateBrushScatterDrawArea�֐�                �@ *
* �u���V�̎U�z�ɑ΂���X�V����͈͂�ݒ肷��    �@ *
* ����                                             *
* window		: �L�����o�X�̏��                 *
* area			: �X�V�͈͂��L������A�h���X       *
* core			: �u���V�̊�{���                 *
* x				: �`��͈͂̒��S��X���W            *
* y				: �`��͈͂̒��S��Y���W            *
* size			: �u���V�̒��ӂ̒���               *
* brush_area	: �u���V�X�g���[�N�I�����̍X�V�͈� *
***************************************************/
EXTERN void UpdateBrushScatterDrawArea(
	DRAW_WINDOW* window,
	BRUSH_UPDATE_INFO* area,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T size,
	BRUSH_UPDATE_AREA* brush_area
);

/***************************************
* DefaultToolUpdate�֐�                *
* �f�t�H���g�̃c�[���A�b�v�f�[�g�̊֐� *
* ����                                 *
* window	: �A�N�e�B�u�ȕ`��̈�     *
* x			: �}�E�X�J�[�\����X���W    *
* y			: �}�E�X�J�[�\����Y���W    *
* dummy		: �_�~�[�|�C���^           *
***************************************/
EXTERN void DefaultToolUpdate(DRAW_WINDOW* window, FLOAT_T x, FLOAT_T y, void* dummy);

/***************************************************
* UpdateBrushPreviewWindow�֐�                     *
* �u���V�̃v���r���[�L�����o�X���X�V����           *
* ����                                             *
* window		: �u���V�̃v���r���[�L�����o�X     *
* core			: �v���r���[����u���V�̊�{���   *
* press_func	: �N���b�N���̃R�[���o�b�N�֐�     *
* motion_func	: �h���b�O���̃R�[���o�b�N�֐�     *
* release_func	: �h���b�O�I�����̃R�[���o�b�N�֐� *
***************************************************/
EXTERN void UpdateBrushPreviewWindow(
	DRAW_WINDOW* window,
	BRUSH_CORE* core,
	brush_core_func press_func,
	brush_core_func motion_func,
	brush_core_func release_func
);

/*********************************************************
* AdaptSmudge�֐�                                        *
* �w��c�[���̍�ƃ��C���[�Ƃ̍�������                   *
* ����                                                   *
* canvas		: �L�����o�X�̏��                       *
* start_x		: �`��͈͂̍����X���W                  *
* start_y		: �`��͈͂̍����Y���W                  *
* width			: �`��͈͂̕�                           *
* height		: �`��͈͂̍���                         *
* before_width	: �O��`�掞�̕`��͈͂̕�               *
* before_height	: �O��`�掞�̕`��͈͂̍���             *
* mask			: �u���V�̕`�挋�ʂ̓������s�N�Z���f�[�^ *
* extend		: �F���т̊���                           *
* initialized	: �������ς݂��ۂ�(�������ς�:0�ȊO)     *
*********************************************************/
EXTERN void AdaptSmudge(
	DRAW_WINDOW* canvas,
	int start_x,
	int start_y,
	int width,
	int height,
	int before_width,
	int before_height,
	uint8* mask,
	uint8 extend,
	int initialized
);

/*****************************************************
* AdaptPickerBrush�֐�                               *
*�X�|�C�g�u���V�̕`�挋�ʂ���ƃ��C���[�ɔ��f����    *
* ����                                               *
* window			: �L�����o�X�̏��               *
* draw_pixel		: �`�挋�ʂ̓������s�N�Z���f�[�^ *
* width				: �`��͈͂̕�                   *
* height			: �`��͈͂̍���                 *
* start_x			: �`��͈͂̍����X���W          *
* start_y			: �`��͈͂̍����Y���W          *
* anti_alias		: �A���`�G�C���A�X���s�����ۂ�   *
* pick_target		: �F���E���Ώ�                   *
* picker_mode		: �F�̎��W���[�h                 *
* blend_mode		: �u���V�̍������[�h             *
* change_hue		: �F���̕ω���                   *
* change_saturation	: �ʓx�̕ω���                   *
* change_brightness	: ���x�̕ω���                   *
*****************************************************/
EXTERN void AdaptPickerBrush(
	DRAW_WINDOW* window,
	uint8* draw_pixel,
	int width,
	int height,
	int start_x,
	int start_y,
	int anti_alias,
	int pick_target,
	int picker_mode,
	int blend_mode,
	int16 change_hue,
	int16 change_saturation,
	int16 change_brightness
);

/*********************************************************
* AdaptSmudgeScatter�֐�                                 *
* �w��c�[���̍�ƃ��C���[�Ƃ̍�������(�U�z�p)           *
* ����                                                   *
* canvas		: �L�����o�X�̏��                       *
* start_x		: �`��͈͂̍����X���W                  *
* start_y		: �`��͈͂̍����Y���W                  *
* width			: �`��͈͂̕�                           *
* height		: �`��͈͂̍���                         *
* before_width	: �O��`�掞�̕`��͈͂̕�               *
* before_height	: �O��`�掞�̕`��͈͂̍���             *
* mask			: �u���V�̕`�挋�ʂ̓������s�N�Z���f�[�^ *
* extend		: �F���т̊���                           *
* initialized	: �������ς݂��ۂ�(�������ς�:0�ȊO)     *
*********************************************************/
EXTERN void AdaptSmudgeScatter(
	DRAW_WINDOW* canvas,
	int start_x,
	int start_y,
	int width,
	int height,
	int before_width,
	int before_height,
	uint8* mask,
	uint8 extend,
	int initialized
);

/*********************************************************
* BlendWaterBrush�֐�                                    *
* ���ʃu���V�̍�ƃ��C���[�Ƃ̍�������                   *
* ����                                                   *
* canvas				: �L�����o�X�̏��               *
* core					: �u���V�̊�{���               *
* x						: �`�悷��X���W                  *
* y						: �`�悷��Y���W                  *
* before_x				: �O��`�掞��X���W              *
* before_y				: �O��`�掞��Y���W              *
* brush_size			: �u���V�̔��a                   *
* start_x				: �`��͈͂̍����X���W          *
* start_y				: �`��͈͂̍����Y���W          *
* width					: �`��͈͂̕�                   *
* height				: �`��͈͂̍���                 *
* work_pixel			: ��ƃ��C���[�̃s�N�Z���f�[�^   *
* mask					: �`�挋�ʂ̓������s�N�Z���f�[�^ *
* brush_alpha			: �u���V�̔Z�x                   *
* brush_before_color	: �O��`�掞�ɋL�������F         *
* mix					: ���F���銄��                   *
* extend				: �F�����΂�����                 *
*********************************************************/
EXTERN void BlendWaterBrush(
	DRAW_WINDOW* canvas,
	BRUSH_CORE* core,
	FLOAT_T x,
	FLOAT_T y,
	FLOAT_T before_x,
	FLOAT_T before_y,
	FLOAT_T brush_size,
	int start_x,
	int start_y,
	int width,
	int height,
	uint8* work_pixel,
	uint8* mask,
	FLOAT_T brush_alpha,
	uint8 brush_before_color[4],
	uint8 mix,
	uint8 extend
);

/*****************************************
* InitializeBrushChain�֐�               *
* �ȈՃu���V�؂�ւ��̃f�[�^������������ *
* ����                                   *
* chain	: �ȈՃu���V�؂�ւ��̃f�[�^     *
*****************************************/
void InitializeBrushChain(BRUSH_CHAIN* chain);

/********************************************
* InitializeBrushChainItem�֐�              *
* �ȈՃu���V�؂�ւ���1�Z�b�g�������������� *
* ����                                      *
* item	: �ȈՃu���V�؂�ւ���1�Z�b�g       *
********************************************/
void InitializeBrushChainItem(BRUSH_CHAIN_ITEM* item);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_BRUSH_CORE_H_
