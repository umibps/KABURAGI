#ifndef _INCLUDED_LAYER_H_
#define _INCLUDED_LAYER_H_

#define MAX_LAYER_NAME_LENGTH 256

#include <gtk/gtk.h>
#include <cairo.h>
#include "configure.h"
#include "types.h"
#include "vector_layer.h"
#include "text_layer.h"
#include "layer_set.h"
#include "adjustment_layer.h"
#include "pattern.h"
#include "texture.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LAYER_CHAIN_BUFFER_SIZE 1024
#define MAX_LAYER_EXTRA_DATA_NUM 8

typedef enum _eLAYER_FLAGS
{
	LAYER_FLAG_INVISIBLE = 0x01,
	LAYER_MASKING_WITH_UNDER_LAYER = 0x02,
	LAYER_LOCK_OPACITY = 0x04,
	LAYER_CHAINED = 0x08,
	LAYER_SET_CLOSE = 0x10
} eLAYER_FLAGS;

typedef enum _eLAYER_TYPE
{
	TYPE_NORMAL_LAYER,
	TYPE_VECTOR_LAYER,
	TYPE_TEXT_LAYER,
	TYPE_LAYER_SET,
	TYPE_3D_LAYER,
	TYPE_ADJUSTMENT_LAYER,
	NUM_LAYER_TYPE
} eLAYER_TYPE;

typedef enum _eLAYER_BLEND_MODE
{
	LAYER_BLEND_NORMAL,
	LAYER_BLEND_ADD,
	LAYER_BLEND_MULTIPLY,
	LAYER_BLEND_SCREEN,
	LAYER_BLEND_OVERLAY,
	LAYER_BLEND_LIGHTEN,
	LAYER_BLEND_DARKEN,
	LAYER_BLEND_DODGE,
	LAYER_BLEND_BURN,
	LAYER_BLEND_HARD_LIGHT,
	LAYER_BLEND_SOFT_LIGHT,
	LAYER_BLEND_DIFFERENCE,
	LAYER_BLEND_EXCLUSION,
	LAYER_BLEND_HSL_HUE,
	LAYER_BLEND_HSL_SATURATION,
	LAYER_BLEND_HSL_COLOR,
	LAYER_BLEND_HSL_LUMINOSITY,
	LAYER_BLEND_BINALIZE,
	LAYER_BLEND_SLELECTABLE_NUM,
	LAYER_BLEND_COLOR_REVERSE = LAYER_BLEND_SLELECTABLE_NUM,
	LAYER_BLEND_GREATER,
	LAYER_BLEND_ALPHA_MINUS,
	LAYER_BLEND_SOURCE,
	LAYER_BLEND_ATOP,
	LAYER_BLEND_SOURCE_OVER,
	LAYER_BLEND_OVER,
	NUM_LAYER_BLEND_FUNCTIONS
} eLAYER_BLEND_MODE;

/***************************
* EXTRA_LAYER_DATA�\����   *
* ���C���[�̒ǉ������i�[ *
***************************/
typedef struct _EXTRA_LAYER_DATA
{
	char *name;
	size_t data_size;
	void *data;
} EXTRA_LAYER_DATA;

