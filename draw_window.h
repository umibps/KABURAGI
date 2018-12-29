#ifndef _INCLUDED_DRAW_WINDOW_H_
#define _INCLUDED_DRAW_WINDOW_H_

#include "configure.h"
#include <GL/glew.h>
#include <gtk/gtk.h>
#include "lcms/lcms2.h"
#include "layer.h"
#include "history.h"
#include "selection_area.h"
#include "memory_stream.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum _eDRAW_WINDOW_FLAGS
{
	DRAW_WINDOW_HAS_SELECTION_AREA = 0x01,
	DRAW_WINDOW_UPDATE_ACTIVE_UNDER = 0x02,
	DRAW_WINDOW_UPDATE_ACTIVE_OVER = 0x04,
	DRAW_WINDOW_DISPLAY_HORIZON_REVERSE = 0x08,
	DRAW_WINDOW_EDIT_SELECTION = 0x10,
	DRAW_WINDOW_UPDATE_PART = 0x20,
	DRAW_WINDOW_DRAWING_STRAIGHT = 0x40,
	DRAW_WINDOW_SECOND_BG = 0x80,
	DRAW_WINDOW_TOOL_CHANGING = 0x100,
	DRAW_WINDOW_EDITTING_3D_MODEL = 0x200,
	DRAW_WINDOW_IS_FOCAL_WINDOW = 0x400,
	DRAW_WINDOW_INITIALIZED = 0x800,
	DRAW_WINDOW_DISCONNECT_3D = 0x1000,
	DRAW_WINDOW_UPDATE_AREA_INITIALIZED = 0x2000,
	DRAW_WINDOW_IN_RASTERIZING_VECTOR_SCRIPT = 0x4000,
	DRAW_WINDOW_FIRST_DRAW = 0x8000
} eDRAW_WINDOW_FLAGS;

typedef struct _UPDATE_RECTANGLE
{
	FLOAT_T x, y;
	FLOAT_T width, height;
	cairo_t *cairo_p;
	cairo_surface_t *surface_p;
} UPDATE_RECTANGLE;

typedef struct _CALLBACK_IDS
{
	unsigned int display;
	unsigned int mouse_button_press;
	unsigned int mouse_move;
	unsigned int mouse_button_release;
	unsigned int mouse_wheel;
	unsigned int configure;
	unsigned int enter;
	unsigned int leave;
} CALLBACK_IDS;

