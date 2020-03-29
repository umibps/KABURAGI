#ifndef _INCLUDED_TEXT_LAYER_H_
#define _INCLUDED_TEXT_LAYER_H_

#include <gtk/gtk.h>
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

#define TEXT_LAYER_SELECT_POINT_DISTANCE 25	// �e�L�X�g�`��͈͂�ύX����ۂ̎�t�͈�

typedef enum _eTEXT_STYLE
{
	TEXT_STYLE_NORMAL,
	TEXT_STYLE_ITALIC,
	TEXT_STYLE_OBLIQUE
} eTEXT_STYLE;

/*********************************
* eTEXT_LAYER_FLAGS�񋓑�        *
* �e�L�X�g���C���[�̕\���p�t���O *
*********************************/
typedef enum _eTEXT_LAYER_FLAGS
{
	TEXT_LAYER_VERTICAL = 0x01,					// �c����
	TEXT_LAYER_BOLD = 0x02,						// ����
	TEXT_LAYER_ITALIC = 0x04,					// �C�^���b�N��
	TEXT_LAYER_OBLIQUE = 0x08,					// �Α�
	TEXT_LAYER_BALLOON_HAS_EDGE = 0x10,			// �����o���Ɋp�����邩
	TEXT_LAYER_CENTERING_HORIZONTALLY = 0x20,	// ���������𒆉�����
	TEXT_LAYER_CENTERING_VERTICALLY = 0x40,		// ���������𒆉�����
	TEXT_LAYER_ADJUST_RANGE_TO_TEXT = 0x80		// �e�L�X�g�ɍ��킹�ĕ`��͈͂𒲐�
} eTEXT_LAYER_FLAGS;

/*********************************
* eTEXT_LAYER_BALLOON_TYPE�񋓑� *
* �����o���̃^�C�v               *
*********************************/
typedef enum _eTEXT_LAYER_BALLOON_TYPE
{
	TEXT_LAYER_BALLOON_TYPE_SQUARE,
	TEXT_LAYER_BALLOON_TYPE_ECLIPSE,
	TEXT_LAYER_BALLOON_TYPE_ECLIPSE_THINKING,
	TEXT_LAYER_BALLOON_TYPE_CRASH,
	TEXT_LAYER_BALLOON_TYPE_SMOKE,
	TEXT_LAYER_BALLOON_TYPE_SMOKE_THINKING1,
	TEXT_LAYER_BALLOON_TYPE_SMOKE_THINKING2,
	TEXT_LAYER_BALLOON_TYPE_BURST,
	NUM_TEXT_LAYER_BALLOON_TYPE
} eTEXT_LAYER_BALLOON_TYPE;

/********************************
* TEXT_LAYER_BALLOON_DATA�\���� *
* �����o���̌`������߂�f�[�^  *
********************************/
typedef struct _TEXT_LAYER_BALLOON_DATA
{
	uint16 num_edge;				// ���̐�
	uint16 num_children;			// �����o�����̏����������o���̐�
	FLOAT_T edge_size;				// ���̃T�C�Y
	gint32 random_seed;				// �����̏����l
	FLOAT_T edge_random_size;		// ���̃T�C�Y�̃����_���ω���
	FLOAT_T edge_random_distance;	// ���̊Ԋu�̃����_���ω���
	FLOAT_T start_child_size;		// �����o�����̏����������o���̊J�n�T�C�Y
	FLOAT_T end_child_size;			// �����o�����̏����������o���̏I���T�C�Y
} TEXT_LAYER_BALLOON_DATA;

/*********************************************
* TEXT_LAYER_BALLOON_DATA_WIDGETS�\����      *
* �����o���̏ڍ׌`���ݒ肷��E�B�W�F�b�g�Q *
*********************************************/
typedef struct _TEXT_LAYER_BALLOON_DATA_WIDGETS
{
	GtkAdjustment *num_edge;
	GtkAdjustment *edge_size;
	GtkAdjustment *random_seed;
	GtkAdjustment *edge_random_size;
	GtkAdjustment *edge_random_distance;
	GtkAdjustment *num_children;
	GtkAdjustment *start_child_size;
	GtkAdjustment *end_child_size;
} TEXT_LAYER_BALLOON_DATA_WIDGETS;