/***********************
* LAYER�\����          *
* ���C���[�̏����i�[ *
***********************/
typedef struct _LAYER
{
	uint8 *pixels;			// �s�N�Z���f�[�^
	char *name;				// ���C���[��
	uint16 layer_type;		// ���C���[�^�C�v(�m�[�}���A�x�N�g���A�e�L�X�g)
	uint16 layer_mode;		// ���C���[�̍������[�h
	int32 x, y;				// ���C���[������̍��W
	// ���C���[�̕��A�����A��s���̃o�C�g��
	int width, height, stride;
	unsigned int flags;		// ���C���[�\���E��\�����̃t���O
	int8 alpha;				// ���C���[�̕s�����x
	int8 channel;			// ���C���[�̃`�����l����
	uint8 num_extra_data;	// �ǉ����̐�

	// CAIRO�̏������݃T�[�t�F�C�X
	cairo_surface_t *surface_p;
	cairo_t *cairo_p;	// CAIRO�R���e�L�X�g

	// layer_data���p��
		// �x�N�g�����C���[�A�e�L�X�g���C���[�̏����i�[
	union
	{
		VECTOR_LAYER *vector_layer_p;
		TEXT_LAYER *text_layer_p;
		LAYER_SET *layer_set_p;
		ADJUSTMENT_LAYER *adjustment_layer_p;

		// 3D���f�����O�p�f�[�^
		void *project;
	} layer_data;

	// �O��̃��C���[�ւ̃|�C���^
	struct _LAYER *prev, *next;
	// ���C���[�Z�b�g�ւ̃|�C���^
	struct _LAYER *layer_set;
	// ���C���[�r���[�̃E�B�W�F�b�g
	struct _LAYER_WIDGET *widget;

	// �ǉ����
	EXTRA_LAYER_DATA extra_data[MAX_LAYER_EXTRA_DATA_NUM];

	// �t�@�C���ǂݍ��ݎ���3D���f���f�[�^
	void *modeling_data;
	size_t modeling_data_size;

	// �`��̈�ւ̃|�C���^
	struct _DRAW_WINDOW *window;
} LAYER;

/*************************************
* LAYER_BASE_DATA�\����              *
* ���C���[�̊�{���B�����o���Ɏg�p *
*************************************/
typedef struct _LAYER_BASE_DATA
{
	uint16 layer_type;	// ���C���[�^�C�v(�m�[�}���A�x�N�g���A�e�L�X�g)
	uint16 layer_mode;	// ���C���[�̍������[�h
	int32 x, y;			// ���C���[������̍��W
	// ���C���[�̕��A����
	int32 width, height;
	uint32 flags;		// ���C���[�\���E��\�����̃t���O
	int8 alpha;			// ���C���[�̕s�����x
	int8 channel;		// ���C���[�̃`�����l����
	int8 layer_set;		// ���C���[�Z�b�g���̃��C���[���ۂ�
} LAYER_BASE_DATA;

/***************************************
* LAYER_GROUP_TEMPLATE_NODE�\����      *
* �܂Ƃ߂ă��C���[�����ۂ̕����f�[�^ *
***************************************/
typedef struct _LAYER_GROUP_TEMPLATE_NODE
{
	char *name;									// �m�[�h�̖��O
	eLAYER_TYPE layer_type;						// �쐬���郌�C���[�̃^�C�v
	struct _LAYER_GROUP_TEMPLATE_NODE *next;	// ���̃m�[�h
} LAYER_GROUP_TEMPLATE_NODE;

/*******************************
* LAYER_GROUP_TEMPLATE�\����   *
* �܂Ƃ߂ă��C���[�����f�[�^ *
*******************************/
typedef struct _LAYER_GROUP_TEMPLATE
{
	char *group_name;					// ���C���[�ɕt�^���閼�O(�O��)
	LAYER_GROUP_TEMPLATE_NODE *names;	// �쐬���郌�C���[�̖��O(�㔼)
	uint8 *add_flags;					// ���C���[��ǉ����邩�̃t���O
	int num_layers;						// �쐬���郌�C���[�̐�
	int flags;							// �ݒ�t���O
} LAYER_GROUP_TEMPLATE;

/*************************************************************
* LAYER_GROUP_TEMPLATES�\����                                *
* �A�v���P�[�V�������Ǘ����郌�C���[���܂Ƃ߂č쐬����f�[�^ *
*************************************************************/
typedef struct _LAYER_GROUP_TEMPLATES
{
	LAYER_GROUP_TEMPLATE *templates;
	int num_templates;
} LAYER_GROUP_TEMPLATES;