typedef struct _DRAW_WINDOW
{
	uint8 channel;		// �`�����l����
	uint8 color_mode;	// �J���[���[�h(���݂�RGBA32�̂�)

	int16 degree;		// ��]�p(�x���@)

	char *file_name;	// �t�@�C����

	char *file_path;	// �t�@�C���p�X

	// ���Ԃōĕ`����ĂԊ֐���ID
	unsigned int timer_id;
	unsigned int auto_save_id;

	// �^�u�؂�ւ��O�̃��C���[�r���[�̈ʒu
	int layer_view_position;

	// �摜�̃I���W�i�����A����
	int original_width, original_height;
	// �`��̈�̕��A����(4�̔{��)
	int width, height;
	// ��s���̃o�C�g���A���C���[�ꖇ���̃o�C�g��
	int stride, pixel_buf_size;
	// ��]�p�Ƀ}�[�W�����Ƃ����`��̈�̍L���A��s���̃o�C�g��
	int disp_size, disp_stride;
	// ��]��̍��W�v�Z�p
	FLOAT_T half_size;
	FLOAT_T angle;		// ��]�p(�ʓx�@)
	// ��]�����p�̎O�p�֐��̒l
	FLOAT_T sin_value, cos_value;
	// ��]�����ł̈ړ���
	FLOAT_T trans_x, trans_y;
	// �J�[�\�����W
	FLOAT_T cursor_x, cursor_y;
	FLOAT_T before_cursor_x, before_cursor_y;
	// �O��`�F�b�N���̃J�[�\�����W
	FLOAT_T before_x, before_y;
	// �Ō�ɃN���b�N�܂��̓h���b�O���ꂽ���W
	FLOAT_T last_x, last_y;
	// �Ō�ɃN���b�N�܂��̓h���b�O���ꂽ���̕M��
	FLOAT_T last_pressure;
	// �J�[�\�����W�␳�p
	FLOAT_T add_cursor_x, add_cursor_y;
	// ���ϖ@��u���␳�ł̍��W�ϊ��p
	FLOAT_T rev_add_cursor_x, rev_add_cursor_y;
	// �\���p�̃p�^�[��
	cairo_pattern_t *rotate;
	// ��ʕ����X�V�p
	UPDATE_RECTANGLE update, temp_update;
	// �`��̈�X�N���[���̍��W
	int scroll_x, scroll_y;
	// ��ʍX�V���̃N���b�s���O�p
	int update_clip_area[4][2];

	// Windows��Linux�ŃC�x���g�̌Ă΂�鏇�����قȂ�悤�Ȃ̂ŉ��p
	GdkModifierType state;

	// �R�[���o�b�N�֐���ID���L��
	CALLBACK_IDS callbacks;

	uint8 *back_ground;		// �w�i�̃s�N�Z���f�[�^
	uint8 *brush_buffer;	// �u���V�p�̃o�b�t�@

	LAYER *layer;			// ��ԉ��̃��C���[
	// �\���p�A�G�t�F�N�g�p�A�u���V�J�[�\���\���p�̈ꎞ�ۑ�
	LAYER *disp_layer, *effect, *disp_temp, *scaled_mixed;
	// �A�N�e�B�u�ȃ��C���[&���C���[�Z�b�g�ւ̃|�C���^
		// �y�ѕ\�����C���[�������������C���[
	LAYER *active_layer, *active_layer_set, *mixed_layer;
	// ��Ɨp�A�ꎞ�ۑ��p�A�I��͈́A�A�N�e�B�u���C���[��艺�̃��C���[
	LAYER *work_layer, *temp_layer,
		*selection, *under_active;
	// �}�X�N�ƃ}�X�N�K�p�O�̈ꎞ�ۑ��p
	LAYER *mask, *mask_temp;
	// �e�N�X�`���p
	LAYER *texture;
	// �\���p�p�^�[��
	cairo_pattern_t *mixed_pattern;
	// ���`�����l���݂̂̃}�X�N�p�T�[�t�F�[�X
	cairo_surface_t *alpha_surface, *alpha_temp, *gray_mask_temp;
	// ���`�����l���݂̂̃C���[�W
	cairo_t *alpha_cairo, *alpha_temp_cairo, *gray_mask_cairo;

	// ���C���[�����p�̊֐��Q
	void (**layer_blend_functions)(LAYER* src, LAYER* dst);
	void (**part_layer_blend_functions)(LAYER* src, UPDATE_RECTANGLE* update);

	uint16 num_layer;		// ���C���[�̐�
	uint16 zoom;			// �g��E�k����
	FLOAT_T zoom_rate;		// ���������_�^�̊g��E�k����
	FLOAT_T rev_zoom;		// �g��E�k�����̋t��

	// �𑜓x
	int16 resolution;

	// �I��͈̓A�j���[�V�����p�̃^�C�}�[
	GTimer *timer;

	// �����ۑ��p�̃^�C�}�[
	GTimer *auto_save_timer;

	// �I��͈͕\���p�f�[�^
	SELECTION_AREA selection_area;

	// �����f�[�^
	HISTORY history;

	// �`��̈�̃E�B�W�F�b�g
	GtkWidget *window;

	// �`��̈�̃X�N���[��
	GtkWidget *scroll;

	// �\���X�V���̃t���O
	unsigned int flags;

	// �ό`�����p�̃f�[�^
	struct _TRANSFORM_DATA *transform;

	// 2�߂̔w�i�F
	uint8 second_back_ground[3];

	// �\���t�B���^�[�̏��
	uint8 display_filter_mode;

	// ICC�v���t�@�C���̃f�[�^
	char *icc_profile_path;
	void *icc_profile_data;
	int32 icc_profile_size;
	// ICC�v���t�@�C���̓��e
	cmsHPROFILE input_icc;
	// ICC�v���t�@�C���ɂ��F�ϊ��p
	cmsHTRANSFORM icc_transform;

	// �Ǐ��L�����o�X
	struct _DRAW_WINDOW *focal_window;
	// ���[�y���[�h�O�̃X�N���[���o�[�̈ʒu
	int16 focal_x, focal_y;

	// �A�v���P�[�V�����S�̊Ǘ��p�\���̂ւ̃|�C���^
	struct _APPLICATION* app;

	// 3D���f�����O�p�f�[�^
	void *first_project;
	// OpenGL�Ή��E�B�W�F�b�g
	GtkWidget *gl_area;
} DRAW_WINDOW;