/*********************************************
* TEXT_LAYER_POINTS�\����                    *
* �e�L�X�g���C���[��ł̃h���b�O�����̃f�[�^ *
*********************************************/
typedef struct _TEXT_LAYER_POINTS
{
	int point_index;
	gdouble points[9][2];
} TEXT_LAYER_POINTS;

/*******************************
* TEXT_LAYER�\����             *
* �e�L�X�g���C���[�̏����i�[ *
*******************************/
typedef struct _TEXT_LAYER
{
	GtkWidget* text_field;								// �e�L�X�g�r���[�E�B�W�F�b�g
	GtkTextBuffer* buffer;								// �`��e�L�X�g�o�b�t�@
	char* text;											// �o�b�t�@������o�����e�L�X�g���
	gdouble x, y;										// �e�L�X�g�\���̈捶��̍��W
	gdouble width, height;								// �e�L�X�g�\���̈�̕��A����
	uint16 balloon_type;								// �����o���̃^�C�v
	gdouble line_width;									// ���̑���
	gdouble edge_position[3][2];						// �����o���̐���
	gdouble arc_start, arc_end;							// �����o���̉~�ʊJ�n�ƏI���p�x
	TEXT_LAYER_POINTS *drag_points;						// �h���b�O�����̃f�[�^
	TEXT_LAYER_BALLOON_DATA balloon_data;				// �����o���`��p�̃f�[�^
	TEXT_LAYER_BALLOON_DATA_WIDGETS balloon_widgets;	// �����o���̏ڍאݒ�E�B�W�F�b�g
	int base_size;										// �t�H���g�T�C�Y�̔{��
	gdouble font_size;									// �\���e�L�X�g�̃T�C�Y
	int32 font_id;										// �t�H���g�t�@�C������ID
	uint8 color[3];										// �\�������F
	uint8 back_color[4];								// �w�i�F
	uint8 line_color[4];								// �����o���g�̐F
	uint32 flags;										// �c�����A�������̃t���O
} TEXT_LAYER;

/*****************************************
* TEXT_LAYER_BASE_DATA�\����             *
* �e�L�X�g���C���[�̊�{���(�����o���p) *
*****************************************/
typedef struct _TEXT_LAYER_BASE_DATA
{
	gdouble x, y;							// �e�L�X�g�\���̈捶��̍��W
	gdouble width, height;					// �e�L�X�g�\���̈�̕��A����
	uint16 balloon_type;					// �����o���̃^�C�v
	gdouble font_size;						// �\���e�L�X�g�̃T�C�Y
	uint8 base_size;						// �����T�C�Y�̔{��
	uint8 color[3];							// �\�������F
	gdouble edge_position[3][2];			// �����o���̐���
	gdouble arc_start;						// �����o���̉~�ʊJ�n�p
	gdouble arc_end;						// �����o���̉~�ʏI���p
	uint8 back_color[4];					// �����o���̔w�i�F
	uint8 line_color[4];					// �����o���̐��̐F
	gdouble line_width;						// �����o���̐��̑���
	TEXT_LAYER_BALLOON_DATA balloon_data;	// �����o���̏ڍ׃f�[�^
	guint32 flags;							// �c�����A�������̃t���O
} TEXT_LAYER_BASE_DATA;