/***************************************
* eLAYER_GROUP_TEMPLATE_FLAGS�񋓑�    *
* �܂Ƃ߂ă��C���[�����ۂ̐ݒ�t���O *
***************************************/
enum _eLAYER_GROUP_TEMPLATE_FLAGS
{
	LAYER_GROUP_TEMPLATE_FLAG_MAKE_LAYER_SET = 0x01,
	LAYER_GROUP_TEMPLATE_FLAG_ADD_UNDER_ACTIVE_LAYER = 0x02
} eLAYER_GROUP_TEMPLATE_FLAG;

/********************
* RGAB_DATA�\����   *
* 1�s�N�Z���̃f�[�^ *
********************/
typedef struct _RGBA_DATA
{
	uint8 r, g, b, a;
} RGBA_DATA;

// �֐��̃v���g�^�C�v�錾
/*****************************************************
* ���C���[�̃������m�ۂƏ�����                       *
* CreateLayer�֐�                                    *
* ����                                               *
* x				: ���C���[�����X���W                *
* y				: ���C���[�����Y���W                *
* width			: ���C���[�̕�                       *
* height		: ���C���[�̍���                     *
* channel		: ���C���[�̃`�����l����             *
* layer_type	: ���C���[�̃^�C�v                   *
* prev_layer	: �ǉ����郌�C���[�̑O�̃��C���[     *
* next_layer	: �ǉ����郌�C���[�̎��̃��C���[     *
* name			: �ǉ����郌�C���[�̖��O             *
* window		: �ǉ����郌�C���[���Ǘ�����`��̈� *
* �Ԃ�l                                             *
*	���������ꂽ�\���̂̃A�h���X                     *
*****************************************************/
EXTERN LAYER* CreateLayer(
	int32 x,
	int32 y,
	int32 width,
	int32 height,
	int8 channel,
	eLAYER_TYPE layer_type,
	LAYER* prev_layer,
	LAYER* next_layer,
	const char *name,
	struct _DRAW_WINDOW* window
);

/*****************************************************
* CreateDispTempLayer�֐�                            *
* �\���p�ꎞ�ۑ����C���[���쐬                       *
* ����                                               *
* x				: ���C���[�����X���W                *
* y				: ���C���[�����Y���W                *
* width			: ���C���[�̕�                       *
* height		: ���C���[�̍���                     *
* channel		: ���C���[�̃`�����l����             *
* layer_type	: ���C���[�̃^�C�v                   *
* window		: �ǉ����郌�C���[���Ǘ�����`��̈� *
* �Ԃ�l                                             *
*	���������ꂽ�\���̂̃A�h���X                     *
*****************************************************/
LAYER* CreateDispTempLayer(
	int32 x,
	int32 y,
	int32 width,
	int32 height,
	int8 channel,
	eLAYER_TYPE layer_type,
	struct _DRAW_WINDOW* window
);

/*********************************************
* DeleteLayer�֐�                            *
* ���C���[���폜����                         *
* ����                                       *
* layer	: �폜���郌�C���[�|�C���^�̃A�h���X *
*********************************************/
EXTERN void DeleteLayer(LAYER** layer);

/*********************************************************
* RemoveLayer�֐�                                        *
* ���C���[���폜����(�L�����o�X�ɓo�^����Ă�����̂̂�) *
* ����                                                   *
* target	: �폜���郌�C���[                           *
*********************************************************/
EXTERN void RemoveLayer(LAYER* target);

/*********************************************
* DeleteTempLayer�֐�                        *
* �ꎞ�I�ɍ쐬�������C���[���폜����         *
* ����                                       *
* layer	: �폜���郌�C���[�|�C���^�̃A�h���X *
*********************************************/
EXTERN void DeleteTempLayer(LAYER** layer);