// �֐��̃v���g�^�C�v�錾
/***************************************************************
* CreateDrawWindow�֐�                                         *
* �`��̈���쐬����                                           *
* ����                                                         *
* width		: �L�����o�X�̕�                                   *
* height	: �L�����o�X�̍���                                 *
* channel	: �L�����o�X�̃`�����l����(RGB:3, RGBA:4)          *
* name		: �L�����o�X�̖��O                                 *
* note_book	: �`��̈�^�u�E�B�W�F�b�g                         *
* window_id	: �`��̈�z�񒆂�ID                               *
* app		: �A�v���P�[�V�����̏����Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                       *
*	�`��̈�̏����Ǘ�����\���̂̃A�h���X                   *
***************************************************************/
EXTERN DRAW_WINDOW* CreateDrawWindow(
	int32 width,
	int32 height,
	uint8 channel,
	const gchar* name,
	GtkWidget* note_book,
	uint16 window_id,
	struct _APPLICATION* app
);

/***************************************************************
* CreateTempDrawWindow�֐�                                     *
* �ꎞ�I�ȕ`��̈���쐬����                                   *
* ����                                                         *
* width		: �L�����o�X�̕�                                   *
* height	: �L�����o�X�̍���                                 *
* channel	: �L�����o�X�̃`�����l����(RGB:3, RGBA:4)          *
* name		: �L�����o�X�̖��O                                 *
* note_book	: �`��̈�^�u�E�B�W�F�b�g                         *
* window_id	: �`��̈�z�񒆂�ID                               *
* app		: �A�v���P�[�V�����̏����Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                       *
*	�`��̈�̏����Ǘ�����\���̂̃A�h���X                   *
***************************************************************/
EXTERN DRAW_WINDOW* CreateTempDrawWindow(
	int32 width,
	int32 height,
	uint8 channel,
	const gchar* name,
	GtkWidget* note_book,
	uint16 window_id,
	struct _APPLICATION* app
);

/***************************************
* DeleteDrawWindow�֐�                 *
* �`��̈�̏����폜                 *
* ����                                 *
* window	: �`��̈�̏��̃A�h���X *
***************************************/
EXTERN void DeleteDrawWindow(DRAW_WINDOW** window);

/*********************************************
* TimerCallBack�֐�                          *
* ����(1/60�b)�ŌĂяo�����R�[���o�b�N�֐� *
* ����                                       *
* window	: �Ή�����`��̈�               *
* �Ԃ�l                                     *
*	���TRUE                                 *
*********************************************/
EXTERN gboolean TimerCallBack(DRAW_WINDOW* window);

/*******************************************************
* SetDrawWindowCallbacks�֐�                           *
* �`��̈�̃R�[���o�b�N�֐��̐ݒ���s��               *
* ����                                                 *
* widget	: �R�[���o�b�N�֐����Z�b�g����E�B�W�F�b�g *
* window	: �`��̈�̏��                           *
*******************************************************/
EXTERN void SetDrawWindowCallbacks(
	GtkWidget* widget,
	DRAW_WINDOW* window
);

/***************************************************
* DisconnectDrawWindowCallbacks�֐�                *
* �`��̈�̃R�[���o�b�N�֐����~�߂�               *
* ����                                             *
* widget	: �R�[���o�b�N�֐����~�߂�E�B�W�F�b�g *
* window	: �`��̈�̏��                       *
***************************************************/
EXTERN void DisconnectDrawWindowCallbacks(
	GtkWidget* widget,
	DRAW_WINDOW* window
);

