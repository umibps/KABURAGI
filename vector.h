#ifndef _INCLUDED_VECTOR_H_
#define _INCLUDED_VECTOR_H_

#include <gtk/gtk.h>
#include "types.h"
#include "memory_stream.h"

#ifdef __cplusplus
extern "C" {
#endif

#define VECTOR_LINE_BUFFER_SIZE 32

typedef struct _VECTOR_LINE_LAYER
{
	int32 x, y, width, height, stride;
	uint8* pixels;
	cairo_t* cairo_p;
	cairo_surface_t* surface_p;
} VECTOR_LINE_LAYER;

typedef struct _VECTOR_BASE_DATA
{
	uint8 vector_type;			// �x�N�g���f�[�^�̃^�C�v(����or�Ȑ�or�`��)
	void *prev, *next;			// �O��̃x�N�g���f�[�^
	VECTOR_LINE_LAYER *layer;	// ���X�^���C�Y��̃f�[�^
								// ���X�^���C�Y�����y���p
} VECTOR_BASE_DATA;

typedef enum _eVECTOR_TYPE
{
	VECTOR_TYPE_STRAIGHT,
	VECTOR_TYPE_BEZIER_OPEN,
	VECTOR_TYPE_STRAIGHT_CLOSE,
	VECTOR_TYPE_BEZIER_CLOSE,
	NUM_VECTOR_LINE_TYPE,
	VECTOR_TYPE_SHAPE = NUM_VECTOR_LINE_TYPE,
	VECTOR_TYPE_SQUARE,
	VECTOR_TYPE_RHOMBUS,
	VECTOR_TYPE_ECLIPSE,
	VECTOR_SHAPE_END,
	VECTOR_TYPE_SCRIPT = VECTOR_SHAPE_END
} eVECTOR_TYPE;

typedef enum _eVECTOR_POINT_FLAGS
{
	VECTOR_POINT_SELECTED = 0x01,
} eVECTOR_POINT_FLAGS;

typedef struct _VECTOR_POINT
{
	uint8 vector_type;
	uint8 flags;
	uint8 color[4];
	gdouble pressure, size;
	gdouble x, y;
} VECTOR_POINT;

typedef enum _eVECTOR_LINE_FLAGS
{
	VECTOR_LINE_ANTI_ALIAS = 0x01,
	VECTOR_LINE_FIX = 0x02,
	VECTOR_LINE_MARKED = 0x04
} eVECTOR_LINE_FLAGS;

/*************************************
* VECTOR_LINE�\����                  *
* �x�N�g�����C���[�̐���{���̃f�[�^ *
*************************************/
typedef struct _VECTOR_LINE
{
	VECTOR_BASE_DATA base_data;	// ���̃^�C�v���̊�{���
	uint8 flags;				// �A���`�G�C���A�X���̃t���O
	uint16 num_points;			// ����_�̐�
	uint32 buff_size;			// ����_�o�b�t�@�̃T�C�Y
	gdouble blur;				// �{�P���̊J�n�I�t�Z�b�g
	gdouble outline_hardness;	// �֊s�̍d��
	VECTOR_POINT *points;		// ����_���W�z��
} VECTOR_LINE;

/****************************************
* VECTOR_SQUARE�\����                   *
* �x�N�g�����C���[�̎l�p�`1���̃f�[�^ *
****************************************/
typedef struct _VECTOR_SQUARE
{
	VECTOR_BASE_DATA base_data;	// ���̃^�C�v���̊�{���
	uint8 flags;				// �p�̌`�󓙂̃t���O
	uint8 line_joint;			// ���̌q����
	FLOAT_T left;				// �����`�̍����X���W
	FLOAT_T top;				// �����`�̍����Y���W
	FLOAT_T width;				// �����`�̕�
	FLOAT_T height;				// �����`�̍���
	FLOAT_T rotate;				// ��]
	FLOAT_T line_width;			// ���̑���
	uint8 line_color[4];		// ���̐F
	uint8 fill_color[4];		// �h��ׂ��̐F
} VECTOR_SQUARE;

/****************************************************
* VECTOR_SQUARE_BASE�\����                          *
* �x�N�g�����C���[�̎l�p�`1���̃f�[�^(�����o���p) *
****************************************************/
typedef struct _VECTOR_SQUARE_BASE
{
	uint8 flags;				// �p�̌`�󓙂̃t���O
	uint8 line_joint;			// ���̌q����
	FLOAT_T left;				// �����`�̍����X���W
	FLOAT_T top;				// �����`�̍����Y���W
	FLOAT_T width;				// �����`�̕�
	FLOAT_T height;				// �����`�̍���
	FLOAT_T rotate;				// ��]
	FLOAT_T line_width;			// ���̑���
	uint8 line_color[4];		// ���̐F
	uint8 fill_color[4];		// �h��ׂ��̐F
} VECTOR_SQUARE_BASE;

/**************************************
* VECTOR_ECLIPSE�\����                *
* �x�N�g�����C���[�̑ȉ~1���̃f�[�^ *
**************************************/
typedef struct _VECTOR_ECLIPSE
{
	VECTOR_BASE_DATA base_data;	// ���̃^�C�v���̊�{���
	uint8 flags;				// �p�̌`�󓙂̃t���O
	uint8 line_joint;			// ���̌q����
	FLOAT_T x;					// �ȉ~�̒��S��X���W
	FLOAT_T y;					// �ȉ~�̒��S��Y���W
	FLOAT_T radius;				// ���a
	FLOAT_T ratio;				// ��:����
	FLOAT_T rotate;				// ��]
	FLOAT_T line_width;			// ���̑���
	uint8 line_color[4];		// ���̐F
	uint8 fill_color[4];		// �h��ׂ��̐F
} VECTOR_ECLIPSE;

/**************************************************
* VECTOR_ECLIPSE_BASE�\����                       *
* �x�N�g�����C���[�̑ȉ~1���̃f�[�^(�����o���p) *
**************************************************/
typedef struct _VECTOR_ECLIPSE_BASE
{
	uint8 flags;				// �p�̌`�󓙂̃t���O
	FLOAT_T x;					// �ȉ~�̍����X���W
	FLOAT_T y;					// �ȉ~�̍����Y���W
	FLOAT_T radius;				// ���a
	FLOAT_T ratio;				// ��:����
	FLOAT_T rotate;				// ��]
	FLOAT_T line_width;			// ���̑���
	uint8 line_color[4];		// ���̐F
	uint8 fill_color[4];		// �h��ׂ��̐F
} VECTOR_ECLIPSE_BASE;

/****************************************************************
* VECTOR_SCRIPT�\����                                           *
* �x�N�g�����C���[�̃X�N���v�g�ɂ��x�N�g���f�[�^1���̃f�[�^ *
****************************************************************/
typedef struct _VECTOR_SCRIPT
{
	VECTOR_BASE_DATA base_data;	// �x�N�g���^�C�v���̊�{���
	char *script_data;			// �X�N���v�g�̕�����
} VECTOR_SCRIPT;

/****************************
* VECTOR_DATA�\����         *
* �x�N�g���`��1���̃f�[�^ *
****************************/
typedef union _VECTOR_DATA
{
	VECTOR_BASE_DATA base_data;
	VECTOR_LINE line;
	VECTOR_SQUARE square;
	VECTOR_ECLIPSE eclipse;
	VECTOR_SCRIPT script;
} VECTOR_DATA;

/*********************************************************
* VECTOR_LINE_BASE_DATA�\����                            *
* �x�N�g�����C���[�̐���{���̊�{���(�f�[�^�����o���p) *
*********************************************************/
typedef struct _VECTOR_LINE_BASE_DATA
{
	uint8 vector_type;			// ���̃^�C�v
	uint8 flags;				// ���ɐݒ肳��Ă���t���O
	uint16 num_points;			// ����_�̐�
	gdouble blur;				// �{�P���̊J�n�I�t�Z�b�g
	gdouble outline_hardness;	// �֊s�̍d��
} VECTOR_LINE_BASE_DATA;

typedef struct _VECTOR_LAYER_RECTANGLE
{
	gdouble min_x, min_y, max_x, max_y;
} VECTOR_LAYER_RECTANGLE;

typedef struct _DIVIDE_LINE_CHANGE_DATA
{
	size_t data_size;
	int8 added;
	int8 line_type;
	uint16 before_num_points;
	uint16 after_num_points;

	int index;
	VECTOR_POINT *before;
	VECTOR_POINT *after;
} DIVIDE_LINE_CHANGE_DATA;

typedef struct _DIVIDE_LINE_ADD_DATA
{
	size_t data_size;
	int8 line_type;
	uint16 num_points;

	int index;
	VECTOR_POINT *after;
} DIVIDE_LINE_ADD_DATA;

typedef struct _DIVIDE_LINE_DATA
{
	int num_change;
	int num_add;
	int change_size;
	int layer_name_length;

	char *layer_name;
	DIVIDE_LINE_CHANGE_DATA *change;
	DIVIDE_LINE_ADD_DATA *add;
} DIVIDE_LINE_DATA;

EXTERN VECTOR_LINE* CreateVectorLine(VECTOR_LINE* prev, VECTOR_LINE* next);

EXTERN void DeleteVectorLine(VECTOR_LINE** line);

EXTERN VECTOR_DATA* CreateVectorShape(VECTOR_BASE_DATA* prev, VECTOR_BASE_DATA* next, uint8 vector_type);

EXTERN VECTOR_DATA* CreateVectorScript(VECTOR_BASE_DATA* prev, VECTOR_BASE_DATA* next, const char* script);

EXTERN void DeleteVectorShape(VECTOR_BASE_DATA** shape);

EXTERN void DeleteVectorScript(VECTOR_SCRIPT** script);

EXTERN void ReadVectorShapeData(VECTOR_DATA* data, MEMORY_STREAM* stream);

EXTERN void WriteVectorShapeData(VECTOR_DATA* data, MEMORY_STREAM* stream);

EXTERN void ReadVectorScriptData(VECTOR_SCRIPT* script, MEMORY_STREAM* stream);

EXTERN void WriteVectorScriptData(VECTOR_SCRIPT* script, MEMORY_STREAM* stream);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_VECTOR_H_