/*********************************************
* CreateTextLayer�֐�                        *
* �e�L�X�g���C���[�̃f�[�^�������m�ۂƏ����� *
* ����                                       *
* window		: �L�����o�X�̏��           *
* x				: ���C���[��X���W            *
* y				: ���C���[��Y���W            *
* width			: ���C���[�̕�               *
* height		: ���C���[�̍���             *
* base_size		: �����T�C�Y�̔{��           *
* font_size		: �����T�C�Y                 *
* color			: �����F                     *
* balloon_type	: �����o���̃^�C�v           *
* back_color	: �����o���̔w�i�F           *
* line_color	: �����o���̐��̐F           *
* line_width	: �����o���̐��̑���         *
* balloon_data	: �����o���`��̏ڍ׃f�[�^   *
* flags			: �e�L�X�g�\���̃t���O       *
* �Ԃ�l                                     *
*	���������ꂽ�\���̂̃A�h���X             *
*********************************************/
EXTERN TEXT_LAYER* CreateTextLayer(
	void* window,
	gdouble x,
	gdouble y,
	gdouble width,
	gdouble height,
	int base_size,
	gdouble font_size,
	int32 font_id,
	const uint8 color[3],
	uint16 balloon_type,
	const uint8 back_color[4],
	const uint8 line_color[4],
	gdouble line_width,
	TEXT_LAYER_BALLOON_DATA* balloon_data,
	uint32 flags
);

/*****************************************************
* DeleteTextLayer�֐�                                *
* �e�L�X�g���C���[�̃f�[�^���폜����                 *
* ����                                               *
* layer	: �e�L�X�g���C���[�̃f�[�^�|�C���^�̃A�h���X *
*****************************************************/
EXTERN void DeleteTextLayer(TEXT_LAYER** layer);

/***************************************************
* TextLayerGetBalloonArcPoint�֐�                  *
* �����o���̐��J�n���W���擾����                 *
* layer	: �e�L�X�g���C���[�̏��                   *
* angle	: �����o���̒��S�ɑ΂�����J�n�ʒu�̊p�x *
* point	: ���W���擾����z��                       *
***************************************************/
EXTERN void TextLayerGetBalloonArcPoint(
	TEXT_LAYER* layer,
	FLOAT_T angle,
	FLOAT_T point[2]
);

/***********************************************************************
* TextLayerSelectBalloonTypeWidgetNew�֐�                              *
* �����o���̃^�C�v��I������E�B�W�F�b�g���쐬����                     *
* ����                                                                 *
* balloon_type			: �����o���̃^�C�v���L������ϐ��̃A�h���X     *
* select_callback		: �^�C�v�I�����̃R�[���o�b�N�֐��̊֐��|�C���^ *
* select_callback_data	: �^�C�v�I�����̃R�[���o�b�N�֐��Ŏg���f�[�^   *
* widgets				: �ڍאݒ���s���E�B�W�F�b�g�Q���L������\���� *
* application			: �A�v���P�[�V�������Ǘ�����f�[�^             *
* �Ԃ�l                                                               *
*	�����o���̃^�C�v��I������{�^���E�B�W�F�b�g�̃e�[�u���E�B�W�F�b�g *
***********************************************************************/
EXTERN GtkWidget* TextLayerSelectBalloonTypeWidgetNew(
	uint16* balloon_type,
	void (*select_callback)(void*),
	void* select_callback_data,
	TEXT_LAYER_BALLOON_DATA_WIDGETS* widgets,
	void* application
);

/*********************************************************************
* CreateBalloonDetailSettingWidget�֐�                               *
* �����o���̏ڍ׌`���ݒ肷��E�B�W�F�b�g���쐬����                 *
* ����                                                               *
* data					: �ڍאݒ�f�[�^                             *
* widgets				: �ڍאݒ�p�̃E�B�W�F�b�g�Q                 *
* setting_callback		: �ݒ�ύX���̃R�[���o�b�N�֐��̊֐��|�C���^ *
* setting_callback_data	: �ݒ�ύX���̃R�[���o�b�N�֐��Ɏg���f�[�^   *
* application			: �A�v���P�[�V�������Ǘ�����f�[�^           *
* �Ԃ�l                                                             *
*	�ڍאݒ�p�E�B�W�F�b�g                                           *
*********************************************************************/
EXTERN GtkWidget* CreateBalloonDetailSettinWidget(
	TEXT_LAYER_BALLOON_DATA* data,
	TEXT_LAYER_BALLOON_DATA_WIDGETS* widgets,
	void (*setting_callback)(void*),
	void* setting_callback_data,
	void* application
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_TEXT_LAYER_H_