/*****************************************************
* SwapDrawWindowFromMemoryStream�֐�                 *
* �������[��̕`��̈�f�[�^�Ɠ���ւ���             *
* ����                                               *
* window	: �`��̈�̏��                         *
* stream	: �������[��̕`��̈�̃f�[�^�X�g���[�� *
*****************************************************/
EXTERN void SwapDrawWindowFromMemoryStream(DRAW_WINDOW* window, MEMORY_STREAM_PTR stream);

/***********************************************
* OnCloseDrawWindow�֐�                        *
* �^�u��������Ƃ��̃R�[���o�b�N�֐�       *
* ����                                         *
* data	: �`��̈�̃f�[�^                     *
* page	: ����^�u��ID                       *
* �Ԃ�l                                       *
*	���鑀��̒��~:TRUE ���鑀�쑱�s:FALSE *
***********************************************/
EXTERN gboolean OnCloseDrawWindow(void* data, gint page);

/***********************************************************
* GetWindowID�֐�                                          *
* �`��̈��ID���擾����                                   *
* ����                                                     *
* window	: �`��̈�̏��                               *
* app		: 	�A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                   *
*	�`��̈��ID (�s���ȕ`��̈�Ȃ��-1)                  *
***********************************************************/
EXTERN int GetWindowID(DRAW_WINDOW* window, struct _APPLICATION* app);

/*********************************************************
* GetWindowTitle�֐�                                     *
* �`��̈�̏��Ɋ�Â��E�C���h�E�^�C�g�����擾����     *
* ����                                                   *
* window	: �`��̈�̏��                             *
* app		: �A�v���P�[�V�������Ǘ�����\���̂̃A�h���X *
* �Ԃ�l                                                 *
*	�E�C���h�E�^�C�g��                                   *
*********************************************************/
EXTERN gchar *GetWindowTitle(DRAW_WINDOW* window, struct _APPLICATION* app);

/*********************************
* DrawWindowChangeZoom�֐�       *
* �`��̈�̊g��k������ύX���� *
* ����                           *
* window	: �`��̈�̏��     *
* zoom		: �V�����g��k����   *
*********************************/
EXTERN void DrawWindowChangeZoom(
	DRAW_WINDOW* window,
	int16 zoom
);

/*****************************************
* FlipDrawWindowHorizontally�֐�         *
* �`��̈�𐅕����]����                 *
* ����                                   *
* window	: �������]����`��̈�̏�� *
*****************************************/
EXTERN void FlipDrawWindowHorizontally(DRAW_WINDOW* window);

/*****************************************
* FlipDrawWindowVertically�֐�           *
* �`��̈�𐂒����]����                 *
* ����                                   *
* window	: �������]����`��̈�̏�� *
*****************************************/
EXTERN void FlipDrawWindowVertically(DRAW_WINDOW* window);

/***************************************
* LayerAlpha2SelectionArea�֐�         *
* ���C���[�̕s����������I��͈͂ɂ��� *
* ����                                 *
* window	: �`��̈�̏��           *
***************************************/
EXTERN void LayerAlpha2SelectionArea(DRAW_WINDOW* window);

/*****************************************
* LayerAlphaAddSelectionArea�֐�         *
* ���C���[�̕s����������I��͈͂ɉ����� *
* ����                                   *
* window	: �`��̈�̏��             *
*****************************************/
EXTERN void LayerAlphaAddSelectionArea(DRAW_WINDOW* window);

/*****************************
* MergeAllLayer�֐�          *
* �S�Ẵ��C���[����������   *
* ����                       *
* window	: �`��̈�̏�� *
*****************************/
EXTERN void MergeAllLayer(DRAW_WINDOW* window);

/*******************************************
* ChangeDrawWindowResolution�֐�           *
* �𑜓x��ύX����                         *
* ����                                     *
* window		: �𑜓x��ύX����`��̈� *
* new_width		: �V������                 *
* new_height	: �V��������               *
*******************************************/
EXTERN void ChangeDrawWindowResolution(DRAW_WINDOW* window, int32 new_width, int32 new_height);

/*************************************************
* AddChangeDrawWindowResolutionHistory�֐�       *
* �𑜓x�ύX�̗����f�[�^��ǉ�����               *
* ����                                           *
* window		: �𑜓x��ύX����`��̈�̏�� *
* new_width		: �V������                       *
* new_height	: �V��������                     *
*************************************************/
EXTERN void AddChangeDrawWindowResolutionHistory(
	DRAW_WINDOW* window,
	int32 new_width,
	int32 new_height
);