/*********************************
* AddDeleteLayerHistory�֐�      *
* ���C���[�̍폜�����f�[�^���쐬 *
* ����                           *
* window	: �`��̈�̃f�[�^   *
* target	: �폜���郌�C���[   *
*********************************/
EXTERN void AddDeleteLayerHistory(
	struct _DRAW_WINDOW* window,
	LAYER* target
);

/***********************************************************
* ResizeLayerBuffer�֐�                                    *
* ���C���[�̃s�N�Z���f�[�^�̃T�C�Y��ύX����               *
* ����                                                     *
* target		: �s�N�Z���f�[�^�̃T�C�Y��ύX���郌�C���[ *
* new_width		: �V�������C���[�̕�                       *
* new_height	: �V�������C���[�̍���                     *
***********************************************************/
EXTERN void ResizeLayerBuffer(
	LAYER* target,
	int32 new_width,
	int32 new_height
);

/*******************************************
* ResizeLayer�֐�                          *
* ���C���[�̃T�C�Y��ύX����(�g��k���L)   *
* ����                                     *
* target		: �T�C�Y��ύX���郌�C���[ *
* new_width		: �V�������C���[�̕�       *
* new_height	: �V�������C���[�̍���     *
*******************************************/
EXTERN void ResizeLayer(
	LAYER* target,
	int32 new_width,
	int32 new_height
);

/*******************************************
* ChangeLayerSize�֐�                      *
* ���C���[�̃T�C�Y��ύX����(�g��k����)   *
* ����                                     *
* target		: �T�C�Y��ύX���郌�C���[ *
* new_width		: �V�������C���[�̕�       *
* new_height	: �V�������C���[�̍���     *
*******************************************/
EXTERN void ChangeLayerSize(
	LAYER* target,
	int32 new_width,
	int32 new_height
);

EXTERN int CorrectLayerName(const LAYER* bottom_layer, const char* name);

EXTERN void ChangeActiveLayer(struct _DRAW_WINDOW* window, LAYER* layer);

/*******************************
* LayerMergeDown�֐�           *
* ���̃��C���[�ƌ�������       *
* ����                         *
* target	: �������郌�C���[ *
*******************************/
EXTERN void LayerMergeDown(LAYER* target);

/*******************************************
* AddLayerMergeDownHistory�֐�             *
* ���̃��C���[�ƌ����̗����f�[�^��ǉ����� *
* ����                                     *
* window	: �`��̈�̏��               *
* target	: �������郌�C���[             *
*******************************************/
EXTERN void AddLayerMergeDownHistory(
	struct _DRAW_WINDOW* window,
	LAYER* target
);

EXTERN void ChangeLayerOrder(LAYER* change_layer, LAYER* new_prev, LAYER** bottom);

EXTERN LAYER* SearchLayer(LAYER* bottom_layer, const gchar* name);

EXTERN int32 GetLayerID(const LAYER* bottom, const LAYER* prev, uint16 num_layer);

/*********************************
* AddDeleteLayerHistory�֐�      *
* ���C���[�̍폜�����f�[�^���쐬 *
* ����                           *
* window	: �`��̈�̃f�[�^   *
* target	: �폜���郌�C���[   *
*********************************/
EXTERN void AddNewLayerHistory(
	const LAYER* new_layer,
	uint16 layer_type
);

/*********************************************
* AddNewLayerWithImageHistory�֐�            *
* �摜�f�[�^�t���ł̃��C���[�ǉ��̗���ǉ�   *
* ����                                       *
* new_layer		: �ǉ��������C���[           *
* pixels		: �摜�f�[�^�̃s�N�Z���f�[�^ *
* width			: �摜�f�[�^�̕�             *
* height		: �摜�f�[�^�̍���           *
* stride		: �摜�f�[�^��s���̃o�C�g�� *
* channel		: �摜�f�[�^�̃`�����l����   *
* history_name	: �����̖��O                 *
*********************************************/
EXTERN void AddNewLayerWithImageHistory(
	const LAYER* new_layer,
	uint8* pixels,
	int32 width,
	int32 height,
	int32 stride,
	uint8 channel,
	const char* history_name
);