/*******************************************
* ChangeDrawWindowSize�֐�                 *
* �`��̈�̃T�C�Y��ύX����               *
* ����                                     *
* window		: �𑜓x��ύX����`��̈� *
* new_width		: �V������                 *
* new_height	: �V��������               *
*******************************************/
EXTERN void ChangeDrawWindowSize(DRAW_WINDOW* window, int32 new_width, int32 new_height);

/*****************************************
* UpdateDrawWindowClippingArea           *
* ��ʍX�V���ɃN���b�s���O����̈���X�V *
* ����                                   *
* window	: �`��̈�̏��             *
*****************************************/
EXTERN void UpdateDrawWindowClippingArea(DRAW_WINDOW* window);

/*************************************************
* ClipUpdateArea�֐�                             *
* ��ʂ̃X�N���[���ɓ����Ă��镔���ŃN���b�s���O *
* ����                                           *
* window	: �`��̈�̏��                     *
* cairo_p	: Cairo���                          *
*************************************************/
EXTERN void ClipUpdateArea(DRAW_WINDOW* window, cairo_t* cairo_p);

/*******************************************************************
* DrawWindowSetIccProfile�֐�                                      *
* �L�����o�X��ICC�v���t�@�C�������蓖�Ă�                          *
* ����                                                             *
* window	: �`��̈�̏��(icc_profile_data�Ƀf�[�^���蓖�čς�) *
* data_size	: ICC�v���t�@�C���̃f�[�^�̃o�C�g��                    *
* ask_set	: �\�t�g�v���[�t�\����K�p���邩��q�˂邩�ۂ�         *
*******************************************************************/
EXTERN void DrawWindowSetIccProfile(DRAW_WINDOW* window, int32 data_size, gboolean ask_set);

/*************************************************
* AddChangeDrawWindowSizeHistory�֐�             *
* �L�����o�X�T�C�Y�ύX�̗����f�[�^��ǉ�����     *
* ����                                           *
* window		: �𑜓x��ύX����`��̈�̏�� *
* new_width		: �V������                       *
* new_height	: �V��������                     *
*************************************************/
EXTERN void AddChangeDrawWindowSizeHistory(
	DRAW_WINDOW* window,
	int32 new_width,
	int32 new_height
);

/*************************************************************
* AddLayer�֐�                                               *
* �`��̈�Ƀ��C���[��ǉ�����                               *
* ����                                                       *
* canvas		: ���C���[��ǉ�����`��̈�                 *
* layer_type	: �ǉ����郌�C���[�̃^�C�v(�ʏ�A�x�N�g����) *
* layer_name	: �ǉ����郌�C���[�̖��O                     *
* prev_layer	: �ǉ��������C���[�̉��ɂ��郌�C���[         *
* �Ԃ�l                                                     *
*	����:���C���[�\���̂̃A�h���X	���s:NULL                *
*************************************************************/
EXTERN LAYER* AddLayer(
	DRAW_WINDOW* window,
	eLAYER_TYPE layer_type,
	const char* layer_name,
	LAYER* prev_layer
);

EXTERN void RasterizeVectorLine(
	DRAW_WINDOW* window,
	VECTOR_LAYER* layer,
	VECTOR_LINE* line,
	VECTOR_LAYER_RECTANGLE* rect
);

EXTERN void RasterizeVectorLayer(
	DRAW_WINDOW* window,
	LAYER* target,
	VECTOR_LAYER* layer
);

EXTERN void RasterizeVectorSquare(
	DRAW_WINDOW* window,
	VECTOR_SQUARE* square,
	VECTOR_LAYER_RECTANGLE* rectangle
);

EXTERN void RasterizeVectorRhombus(
	DRAW_WINDOW* window,
	VECTOR_ECLIPSE* eclipse,
	VECTOR_LAYER_RECTANGLE* rectangle
);

EXTERN void RasterizeVectorEclipse(
	DRAW_WINDOW* window,
	VECTOR_ECLIPSE* eclipse,
	VECTOR_LAYER_RECTANGLE* rectangle
);

EXTERN void RasterizeVectorScript(
	DRAW_WINDOW* window,
	VECTOR_SCRIPT* script,
	VECTOR_LAYER_RECTANGLE* rectangle
);

/***********************************************
* ReadVectorLineData�֐�                       *
* �x�N�g�����C���[�̃f�[�^��ǂݍ���           *
* ����                                         *
* data		: �x�N�g�����C���[�̃f�[�^(���k��) *
* target	: �f�[�^��ǂݍ��ރ��C���[         *
* �Ԃ�l                                       *
*	�ǂݍ��񂾃o�C�g��                         *
***********************************************/
EXTERN uint32 ReadVectorLineData(uint8* data, LAYER* target);

/*********************************************************
* WriteVectorLineData�֐�                                *
* �x�N�g�����C���[�̃f�[�^�������o��                     *
* ����                                                   *
* target			: �f�[�^�������o�����C���[           *
* dst				: �����o����̃|�C���^               *
* write_func		: �����o���Ɏg���֐��|�C���^         *
* data_stream		: ���k�O�̃f�[�^���쐬����X�g���[�� *
* write_stream		: �����o���f�[�^���쐬����X�g���[�� *
* compress_level	: ZIP���k�̃��x��                    *
*********************************************************/
EXTERN void WriteVectorLineData(
	LAYER* target,
	void* dst,
	stream_func_t write_func,
	MEMORY_STREAM* data_stream,
	MEMORY_STREAM* write_stream,
	int compress_level
);

/***********************************************
* IsVectorLineInSelectionArea�֐�              *
* �����I��͈͓����ۂ��𔻒肷��               *
* ����                                         *
* window			: �`��̈�̏��           *
* selection_pixels	: �I��͈͂̃s�N�Z���f�[�^ *
* line				: ���肷���               *
* �Ԃ�l                                       *
*	�I��͈͓�:1	�I��͈͊O:0               *
***********************************************/
EXTERN int IsVectorLineInSelectionArea(
	DRAW_WINDOW* window,
	uint8* selection_pixels,
	VECTOR_LINE* line
);

/*******************************************
* AddControlPointHistory�֐�               *
* ����_�ǉ��̗������쐬����               *
* ����                                     *
* window	: �`��̈�̏��               *
* layer		: ����_��ǉ��������C���[     *
* line		: ����_��ǉ��������C��       *
* point		: �ǉ���������_�̃A�h���X     *
* tool_name	: ����_��ǉ������c�[���̖��O *
*******************************************/
EXTERN void AddControlPointHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_LINE* line,
	VECTOR_POINT* point,
	const char* tool_name
);

/***************************************************
* AddTopLineControlPointHistory�֐�                *
* �܂���c�[�����Ȑ��c�[���ł̐���_�ǉ�������ǉ� *
* ����                                             *
* window		: �`��̈�̏��                   *
* layer			: ����_��ǉ��������C���[         *
* line			: ����_��ǉ��������C��           *
* point			: �ǉ���������_                   *
* tool_name		: ����_��ǉ������c�[���̖��O     *
* add_line_flag	: ���C���ǉ��������Ƃ��̃t���O     *
***************************************************/
EXTERN void AddTopLineControlPointHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_LINE* line,
	VECTOR_POINT* point,
	const char* tool_name,
	uint8 add_line_flag
);

/***********************************************
* AddTopSquareHistory�֐�                      *
* �l�p�`�ǉ�������ǉ�                         *
* ����                                         *
* window		: �`��̈�̏��               *
* layer			: �l�p�`��ǉ��������C���[     *
* square		: �ǉ������l�p�`               *
* tool_name		: �l�p�`��ǉ������c�[���̖��O *
***********************************************/
EXTERN void AddTopSquareHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_SQUARE* square,
	const char* tool_name
);

/*********************************************
* AddTopEclipseHistory�֐�                   *
* �ȉ~�ǉ�������ǉ�                         *
* ����                                       *
* window		: �`��̈�̏��             *
* layer			: �ȉ~��ǉ��������C���[     *
* eclipse		: �ǉ������ȉ~               *
* tool_name		: �ȉ~��ǉ������c�[���̖��O *
*********************************************/
EXTERN void AddTopEclipseHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_ECLIPSE* eclipse,
	const char* tool_name
);