/*************************************************
* AddChangeLayerOrderHistory�֐�                 *
* ���C���[�̏����ύX�̗����f�[�^��ǉ�           *
* ����                                           *
* change_layer	: �����ύX�������C���[           *
* before_prev	: �����ύX�O�̑O�̃��C���[       *
* after_prev	: �����ύX��̑O�̃��C���[       *
* before_parent	: �����ύX�O�̏������C���[�Z�b�g *
*************************************************/
EXTERN void AddChangeLayerOrderHistory(
	const LAYER* change_layer,
	const LAYER* before_prev,
	const LAYER* after_prev,
	const LAYER* before_parent
);

/***********************************************************
* AddChangeLyaerSetHistory�֐�                             *
* ���C���[�̏������C���[�Z�b�g�ύX�̗����f�[�^��ǉ�       *
* ����                                                     *
* change_layer	: �������C���[�Z�b�g��ύX���郌�C���[     *
* before_parent	: ���C���[�Z�b�g�ύX�O�̏������C���[�Z�b�g *
* after_parent	: ���C���[�Z�b�g�ύX��̏������C���[�Z�b�g *
***********************************************************/
EXTERN void AddChangeLayerSetHistory(
	const LAYER* change_layer,
	const LAYER* before_parent,
	const LAYER* after_parent
);

EXTERN void FillLayerColor(LAYER* target, uint8 color[3]);

EXTERN void FlipLayerHorizontally(LAYER* target, LAYER* temp);
EXTERN void FlipLayerVertically(LAYER* target, LAYER* temp);

/***************************************
* RasterizeLayer�֐�                   *
* ���C���[�����X�^���C�Y               *
* ����                                 *
* target	: ���X�^���C�Y���郌�C���[ *
***************************************/
EXTERN void RasterizeLayer(LAYER* target);

/*****************************
* CreateLayerCopy�֐�        *
* ���C���[�̃R�s�[���쐬���� *
* ����                       *
* src	: �R�s�[���̃��C���[ *
* �Ԃ�l                     *
*	�쐬�������C���[         *
*****************************/
EXTERN LAYER* CreateLayerCopy(LAYER* src);

/*************************************************
* GetLayerChain�֐�                              *
* �s�����߂��ꂽ���C���[�z����擾����           *
* ����                                           *
* window	: �`��̈�̏��                     *
* num_layer	: ���C���[�����i�[����ϐ��̃A�h���X *
* �Ԃ�l                                         *
*	���C���[�z��                                 *
*************************************************/
EXTERN LAYER** GetLayerChain(struct _DRAW_WINDOW* window, uint16* num_layer);

/***************************************************
* LayerSetShowChildren�֐�                         *
* ���C���[�Z�b�g�̎q���C���[��\������             *
* ����                                             *
* layer_set	: �q��\�����郌�C���[�Z�b�g           *
* prev		: �֐��I����̎��Ƀ`�F�b�N���郌�C���[ *
***************************************************/
EXTERN void LayerSetShowChildren(LAYER* layer_set, LAYER **prev);

/***************************************************
* LayerSetHideChildren�֐�                         *
* ���C���[�Z�b�g�̎q���C���[���\������           *
* ����                                             *
* layer_set	: �q��\�����郌�C���[�Z�b�g           *
* prev		: �֐��I����̎��Ƀ`�F�b�N���郌�C���[ *
***************************************************/
EXTERN void LayerSetHideChildren(LAYER* layer_set, LAYER **prev);

/****************************
* DeleteAdjustmentLayer�֐� *
* �������C���[���폜����    *
* ����                      *
* layer	: �������C���[      *
****************************/
EXTERN void DeleteAdjustmentLayer(LAYER* layer);