/*********************************************
* AddTopScriptHistory�֐�                    *
* �X�N���v�g�̂��x�N�g���ǉ�������ǉ�       *
* ����                                       *
* window		: �`��̈�̏��             *
* layer			: �ȉ~��ǉ��������C���[     *
* eclipse		: �ǉ������ȉ~               *
* tool_name		: �ȉ~��ǉ������c�[���̖��O *
*********************************************/
EXTERN void AddTopScriptHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_SCRIPT* script,
	const char* tool_name
);

/*************************************************
* AddDeleteVectorShapeHistory�֐�                *
* �l�p�`, �ȉ~�폜�̗�����ǉ�                   *
* ����                                           *
* window		: �`��̈�̏��                 *
* layer			: �x�N�g�����폜�������C���[     *
* delete_shape	: �폜�����x�N�g���f�[�^         *
* tool_name		: �x�N�g���f�[�^���폜�����c�[�� *
*************************************************/
EXTERN void AddDeleteVectorShapeHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_BASE_DATA* delete_shape,
	const char* tool_name
);

/*************************************************
* AddDeleteVectorScriptHistory�֐�               *
* �X�N���v�g�̃x�N�g���폜�̗�����ǉ�           *
* ����                                           *
* window		: �`��̈�̏��                 *
* layer			: �x�N�g�����폜�������C���[     *
* delete_script	: �폜�����x�N�g���f�[�^         *
* tool_name		: �x�N�g���f�[�^���폜�����c�[�� *
*************************************************/
EXTERN void AddDeleteVectorScriptHistory(
	DRAW_WINDOW* window,
	LAYER* layer,
	VECTOR_SCRIPT* delete_script,
	const char* tool_name
);

/***********************************************************
* AddDeleteLinesHistory�֐�                                *
* �����̐��𓯎��ɍ폜�����ۂ̗����f�[�^��ǉ�             *
* ����                                                     *
* window		: �`��̈�̏����Ǘ�����\���̂̃A�h���X *
* active_layer	: �A�N�e�B�u�ȃ��C���[                     *
* line_data		: �폜������̃f�[�^                       *
* line_indexes	: �폜������̈�ԉ����琔�����C���f�b�N�X *
* num_lines		: �폜������̐�                           *
* tool_name		: �폜�Ɏg�p�����c�[���̖��O               *
***********************************************************/
EXTERN void AddDeleteLinesHistory(
	DRAW_WINDOW* window,
	LAYER* active_layer,
	VECTOR_LINE* line_data,
	uint32* line_indexes,
	uint32 num_lines,
	const char* tool_name
);

/*******************************************
* DeleteLayerSet�֐�                       *
* ���C���[�Z�b�g�̍폜���s��               *
* ����                                     *
* layer_set	: �폜���郌�C���[�Z�b�g       *
* window	: ���C���[�Z�b�g�����`��̈� *
*******************************************/
EXTERN void DeleteLayerSet(LAYER* layer_set, DRAW_WINDOW* window);

/*************************************************
* MixLayerSet�֐�                                *
* ���C���[�Z�b�g��������                         *
* ����                                           *
* bottom	: ���C���[�Z�b�g�̈�ԉ��̃��C���[   *
* next		: ������Ɏ��ɍ������郌�C���[       *
* window	: �`��̈���Ǘ�����\���̂̃A�h���X *
*************************************************/
EXTERN void MixLayerSet(LAYER* bottom, LAYER** next, DRAW_WINDOW* window);

/***************************************************************
* MixLayerSetActiveOver�֐�                                    *
* ���C���[�Z�b�g���̃A�N�e�B�u���C���[�ȏ�̃��C���[���������� *
* ����                                                         *
* start		: �A�N�e�B�u���C���[                               *
* next		: ������̎��ɍ������郌�C���[                     *
* window	: �`��̈���Ǘ�����\���̂̃A�h���X               *
***************************************************************/
EXTERN void MixLayerSetActiveOver(LAYER* start, LAYER** next, DRAW_WINDOW* window);