/***********************************************
* DeleteSelectAreaPixels�֐�                   *
* �I��͈͂̃s�N�Z���f�[�^���폜����           *
* ����                                         *
* window	: �`��̈�̏��                   *
* target	: �s�N�Z���f�[�^���폜���郌�C���[ *
* selection	: �I��͈͂��Ǘ����郌�C���[       *
***********************************************/
EXTERN void DeleteSelectAreaPixels(
	struct _DRAW_WINDOW* window,
	LAYER* target,
	LAYER* selection
);

/*******************************************************
* AddDeletePixelsHistory�֐�                           *
* �s�N�Z���f�[�^��������̗����f�[�^��ǉ�����         *
* ����                                                 *
* tool_name	: �s�N�Z���f�[�^�����Ɏg�p�����c�[���̖��O *
* window	: �`��̈�̏��                           *
* min_x		: �s�N�Z���f�[�^��X���W�ŏ��l              *
* min_y		: �s�N�Z���f�[�^��Y���W�ŏ��l              *
* max_x		: �s�N�Z���f�[�^��X���W�ő�l              *
* max_y		: �s�N�Z���f�[�^��Y���W�ő�l              *
*******************************************************/
EXTERN void AddDeletePixelsHistory(
	const char* tool_name,
	struct _DRAW_WINDOW* window,
	LAYER* target,
	int32 min_x,
	int32 min_y,
	int32 max_x,
	int32 max_y
);

/*****************************************************
* CalcLayerAverageRGBA�֐�                           *
* ���C���[�̕��σs�N�Z���f�[�^�l���v�Z               *
* ����                                               *
* target	: ���σs�N�Z���f�[�^�l���v�Z���郌�C���[ *
* �Ԃ�l                                             *
*	�Ώۃ��C���[�̕��σs�N�Z���f�[�^�l               *
*****************************************************/
EXTERN RGBA_DATA CalcLayerAverageRGBA(LAYER* target);

/*****************************************************
* CalcLayerAverageRGBAwithAlpha�֐�                  *
* ���l���l�����ă��C���[�̕��σs�N�Z���f�[�^�l���v�Z *
* ����                                               *
* target	: ���σs�N�Z���f�[�^�l���v�Z���郌�C���[ *
* �Ԃ�l                                             *
*	�Ώۃ��C���[�̕��σs�N�Z���f�[�^�l               *
*****************************************************/
EXTERN RGBA_DATA CalcLayerAverageRGBAwithAlpha(LAYER* target);

EXTERN void OnChangeTextCallBack(GtkTextBuffer* buffer, LAYER* layer);

EXTERN VECTOR_LINE_LAYER* CreateVectorLineLayer(
	struct _LAYER* work,
	VECTOR_LINE* line,
	VECTOR_LAYER_RECTANGLE* rect
);

/*****************************************************
* CreateLayerSetChildButton�֐�                      *
* ���C���[�Z�b�g�̎q���C���[�\���E��\���{�^�����쐬 *
* ����                                               *
* layer_set	: �{�^���쐬�Ώۂ̃��C���[�Z�b�g         *
* �Ԃ�l                                             *
*	�쐬�����{�^���E�B�W�F�b�g                       *
*****************************************************/
EXTERN GtkWidget* CreateLayerSetChildButton(LAYER* layer_set);

/*******************************************************
* FillTextureLayer�֐�                                 *
* �e�N�X�`�����C���[��I�����ꂽ�p�����[�^�[�œh��ׂ� *
* ����                                                 *
* layer		: �e�N�X�`�����C���[                       *
* textures	: �e�N�X�`���̃p�����[�^�[                 *
*******************************************************/
EXTERN void FillTextureLayer(LAYER* layer, TEXTURES* textures);

/*********************************************
* GetLayerTypeString�֐�                     *
* ���C���[�̃^�C�v�萔�𕶎���ɂ���         *
* ����                                       *
* type	: ���C���[�̃^�C�v                   *
* �Ԃ�l                                     *
*	���C���[�̃^�C�v�̕�����(free���Ȃ�����) *
*********************************************/
EXTERN const char* GetLayerTypeString(eLAYER_TYPE type);

/*********************************************
* GetLayerTypeFromString�֐�                 *
* �����񂩂烌�C���[�̃^�C�v�̒萔���擾���� *
* ����                                       *
* str	: ���C���[�̃^�C�v�̕�����           *
* �Ԃ�l                                     *
*	���C���[�̃^�C�v�̒萔                   *
*********************************************/
EXTERN eLAYER_TYPE GetLayerTypeFromString(const char* str);

/******************************************
* GetAverageColor�֐�                     *
* �w����W���ӂ̕��ϐF���擾              *
* ����                                    *
* target	: �F���擾���郌�C���[        *
* x			: X���W                       *
* y			: Y���W                       *
* size		: �F���擾����͈�            *
* color		: �擾�����F���i�[(4�o�C�g��) *
******************************************/
EXTERN void GetAverageColor(LAYER* target, int x, int y, int size, uint8 color[4]);

/********************************************************************************
* CreateAdjutmentLayer�֐�                                                      *
* �������C���[�̃������m�ۂƏ�����                                              *
* ����                                                                          *
* type			: �������C���[�̃��[�h                                          *
* target		: �������C���[���K�p���鑊��(�����̃��C���[/���̃��C���[�̓���) *
* target_layer	: �������C���[��K�p���郌�C���[                                *
* self			: �������C���[���g�̃A�h���X                                    *
* �Ԃ�l                                                                        *
*	���������ꂽ�\���̂̃A�h���X                                                *
********************************************************************************/
EXTERN ADJUSTMENT_LAYER* CreateAdjustmentLayer(
	eADJUSTMENT_LAYER_TYPE type,
	eADJUSTMENT_LAYER_TARGET target,
	LAYER* target_layer,
	LAYER* self
);

/*************************************************
* SetAdjustmentLayerTargetLayer�֐�              *
* �������C���[��K�p���郌�C���[��ݒ�           *
* layer			: �������C���[                   *
* target_layer	: �������C���[��K�p���郌�C���[ *
*************************************************/
EXTERN void SetAdjustmentLayerTargetLayer(ADJUSTMENT_LAYER* layer, LAYER* target_layer);

/***************************************************
* AddLayerGroupTemplate�֐�                        *
* ���C���[���܂Ƃ߂č쐬����                       *
* ����                                             *
* group			: ���C���[���܂Ƃ߂č쐬����f�[�^ *
* add_flags		: ���C���[��ǉ����邩�̃t���O     *
* previous		: �O�̃��C���[                     *
* next			: ���̃��C���[                     *
* num_layers	: �ǉ��������C���[�̐�             *
* �Ԃ�l                                           *
*	����I��:�ǉ��������C���[	�ُ�I��:NULL      *
***************************************************/
EXTERN LAYER** AddLayerGroupTemplate(
	LAYER_GROUP_TEMPLATE* group,
	uint8* add_flags,
	LAYER* previous,
	LAYER* next,
	int* num_layers
);

/*************************************************
* AddLayerGroupHistory�֐�                       *
* ���C���[���܂Ƃ߂č쐬���闚�����쐬           *
* ����                                           *
* layers		: �ǉ��������C���[               *
* num_layers	: �ǉ��������C���[�̐�           *
* prev			: �ǉ��������C���[�̑O�̃��C���[ *
*************************************************/
EXTERN void AddLayerGroupHistory(LAYER**layers, int num_layers, LAYER* prev);

/***************************************
* UpdateLayerThumbnailWidget�֐�       *
* ���C���[�̃T���l�C�����X�V����       *
* ����                                 *
* layer	: �T���l�C�����X�V���郌�C���[ *
***************************************/
EXTERN INLINE void UpdateLayerThumbnailWidget(LAYER* layer);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_LAYER_H_