EXTERN void RenderTextLayer(DRAW_WINDOW* window, struct _LAYER* target, TEXT_LAYER* layer);

EXTERN void DisplayTextLayerRange(DRAW_WINDOW* window, TEXT_LAYER* layer);

EXTERN void DisplayEditSelection(DRAW_WINDOW* window);

/*********************************************************
* SetLayerBlendFunctions�֐�                             *
* ���C���[�����Ɏg�p����֐��|�C���^�z��̒��g��ݒ肷�� *
* ����                                                   *
* layer_blend_functions	: ���g��ݒ肷��֐��|�C���^�z�� *
*********************************************************/
extern void SetLayerBlendFunctions(void (*layer_blend_functions[])(LAYER* src, LAYER* dst));

/*************************************************************
* SetPartLayerBlendFunctions�֐�                             *
* �u���V�g�p���̃��C���[�����֐��|�C���^�z��̒��g��ݒ肷�� *
* ����                                                       *
* layer_blend_functions	: ���g��ݒ肷��֐��|�C���^�z��     *
*************************************************************/
extern void SetPartLayerBlendFunctions(void (*layer_blend_functions[])(LAYER* src, UPDATE_RECTANGLE* update));

/********************************************************************
* SetLayerBlendOperators�֐�                                        *
* ���C���[�������[�h�ɑΉ�����CAIRO�O���t�B�b�N�̒萔�z���ݒ肷�� *
* ����                                                              *
* operators	: �ݒ���s���z��                                        *
********************************************************************/
extern void SetLayerBlendOperators(cairo_operator_t operators[]);

/*****************************************
* AutoSave�֐�                           *
* �o�b�N�A�b�v�Ƃ��ăt�@�C����ۑ�����   *
* ����                                   *
* window	: �o�b�N�A�b�v�����`��̈� *
*****************************************/
EXTERN void AutoSave(DRAW_WINDOW* window);

/***************************************************
* GetBlendedUnderLayer�֐�                         *
* �Ώۂ�艺�̃��C���[�������������C���[���擾���� *
* ����                                             *
* target			: �Ώۂ̃��C���[               *
* window			: �`��̈�̏��               *
* use_back_ground	: �w�i�F���g�p���邩�ǂ���     *
* �Ԃ�l                                           *
*	�����������C���[                               *
***************************************************/
EXTERN LAYER* GetBlendedUnderLayer(LAYER* target, DRAW_WINDOW* window, int use_back_ground);

EXTERN void DivideLinesUndo(DRAW_WINDOW* window, void* p);
EXTERN void DivideLinesRedo(DRAW_WINDOW* window, void* p);

EXTERN void ScrollSizeChangeEvent(GtkWidget* scroll, GdkRectangle* size, DRAW_WINDOW* window);

EXTERN gboolean DrawWindowConfigurEvent(GtkWidget* widget, GdkEventConfigure* event_info, DRAW_WINDOW* window);

EXTERN void UpdateDrawWindow(DRAW_WINDOW* window);

/*********************************
* Change2FocalMode�֐�           *
* �Ǐ��L�����o�X���[�h�Ɉڍs���� *
* ����                           *
* parent_window	: �e�L�����o�X   *
*********************************/
EXTERN void Change2FocalMode(DRAW_WINDOW* parent_window);

/*********************************
* ReturnFromFocalMode�֐�        *
* �Ǐ��L�����o�X���[�h����߂�   *
* ����                           *
* parent_window	: �e�L�����o�X   *
*********************************/
EXTERN void ReturnFromFocalMode(DRAW_WINDOW* parent_window);

/***********************************************************************
* SetTextLayerDrawBalloonFunctions�֐�                                 *
* �����o����`�悷��֐��̊֐��|�C���^�z��̒��g��ݒ�                 *
* ����                                                                 *
* draw_balloon_functions	: �����o����`�悷��֐��̊֐��|�C���^�z�� *
***********************************************************************/
EXTERN void SetTextLayerDrawBalloonFunctions(void (*draw_balloon_functions[])(TEXT_LAYER*, LAYER*, DRAW_WINDOW*)
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_DRAW_WINDOW_H_
