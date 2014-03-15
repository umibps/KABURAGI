// Visual Studio 2005�ȍ~�ł͌Â��Ƃ����֐����g�p����̂�
	// �x�����o�Ȃ��悤�ɂ���
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <png.h>
#include <setjmp.h>
#include <zlib.h>
#include "libtiff/tiffio.h"
#include "libjpeg/jpeglib.h"
#include "types.h"
#include "memory.h"
#include "draw_window.h"
#include "memory_stream.h"
#include "application.h"
#include "image_read_write.h"
#include "utils.h"
#include "tlg.h"
#include "display.h"
#include "application.h"

#ifdef __cplusplus
extern "C" {
#endif

/***********************************************************
* PNG_IO�\����                                             *
* pnglib�ɃX�g���[���̃A�h���X�Ɠǂݏ����֐��|�C���^�n���p *
***********************************************************/
typedef struct _PNG_IO
{
	void* data;
	stream_func rw_func;
	void (*flush_func)(void* stream);
} PNG_IO;

/***************************************************
* PngReadWrite�֐�                                 *
* ����                                             *
* png_p		: pnglib�̈��k�E�W�J�Ǘ��\���̃A�h���X *
* data		: �ǂݏ�����̃A�h���X                 *
* length	: �ǂݏ�������o�C�g��                 *
***************************************************/
static void PngReadWrite(
	png_structp png_p,
	png_bytep data,
	png_size_t length
)
{
	PNG_IO *io = (PNG_IO*)png_get_io_ptr(png_p);
	(void)io->rw_func(data, 1, length, io->data);
}

/***************************************************
* PngFlush�֐�                                     *
* �X�g���[���Ɏc�����f�[�^���N���A                 *
* ����                                             *
* png_p		: pnglib�̈��k�E�W�J�Ǘ��\���̃A�h���X *
***************************************************/
static void PngFlush(png_structp png_p)
{
	PNG_IO* io = (PNG_IO*)png_get_io_ptr(png_p);
	if(io->flush_func != NULL)
	{
		io->flush_func(io->data);
	}
}

/***********************************************************
* ReadPNGStream�֐�                                        *
* PNG�C���[�W�f�[�^��ǂݍ���                              *
* ����                                                     *
* stream	: �f�[�^�X�g���[��                             *
* read_func	: �ǂݍ��ݗp�̊֐��|�C���^                     *
* width		: �C���[�W�̕����i�[����A�h���X               *
* height	: �C���[�W�̍������i�[����A�h���X             *
* stride	: �C���[�W�̈�s���̃o�C�g�����i�[����A�h���X *
* �Ԃ�l                                                   *
*	�s�N�Z���f�[�^                                         *
***********************************************************/
uint8* ReadPNGStream(
	void* stream,
	stream_func read_func,
	gint32* width,
	gint32* height,
	gint32* stride
)
{
	PNG_IO io;
	png_structp png_p;
	png_infop info_p;
	uint32 local_width, local_height, local_stride;
	int32 bit_depth, color_type, interlace_type;
	uint8* pixels;
	uint8** image;
	uint32 i;

	// �X�g���[���̃A�h���X�Ɗ֐��|�C���^���Z�b�g
	io.data = stream;
	io.rw_func = read_func;

	// PNG�W�J�p�̃f�[�^�𐶐�
	png_p = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL
	);

	// �摜�����i�[����f�[�^�̈�쐬
	info_p = png_create_info_struct(png_p);

	if(setjmp(png_jmpbuf(png_p)) != 0)
	{
		png_destroy_read_struct(&png_p, &info_p, NULL);

		return NULL;
	}

	// �ǂݍ��ݗp�̃X�g���[���A�֐��|�C���^���Z�b�g
	png_set_read_fn(png_p, (void*)&io, PngReadWrite);

	// �摜���̓ǂݍ���
	png_read_info(png_p, info_p);
	png_get_IHDR(png_p, info_p, &local_width, &local_height,
		&bit_depth, &color_type, &interlace_type, NULL, NULL
	);
	local_stride = (uint32)png_get_rowbytes(png_p, info_p);

	// �s�N�Z���������̊m��
	pixels = (uint8*)MEM_ALLOC_FUNC(local_stride*local_height);
	// pnglib�ł�2�����z��ɂ���K�v������̂�
		// �������̃|�C���^�z����쐬
	image = (uint8**)MEM_ALLOC_FUNC(sizeof(*image)*local_height);

	// 2�����z��ɂȂ�悤�A�h���X���Z�b�g
	for(i=0; i<local_height; i++)
	{
		image[i] = &pixels[local_stride*i];
	}
	// �摜�f�[�^�̓ǂݍ���
	png_read_image(png_p, image);

	// �������̊J��
	png_destroy_read_struct(&png_p, &info_p, NULL);
	MEM_FREE_FUNC(image);

	// �摜�f�[�^���w��A�h���X�ɃZ�b�g
	*width = (int32)local_width;
	*height = (int32)local_height;
	*stride = (int32)local_stride;

	return pixels;
}

/**************************************************************************
* ReadPNGHeader�֐�                                                       *
* PNG�C���[�W�̃w�b�_����ǂݍ���                                       *
* ����                                                                    *
* stream	: �f�[�^�X�g���[��                                            *
* read_func	: �ǂݍ��ݗp�̊֐��|�C���^                                    *
* width		: �C���[�W�̕����i�[����A�h���X                              *
* height	: �C���[�W�̍������i�[����A�h���X                            *
* stride	: �C���[�W�̈�s���̃o�C�g�����i�[����A�h���X                *
* dpi		: �C���[�W�̉𑜓x(DPI)���i�[����A�h���X                     *
* icc_profile_name	: ICC�v���t�@�C���̖��O���󂯂�|�C���^               *
* icc_profile_data	: ICC�v���t�@�C���̃f�[�^���󂯂�|�C���^             *
* icc_profile_size	: ICC�v���t�@�C���̃f�[�^�̃o�C�g�����i�[����A�h���X *
**************************************************************************/
void ReadPNGHeader(
	void* stream,
	stream_func read_func,
	gint32* width,
	gint32* height,
	gint32* stride,
	int32* dpi,
	char** icc_profile_name,
	uint8** icc_profile_data,
	uint32* icc_profile_size
)
{
	PNG_IO io;
	png_structp png_p;
	png_infop info_p;
	uint32 local_width, local_height, local_stride;
	int32 bit_depth, color_type, interlace_type;

	// �X�g���[���̃A�h���X�Ɗ֐��|�C���^���Z�b�g
	io.data = stream;
	io.rw_func = read_func;

	// PNG�W�J�p�̃f�[�^�𐶐�
	png_p = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL
	);

	// �摜�����i�[����f�[�^�̈�쐬
	info_p = png_create_info_struct(png_p);

	if(setjmp(png_jmpbuf(png_p)) != 0)
	{
		png_destroy_read_struct(&png_p, &info_p, NULL);

		return;
	}

	// �ǂݍ��ݗp�̃X�g���[���A�֐��|�C���^���Z�b�g
	png_set_read_fn(png_p, (void*)&io, PngReadWrite);

	// �摜���̓ǂݍ���
	png_read_info(png_p, info_p);
	png_get_IHDR(png_p, info_p, &local_width, &local_height,
		&bit_depth, &color_type, &interlace_type, NULL, NULL
	);
	local_stride = (uint32)png_get_rowbytes(png_p, info_p);

	// �𑜓x�̎擾
	if(dpi != NULL)
	{
		png_uint_32 res_x, res_y;
		int unit_type;

		if(png_get_pHYs(png_p, info_p, &res_x, &res_y, &unit_type) == PNG_INFO_pHYs)
		{
			if(unit_type == PNG_RESOLUTION_METER)
			{
				if(res_x > res_y)
				{
					*dpi = (int32)(res_x * 0.0254 + 0.5);
				}
				else
				{
					*dpi = (int32)(res_y * 0.0254 + 0.5);
				}
			}
		}
	}

	// ICC�v���t�@�C���̎擾
	if(icc_profile_data != NULL && icc_profile_size != NULL)
	{
		png_charp name;
		png_charp profile;
		png_uint_32 profile_size;
		int compression_type;

		*icc_profile_data = NULL;
		*icc_profile_size = 0;

		if(png_get_iCCP(png_p, info_p, &name, &compression_type,
			&profile, &profile_size) == PNG_INFO_iCCP)
		{
			if(name != NULL && icc_profile_name != NULL)
			{
				*icc_profile_name = MEM_STRDUP_FUNC(name);
			}

			*icc_profile_data = (uint8*)MEM_ALLOC_FUNC(profile_size);
			(void)memcpy(*icc_profile_data, profile, profile_size);
			*icc_profile_size = profile_size;
		}
	}

	// �������̊J��
	png_destroy_read_struct(&png_p, &info_p, NULL);

	// �摜�f�[�^���w��A�h���X�ɃZ�b�g
	if(width != NULL)
	{
		*width = (int32)local_width;
	}
	if(height != NULL)
	{
		*height = (int32)local_height;
	}
	if(stride != NULL)
	{
		*stride = (int32)local_stride;
	}
}

/**************************************************************************
* ReadPNGDetailData�֐�                                                   *
* PNG�C���[�W�f�[�^���ڍ׏��t���œǂݍ���                               *
* ����                                                                    *
* stream	: �f�[�^�X�g���[��                                            *
* read_func	: �ǂݍ��ݗp�̊֐��|�C���^                                    *
* width		: �C���[�W�̕����i�[����A�h���X                              *
* height	: �C���[�W�̍������i�[����A�h���X                            *
* stride	: �C���[�W�̈�s���̃o�C�g�����i�[����A�h���X                *
* dpi		: �C���[�W�̉𑜓x(DPI)���i�[����A�h���X                     *
* icc_profile_name	: ICC�v���t�@�C���̖��O���󂯂�|�C���^               *
* icc_profile_data	: ICC�v���t�@�C���̃f�[�^���󂯂�|�C���^             *
* icc_profile_size	: ICC�v���t�@�C���̃f�[�^�̃o�C�g�����i�[����A�h���X *
* �Ԃ�l                                                                  *
*	�s�N�Z���f�[�^                                                        *
**************************************************************************/
uint8* ReadPNGDetailData(
	void* stream,
	stream_func read_func,
	gint32* width,
	gint32* height,
	gint32* stride,
	int32* dpi,
	char** icc_profile_name,
	uint8** icc_profile_data,
	uint32* icc_profile_size
)
{
	PNG_IO io;
	png_structp png_p;
	png_infop info_p;
	uint32 local_width, local_height, local_stride;
	int32 bit_depth, color_type, interlace_type;
	uint8* pixels;
	uint8** image;
	uint32 i;

	// �X�g���[���̃A�h���X�Ɗ֐��|�C���^���Z�b�g
	io.data = stream;
	io.rw_func = read_func;

	// PNG�W�J�p�̃f�[�^�𐶐�
	png_p = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL
	);

	// �摜�����i�[����f�[�^�̈�쐬
	info_p = png_create_info_struct(png_p);

	if(setjmp(png_jmpbuf(png_p)) != 0)
	{
		png_destroy_read_struct(&png_p, &info_p, NULL);

		return NULL;
	}

	// �ǂݍ��ݗp�̃X�g���[���A�֐��|�C���^���Z�b�g
	png_set_read_fn(png_p, (void*)&io, PngReadWrite);

	// �摜���̓ǂݍ���
	png_read_info(png_p, info_p);
	png_get_IHDR(png_p, info_p, &local_width, &local_height,
		&bit_depth, &color_type, &interlace_type, NULL, NULL
	);
	local_stride = (uint32)png_get_rowbytes(png_p, info_p);

	// �𑜓x�̎擾
	if(dpi != NULL)
	{
		png_uint_32 res_x, res_y;
		int unit_type;

		if(png_get_pHYs(png_p, info_p, &res_x, &res_y, &unit_type) == PNG_INFO_pHYs)
		{
			if(unit_type == PNG_RESOLUTION_METER)
			{
				if(res_x > res_y)
				{
					*dpi = (int32)(res_x * 0.0254 + 0.5);
				}
				else
				{
					*dpi = (int32)(res_y * 0.0254 + 0.5);
				}
			}
		}
	}

	// ICC�v���t�@�C���̎擾
	if(icc_profile_data != NULL && icc_profile_size != NULL)
	{
		png_charp name;
		png_charp profile;
		png_uint_32 profile_size;
		int compression_type;

		if(png_get_iCCP(png_p, info_p, &name, &compression_type,
			&profile, &profile_size) == PNG_INFO_iCCP)
		{
			if(name != NULL)
			{
				*icc_profile_name = MEM_STRDUP_FUNC(name);
			}

			*icc_profile_data = (uint8*)MEM_ALLOC_FUNC(profile_size);
			(void)memcpy(*icc_profile_data, profile, profile_size);
			*icc_profile_size = profile_size;
		}
	}

	// �s�N�Z���������̊m��
	pixels = (uint8*)MEM_ALLOC_FUNC(local_stride*local_height);
	// pnglib�ł�2�����z��ɂ���K�v������̂�
		// �������̃|�C���^�z����쐬
	image = (uint8**)MEM_ALLOC_FUNC(sizeof(*image)*local_height);

	// 2�����z��ɂȂ�悤�A�h���X���Z�b�g
	for(i=0; i<local_height; i++)
	{
		image[i] = &pixels[local_stride*i];
	}
	// �摜�f�[�^�̓ǂݍ���
	png_read_image(png_p, image);

	// �������̊J��
	png_destroy_read_struct(&png_p, &info_p, NULL);
	MEM_FREE_FUNC(image);

	// �摜�f�[�^���w��A�h���X�ɃZ�b�g
	*width = (int32)local_width;
	*height = (int32)local_height;
	*stride = (int32)local_stride;

	return pixels;
}

/*********************************************
* WritePNGStream�֐�                         *
* PNG�̃f�[�^���X�g���[���ɏ�������          *
* ����                                       *
* stream		: �f�[�^�X�g���[��           *
* write_func	: �������ݗp�̊֐��|�C���^   *
* pixels		: �s�N�Z���f�[�^             *
* width			: �摜�̕�                   *
* height		: �摜�̍���                 *
* stride		: �摜�f�[�^��s���̃o�C�g�� *
* channel		: �摜�̃`�����l����         *
* interlace		: �C���^�[���[�X�̗L��       *
* compression	: ���k���x��                 *
*********************************************/
void WritePNGStream(
	void* stream,
	stream_func write_func,
	void (*flush_func)(void*),
	uint8* pixels,
	int32 width,
	int32 height,
	int32 stride,
	uint8 channel,
	int32 interlace,
	int32 compression
)
{
	PNG_IO io = {stream, write_func, flush_func};
	png_structp png_p;
	png_infop info_p;
	uint8** image;
	int color_type;
	int i;

	// �`�����l�����ɍ��킹�ăJ���[�^�C�v��ݒ�
	switch(channel)
	{
	case 1:
		color_type = PNG_COLOR_TYPE_GRAY;
		break;
	case 2:
		color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
		break;
	case 3:
		color_type = PNG_COLOR_TYPE_RGB;
		break;
	case 4:
		color_type = PNG_COLOR_TYPE_RGBA;
		break;
	default:
		return;
	}

	// PNG�������ݗp�̃f�[�^�𐶐�
	png_p = png_create_write_struct(
		PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL
	);

	// �摜���i�[�p�̃��������m��
	info_p = png_create_info_struct(png_p);

	// �������ݗp�̃X�g���[���Ɗ֐��|�C���^���Z�b�g
	png_set_write_fn(png_p, &io, PngReadWrite, PngFlush);
	// ���k�ɂ͑S�Ẵt�B���^���g�p
	png_set_filter(png_p, 0, PNG_ALL_FILTERS);
	// ���k���x����ݒ�
	png_set_compression_level(png_p, compression);

	// PNG�̏����Z�b�g
	png_set_IHDR(png_p, info_p, width, height, 8, color_type,
		interlace, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	// pnglib�p��2�����z����쐬
	image = (uint8**)MEM_ALLOC_FUNC(height*sizeof(*image));
	for(i=0; i<height; i++)
	{
		image[i] = &pixels[stride*i];
	}

	// �摜�f�[�^�̏�������
	png_write_info(png_p, info_p);
	png_write_image(png_p, image);
	png_write_end(png_p, info_p);

	// �������̊J��
	png_destroy_write_struct(&png_p, &info_p);

	MEM_FREE_FUNC(image);
}

/********************************************************
* WritePNGDetailData�֐�                                *
* PNG�̃f�[�^���ڍ׏��t���ŃX�g���[���ɏ�������       *
* ����                                                  *
* stream			: �f�[�^�X�g���[��                  *
* write_func		: �������ݗp�̊֐��|�C���^          *
* pixels			: �s�N�Z���f�[�^                    *
* width				: �摜�̕�                          *
* height			: �摜�̍���                        *
* stride			: �摜�f�[�^��s���̃o�C�g��        *
* channel			: �摜�̃`�����l����                *
* interlace			: �C���^�[���[�X�̗L��              *
* compression		: ���k���x��                        *
* dpi				: �𑜓x(DPI)                       *
* icc_profile_name	: ICC�v���t�@�C���̖��O             *
* icc_profile_data	: ICC�v���t�@�C���̃f�[�^           *
* icc_profile_data	: ICC�v���t�@�C���̃f�[�^�̃o�C�g�� *
********************************************************/
void WritePNGDetailData(
	void* stream,
	stream_func write_func,
	void (*flush_func)(void*),
	uint8* pixels,
	int32 width,
	int32 height,
	int32 stride,
	uint8 channel,
	int32 interlace,
	int32 compression,
	int32 dpi,
	char* icc_profile_name,
	uint8* icc_profile_data,
	uint32 icc_profile_size
)
{
	PNG_IO io = {stream, write_func, flush_func};
	png_structp png_p;
	png_infop info_p;
	uint8** image;
	int color_type;
	int i;

	// �`�����l�����ɍ��킹�ăJ���[�^�C�v��ݒ�
	switch(channel)
	{
	case 1:
		color_type = PNG_COLOR_TYPE_GRAY;
		break;
	case 2:
		color_type = PNG_COLOR_TYPE_GRAY_ALPHA;
		break;
	case 3:
		color_type = PNG_COLOR_TYPE_RGB;
		break;
	case 4:
		color_type = PNG_COLOR_TYPE_RGBA;
		break;
	default:
		return;
	}

	// PNG�������ݗp�̃f�[�^�𐶐�
	png_p = png_create_write_struct(
		PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL
	);

	// �摜���i�[�p�̃��������m��
	info_p = png_create_info_struct(png_p);

	// �������ݗp�̃X�g���[���Ɗ֐��|�C���^���Z�b�g
	png_set_write_fn(png_p, &io, PngReadWrite, PngFlush);
	// ���k�ɂ͑S�Ẵt�B���^���g�p
	png_set_filter(png_p, 0, PNG_ALL_FILTERS);
	// ���k���x����ݒ�
	png_set_compression_level(png_p, compression);

	// PNG�̏����Z�b�g
	png_set_IHDR(png_p, info_p, width, height, 8, color_type,
		interlace, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	// pnglib�p��2�����z����쐬
	image = (uint8**)MEM_ALLOC_FUNC(height*sizeof(*image));
	for(i=0; i<height; i++)
	{
		image[i] = &pixels[stride*i];
	}

	// �𑜓x�̏����Z�b�g
	if(dpi > 0)
	{
		png_uint_32 dpm = (png_uint_32)((dpi * 10000 + 127) / 254);
		png_set_pHYs(png_p, info_p, dpm, dpm, PNG_RESOLUTION_METER);
	}

	// ICC�v���t�@�C���̏����Z�b�g
	if(icc_profile_data != NULL && icc_profile_size > 0)
	{
		png_set_iCCP(png_p, info_p, icc_profile_name, PNG_COMPRESSION_TYPE_BASE,
			icc_profile_data, icc_profile_size);
	}

	// �摜�f�[�^�̏�������
	png_write_info(png_p, info_p);
	png_write_image(png_p, image);
	png_write_end(png_p, info_p);

	// �������̊J��
	png_destroy_write_struct(&png_p, &info_p);

	MEM_FREE_FUNC(image);
}

typedef struct _JPEG_WRITE_ERROR_MANAGER
{
	struct jpeg_error_mgr jerr;
	jmp_buf buf;
} JPEG_WRITE_ERROR_MANAGER;

static void JpegWriteErrorHandler(j_common_ptr cinfo)
{
	JPEG_WRITE_ERROR_MANAGER *manager = (JPEG_WRITE_ERROR_MANAGER*)(cinfo->err);

	longjmp(manager->buf, 1);
}

#define ICC_MARKER  (JPEG_APP0 + 2)			// ICC�v���t�@�C���̃}�[�N�̈ʒu
#define ICC_OVERHEAD_LEN 14					// ICC�v���t�@�C���f�[�^�̃I�[�o�[�w�b�h�̃o�C�g��
#define MAX_BYTES_IN_MARKER 65533			// ICC�v���t�@�C���̃}�[�N�̍ő�o�C�g��
#define MAX_DATA_BYTES_IN_MARKER (MAX_BYTES_IN_MARKER - ICC_OVERHEAD_LEN)

static int JpegMarkerIsICC (jpeg_saved_marker_ptr marker)
{
  return
    marker->marker == ICC_MARKER &&
    marker->data_length >= ICC_OVERHEAD_LEN &&
    /* verify the identifying string */
    GETJOCTET(marker->data[0]) == 0x49 &&
    GETJOCTET(marker->data[1]) == 0x43 &&
    GETJOCTET(marker->data[2]) == 0x43 &&
    GETJOCTET(marker->data[3]) == 0x5F &&
    GETJOCTET(marker->data[4]) == 0x50 &&
    GETJOCTET(marker->data[5]) == 0x52 &&
    GETJOCTET(marker->data[6]) == 0x4F &&
    GETJOCTET(marker->data[7]) == 0x46 &&
    GETJOCTET(marker->data[8]) == 0x49 &&
    GETJOCTET(marker->data[9]) == 0x4C &&
    GETJOCTET(marker->data[10]) == 0x45 &&
    GETJOCTET(marker->data[11]) == 0x0;
}

static int JpegReadICCProfile(
	j_decompress_ptr dinfo,
	uint8** icc_profile_data,
	int32* icc_profile_size
)
{
	jpeg_saved_marker_ptr marker;
	int num_markers = 0;
	int seq_no;
	int total_length = 0;
#define MAX_SEQ_NO 255
	char marker_present[MAX_SEQ_NO+1] = {0};
	int data_length[MAX_SEQ_NO+1];
	int data_offset[MAX_SEQ_NO+1];

	*icc_profile_data = NULL;
	*icc_profile_size = 0;

	for(marker = dinfo->marker_list; marker != NULL; marker = marker->next)
	{
		if(JpegMarkerIsICC(marker))
		{
			if(num_markers == 0)
			{
				num_markers = GETJOCTET(marker->data[13]);
			}
			else if(num_markers != GETJOCTET(marker->data[13]))
			{
				return -1;
			}

			seq_no = GETJOCTET(marker->data[12]);
			if(seq_no <= 0 || seq_no > num_markers)
			{
				return -2;
			}

			if(marker_present[seq_no] != 0)
			{
				return -3;
			}

			marker_present[seq_no] = 1;
			data_length[seq_no] = marker->data_length - ICC_OVERHEAD_LEN;
		}
	}

	for(seq_no = 1; seq_no <= num_markers; seq_no++)
	{
		if(marker_present[seq_no] == 0)
		{
			return -4;
		}

		data_offset[seq_no] = total_length;
		total_length += data_length[seq_no];
	}

	if(total_length <= 0)
	{
		return -5;
	}

	*icc_profile_data = (uint8*)MEM_ALLOC_FUNC(total_length);
	*icc_profile_size = (int32)total_length;

	for(marker = dinfo->marker_list; marker != NULL; marker = marker->next)
	{
		if(JpegMarkerIsICC(marker))
		{
			uint8 *src_ptr;
			uint8 *dst_ptr;

			seq_no = GETJOCTET(marker->data[12]);
			dst_ptr = *icc_profile_data + data_offset[seq_no];
			src_ptr = marker->data + ICC_OVERHEAD_LEN;
			(void)memcpy(dst_ptr, src_ptr, data_length[seq_no]);
		}
	}

	return 0;
}

/********************************************************************
* ReadJpegHeader�֐�                                                *
* JPEG�̃w�b�_����ǂݍ���                                        *
* ����                                                              *
* system_file_path	: OS�̃t�@�C���V�X�e���ɑ������t�@�C���p�X      *
* width				: �摜�̕��f�[�^�̓ǂݍ��ݐ�                    *
* height			: �摜�̍����f�[�^�̓ǂݍ��ݐ�                  *
* resolution		: �摜�̉𑜓x�f�[�^�̓ǂݍ��ݐ�                *
* icc_profile_data	: ICC�v���t�@�C���̃f�[�^���󂯂�|�C���^       *
* icc_profile_size	: ICC�v���t�@�C���̃f�[�^�̃o�C�g���̓ǂݍ��ݐ� *
********************************************************************/
void ReadJpegHeader(
	const char* system_file_path,
	int* width,
	int* height,
	int* resolution,
	uint8** icc_profile_data,
	int32* icc_profile_size
)
{
	struct jpeg_decompress_struct dinfo;
	FILE *fp = fopen(system_file_path, "rb");
	JPEG_WRITE_ERROR_MANAGER error_manager;

	if(fp == NULL)
	{
		return;
	}
	
	error_manager.jerr.error_exit = JpegWriteErrorHandler;
	dinfo.err = jpeg_std_error(&error_manager.jerr);

	jpeg_create_decompress(&dinfo);

	if(setjmp(error_manager.buf) != 0)
	{
		return;
	}

	jpeg_create_decompress(&dinfo);
	jpeg_stdio_src(&dinfo, fp);

	if(icc_profile_data != NULL && icc_profile_size != NULL)
	{
		jpeg_save_markers(&dinfo, ICC_MARKER, 0xffff);
	}

	jpeg_read_header(&dinfo, FALSE);

	(void)JpegReadICCProfile(&dinfo, icc_profile_data, icc_profile_size);

	if(width != NULL)
	{
		*width = dinfo.image_width;
	}
	if(height != NULL)
	{
		*height = dinfo.image_height;
	}
	if(resolution != NULL)
	{
		*resolution = dinfo.X_density;
	}

	jpeg_destroy_decompress(&dinfo);
	(void)fclose(fp);
}

/****************************************************************
* WriteJpegFile�֐�                                             *
* JPEG�`���ŉ摜�f�[�^�������o��                                *
* ����                                                          *
* system_file_path	: OS�̃t�@�C���V�X�e���ɑ������t�@�C���p�X  *
* pixels			: �s�N�Z���f�[�^                            *
* width				: �摜�f�[�^�̕�                            *
* height			: �摜�f�[�^�̍���                          *
* channel			: �摜�̃`�����l����                        *
* data_type			: �s�N�Z���f�[�^�̌`��(RGB or CMYK)         *
* quality			: JPEG�摜�̉掿                            *
* optimize_coding	: �œK���G���R�[�h�̗L��                    *
* icc_profile_data	: ���ߍ���ICC�v���t�@�C���̃f�[�^           *
*						(���ߍ��܂Ȃ�����NULL)                  *
* icc_profile_size	: ���ߍ���ICC�v���t�@�C���̃f�[�^�̃o�C�g�� *
* resolution		: �𑜓x(dpi)                               *
****************************************************************/
void WriteJpegFile(
	const char* system_file_path,
	uint8* pixels,
	int width,
	int height,
	int channel,
	eIMAGE_DATA_TYPE data_type,
	int quality,
	int optimize_coding,
	void* icc_profile_data,
	int icc_profile_size,
	int resolution
)
{
#define MARKER_LENGTH 65533
#define ICC_MARKER_HEADER_LENGTH 14
#define ICC_MARKER_DATA_LENGTH (MARKER_LENGTH - ICC_MARKER_HEADER_LENGTH)

	struct jpeg_compress_struct cinfo;
	JPEG_WRITE_ERROR_MANAGER error_manager;
	FILE *fp;
	uint8 *write_array;
	int stride = width * channel;
	int i;

	if((fp = fopen(system_file_path, "wb")) == NULL)
	{
		return;
	}

	error_manager.jerr.error_exit = JpegWriteErrorHandler;
	cinfo.err = jpeg_std_error(&error_manager.jerr);

	jpeg_create_compress(&cinfo);

	if(setjmp(error_manager.buf) != 0)
	{
		return;
	}

	jpeg_stdio_dest(&cinfo, fp);

	cinfo.image_width = width;
	cinfo.image_height = height;
	cinfo.input_components = channel;

	if(data_type == IMAGE_DATA_CMYK)
	{
		cinfo.in_color_space = JCS_CMYK;
		cinfo.jpeg_color_space = JCS_YCCK;
		jpeg_set_defaults(&cinfo);
		jpeg_set_colorspace(&cinfo, JCS_YCCK);
	}
	else
	{
		cinfo.in_color_space = JCS_RGB;
		jpeg_set_defaults(&cinfo);
	}

	if(resolution > 0)
	{
		cinfo.X_density = resolution;
		cinfo.Y_density = resolution;
		cinfo.density_unit = 1;
	}
	cinfo.optimize_coding = (boolean)optimize_coding;
	cinfo.dct_method = JDCT_FLOAT;
	cinfo.write_JFIF_header = TRUE;

	jpeg_set_quality(&cinfo, quality, TRUE);

	jpeg_start_compress(&cinfo, TRUE);

	if(icc_profile_data != NULL)
	{
		int n_markers, current_marker = 1;
		uint8 *icc_profile_byte_data = (uint8*)icc_profile_data;
		int data_point = 0;

		n_markers = icc_profile_size / ICC_MARKER_DATA_LENGTH;

		if(icc_profile_size % ICC_MARKER_DATA_LENGTH != 0)
		{
			n_markers++;
		}

		while(icc_profile_size > 0)
		{
			int length;

			length = MIN(icc_profile_size, ICC_MARKER_DATA_LENGTH);
			icc_profile_size -= length;

			jpeg_write_m_header(&cinfo, JPEG_APP0 + 2, length + ICC_MARKER_HEADER_LENGTH);
			jpeg_write_m_byte (&cinfo, 'I');
			jpeg_write_m_byte (&cinfo, 'C');
			jpeg_write_m_byte (&cinfo, 'C');
			jpeg_write_m_byte (&cinfo, '_');
			jpeg_write_m_byte (&cinfo, 'P');
			jpeg_write_m_byte (&cinfo, 'R');
			jpeg_write_m_byte (&cinfo, 'O');
			jpeg_write_m_byte (&cinfo, 'F');
			jpeg_write_m_byte (&cinfo, 'I');
			jpeg_write_m_byte (&cinfo, 'L');
			jpeg_write_m_byte (&cinfo, 'E');
			jpeg_write_m_byte (&cinfo, '\0');

			jpeg_write_m_byte(&cinfo, current_marker);
			jpeg_write_m_byte(&cinfo, n_markers);

			while(length -- > 0)
			{
				jpeg_write_m_byte(&cinfo, icc_profile_byte_data[data_point]);
				data_point++;
			}

			current_marker++;
		}
	}	// if(icc_profile_data != NULL)

	for(i=0; i<height; i++)
	{
		write_array = &pixels[i*stride];
		jpeg_write_scanlines(&cinfo,
			(JSAMPARRAY)&write_array, 1);
	}

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	(void)fclose(fp);
}

/***********************************************************
* ReadOriginalFormatLayers�֐�                             *
* �Ǝ��`���̃��C���[�f�[�^��ǂݍ���                       *
* ����                                                     *
* stream		: �ǂݍ��݌��̃A�h���X                     *
* progress_step	: �i���󋵂̍X�V��                         *
* window		: �`��̈�̏��                           *
* app			: �A�v���P�[�V�������Ǘ�����\���̃A�h���X *
* �Ԃ�l                                                   *
*	�ǂݍ��񂾃��C���[�f�[�^                               *
***********************************************************/
LAYER* ReadOriginalFormatLayers(
	MEMORY_STREAM_PTR stream,
	FLOAT_T progress_step,
	DRAW_WINDOW *window,
	APPLICATION* app,
	uint16 num_layer
)
{
	// ���C���[�̊�{���ǂݍ��ݗp
	LAYER_BASE_DATA base;
	// �x�N�g�����C���[�̊�{���ǂݍ��ݗp
	VECTOR_LINE_BASE_DATA line_base;
	// �e�L�X�g���C���[�̊�{���ǂݍ��ݗp
	TEXT_LAYER_BASE_DATA text_base;
	// �摜�f�[�^
	uint8* pixels;
	// �摜�f�[�^�̃o�C�g��
	guint32 data_size;
	// ���̃��C���[�f�[�^���J�n����|�C���g
	guint32 next_data_point;
	// �ǉ����郌�C���[
	LAYER* layer = NULL;
	// �s�N�Z���f�[�^�W�J�p
	MEMORY_STREAM_PTR image;
	// �s�N�Z���f�[�^�̕��A�����A��s���̃o�C�g��
	gint32 width, height, stride;
	// 32bit�ǂݍ��ݗp
	guint32 size_t_temp;
	// ���C���[�E�t�H���g�̖��O�Ƃ��̒���
	char* name;
	uint16 name_length;
	// ���C���[�Z�b�g�̊K�w
	int8 current_hierarchy = 0;
	int8 hierarchy[2048] = {0};
	// ���݂̐i����
	FLOAT_T current_progress = progress_step;
	// �i���󋵂̃p�[�Z���e�[�W�\���p
	gchar show_text[16];
	// �i���󋵕\���X�V�p
	GdkEvent *queued_event;
	// for���p�̃J�E���^
	unsigned int i, j;
	int k;

	for(i=0; i<num_layer; i++)
	{
		// ���C���[�f�[�^�ǂݍ���
			// ���O
		(void)MemRead(&name_length, sizeof(name_length), 1, stream);
		name = (char*)MEM_ALLOC_FUNC(name_length);
		(void)MemRead(name, 1, name_length, stream);
		// ��{���
		(void)MemRead(&base.layer_type, sizeof(base.layer_type), 1, stream);
		(void)MemRead(&base.layer_mode, sizeof(base.layer_mode), 1, stream);
		(void)MemRead(&base.x, sizeof(base.x), 1, stream);
		(void)MemRead(&base.y, sizeof(base.y), 1, stream);
		(void)MemRead(&base.width, sizeof(base.width), 1, stream);
		(void)MemRead(&base.height, sizeof(base.height), 1, stream);
		(void)MemRead(&base.flags, sizeof(base.flags), 1, stream);
		(void)MemRead(&base.alpha, sizeof(base.alpha), 1, stream);
		(void)MemRead(&base.channel, sizeof(base.channel), 1, stream);
		(void)MemRead(&base.layer_set, sizeof(base.layer_set), 1, stream);
		// �l�̃Z�b�g
		layer = CreateLayer(base.x, base.y, base.width, base.height,
			base.channel, base.layer_type, layer, NULL, name, window);
		layer->alpha = base.alpha;
		layer->layer_mode = base.layer_mode;
		layer->flags = base.flags;
		hierarchy[i] = base.layer_set;
		MEM_FREE_FUNC(name);
		window->active_layer = layer;

		// ���C���[�̃^�C�v�ŏ����؂�ւ�
		switch(base.layer_type)
		{
		case TYPE_NORMAL_LAYER:	// �ʏ탌�C���[
			// PNG���k���ꂽ�s�N�Z���f�[�^��W�J���ēǂݍ���
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			next_data_point = (uint32)(stream->data_point + data_size);
			image = CreateMemoryStream(data_size);
			(void)MemRead(image->buff_ptr, 1, data_size, stream);
			pixels = ReadPNGStream(image, (stream_func)MemRead,
				&width, &height, &stride);
			if(pixels != NULL)
			{
				(void)memcpy(layer->pixels, pixels, height*stride);
			}
			DeleteMemoryStream(image);
			MEM_FREE_FUNC(pixels);
			break;
		case TYPE_VECTOR_LAYER:	// �x�N�g�����C���[
			{	// �����s�N�Z���f�[�^�֕ύX���邽�߂̏���
				VECTOR_LAYER_RECTANGLE rect = {0, 0, base.width, base.height};
				VECTOR_LINE* line;
				// ZIP���k�W�J�p
				z_stream decompress_stream;
				// �W�J��̃f�[�^�o�C�g��
				guint32 vector_data_size;

				layer->layer_data.vector_layer_p =
					(VECTOR_LAYER*)MEM_ALLOC_FUNC(sizeof(*layer->layer_data.vector_layer_p));
				(void)memset(layer->layer_data.vector_layer_p, 0,
					sizeof(*layer->layer_data.vector_layer_p));
				// ���������X�^���C�Y�������C���[
				(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
				layer->layer_data.vector_layer_p->mix =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);

				// ��ԉ��ɋ�̃��C���[�쐬
				layer->layer_data.vector_layer_p->base =
					CreateVectorLine(NULL, NULL);
				(void)memset(layer->layer_data.vector_layer_p->base, 0,
					sizeof(*layer->layer_data.vector_layer_p->base));
				layer->layer_data.vector_layer_p->base->layer =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);
				layer->layer_data.vector_layer_p->top_line =
					layer->layer_data.vector_layer_p->base;

				// �f�[�^�̃o�C�g����ǂݍ���
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);
				// �W�J��̃o�C�g����ǂݍ���
				(void)MemRead(&vector_data_size, sizeof(vector_data_size), 1, stream);

				// �W�J��̃f�[�^���������߂̃X�g���[���쐬
				image = CreateMemoryStream(vector_data_size);

				// �f�[�^��W�J
					// ZIP�f�[�^�W�J�O�̏���
				decompress_stream.zalloc = Z_NULL;
				decompress_stream.zfree = Z_NULL;
				decompress_stream.opaque = Z_NULL;
				decompress_stream.avail_in = 0;
				decompress_stream.next_in = Z_NULL;
				(void)inflateInit(&decompress_stream);
				// ZIP�f�[�^�W�J
				decompress_stream.avail_in = (uInt)data_size;
				decompress_stream.next_in = &stream->buff_ptr[stream->data_point];
				decompress_stream.avail_out = (uInt)vector_data_size;
				decompress_stream.next_out = image->buff_ptr;
				(void)inflate(&decompress_stream, Z_NO_FLUSH);
				(void)inflateEnd(&decompress_stream);

				// �����̓ǂݍ���
				(void)MemRead(&size_t_temp, sizeof(size_t_temp), 1, image);
				layer->layer_data.vector_layer_p->num_lines = size_t_temp;
				(void)MemRead(&size_t_temp, sizeof(size_t_temp), 1, image);
				layer->layer_data.vector_layer_p->flags = size_t_temp;

				for(j=0; j<layer->layer_data.vector_layer_p->num_lines; j++)
				{
					(void)MemRead(&line_base.vector_type, sizeof(line_base.vector_type), 1, image);
					(void)MemRead(&line_base.flags, sizeof(line_base.flags), 1, image);
					(void)MemRead(&line_base.num_points, sizeof(line_base.num_points), 1, image);
					(void)MemRead(&line_base.blur, sizeof(line_base.blur), 1, image);
					(void)MemRead(&line_base.outline_hardness, sizeof(line_base.outline_hardness), 1, image);
					line = CreateVectorLine(layer->layer_data.vector_layer_p->top_line, NULL);
					line->vector_type = line_base.vector_type;
					line->flags = line_base.flags;
					line->num_points = line_base.num_points;
					line->blur = line_base.blur;
					line->outline_hardness = line_base.outline_hardness;
					// ����_���̓ǂݍ���
					line->buff_size =
						((line_base.num_points / VECTOR_LINE_BUFFER_SIZE) + 1) * VECTOR_LINE_BUFFER_SIZE;
					line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*line->points)*line->buff_size);
					(void)memset(line->points, 0, sizeof(*line->points)*line->buff_size);
					for(k=0; k<line->num_points; k++)
					{
						(void)MemRead(&line->points[k].vector_type, sizeof(line->points->vector_type), 1, image);
						(void)MemRead(&line->points[k].flags, sizeof(line->points->flags), 1, image);
						(void)MemRead(&line->points[k].color, sizeof(line->points->color), 1, image);
						(void)MemRead(&line->points[k].pressure, sizeof(line->points->pressure), 1, image);
						(void)MemRead(&line->points[k].size, sizeof(line->points->size), 1, image);
						(void)MemRead(&line->points[k].x, sizeof(line->points->x), 1, image);
						(void)MemRead(&line->points[k].y, sizeof(line->points->y), 1, image);
					}

					if(line->points->size == 0.0)
					{
						DeleteVectorLine(&line);
						layer->layer_data.vector_layer_p->num_lines--;
					}
					else
					{
						layer->layer_data.vector_layer_p->top_line = line;
					}
				}

				// �x�N�g���f�[�^�����X�^���C�Y
				layer->layer_data.vector_layer_p->flags =
					(VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ALL);
				RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);

				(void)DeleteMemoryStream(image);
			}
			break;
		case TYPE_TEXT_LAYER:	// �e�L�X�g���C���[
			{
				// �t�H���g�T�[�`�p
				PangoFontFamily** search_font;
				// �t�H���gID
				gint32 font_id;

				// �f�[�^�̃T�C�Y��ǂݍ���
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);

				// �����`��̈�̍��W�A���A�����A�����T�C�Y
					// �t�H���g�t�@�C�����A�F����ǂݍ���
				(void)MemRead(&text_base.x, sizeof(text_base.x), 1, stream);
				(void)MemRead(&text_base.y, sizeof(text_base.y), 1, stream);
				(void)MemRead(&text_base.width, sizeof(text_base.width), 1, stream);
				(void)MemRead(&text_base.height, sizeof(text_base.height), 1, stream);
				(void)MemRead(&text_base.font_size, sizeof(text_base.font_size), 1, stream);
				(void)MemRead(&text_base.color, sizeof(text_base.color), 1, stream);
				(void)MemRead(&text_base.base_size, sizeof(text_base.base_size), 1, stream);
				(void)MemRead(&text_base.flags, sizeof(text_base.flags), 1, stream);
				(void)MemRead(&name_length, sizeof(name_length), 1, stream);
				name = (char*)MEM_ALLOC_FUNC(name_length);
				(void)MemRead(name, 1, name_length, stream);
				search_font = (PangoFontFamily**)bsearch(
					name, app->font_list, app->num_font, sizeof(*app->font_list),
					(int (*)(const void*, const void*))ForFontFamilySearchCompare);
				if(search_font == NULL)
				{
					font_id = 0;
				}
				else
				{
					font_id = (gint32)(search_font - app->font_list);
				}
				MEM_FREE_FUNC(name);
				layer->layer_data.text_layer_p =
					CreateTextLayer(text_base.x, text_base.y, text_base.width, text_base.height,
						text_base.base_size, text_base.font_size, font_id, text_base.color, text_base.flags);

				(void)MemRead(&name_length, sizeof(name_length), 1, stream);
				layer->layer_data.text_layer_p->text =
					(char*)MEM_ALLOC_FUNC(name_length);
				(void)MemRead(layer->layer_data.text_layer_p->text, 1, name_length, stream);
				// ���C���[�����X�^���C�Y
				RenderTextLayer(window, layer, layer->layer_data.text_layer_p);
			}

			break;
		case TYPE_LAYER_SET:	// ���C���[�Z�b�g
			{
				LAYER *target = layer->prev;
				current_hierarchy = hierarchy[i]+1;

				for(k=i-1; k>=0 && current_hierarchy == hierarchy[k]; k--)
				{
					hierarchy[k] = 0;
					target->layer_set = layer;
					target = target->prev;
				}

				next_data_point = (uint32)stream->data_point;
			}

			break;
#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
		case TYPE_3D_LAYER:	// 3D���f�����O���C���[
			// PNG���k���ꂽ�s�N�Z���f�[�^��W�J���ēǂݍ���
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			next_data_point = (uint32)(stream->data_point + data_size);
			image = CreateMemoryStream(data_size);
			(void)MemRead(image->buff_ptr, 1, data_size, stream);
			pixels = ReadPNGStream(image, (stream_func)MemRead,
				&width, &height, &stride);
			if(pixels != NULL)
			{
				(void)memcpy(layer->pixels, pixels, height*stride);
			}
			DeleteMemoryStream(image);
			MEM_FREE_FUNC(pixels);
			break;
#endif
		}

		// �ǉ����f�[�^�Ɉړ�
		(void)MemSeek(stream, (long)next_data_point, SEEK_SET);
		// �ǉ�����ǂݍ���
		(void)MemRead(&layer->num_extra_data, sizeof(layer->num_extra_data), 1, stream);
		for(j=0; j<layer->num_extra_data; j++)
		{
			// �f�[�^�̖��O��ǂݍ���
			(void)MemRead(&name_length, sizeof(name_length), 1, stream);
			layer->extra_data[j].name = (char*)MEM_ALLOC_FUNC(name_length);
			(void)MemRead(layer->extra_data[j].name, 1, name_length, stream);
			(void)MemRead(&size_t_temp, sizeof(size_t_temp), 1, stream);
			layer->extra_data[j].data_size = size_t_temp;
			layer->extra_data[j].data = MEM_ALLOC_FUNC(layer->extra_data[j].data_size);
			(void)MemRead(layer->extra_data[j].data, 1, layer->extra_data[j].data_size, stream);
		}

		// �i���󋵂��X�V
		current_progress += progress_step;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress), current_progress);
		(void)sprintf(show_text, "%.0f%%", current_progress * 100);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app->progress), show_text);
#if MAJOR_VERSION == 1
		gdk_window_process_updates(app->progress->window, FALSE);
#else
		gdk_window_process_updates(gtk_widget_get_window(app->progress), FALSE);
#endif
		while(gdk_events_pending() != FALSE)
		{
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event != NULL)
			{
#if MAJOR_VERSION == 1
				if(queued_event->any.window == app->progress->window
#else
				if(queued_event->any.window == gtk_widget_get_window(app->progress)
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
	}

	// �擪�̃��C���[��Ԃ�
	while(layer->prev != NULL)
	{
		layer = layer->prev;
	}

	return layer;
}

/**************************************************************
* ReadOriginalFormatLayersOldVersion3�֐�                     *
* ���Ǝ��`���̃��C���[�f�[�^��ǂݍ���(�t�@�C���o�[�W�����F 3 *
* ����                                                        *
* stream	: �ǂݍ��݌��̃A�h���X                            *
* window	: �`��̈�̏��                                  *
* app		: �A�v���P�[�V�������Ǘ�����\���̃A�h���X        *
* �Ԃ�l                                                      *
*	�ǂݍ��񂾃��C���[�f�[�^                                  *
**************************************************************/
LAYER* ReadOriginalFormatLayersOldVersion3(
	MEMORY_STREAM_PTR stream,
	DRAW_WINDOW *window,
	APPLICATION* app,
	uint16 num_layer
)
{
	// ���C���[�̊�{���ǂݍ��ݗp
	LAYER_BASE_DATA base;
	// �x�N�g�����C���[�̊�{���ǂݍ��ݗp
	VECTOR_LINE_BASE_DATA line_base;
	// �e�L�X�g���C���[�̊�{���ǂݍ��ݗp
	TEXT_LAYER_BASE_DATA text_base;
	// �摜�f�[�^
	uint8* pixels;
	// �摜�f�[�^�̃o�C�g��
	guint32 data_size;
	// ���̃��C���[�f�[�^���J�n����|�C���g
	guint32 next_data_point;
	// �ǉ����郌�C���[
	LAYER* layer = NULL;
	// �s�N�Z���f�[�^�W�J�p
	MEMORY_STREAM_PTR image;
	// �s�N�Z���f�[�^�̕��A�����A��s���̃o�C�g��
	gint32 width, height, stride;
	// 32bit�ǂݍ��ݗp
	guint32 size_t_temp;
	// ���C���[�E�t�H���g�̖��O�Ƃ��̒���
	char* name;
	uint16 name_length;
	// ���C���[�Z�b�g�̊K�w
	int8 current_hierarchy = 0;
	int8 hierarchy[2048] = {0};
	// BGR��RGB�p
	uint8 r;
	// for���p�̃J�E���^
	int x;
	unsigned int i, j, k;

	for(i=0; i<num_layer; i++)
	{
		// ���C���[�f�[�^�ǂݍ���
			// ���O
		(void)MemRead(&name_length, sizeof(name_length), 1, stream);
		name = (char*)MEM_ALLOC_FUNC(name_length);
		(void)MemRead(name, 1, name_length, stream);
		// ��{���
		(void)MemRead(&base.layer_type, sizeof(base.layer_type), 1, stream);
		(void)MemRead(&base.layer_mode, sizeof(base.layer_mode), 1, stream);
		(void)MemRead(&base.x, sizeof(base.x), 1, stream);
		(void)MemRead(&base.y, sizeof(base.y), 1, stream);
		(void)MemRead(&base.width, sizeof(base.width), 1, stream);
		(void)MemRead(&base.height, sizeof(base.height), 1, stream);
		(void)MemRead(&base.flags, sizeof(base.flags), 1, stream);
		(void)MemRead(&base.alpha, sizeof(base.alpha), 1, stream);
		(void)MemRead(&base.channel, sizeof(base.channel), 1, stream);
		(void)MemRead(&base.layer_set, sizeof(base.layer_set), 1, stream);
		// �l�̃Z�b�g
		layer = CreateLayer(base.x, base.y, base.width, base.height,
			base.channel, base.layer_type, layer, NULL, name, window);
		layer->alpha = base.alpha;
		layer->layer_mode = base.layer_mode;
		layer->flags = base.flags;
		hierarchy[i] = base.layer_set;
		MEM_FREE_FUNC(name);
		window->active_layer = layer;

		// ���C���[�̃^�C�v�ŏ����؂�ւ�
		switch(base.layer_type)
		{
		case TYPE_NORMAL_LAYER:	// �ʏ탌�C���[
			// PNG���k���ꂽ�s�N�Z���f�[�^��W�J���ēǂݍ���
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			next_data_point = (uint32)(stream->data_point + data_size);
			image = CreateMemoryStream(data_size);
			(void)MemRead(image->buff_ptr, 1, data_size, stream);
			pixels = ReadPNGStream(image, (stream_func)MemRead,
				&width, &height, &stride);
			for(x=0; x<width*height; x++)
			{
				r = pixels[x*4];
				pixels[x*4] = pixels[x*4+2];
				pixels[x*4+2] = r;
			}
			(void)memcpy(layer->pixels, pixels, height*stride);
			DeleteMemoryStream(image);
			MEM_FREE_FUNC(pixels);
			break;
		case TYPE_VECTOR_LAYER:	// �x�N�g�����C���[
			{	// �����s�N�Z���f�[�^�֕ύX���邽�߂̏���
				VECTOR_LAYER_RECTANGLE rect = {0, 0, base.width, base.height};
				VECTOR_LINE* line;
				// ZIP���k�W�J�p
				z_stream decompress_stream;
				// �W�J��̃f�[�^�o�C�g��
				guint32 vector_data_size;

				layer->layer_data.vector_layer_p =
					(VECTOR_LAYER*)MEM_ALLOC_FUNC(sizeof(*layer->layer_data.vector_layer_p));
				(void)memset(layer->layer_data.vector_layer_p, 0,
					sizeof(*layer->layer_data.vector_layer_p));
				// ���������X�^���C�Y�������C���[
				(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
				layer->layer_data.vector_layer_p->mix =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);

				// ��ԉ��ɋ�̃��C���[�쐬
				layer->layer_data.vector_layer_p->base =
					CreateVectorLine(NULL, NULL);
				(void)memset(layer->layer_data.vector_layer_p->base, 0,
					sizeof(*layer->layer_data.vector_layer_p->base));
				layer->layer_data.vector_layer_p->base->layer =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);
				layer->layer_data.vector_layer_p->top_line =
					layer->layer_data.vector_layer_p->base;

				// �f�[�^�̃o�C�g����ǂݍ���
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);
				// �W�J��̃o�C�g����ǂݍ���
				(void)MemRead(&vector_data_size, sizeof(vector_data_size), 1, stream);

				// �W�J��̃f�[�^���������߂̃X�g���[���쐬
				image = CreateMemoryStream(vector_data_size);

				// �f�[�^��W�J
					// ZIP�f�[�^�W�J�O�̏���
				decompress_stream.zalloc = Z_NULL;
				decompress_stream.zfree = Z_NULL;
				decompress_stream.opaque = Z_NULL;
				decompress_stream.avail_in = 0;
				decompress_stream.next_in = Z_NULL;
				(void)inflateInit(&decompress_stream);
				// ZIP�f�[�^�W�J
				decompress_stream.avail_in = (uInt)data_size;
				decompress_stream.next_in = &stream->buff_ptr[stream->data_point];
				decompress_stream.avail_out = (uInt)vector_data_size;
				decompress_stream.next_out = image->buff_ptr;
				(void)inflate(&decompress_stream, Z_NO_FLUSH);
				(void)inflateEnd(&decompress_stream);

				// �����̓ǂݍ���
				(void)MemRead(&size_t_temp, sizeof(size_t_temp), 1, image);
				layer->layer_data.vector_layer_p->num_lines = size_t_temp;
				(void)MemRead(&size_t_temp, sizeof(size_t_temp), 1, image);
				layer->layer_data.vector_layer_p->flags = size_t_temp;

				for(j=0; j<layer->layer_data.vector_layer_p->num_lines; j++)
				{
					(void)MemRead(&line_base.vector_type, sizeof(line_base.vector_type), 1, image);
					(void)MemRead(&line_base.flags, sizeof(line_base.flags), 1, image);
					(void)MemRead(&line_base.num_points, sizeof(line_base.num_points), 1, image);
					(void)MemRead(&line_base.blur, sizeof(line_base.blur), 1, image);
					(void)MemRead(&line_base.outline_hardness, sizeof(line_base.outline_hardness), 1, image);
					line = CreateVectorLine(layer->layer_data.vector_layer_p->top_line, NULL);
					line->vector_type = line_base.vector_type;
					line->flags = line_base.flags;
					line->num_points = line_base.num_points;
					line->blur = line_base.blur;
					line->outline_hardness = line_base.outline_hardness;
					// ����_���̓ǂݍ���
					line->buff_size =
						((line_base.num_points / VECTOR_LINE_BUFFER_SIZE) + 1) * VECTOR_LINE_BUFFER_SIZE;
					line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*line->points)*line->buff_size);
					(void)memset(line->points, 0, sizeof(*line->points)*line->buff_size);
					for(k=0; k<line->num_points; k++)
					{
						(void)MemRead(&line->points[k].vector_type, sizeof(line->points->vector_type), 1, image);
						(void)MemRead(&line->points[k].flags, sizeof(line->points->flags), 1, image);
						(void)MemRead(&line->points[k].color, sizeof(line->points->color), 1, image);
						r = line->points[k].color[0];
						line->points[k].color[0] = line->points[k].color[2];
						line->points[k].color[2] = r;
						(void)MemRead(&line->points[k].pressure, sizeof(line->points->pressure), 1, image);
						(void)MemRead(&line->points[k].size, sizeof(line->points->size), 1, image);
						(void)MemRead(&line->points[k].x, sizeof(line->points->x), 1, image);
						(void)MemRead(&line->points[k].y, sizeof(line->points->y), 1, image);
					}

					if(line->points->size == 0.0)
					{
						DeleteVectorLine(&line);
						layer->layer_data.vector_layer_p->num_lines--;
					}
					else
					{
						layer->layer_data.vector_layer_p->top_line = line;
					}
				}

				// �x�N�g���f�[�^�����X�^���C�Y
				layer->layer_data.vector_layer_p->flags =
					(VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ALL);
				RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);

				(void)DeleteMemoryStream(image);
			}
			break;
		case TYPE_TEXT_LAYER:	// �e�L�X�g���C���[
			{
				// �t�H���g�T�[�`�p
				PangoFontFamily** search_font;
				// �t�H���gID
				gint32 font_id;

				// �f�[�^�̃T�C�Y��ǂݍ���
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);

				// �����`��̈�̍��W�A���A�����A�����T�C�Y
					// �t�H���g�t�@�C�����A�F����ǂݍ���
				(void)MemRead(&text_base.x, sizeof(text_base.x), 1, stream);
				(void)MemRead(&text_base.y, sizeof(text_base.y), 1, stream);
				(void)MemRead(&text_base.width, sizeof(text_base.width), 1, stream);
				(void)MemRead(&text_base.height, sizeof(text_base.height), 1, stream);
				(void)MemRead(&text_base.font_size, sizeof(text_base.font_size), 1, stream);
				(void)MemRead(&text_base.color, sizeof(text_base.color), 1, stream);
				r = text_base.color[0];
				text_base.color[0] = text_base.color[2];
				text_base.color[2] = r;
				(void)MemRead(&text_base.base_size, sizeof(text_base.base_size), 1, stream);
				(void)MemRead(&text_base.flags, sizeof(text_base.flags), 1, stream);
				(void)MemRead(&name_length, sizeof(name_length), 1, stream);
				name = (char*)MEM_ALLOC_FUNC(name_length);
				(void)MemRead(name, 1, name_length, stream);
				search_font = (PangoFontFamily**)bsearch(
					name, app->font_list, app->num_font, sizeof(*app->font_list),
					(int (*)(const void*, const void*))ForFontFamilySearchCompare);
				if(search_font == NULL)
				{
					font_id = 0;
				}
				else
				{
					font_id = (gint32)(search_font - app->font_list);
				}
				MEM_FREE_FUNC(name);
				layer->layer_data.text_layer_p =
					CreateTextLayer(text_base.x, text_base.y, text_base.width, text_base.height,
						text_base.base_size, text_base.font_size, font_id, text_base.color, text_base.flags);

				(void)MemRead(&name_length, sizeof(name_length), 1, stream);
				layer->layer_data.text_layer_p->text =
					(char*)MEM_ALLOC_FUNC(name_length);
				(void)MemRead(layer->layer_data.text_layer_p->text, 1, name_length, stream);
				// ���C���[�����X�^���C�Y
				RenderTextLayer(window, layer, layer->layer_data.text_layer_p);
			}

			break;
		case TYPE_LAYER_SET:	// ���C���[�Z�b�g
			{
				LAYER *target = layer->prev;
				current_hierarchy = hierarchy[i]+1;

				for(j=i-1; j>=0 && current_hierarchy == hierarchy[j]; j--)
				{
					hierarchy[j] = 0;
					target->layer_set = layer;
					target = target->prev;
				}

				next_data_point = (uint32)stream->data_point;
			}

			break;
		}

		// ���̃��C���[�f�[�^�Ɉړ�
		(void)MemSeek(stream, (long)next_data_point, SEEK_SET);
	}

	// �擪�̃��C���[��Ԃ�
	while(layer->prev != NULL)
	{
		layer = layer->prev;
	}

	return layer;
}

/*******************************************************
* ReadOriginalFormatLayersOldVersion2�֐�              *
* �Ǝ��`���̃��C���[�f�[�^��ǂݍ���(Ver.2)            *
* ����                                                 *
* stream	: �ǂݍ��݌��̃A�h���X                     *
* window	: �`��̈�̏��                           *
* app		: �A�v���P�[�V�������Ǘ�����\���̃A�h���X *
* �Ԃ�l                                               *
*	�ǂݍ��񂾃��C���[�f�[�^                           *
*******************************************************/
LAYER* ReadOriginalFormatLayersOldVersion2(
	MEMORY_STREAM_PTR stream,
	DRAW_WINDOW *window,
	APPLICATION* app,
	uint16 num_layer
)
{
	// ���C���[�̊�{���ǂݍ��ݗp
	LAYER_BASE_DATA base;
	// �x�N�g�����C���[�̊�{���ǂݍ��ݗp
	VECTOR_LINE_BASE_DATA line_base;
	// �e�L�X�g���C���[�̊�{���ǂݍ��ݗp
	TEXT_LAYER_BASE_DATA text_base;
	// �摜�f�[�^
	uint8* pixels;
	// �摜�f�[�^�̃o�C�g��
	uint32 data_size;
	// ���̃��C���[�f�[�^���J�n����|�C���g
	uint32 next_data_point;
	// �ǉ����郌�C���[
	LAYER* layer = NULL;
	// �s�N�Z���f�[�^�W�J�p
	MEMORY_STREAM_PTR image;
	// �s�N�Z���f�[�^�̕��A�����A��s���̃o�C�g��
	int32 width, height, stride;
	// ���C���[�E�t�H���g�̖��O�Ƃ��̒���
	char* name;
	uint16 name_length;
	// ���C���[�Z�b�g�̊K�w
	int8 current_hierarchy = 0;
	int8 hierarchy[2048] = {0};
	// BGR��RGB�p
	uint8 r;
	// for���p�̃J�E���^
	int x;
	unsigned int i, j;

	for(i=0; i<num_layer; i++)
	{
		// ���C���[�f�[�^�ǂݍ���
			// ���O
		(void)MemRead(&name_length, sizeof(name_length), 1, stream);
		name = (char*)MEM_ALLOC_FUNC(name_length);
		(void)MemRead(name, 1, name_length, stream);
		// ��{���
		(void)MemRead(&base.layer_type, sizeof(base.layer_type), 1, stream);
		(void)MemRead(&base.layer_mode, sizeof(base.layer_mode), 1, stream);
		(void)MemRead(&base.x, sizeof(base.x), 1, stream);
		(void)MemRead(&base.y, sizeof(base.y), 1, stream);
		(void)MemRead(&base.width, sizeof(base.width), 1, stream);
		(void)MemRead(&base.height, sizeof(base.height), 1, stream);
		(void)MemRead(&base.flags, sizeof(base.flags), 1, stream);
		(void)MemRead(&base.alpha, sizeof(base.alpha), 1, stream);
		(void)MemRead(&base.channel, sizeof(base.channel), 1, stream);
		(void)MemRead(&base.layer_set, sizeof(base.layer_set), 1, stream);
		// �l�̃Z�b�g
		layer = CreateLayer(base.x, base.y, base.width, base.height,
			base.channel, base.layer_type, layer, NULL, name, window);
		layer->alpha = base.alpha;
		layer->layer_mode = base.layer_mode;
		layer->flags = base.flags;
		hierarchy[i] = base.layer_set;
		MEM_FREE_FUNC(name);
		window->active_layer = layer;

		// ���C���[�̃^�C�v�ŏ����؂�ւ�
		switch(base.layer_type)
		{
		case TYPE_NORMAL_LAYER:	// �ʏ탌�C���[
			// PNG���k���ꂽ�s�N�Z���f�[�^��W�J���ēǂݍ���
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			next_data_point = (uint32)(stream->data_point + data_size);
			image = CreateMemoryStream(data_size);
			(void)MemRead(image->buff_ptr, 1, data_size, stream);
			pixels = ReadPNGStream(image, (stream_func)MemRead,
				&width, &height, &stride);
			for(x=0; x<width*height; x++)
			{
				r = pixels[x*4];
				pixels[x*4] = pixels[x*4+2];
				pixels[x*4+2] = r;
			}

			(void)memcpy(layer->pixels, pixels, height*stride);
			DeleteMemoryStream(image);
			MEM_FREE_FUNC(pixels);
			break;
		case TYPE_VECTOR_LAYER:	// �x�N�g�����C���[
			{	// �����s�N�Z���f�[�^�֕ύX���邽�߂̏���
				VECTOR_LAYER_RECTANGLE rect = {0, 0, base.width, base.height};
				VECTOR_LINE* line;
				// ZIP���k�W�J�p
				z_stream decompress_stream;
				// �W�J��̃f�[�^�o�C�g��
				uint32 vector_data_size;

				layer->layer_data.vector_layer_p =
					(VECTOR_LAYER*)MEM_ALLOC_FUNC(sizeof(*layer->layer_data.vector_layer_p));
				(void)memset(layer->layer_data.vector_layer_p, 0,
					sizeof(*layer->layer_data.vector_layer_p));
				// ���������X�^���C�Y�������C���[
				(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
				layer->layer_data.vector_layer_p->mix =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);

				// ��ԉ��ɋ�̃��C���[�쐬
				layer->layer_data.vector_layer_p->base =
					CreateVectorLine(NULL, NULL);
				(void)memset(layer->layer_data.vector_layer_p->base, 0,
					sizeof(*layer->layer_data.vector_layer_p->base));
				layer->layer_data.vector_layer_p->base->layer =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);
				layer->layer_data.vector_layer_p->top_line =
					layer->layer_data.vector_layer_p->base;

				// �f�[�^�̃o�C�g����ǂݍ���
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);
				// �W�J��̃o�C�g����ǂݍ���
				(void)MemRead(&vector_data_size, sizeof(vector_data_size), 1, stream);

				// �W�J��̃f�[�^���������߂̃X�g���[���쐬
				image = CreateMemoryStream(vector_data_size);

				// �f�[�^��W�J
					// ZIP�f�[�^�W�J�O�̏���
				decompress_stream.zalloc = Z_NULL;
				decompress_stream.zfree = Z_NULL;
				decompress_stream.opaque = Z_NULL;
				decompress_stream.avail_in = 0;
				decompress_stream.next_in = Z_NULL;
				(void)inflateInit(&decompress_stream);
				// ZIP�f�[�^�W�J
				decompress_stream.avail_in = (uInt)data_size;
				decompress_stream.next_in = &stream->buff_ptr[stream->data_point];
				decompress_stream.avail_out = (uInt)vector_data_size;
				decompress_stream.next_out = image->buff_ptr;
				(void)inflate(&decompress_stream, Z_NO_FLUSH);
				(void)inflateEnd(&decompress_stream);

				// �����̓ǂݍ���
				(void)MemRead(&layer->layer_data.vector_layer_p->num_lines,
					sizeof(layer->layer_data.vector_layer_p->num_lines), 1, image);
				(void)MemRead(&layer->layer_data.vector_layer_p->flags,
					sizeof(layer->layer_data.vector_layer_p->flags), 1, image);
				for(j=0; j<layer->layer_data.vector_layer_p->num_lines; j++)
				{
					//(void)MemRead(&line_base, sizeof(line_base), 1, image);
					(void)MemRead(&line_base.vector_type, sizeof(line_base.vector_type), 1, image);
					(void)MemRead(&line_base.flags, sizeof(line_base.flags), 1, image);
					(void)MemRead(&line_base.num_points, sizeof(line_base.num_points), 1, image);
					(void)MemRead(&line_base.blur, sizeof(line_base.blur), 1, image);
					(void)MemRead(&line_base.outline_hardness, sizeof(line_base.outline_hardness), 1, image);
					line = CreateVectorLine(layer->layer_data.vector_layer_p->top_line, NULL);
					line->vector_type = line_base.vector_type;
					line->flags = line_base.flags;
					line->num_points = line_base.num_points;
					line->blur = line_base.blur;
					line->outline_hardness = line_base.outline_hardness;
					// ����_���̓ǂݍ���
					line->buff_size =
						((line_base.num_points / VECTOR_LINE_BUFFER_SIZE) + 1) * VECTOR_LINE_BUFFER_SIZE;
					line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*line->points)*line->buff_size);
					(void)memset(line->points, 0, sizeof(*line->points)*line->buff_size);
					(void)MemRead(line->points, sizeof(*line->points), line->num_points, image);
					for(x=0; x<line->num_points; x++)
					{
						r = line->points[x].color[0];
						line->points[x].color[0] = line->points[x].color[2];
						line->points[x].color[2] = r;
					}

					if(line->points->size == 0.0)
					{
						DeleteVectorLine(&line);
						layer->layer_data.vector_layer_p->num_lines--;
					}
					else
					{
						layer->layer_data.vector_layer_p->top_line = line;
					}
				}

				// �x�N�g���f�[�^�����X�^���C�Y
				layer->layer_data.vector_layer_p->flags =
					(VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ALL);
				RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);

				(void)DeleteMemoryStream(image);
			}
			break;
		case TYPE_TEXT_LAYER:	// �e�L�X�g���C���[
			{
				// �t�H���g�T�[�`�p
				PangoFontFamily** search_font;
				// �t�H���gID
				int32 font_id;

				// �f�[�^�̃T�C�Y��ǂݍ���
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);

				// �����`��̈�̍��W�A���A�����A�����T�C�Y
					// �t�H���g�t�@�C�����A�F����ǂݍ���
				(void)MemRead(&text_base.x, sizeof(text_base.x), 1, stream);
				(void)MemRead(&text_base.y, sizeof(text_base.y), 1, stream);
				(void)MemRead(&text_base.width, sizeof(text_base.width), 1, stream);
				(void)MemRead(&text_base.height, sizeof(text_base.height), 1, stream);
				(void)MemRead(&text_base.font_size, sizeof(text_base.font_size), 1, stream);
				(void)MemRead(&text_base.color, sizeof(text_base.color), 1, stream);
				r = text_base.color[0];
				text_base.color[0] = text_base.color[2];
				text_base.color[2] = r;
				(void)MemRead(&text_base.flags, sizeof(text_base.flags), 1, stream);
				(void)MemRead(&name_length, sizeof(name_length), 1, stream);
				name = (char*)MEM_ALLOC_FUNC(name_length);
				(void)MemRead(name, 1, name_length, stream);
				search_font = (PangoFontFamily**)bsearch(
					name, app->font_list, app->num_font, sizeof(*app->font_list),
					(int (*)(const void*, const void*))ForFontFamilySearchCompare);
				if(search_font == NULL)
				{
					font_id = 0;
				}
				else
				{
					font_id = (int32)(search_font - app->font_list);
				}
				MEM_FREE_FUNC(name);
				layer->layer_data.text_layer_p =
					CreateTextLayer(text_base.x, text_base.y, text_base.width, text_base.height,
						1, text_base.font_size, font_id, text_base.color, text_base.flags);

				(void)MemRead(&name_length, sizeof(name_length), 1, stream);
				layer->layer_data.text_layer_p->text =
					(char*)MEM_ALLOC_FUNC(name_length);
				(void)MemRead(layer->layer_data.text_layer_p->text, 1, name_length, stream);
				// ���C���[�����X�^���C�Y
				RenderTextLayer(window, layer, layer->layer_data.text_layer_p);
			}

			break;
		case TYPE_LAYER_SET:	// ���C���[�Z�b�g
			{
				LAYER *target = layer->prev;
				current_hierarchy = hierarchy[i]+1;

				for(j=i-1; j>=0 && current_hierarchy == hierarchy[j]; j--)
				{
					hierarchy[j] = 0;
					target->layer_set = layer;
					target = target->prev;
				}

				next_data_point = (uint32)stream->data_point;
			}

			break;
		}

		// ���̃��C���[�f�[�^�Ɉړ�
		(void)MemSeek(stream, (long)next_data_point, SEEK_SET);
	}

	// �擪�̃��C���[��Ԃ�
	while(layer->prev != NULL)
	{
		layer = layer->prev;
	}

	return layer;
}

typedef struct _OLD_VECTOR_POINT1
{
	uint8 vector_type;
	uint8 color[4];
	gdouble pressure, size;
	gdouble x, y;
} OLD_VECTOR_POINT1;

/*******************************************************
* ReadOriginalFormatLayersOldVersion1�֐�              *
* ���Ǝ��`���̃��C���[�f�[�^��ǂݍ���                 *
* ����                                                 *
* stream	: �ǂݍ��݌��̃A�h���X                     *
* window	: �`��̈�̏��                           *
* app		: �A�v���P�[�V�������Ǘ�����\���̃A�h���X *
* �Ԃ�l                                               *
*	�ǂݍ��񂾃��C���[�f�[�^                           *
*******************************************************/
LAYER* ReadOriginalFormatLayersOldVersion1(
	MEMORY_STREAM_PTR stream,
	DRAW_WINDOW *window,
	APPLICATION* app,
	uint16 num_layer
)
{
	// ���C���[�̊�{���ǂݍ��ݗp
	LAYER_BASE_DATA base;
	// �x�N�g�����C���[�̊�{���ǂݍ��ݗp
	VECTOR_LINE_BASE_DATA line_base;
	// �e�L�X�g���C���[�̊�{���ǂݍ��ݗp
	TEXT_LAYER_BASE_DATA text_base;
	// �摜�f�[�^
	uint8* pixels;
	// �摜�f�[�^�̃o�C�g��
	uint32 data_size;
	// ���̃��C���[�f�[�^���J�n����|�C���g
	uint32 next_data_point;
	// �ǉ����郌�C���[
	LAYER* layer = NULL;
	// �s�N�Z���f�[�^�W�J�p
	MEMORY_STREAM_PTR image;
	// �s�N�Z���f�[�^�̕��A�����A��s���̃o�C�g��
	int32 width, height, stride;
	// ���C���[�E�t�H���g�̖��O�Ƃ��̒���
	char* name;
	uint16 name_length;
	// ���C���[�Z�b�g�̊K�w
	int8 current_hierarchy = 0;
	int8 hierarchy[2048] = {0};
	// BGR��RGB�p
	uint8 r;
	// for���p�̃J�E���^
	int x;
	unsigned int i, j;

	for(i=0; i<num_layer; i++)
	{
		// ���C���[�f�[�^�ǂݍ���
			// ���O
		(void)MemRead(&name_length, sizeof(name_length), 1, stream);
		name = (char*)MEM_ALLOC_FUNC(name_length);
		(void)MemRead(name, 1, name_length, stream);
		// ��{���
		(void)MemRead(&base.layer_type, sizeof(base.layer_type), 1, stream);
		(void)MemRead(&base.layer_mode, sizeof(base.layer_mode), 1, stream);
		(void)MemRead(&base.x, sizeof(base.x), 1, stream);
		(void)MemRead(&base.y, sizeof(base.y), 1, stream);
		(void)MemRead(&base.width, sizeof(base.width), 1, stream);
		(void)MemRead(&base.height, sizeof(base.height), 1, stream);
		(void)MemRead(&base.flags, sizeof(base.flags), 1, stream);
		(void)MemRead(&base.alpha, sizeof(base.alpha), 1, stream);
		(void)MemRead(&base.channel, sizeof(base.channel), 1, stream);
		(void)MemRead(&base.layer_set, sizeof(base.layer_set), 1, stream);
		// �l�̃Z�b�g
		layer = CreateLayer(base.x, base.y, base.width, base.height,
			base.channel, base.layer_type, layer, NULL, name, window);
		layer->alpha = base.alpha;
		layer->layer_mode = base.layer_mode;
		layer->flags = base.flags;
		hierarchy[i] = base.layer_set;
		MEM_FREE_FUNC(name);
		window->active_layer = layer;

		// ���C���[�̃^�C�v�ŏ����؂�ւ�
		switch(base.layer_type)
		{
		case TYPE_NORMAL_LAYER:	// �ʏ탌�C���[
			// PNG���k���ꂽ�s�N�Z���f�[�^��W�J���ēǂݍ���
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			next_data_point = (uint32)(stream->data_point + data_size);
			image = CreateMemoryStream(data_size);
			(void)MemRead(image->buff_ptr, 1, data_size, stream);
			pixels = ReadPNGStream(image, (stream_func)MemRead,
				&width, &height, &stride);
			for(x=0; x<width*height; x++)
			{
				r = pixels[x*4];
				pixels[x*4] = pixels[x*4+2];
				pixels[x*4+2] = r;
			}
			(void)memcpy(layer->pixels, pixels, height*stride);
			DeleteMemoryStream(image);
			MEM_FREE_FUNC(pixels);
			break;
		case TYPE_VECTOR_LAYER:	// �x�N�g�����C���[
			{	// �����s�N�Z���f�[�^�֕ύX���邽�߂̏���
				VECTOR_LAYER_RECTANGLE rect = {0, 0, base.width, base.height};
				VECTOR_LINE* line;
				// ZIP���k�W�J�p
				z_stream decompress_stream;
				// �W�J��̃f�[�^�o�C�g��
				uint32 vector_data_size;

				layer->layer_data.vector_layer_p =
					(VECTOR_LAYER*)MEM_ALLOC_FUNC(sizeof(*layer->layer_data.vector_layer_p));
				(void)memset(layer->layer_data.vector_layer_p, 0,
					sizeof(*layer->layer_data.vector_layer_p));
				// ���������X�^���C�Y�������C���[
				(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
				layer->layer_data.vector_layer_p->mix =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);

				// ��ԉ��ɋ�̃��C���[�쐬
				layer->layer_data.vector_layer_p->base =
					CreateVectorLine(NULL, NULL);
				(void)memset(layer->layer_data.vector_layer_p->base, 0,
					sizeof(*layer->layer_data.vector_layer_p->base));
				layer->layer_data.vector_layer_p->base->layer =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);
				layer->layer_data.vector_layer_p->top_line =
					layer->layer_data.vector_layer_p->base;

				// �f�[�^�̃o�C�g����ǂݍ���
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);
				// �W�J��̃o�C�g����ǂݍ���
				(void)MemRead(&vector_data_size, sizeof(vector_data_size), 1, stream);

				// �W�J��̃f�[�^���������߂̃X�g���[���쐬
				image = CreateMemoryStream(vector_data_size);

				// �f�[�^��W�J
					// ZIP�f�[�^�W�J�O�̏���
				decompress_stream.zalloc = Z_NULL;
				decompress_stream.zfree = Z_NULL;
				decompress_stream.opaque = Z_NULL;
				decompress_stream.avail_in = 0;
				decompress_stream.next_in = Z_NULL;
				(void)inflateInit(&decompress_stream);
				// ZIP�f�[�^�W�J
				decompress_stream.avail_in = (uInt)data_size;
				decompress_stream.next_in = &stream->buff_ptr[stream->data_point];
				decompress_stream.avail_out = (uInt)vector_data_size;
				decompress_stream.next_out = image->buff_ptr;
				(void)inflate(&decompress_stream, Z_NO_FLUSH);
				(void)inflateEnd(&decompress_stream);

				// �����̓ǂݍ���
				(void)MemRead(&layer->layer_data.vector_layer_p->num_lines,
					sizeof(layer->layer_data.vector_layer_p->num_lines), 1, image);
				(void)MemRead(&layer->layer_data.vector_layer_p->flags,
					sizeof(layer->layer_data.vector_layer_p->flags), 1, image);
				for(j=0; j<layer->layer_data.vector_layer_p->num_lines; j++)
				{
					OLD_VECTOR_POINT1 *points;
					unsigned int k;
					//(void)MemRead(&line_base, sizeof(line_base), 1, image);
					(void)MemRead(&line_base.vector_type, sizeof(line_base.vector_type), 1, image);
					(void)MemRead(&line_base.num_points, sizeof(line_base.num_points), 1, image);
					(void)MemRead(&line_base.blur, sizeof(line_base.blur), 1, image);
					(void)MemRead(&line_base.outline_hardness, sizeof(line_base.outline_hardness), 1, image);
					line = CreateVectorLine(layer->layer_data.vector_layer_p->top_line, NULL);
					line->vector_type = line_base.vector_type;
					line->num_points = line_base.num_points;
					line->blur = line_base.blur;
					line->outline_hardness = line_base.outline_hardness;
					// ����_���̓ǂݍ���
					line->buff_size =
						((line_base.num_points / VECTOR_LINE_BUFFER_SIZE) + 1) * VECTOR_LINE_BUFFER_SIZE;
					line->points = (VECTOR_POINT*)MEM_ALLOC_FUNC(sizeof(*line->points)*line->buff_size);
					(void)memset(line->points, 0, sizeof(*line->points)*line->buff_size);
					points = (OLD_VECTOR_POINT1*)MEM_ALLOC_FUNC(sizeof(*points)*line->buff_size);
					(void)memset(points, 0, sizeof(*points)*line->num_points);
					(void)MemRead(points, sizeof(*points), line->num_points, image);
					for(k=0; k<line->num_points; k++)
					{
						(void)memcpy(line->points[k].color, points[k].color, 4);
						r = line->points[k].color[0];
						line->points[k].color[0] = line->points[k].color[2];
						line->points[k].color[2] = r;
						line->points[k].pressure = points[k].pressure;
						line->points[k].size = points[k].size;
						line->points[k].vector_type = points[k].vector_type;
						line->points[k].x = points[k].x;
						line->points[k].y = points[k].y;
					}
					MEM_FREE_FUNC(points);

					layer->layer_data.vector_layer_p->top_line = line;
				}

				// �x�N�g���f�[�^�����X�^���C�Y
				layer->layer_data.vector_layer_p->flags =
					(VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ALL);
				RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);

				(void)DeleteMemoryStream(image);
			}
			break;
		case TYPE_TEXT_LAYER:	// �e�L�X�g���C���[
			{
				// �t�H���g�T�[�`�p
				PangoFontFamily** search_font;
				// �t�H���gID
				int32 font_id;

				// �f�[�^�̃T�C�Y��ǂݍ���
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);

				// �����`��̈�̍��W�A���A�����A�����T�C�Y
					// �t�H���g�t�@�C�����A�F����ǂݍ���
				(void)MemRead(&text_base.x, sizeof(text_base.x), 1, stream);
				(void)MemRead(&text_base.y, sizeof(text_base.y), 1, stream);
				(void)MemRead(&text_base.width, sizeof(text_base.width), 1, stream);
				(void)MemRead(&text_base.height, sizeof(text_base.height), 1, stream);
				(void)MemRead(&text_base.font_size, sizeof(text_base.font_size), 1, stream);
				(void)MemRead(&text_base.color, sizeof(text_base.color), 1, stream);
				r = text_base.color[0];
				text_base.color[0] = text_base.color[2];
				text_base.color[2] = r;
				(void)MemRead(&text_base.flags, sizeof(text_base.flags), 1, stream);
				(void)MemRead(&name_length, sizeof(name_length), 1, stream);
				name = (char*)MEM_ALLOC_FUNC(name_length);
				(void)MemRead(name, 1, name_length, stream);
				search_font = (PangoFontFamily**)bsearch(
					name, app->font_list, app->num_font, sizeof(*app->font_list),
					(int (*)(const void*, const void*))ForFontFamilySearchCompare);
				if(search_font == NULL)
				{
					font_id = 0;
				}
				else
				{
					font_id = (int32)(search_font - app->font_list);
				}
				MEM_FREE_FUNC(name);
				layer->layer_data.text_layer_p =
					CreateTextLayer(text_base.x, text_base.y, text_base.width, text_base.height,
						1, text_base.font_size, font_id, text_base.color, text_base.flags);

				(void)MemRead(&name_length, sizeof(name_length), 1, stream);
				layer->layer_data.text_layer_p->text =
					(char*)MEM_ALLOC_FUNC(name_length);
				(void)MemRead(layer->layer_data.text_layer_p->text, 1, name_length, stream);
				// ���C���[�����X�^���C�Y
				RenderTextLayer(window, layer, layer->layer_data.text_layer_p);
			}

			break;
		case TYPE_LAYER_SET:	// ���C���[�Z�b�g
			{
				LAYER *target = layer->prev;
				current_hierarchy = hierarchy[i]+1;

				for(j=i-1; j>=0 && current_hierarchy == hierarchy[j]; j--)
				{
					hierarchy[j] = 0;
					target->layer_set = layer;
					target = target->prev;
				}

				next_data_point = (uint32)stream->data_point;
			}

			break;
		}

		// ���̃��C���[�f�[�^�Ɉړ�
		(void)MemSeek(stream, (long)next_data_point, SEEK_SET);
	}

	// �擪�̃��C���[��Ԃ�
	while(layer->prev != NULL)
	{
		layer = layer->prev;
	}

	return layer;
}

/***********************************
* ReadOriginalFormatExtraData�֐�  *
* �Ǝ��`���̒ǉ�����ǂݍ���     *
* ����                             *
* stream	: �ǂݍ��݌��̃A�h���X *
* window	: �`��̈�̏��       *
***********************************/
void ReadOriginalFormatExtraData(MEMORY_STREAM_PTR stream, DRAW_WINDOW* window)
{
	guint32 tag;		// �f�[�^�̎�ނ�\���^�O
	guint32 data_size;	// �e��f�[�^�̃T�C�Y

	// �^�O����ǂݎ�����胋�[�v
	while(MemRead(&tag, sizeof(tag), 1, stream) >= 1)
	{
		// �f�[�^�T�C�Y��ǂݍ���
		if(MemRead(&data_size, sizeof(data_size), 1, stream) < 1)
		{
			break;
		}

		// �^�O���V�X�e���̃G���f�B�A����
		tag = GUINT32_FROM_BE(tag);
		// �^�O�ɂ���ď�����؂�ւ�
		switch(tag)
		{
		case 'actv':	// �A�N�e�B�u�ȃ��C���[
			{	// ���C���[�̖��O��ǂݍ���ŃT�[�`
				char name[4096];
				(void)MemRead(name, 1, data_size, stream);
				if((window->active_layer = SearchLayer(window->layer, name)) == NULL)
				{
					window->active_layer = window->layer;
				}
			}
			break;
		case 'slct':	// �I��͈�
			{	// �I��͈͂�PNG���k����Ă���̂œW�J
				MEMORY_STREAM select_stream;
				uint8 *pixels;
				int32 width, height, stride;

				select_stream.block_size = 1;
				select_stream.buff_ptr = &stream->buff_ptr[stream->data_point];
				select_stream.data_size = data_size;
				select_stream.data_point = 0;

				pixels = ReadPNGStream((void*)&select_stream, (stream_func)MemRead,
					&width, &height, &stride);

				(void)memcpy(window->selection->pixels, pixels, window->width*window->height);

				if(UpdateSelectionArea(&window->selection_area, window->selection, window->temp_layer) != FALSE)
				{
					window->flags |= DRAW_WINDOW_HAS_SELECTION_AREA;
				}

				MEM_FREE_FUNC(pixels);
			}
			break;
		case 'resl':	// �𑜓x
			{
				gint32 resolution;
				(void)MemRead(&resolution, sizeof(resolution), 1, stream);
				window->resolution = (int16)resolution;
			}
			break;
		case '2ndb':	// 2�߂̔w�i�F
			{
				uint8 second_bg[4];

				(void)MemRead(second_bg, 1, 4, stream);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
				window->second_back_ground[0] = second_bg[2];
				window->second_back_ground[1] = second_bg[1];
				window->second_back_ground[2] = second_bg[0];
#else
				window->second_back_ground[0] = second_bg[0];
				window->second_back_ground[1] = second_bg[1];
				window->second_back_ground[2] = second_bg[2];
#endif
				if(second_bg[3] != 0)
				{
					window->flags |= DRAW_WINDOW_SECOND_BG;

					window->app->flags |= APPLICATION_IN_SWITCH_DRAW_WINDOW;
					gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(window->app->menus.change_back_ground_menu), TRUE);
					window->app->flags &= ~(APPLICATION_IN_SWITCH_DRAW_WINDOW);
				}
			}
			break;
		case 'iccp':	// ICC�v���t�@�C���̓��e
			window->icc_profile_data = MEM_ALLOC_FUNC(data_size);
			(void)MemRead(window->icc_profile_data, 1, data_size, stream);
			DrawWindowSetIccProfile(window, data_size, FALSE);
			break;
		case 'iccf':
			{
				char path[4096];
				(void)MemRead(path, 1, data_size, stream);
				window->icc_profile_path = g_strdup(path);
			}
			break;
		}
	}
}

/***********************************************************
* ReadOriginalFormat�֐�                                   *
* �Ǝ��`���̃f�[�^��ǂݍ���                               *
* ����                                                     *
* stram			: �ǂݍ��݌��̃A�h���X                     *
* read_func		: �ǂݍ��ݗp�̊֐��|�C���^                 *
* stream_size	: �f�[�^�̑��o�C�g��                       *
* app			: �A�v���P�[�V�������Ǘ�����\���̃A�h���X *
* data_name		: �t�@�C����                               *
* �Ԃ�l                                                   *
*	�`��̈�̃f�[�^                                       *
***********************************************************/
DRAW_WINDOW* ReadOriginalFormat(
	void* stream,
	stream_func read_func,
	size_t stream_size,
	APPLICATION* app,
	const char* data_name
)
{
	// �Ԃ�l
	DRAW_WINDOW* window;
	// ���C���[�r���[�ɒǉ����郌�C���[
	LAYER* layer;
	// �f�[�^�͈�x���ă������ɓǂݍ���ł��܂�
	MEMORY_STREAM_PTR mem_stream = CreateMemoryStream(stream_size);
	// �s�N�Z���f�[�^�W�J�p
	MEMORY_STREAM_PTR image;
	// �I���W�i���̕��A����
	gint32 original_width, original_height;
	// �`��̈�A���C���̕��A����
	gint32 width, height;
	// �摜�f�[�^��s���̃o�C�g��
	gint32 stride;
	// ���C���[�̐�
	uint16 num_layer;
	// �`�����l�����A�J���[���[�h
	uint8 channel, color_mode;
	// �摜�f�[�^
	uint8* pixels;
	// �摜�f�[�^�̃o�C�g��
	guint32 data_size;
	// �K���f�[�^����p�̕�����
	const char format_string[] = "Paint Soft KABURAGI";
	// �t�@�C���o�[�W����
	guint32 file_version;
	// �i���󋵍X�V�̕�
	FLOAT_T progress_step;
	// for���p�̃J�E���^
	int i;

	// �������Ƀf�[�^�ǂݍ���
	(void)read_func(mem_stream->buff_ptr, 1, stream_size, stream);

	// �t�@�C���̓K���𔻒�
	if(memcmp(mem_stream->buff_ptr, format_string,
		sizeof(format_string)/sizeof(*format_string)) != 0)
	{	// �s�K��
		return NULL;
	}

	// �o�[�W��������ǂݍ���
	mem_stream->data_point = sizeof(format_string)/sizeof(*format_string);
	(void)MemRead(&file_version, sizeof(file_version), 1, mem_stream);

	{	// �T���l�C���L���̔���
		uint8 has_thumbnail;
		(void)MemRead(&has_thumbnail, 1, 1, mem_stream);

		if(has_thumbnail != 0)
		{
			guint32 skip_bytes;
			(void)MemRead(&skip_bytes, sizeof(skip_bytes), 1, mem_stream);
			(void)MemSeek(mem_stream, skip_bytes, SEEK_CUR);
		}
	}

	// �`�����l�����ƃJ���[���[�h��ǂݍ���
	(void)MemRead(&channel, sizeof(channel), 1, mem_stream);
	(void)MemRead(&color_mode, sizeof(color_mode), 1, mem_stream);

	// �I���W�i���̕��A������ǂݍ���
	(void)MemRead(&original_width, sizeof(original_width), 1, mem_stream);
	(void)MemRead(&original_height, sizeof(original_height), 1, mem_stream);

	// �`��̈�̕��A����(4�̔{��)��ǂݍ���
	(void)MemRead(&width, sizeof(width), 1, mem_stream);
	(void)MemRead(&height, sizeof(height), 1, mem_stream);

	// ���C���[�̐���ǂݍ���
	(void)MemRead(&num_layer, sizeof(num_layer), 1, mem_stream);

	// �`��̈�쐬
	window = CreateDrawWindow(
		original_width, original_height, channel, data_name,
		app->note_book, app->window_num, app
	);

	// �w�i�摜�f�[�^��ǂݍ���
	(void)MemRead(&data_size, sizeof(data_size), 1, mem_stream);
	image = CreateMemoryStream(data_size);
	(void)MemRead(image->buff_ptr, 1, data_size, mem_stream);

	pixels = ReadPNGStream(image, (stream_func)MemRead, &width, &height,&stride);

	// �`��̈�̔w�i�փR�s�[
	(void)memcpy(window->back_ground, pixels, height*stride);

	// �������J��
	DeleteMemoryStream(image);
	MEM_FREE_FUNC(pixels);

	// �i���󋵂��X�V
	{
		GdkEvent *queued_event;
		gchar show_text[16];

		progress_step = 1.0 / (num_layer + 1);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress), progress_step);
		(void)sprintf(show_text, "%.0f%%", progress_step * 100);
#if MAJOR_VERSION == 1
		gdk_window_process_updates(app->progress->window, FALSE);
#else
		gdk_window_process_updates(gtk_widget_get_window(app->progress), FALSE);
#endif
		while(gdk_events_pending() != FALSE)
		{
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event != NULL)
			{
#if MAJOR_VERSION == 1
				if(queued_event->any.window == app->progress->window
#else
				if(queued_event->any.window == gtk_widget_get_window(app->progress)
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
	}

	// ���C���[���̓ǂݍ���
		// ��ԉ��Ɏ����ō���郌�C���[���폜
	DeleteLayer(&window->layer);
	// �t�@�C���o�[�W�����ŏ�����؂�ւ�
	if(file_version == FILE_VERSION)
	{
		window->layer = ReadOriginalFormatLayers(mem_stream, progress_step, window, app, num_layer);
	}
	else if(file_version == 3)
	{
		window->layer = ReadOriginalFormatLayersOldVersion3(mem_stream, window, app, num_layer);
	}
	else if(file_version == 2)
	{
		window->layer = ReadOriginalFormatLayersOldVersion2(
			mem_stream, window, app, num_layer);
	}
	else if(file_version == 1)
	{
		window->layer = ReadOriginalFormatLayersOldVersion1(
			mem_stream, window, app, num_layer);
	}

	window->num_layer = num_layer;

	// ���C���[�r���[�Ƀ��C���[��ǉ�
	layer = window->layer;
	for(i=0; i<num_layer; i++)
	{
		LayerViewAddLayer(layer, window->layer, app->layer_window.view, i+1);
		layer = layer->next;
	}

	// �A�N�e�B�u���C���[�͎b��I�Ɉ�ԉ���
	window->active_layer = window->layer;

	// �ǉ����̓ǂݍ���
	ReadOriginalFormatExtraData(mem_stream, window);

	ChangeActiveLayer(window, window->active_layer);
	LayerViewSetActiveLayer(window->active_layer, app->layer_window.view);

	g_object_set_data(G_OBJECT(window->active_layer->widget->box), "signal_id",
		GUINT_TO_POINTER(g_signal_connect(G_OBJECT(window->active_layer->widget->box), "size-allocate",
		G_CALLBACK(Move2ActiveLayer), app)));

	(void)DeleteMemoryStream(mem_stream);

	// �v���O���X�o�[�����Z�b�g
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress), 0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app->progress), "");

	return window;
}

/***********************************************************
* ReadOriginalFormatMixedData�֐�                          *
* �Ǝ��`���̍����f�[�^���쐬���ĕԂ�                       *
* ����                                                     *
* stram			: �ǂݍ��݌��̃A�h���X                     *
* read_func		: �ǂݍ��ݗp�̊֐��|�C���^                 *
* stream_size	: �f�[�^�̑��o�C�g��                       *
* app			: �A�v���P�[�V�������Ǘ�����\���̃A�h���X *
* data_name		: �t�@�C����                               *
* �Ԃ�l                                                   *
*	�����������C���[                                       *
***********************************************************/
LAYER* ReadOriginalFormatMixedData(
	void* stream,
	stream_func read_func,
	size_t stream_size,
	APPLICATION* app,
	const char* data_name
)
{
	// �Ԃ�l
	LAYER *mixed;
	// �`��̈�̏��
	DRAW_WINDOW* window;
	// �������郌�C���[
	LAYER* layer;
	// �f�[�^�͈�x���ă������ɓǂݍ���ł��܂�
	MEMORY_STREAM_PTR mem_stream = CreateMemoryStream(stream_size);
	// �s�N�Z���f�[�^�W�J�p
	MEMORY_STREAM_PTR image;
	// �I���W�i���̕��A����
	gint32 original_width, original_height;
	// �`��̈�A���C���̕��A����
	gint32 width, height;
	// �摜�f�[�^��s���̃o�C�g��
	gint32 stride;
	// ���C���[�̐�
	uint16 num_layer;
	// �`�����l�����A�J���[���[�h
	uint8 channel, color_mode;
	// �摜�f�[�^
	uint8* pixels;
	// �摜�f�[�^�̃o�C�g��
	guint32 data_size;
	// �K���f�[�^����p�̕�����
	const char format_string[] = "Paint Soft KABURAGI";
	// �t�@�C���o�[�W����
	guint32 file_version;
	// �i���󋵍X�V�̕�
	FLOAT_T progress_step;

	// �������Ƀf�[�^�ǂݍ���
	(void)read_func(mem_stream->buff_ptr, 1, stream_size, stream);

	// �t�@�C���̓K���𔻒�
	if(memcmp(mem_stream->buff_ptr, format_string,
		sizeof(format_string)/sizeof(*format_string)) != 0)
	{	// �s�K��
		return NULL;
	}

	// �o�[�W��������ǂݍ���
	mem_stream->data_point = sizeof(format_string)/sizeof(*format_string);
	(void)MemRead(&file_version, sizeof(file_version), 1, mem_stream);

	{	// �T���l�C���L���̔���
		uint8 has_thumbnail;
		(void)MemRead(&has_thumbnail, 1, 1, mem_stream);

		if(has_thumbnail != 0)
		{
			guint32 skip_bytes;
			(void)MemRead(&skip_bytes, sizeof(skip_bytes), 1, mem_stream);
			(void)MemSeek(mem_stream, skip_bytes, SEEK_CUR);
		}
	}

	// �`�����l�����ƃJ���[���[�h��ǂݍ���
	(void)MemRead(&channel, sizeof(channel), 1, mem_stream);
	(void)MemRead(&color_mode, sizeof(color_mode), 1, mem_stream);

	// �I���W�i���̕��A������ǂݍ���
	(void)MemRead(&original_width, sizeof(original_width), 1, mem_stream);
	(void)MemRead(&original_height, sizeof(original_height), 1, mem_stream);

	// �`��̈�̕��A����(4�̔{��)��ǂݍ���
	(void)MemRead(&width, sizeof(width), 1, mem_stream);
	(void)MemRead(&height, sizeof(height), 1, mem_stream);

	// ���C���[�̐���ǂݍ���
	(void)MemRead(&num_layer, sizeof(num_layer), 1, mem_stream);

	// �`��̈�쐬
	window = CreateTempDrawWindow(
		original_width, original_height, channel, data_name,
		app->note_book, app->window_num, app
	);

	// �w�i�摜�f�[�^��ǂݍ���
	(void)MemRead(&data_size, sizeof(data_size), 1, mem_stream);
	image = CreateMemoryStream(data_size);
	(void)MemRead(image->buff_ptr, 1, data_size, mem_stream);

	pixels = ReadPNGStream(image, (stream_func)MemRead, &width, &height,&stride);

	// �`��̈�̔w�i�փR�s�[
	(void)memcpy(window->back_ground, pixels, height*stride);

	// �������J��
	DeleteMemoryStream(image);
	MEM_FREE_FUNC(pixels);

	// �i���󋵂��X�V
	{
		GdkEvent *queued_event;
		gchar show_text[16];

		progress_step = 1.0 / (num_layer + 1);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress), progress_step);
		(void)sprintf(show_text, "%.0f%%", progress_step * 100);
#if MAJOR_VERSION == 1
		gdk_window_process_updates(app->progress->window, FALSE);
#else
		gdk_window_process_updates(gtk_widget_get_window(app->progress), FALSE);
#endif
		while(gdk_events_pending() != FALSE)
		{
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event != NULL)
			{
#if MAJOR_VERSION == 1
				if(queued_event->any.window == app->progress->window
#else
				if(queued_event->any.window == gtk_widget_get_window(app->progress)
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
	}

	// ���C���[���̓ǂݍ���
		// �t�@�C���o�[�W�����ŏ�����؂�ւ�
	if(file_version == FILE_VERSION)
	{
		window->layer = ReadOriginalFormatLayers(mem_stream, progress_step, window, app, num_layer);
	}
	else if(file_version == 3)
	{
		window->layer = ReadOriginalFormatLayersOldVersion3(mem_stream, window, app, num_layer);
	}
	else if(file_version == 2)
	{
		window->layer = ReadOriginalFormatLayersOldVersion2(
			mem_stream, window, app, num_layer);
	}
	else if(file_version == 1)
	{
		window->layer = ReadOriginalFormatLayersOldVersion1(
			mem_stream, window, app, num_layer);
	}

	window->num_layer = num_layer;

	mixed = CreateLayer(0, 0, window->width, window->height, window->channel, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);
	(void)memcpy(mixed->pixels, window->back_ground, window->pixel_buf_size);

	layer = window->layer;
	while(layer != NULL)
	{
		if((layer->flags & LAYER_FLAG_INVISIBLE) == 0)
		{
			g_layer_blend_funcs[layer->layer_mode](layer, mixed);
		}

		layer = layer->next;
	}

	(void)DeleteMemoryStream(mem_stream);

	// �v���O���X�o�[�����Z�b�g
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->progress), 0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app->progress), "");

	DeleteDrawWindow(&window);

	return mixed;
}

/*******************************************************************
* ReadOriginalFormatMemoryStream�֐�                               *
* �������X�g���[���̓Ǝ��t�H�[�}�b�g�̃f�[�^��ǂݍ���(�A���h�D�p) *
* ����                                                             *
* window	: �f�[�^�̓ǂݍ��ݐ�                                   *
* stream	: �������X�g���[��                                     *
*******************************************************************/
void ReadOriginalFormatMemoryStream(
	DRAW_WINDOW* window,
	MEMORY_STREAM_PTR stream
)
{
	// ���C���[�r���[�ɒǉ����郌�C���[
	LAYER* layer;
	// �s�N�Z���f�[�^�W�J�p
	MEMORY_STREAM_PTR image;
	// �g��E�k���E��]�p�̍s��
	cairo_matrix_t matrix;
	// �摜�f�[�^�̕��A����
	gint32 width, height;
	// �摜�f�[�^��s���̃o�C�g��
	gint32 stride;
	// �`�����l�����A�J���[���[�h
	uint8 channel, color_mode;
	// �摜�f�[�^
	uint8* pixels;
	// �摜�f�[�^�̃o�C�g��
	guint32 data_size;
	// �K���f�[�^����p�̕�����
	const char format_string[] = "Paint Soft KABURAGI";
	// �i���󋵂̍X�V��
	FLOAT_T progress_step;
	// for���p�̃J�E���^
	int i;

	// �f�[�^�̓K���𔻒�
	if(memcmp(format_string, stream->buff_ptr,
		sizeof(format_string)/sizeof(*format_string)) != 0)
	{	// �s�K��
		return;
	}

	// �o�[�W���������X�L�b�v
	stream->data_point = sizeof(format_string)/sizeof(*format_string) + + sizeof(data_size);

	{	// �T���l�C���L���̔���
		uint8 has_thumbnail;
		(void)MemRead(&has_thumbnail, 1, 1, stream);

		if(has_thumbnail != 0)
		{
			guint32 skip_bypte;
			(void)MemRead(&skip_bypte, sizeof(skip_bypte), 1, stream);
			(void)MemSeek(stream, skip_bypte, SEEK_CUR);
		}
	}

	// �`�����l�����ƃJ���[���[�h��ǂݍ���
	(void)MemRead(&channel, sizeof(channel), 1, stream);
	(void)MemRead(&color_mode, sizeof(color_mode), 1, stream);

	// �I���W�i���̕��A������ǂݍ���
	(void)MemRead(&width, sizeof(width), 1, stream);
	window->original_width = width;
	(void)MemRead(&height, sizeof(height), 1, stream);
	window->original_height = height;

	// �`��̈�̕��A����(4�̔{��)��ǂݍ���
	(void)MemRead(&width, sizeof(width), 1, stream);
	window->width = width;
	(void)MemRead(&height, sizeof(height), 1, stream);
	window->height = height;

	// ���C���[�̐���ǂݍ���
	(void)MemRead(&window->num_layer, sizeof(window->num_layer), 1, stream);

	// �w�i�摜�f�[�^��ǂݍ���
	(void)MemRead(&data_size, sizeof(data_size), 1, stream);
	image = CreateMemoryStream(data_size);
	(void)MemRead(image->buff_ptr, 1, data_size, stream);

	pixels = ReadPNGStream(image, (stream_func)MemRead, &width, &height,&stride);

	// �`��̈�̔w�i�փR�s�[
	window->back_ground = (uint8*)MEM_ALLOC_FUNC(stride*height);
	(void)memcpy(window->back_ground, pixels, window->height*stride);

	// �������J��
	DeleteMemoryStream(image);
	MEM_FREE_FUNC(pixels);

	// �s�N�Z���f�[�^�̃o�C�g�����v�Z
	window->pixel_buf_size = width * height * window->channel;
	// 1�s���̃o�C�g�����v�Z
	window->stride = width * window->channel;

	// ��Ɨp�̃��C���[���쎌��
	window->work_layer = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);

	// �A�N�e�B�u���C���[�ƍ�Ɨp���C���[���ꎞ�I�ɍ������郌�C���[
	window->temp_layer = CreateLayer(0, 0, width, height, 5, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);

	// ���C���[��������������
	window->mixed_layer = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);
	(void)memcpy(window->mixed_layer->pixels, window->back_ground, window->pixel_buf_size);
	// �������ʂɑ΂��Ċg��E�k����ݒ肷�邽�߂̃p�^�[��
	window->mixed_pattern = cairo_pattern_create_for_surface(window->mixed_layer->surface_p);
	cairo_pattern_set_filter(window->mixed_pattern, CAIRO_FILTER_FAST);

	// �I��̈�̃��C���[���쐬
	window->selection = CreateLayer(0, 0, width, height, 1, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);

	// ��ԏ�ŃG�t�F�N�g�\�����s�����C���[���쐬
	window->effect = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);

	// �e�N�X�`���p�̃��C���[���쐬
	window->texture = CreateLayer(0, 0, width, height, 1, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);

	// ���̃��C���[�Ń}�X�L���O�A�o�P�c�c�[���ł̃}�X�L���O�p
	window->mask = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);
	window->mask_temp = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);
	// �u���V�`�掞�̕s�����x�ݒ�p�T�[�t�F�[�X�A�C���[�W
	window->alpha_surface = cairo_image_surface_create_for_data(window->mask->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	window->alpha_cairo = cairo_create(window->alpha_surface);
	window->alpha_temp = cairo_image_surface_create_for_data(window->temp_layer->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	window->alpha_temp_cairo = cairo_create(window->alpha_temp);
	window->gray_mask_temp = cairo_image_surface_create_for_data(window->mask_temp->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	window->gray_mask_cairo = cairo_create(window->gray_mask_temp);

	// ��]�\���p�̒l���v�Z
	window->cos_value = 1;
	window->trans_x = - window->half_size + ((width / 2) * window->cos_value + (height / 2) * window->sin_value);
	window->trans_y = - window->half_size - ((width / 2) * window->sin_value - (height / 2) * window->cos_value);
	cairo_matrix_init_rotate(&matrix, 0);
	cairo_matrix_translate(&matrix, window->trans_x, window->trans_y);
	window->rotate = cairo_pattern_create_for_surface(window->disp_layer->surface_p);
	cairo_pattern_set_filter(window->rotate, CAIRO_FILTER_FAST);
	cairo_pattern_set_matrix(window->rotate, &matrix);

	// �\���p�Ɋg��E�k��������̈ꎟ�L��������
	window->disp_temp = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);

	// �A�N�e�B�u���C���[��艺�����������摜�̕ۑ��p
	window->under_active = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);
	(void)memcpy(window->under_active->pixels, window->back_ground, window->pixel_buf_size);

	progress_step = 1.0 / (window->num_layer + 1);
	// ���C���[���̓ǂݍ���
	window->layer = ReadOriginalFormatLayers(stream, progress_step, window, window->app, window->num_layer);

	// ���C���[�r���[�Ƀ��C���[��ǉ�
	layer = window->layer;
	for(i=0; i<window->num_layer; i++)
	{
		LayerViewAddLayer(layer, window->layer, window->app->layer_window.view, i+1);
		layer = layer->next;
	}

	// �A�N�e�B�u���C���[�͎b��I�Ɉ�ԉ���
	window->active_layer = window->layer;

	// �ǉ����̓ǂݍ���
	ReadOriginalFormatExtraData(stream, window);

	ChangeActiveLayer(window, window->active_layer);
	LayerViewSetActiveLayer(window->active_layer, window->app->layer_window.view);

	g_object_set_data(G_OBJECT(window->active_layer->widget->box), "signal_id",
		GUINT_TO_POINTER(g_signal_connect(G_OBJECT(window->active_layer->widget->box), "size-allocate",
			G_CALLBACK(Move2ActiveLayer), window->app)));
}

/*******************************************
* WriteOriginalFormat�֐�                  *
* �Ǝ��`���̃f�[�^�𐶐�����               *
* ����                                     *
* stream		: �������ݐ�̃X�g���[��   *
* write_func	: �������ݗp�̊֐��|�C���^ *
* window		: �`��̈�̏��           *
* add_thumbnail	: �T���l�C���̗L��         *
* compress		: ���k��                   *
*******************************************/
void WriteOriginalFormat(
	void* stream,
	stream_func write_func,
	DRAW_WINDOW* window,
	int add_thumbnail,
	int compress
)
{
	// �i���󋵕\���̃v���O���X�o�[
	GtkWidget *progress;
	// �E�B�W�F�b�g�\���C�x���g�ҋ@�p
	GdkEvent *queued_event;
	// �i����
	FLOAT_T current_progress;
	// �i���p�[�Z���e�[�W�\���p�̃o�b�t�@
	gchar show_text[16] = "";
	// �i���̍X�V��
	FLOAT_T progress_step = 1.0 / (window->num_layer + 1);
	// �s�N�Z���f�[�^���k�p
	MEMORY_STREAM_PTR image =
		CreateMemoryStream(window->pixel_buf_size*2);
	// �x�N�g���f�[�^���k�p
	MEMORY_STREAM_PTR vector_stream =
		CreateMemoryStream(window->pixel_buf_size*2);
	// �����������C���[�ւ̃|�C���^
	LAYER* layer = window->layer;
	// �����������C���[�̊�{���
	LAYER_BASE_DATA base;
	// �x�N�g�����C���[�f�[�^�����o�����̊�{���
	VECTOR_LINE_BASE_DATA line_base;
	// �e�L�X�g���C���[�f�[�^�����o�����̊�{���
	TEXT_LAYER_BASE_DATA text_base;
	// �K���f�[�^����p�̕�����
	char format_string[] = "Paint Soft KABURAGI";
	// ���O�Ƃ��̒���
	const char* name;
	uint16 name_length;
	// �������C���[�Z�b�g
	LAYER *layer_set = NULL;
	// ���C���[�Z�b�g�̊K�w
	int8 hierarchy = 0;
	// 64bit�̏ꍇsize_t�̃o�C�g�����ς�邽�ߕύX�p
	guint32 size_t_temp;
	int i;	// for���p�̃J�E���^

	// �i���p�[�Z���e�[�W��\��
	progress = window->app->progress;
	(void)sprintf(show_text, "0%%");
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), show_text);

	// �C�x���g���񂵂ă��b�Z�[�W��\��
#if MAJOR_VERSION == 1
	gdk_window_process_updates(window->app->status_bar->window, TRUE);
#else
	gdk_window_process_updates(gtk_widget_get_window(window->app->status_bar), TRUE);
#endif
	while(gdk_events_pending() != FALSE)
	{
		queued_event = gdk_event_get();
		gtk_main_iteration();
		if(queued_event != NULL)
		{
#if MAJOR_VERSION == 1
			if(queued_event->any.window == window->app->status_bar->window
#else
			if(queued_event->any.window == gtk_widget_get_window(window->app->status_bar)
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

	// �t�@�C������p�̕�����������o��
	(void)write_func(format_string, 1, sizeof(format_string)/sizeof(*format_string), stream);
	size_t_temp = 0;
	// �t�@�C���o�[�W�����������o��
	size_t_temp = FILE_VERSION;
	(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);

	// �T���l�C���̍쐬�Ə�������
	{
		uint8 has_thumbnail = (add_thumbnail == 0) ? 0 : 1;
		uint8 r;
		(void)write_func(&has_thumbnail, sizeof(has_thumbnail), 1, stream);
		if(add_thumbnail != 0)
		{
			uint8 *pixels = (uint8*)MEM_ALLOC_FUNC(THUMBNAIL_SIZE*THUMBNAIL_SIZE*4);
			cairo_surface_t *surface_p = cairo_image_surface_create_for_data(
				pixels, CAIRO_FORMAT_ARGB32, THUMBNAIL_SIZE, THUMBNAIL_SIZE, THUMBNAIL_SIZE*4);
			cairo_t *cairo_p = cairo_create(surface_p);
			FLOAT_T zoom_x, zoom_y, zoom;

			zoom_x = THUMBNAIL_SIZE / (FLOAT_T)window->width;
			zoom_y = THUMBNAIL_SIZE / (FLOAT_T)window->height;

			if(zoom_x < zoom_y)
			{
				zoom = zoom_x;
			}
			else
			{
				zoom = zoom_y;
			}

			if(zoom > 1)
			{
				zoom = 1;
			}

			cairo_scale(cairo_p, zoom, zoom);
			cairo_set_source_surface(cairo_p, window->mixed_layer->surface_p, 0, 0);
			cairo_paint(cairo_p);

			for(i=0; i<THUMBNAIL_SIZE*THUMBNAIL_SIZE; i++)
			{
				r = pixels[i*4];
				pixels[i*4] = pixels[i*4+2];
				pixels[i*4+2] = r;
			}

			WritePNGStream(image, (stream_func)MemWrite, NULL, pixels,
				THUMBNAIL_SIZE, THUMBNAIL_SIZE, THUMBNAIL_SIZE*4, 4, 0, 5);

			size_t_temp = (guint32)image->data_point;
			(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
			(void)write_func(image->buff_ptr, 1, image->data_point, stream);

			cairo_destroy(cairo_p);
			cairo_surface_destroy(surface_p);
			MEM_FREE_FUNC(pixels);
			image->data_point = 0;
		}
	}

	// �`�����l�����A�J���[���[�h�������o��
	(void)write_func(&window->channel, sizeof(window->channel), 1, stream);
	(void)write_func(&window->color_mode, sizeof(window->color_mode), 1, stream);

	// �I���W�i���̕��A�����������o��
	size_t_temp = window->original_width;
	(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
	size_t_temp = window->original_height;
	(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);

	// ���A����(4�̔{��)�������o��
	size_t_temp = window->width;
	(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
	size_t_temp = window->height;
	(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);

	// ���C���[�̐��������o��
	(void)write_func(&window->num_layer, sizeof(window->num_layer), 1, stream);

	// �w�i�摜�̃s�N�Z���f�[�^�������o��
	WritePNGStream(
		image, (stream_func)MemWrite, NULL, window->back_ground,
		window->width, window->height, window->stride, window->channel,
		0, compress
	);
	// �f�[�^�o�C�g�������o��
	size_t_temp = (uint32)image->data_point;
	(void)write_func(&size_t_temp, sizeof(size_t_temp),
		1, stream);
	// �f�[�^�����o��
	(void)write_func(image->buff_ptr, 1, image->data_point, stream);

	// �i���󋵂��X�V
	current_progress = progress_step;
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), current_progress);
	(void)sprintf(show_text, "%.0f%%", current_progress * 100);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), show_text);
#if MAJOR_VERSION == 1
	gdk_window_process_updates(progress->window, FALSE);
#else
	gdk_window_process_updates(gtk_widget_get_window(progress), FALSE);
#endif
	while(gdk_events_pending() != FALSE)
	{
		queued_event = gdk_event_get();
		gtk_main_iteration();
		if(queued_event != NULL)
		{
#if MAJOR_VERSION == 1
			if(queued_event->any.window == progress->window
#else
			if(queued_event->any.window == gtk_widget_get_window(progress)
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

	// ���C���[�̃^�C�v�ɍ��킹�ď��������f�[�^��ύX
	do
	{
		// ���C���[���������o��
		name_length = (uint16)strlen(layer->name) + 1;
		(void)write_func(&name_length, sizeof(name_length), 1, stream);
		(void)write_func(layer->name, 1, name_length, stream);

		// ���C���[�Z�b�g�̊K�w�𒲂ׂ�
		if(layer == layer_set && layer_set != NULL)
		{
			layer_set = layer_set->layer_set;
			hierarchy--;
		}
		else if(layer->layer_set != layer_set)
		{
			layer_set = layer->layer_set;
			hierarchy++;
		}

		// ���C���[��{���������o��
		base.layer_type = layer->layer_type;
		base.layer_mode = layer->layer_mode;
		base.x = layer->x;
		base.y = layer->y;
		base.width = layer->width;
		base.height = layer->height;
		base.flags = layer->flags;
		base.alpha = layer->alpha;
		base.channel = layer->channel;
		base.layer_set = hierarchy;

		(void)write_func(&base.layer_type, sizeof(base.layer_type), 1, stream);
		(void)write_func(&base.layer_mode, sizeof(base.layer_mode), 1, stream);
		(void)write_func(&base.x, sizeof(base.x), 1, stream);
		(void)write_func(&base.y, sizeof(base.y), 1, stream);
		(void)write_func(&base.width, sizeof(base.width), 1, stream);
		(void)write_func(&base.height, sizeof(base.height), 1, stream);
		(void)write_func(&base.flags, sizeof(base.flags), 1, stream);
		(void)write_func(&base.alpha, sizeof(base.alpha), 1, stream);
		(void)write_func(&base.channel, sizeof(base.channel), 1, stream);
		(void)write_func(&base.layer_set, sizeof(base.layer_set), 1, stream);

		//(void)write_func(&base, sizeof(base), 1, stream);

		// ���C���[�̃^�C�v�ŏ����؂�ւ�
		switch(layer->layer_type)
		{
		case TYPE_NORMAL_LAYER:	// �ʏ탌�C���[
			// �s�N�Z���f�[�^��PNG���k���ď�������
			(void)MemSeek(image, 0, SEEK_SET);
			WritePNGStream(image, (stream_func)MemWrite, NULL, layer->pixels,
				layer->width, layer->height, layer->stride, layer->channel,
				0, compress
			);
			size_t_temp = (guint32)image->data_point;
			(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
			(void)write_func(image->buff_ptr, 1, image->data_point, stream);
			break;
		case TYPE_VECTOR_LAYER:	// �x�N�g�����C���[
			// �x�N�g���̍��W�A�T�C�Y�A�M���A�F����x�������ɗ��߂�
				// ���C���̐����������ޗ̈���J���Ă���
			(void)MemSeek(image, sizeof(guint32), SEEK_SET);
			// �x�N�g�����C���[�̃t���O�������o��
			size_t_temp = layer->layer_data.vector_layer_p->flags;
			(void)MemWrite(&size_t_temp, sizeof(size_t_temp), 1, image);
			{
				// ZIP���k�p
				z_stream compress_stream;
				// �x�N�g�����C���[�̃f�[�^�o�C�g��
				guint32 vector_data_size;
				guint32 data_size;
				// ���C���̐�
				guint32 num_line = 0;
				// ���C���f�[�^�����o���p
				VECTOR_LINE *line = layer->layer_data.vector_layer_p->base->next;

				while(line != NULL)
				{
					line_base.vector_type = line->vector_type;
					line_base.flags = line->flags;
					line_base.num_points = line->num_points;
					line_base.blur = line->blur;
					line_base.outline_hardness = line->outline_hardness;

					(void)MemWrite(&line_base.vector_type, sizeof(line_base.vector_type), 1, image);
					(void)MemWrite(&line_base.flags, sizeof(line_base.flags), 1, image);
					(void)MemWrite(&line_base.num_points, sizeof(line_base.num_points), 1, image);
					(void)MemWrite(&line_base.blur, sizeof(line_base.blur), 1, image);
					(void)MemWrite(&line_base.outline_hardness, sizeof(line_base.outline_hardness), 1, image);
					for(i=0; i<line->num_points; i++)
					{
						(void)MemWrite(&line->points[i].vector_type, sizeof(line->points->vector_type), 1, image);
						(void)MemWrite(&line->points[i].flags, sizeof(line->points->flags), 1, image);
						(void)MemWrite(&line->points[i].color, sizeof(line->points->color), 1, image);
						(void)MemWrite(&line->points[i].pressure, sizeof(line->points->pressure), 1, image);
						(void)MemWrite(&line->points[i].size, sizeof(line->points->size), 1, image);
						(void)MemWrite(&line->points[i].x, sizeof(line->points->x), 1, image);
						(void)MemWrite(&line->points[i].y, sizeof(line->points->y), 1, image);
					}

					line = line->next;
					num_line++;
				}

				// �f�[�^�T�C�Y�L��
				vector_data_size = (guint32)image->data_point;
				// ���C���̐������o��
				MemSeek(image, 0, SEEK_SET);
				(void)MemWrite(&num_line, sizeof(num_line), 1, image);

				// �X�g���[���̃T�C�Y������Ȃ���΍Ċm��
				if(vector_stream->data_size < image->data_size)
				{
					vector_stream->data_size = image->data_size;
					vector_stream->buff_ptr =
						(uint8*)MEM_REALLOC_FUNC(vector_stream->buff_ptr, vector_stream->data_size);
				}

				// �x�N�g���f�[�^��ZIP���k����
					// ���k�p�X�g���[���̃f�[�^���Z�b�g
				compress_stream.zalloc = Z_NULL;
				compress_stream.zfree = Z_NULL;
				compress_stream.opaque = Z_NULL;
				(void)deflateInit(&compress_stream, compress);
				compress_stream.avail_in = (uInt)vector_data_size;
				compress_stream.next_in = image->buff_ptr;
				compress_stream.avail_out = (uInt)vector_stream->data_size;
				compress_stream.next_out = vector_stream->buff_ptr;
				// ���k���s
				(void)deflate(&compress_stream, Z_FINISH);

				// ���k��̃f�[�^�T�C�Y����������
				data_size = (guint32)(vector_stream->data_size - compress_stream.avail_out
					+ sizeof(vector_data_size));
				(void)write_func(&data_size, sizeof(data_size), 1, stream);
				// ���k�O�̃f�[�^�T�C�Y����������
				(void)write_func(&vector_data_size, sizeof(vector_data_size), 1, stream);
				// ���k�����f�[�^����������
				(void)write_func(vector_stream->buff_ptr, 1,
					data_size - sizeof(data_size), stream);

				// ���k�p�X�g���[�����J��
				(void)deflateEnd(&compress_stream);
			}
			break;
		case TYPE_TEXT_LAYER:	// �e�L�X�g���C���[
			// �����`��̈�̍��W�A���A�����A�����T�C�Y
				// �t�H���g�t�@�C�����A�F���������o��
			// �f�[�^�͈�x�������ɗ��߂�
			(void)MemSeek(image, 0, SEEK_SET);

			// �e�L�X�g���C���[�̊�{���(���W�A�c������)����������
			text_base.x = layer->layer_data.text_layer_p->x;
			text_base.y = layer->layer_data.text_layer_p->y;
			text_base.width = layer->layer_data.text_layer_p->width;
			text_base.height = layer->layer_data.text_layer_p->height;
			text_base.font_size = layer->layer_data.text_layer_p->font_size;
			(void)memcpy(text_base.color, layer->layer_data.text_layer_p->color, 3);
			text_base.flags = layer->layer_data.text_layer_p->flags;

			(void)MemWrite(&text_base.x, sizeof(text_base.x), 1, image);
			(void)MemWrite(&text_base.y, sizeof(text_base.y), 1, image);
			(void)MemWrite(&text_base.width, sizeof(text_base.width), 1, image);
			(void)MemWrite(&text_base.height, sizeof(text_base.height), 1, image);
			(void)MemWrite(&text_base.font_size, sizeof(text_base.font_size), 1, image);
			(void)MemWrite(&text_base.color, sizeof(text_base.color), 1, image);
			(void)MemWrite(&text_base.flags, sizeof(text_base.flags), 1, image);
			// �t�H���g�̖��O����������
			name = pango_font_family_get_name(
				window->app->font_list[layer->layer_data.text_layer_p->font_id]);
			name_length = (uint16)strlen(name) + 1;
			(void)MemWrite(&name_length, sizeof(name_length), 1, image);
			(void)MemWrite((void*)name, 1, name_length, image);

			// �e�L�X�g�f�[�^����������
			name_length = (uint16)strlen(layer->layer_data.text_layer_p->text) + 1;
			(void)MemWrite(&name_length, sizeof(name_length), 1, image);
			(void)MemWrite(layer->layer_data.text_layer_p->text,
				1, name_length, image);

			// �o�C�g������������ł���
			size_t_temp = (guint32)image->data_point;
			(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
			// �f�[�^�������o��
			(void)write_func(image->buff_ptr, 1, image->data_point, stream);

			break;
		}

		// �ǉ�������������
			// �ǉ����̐��������o��
		(void)write_func(&layer->num_extra_data, sizeof(layer->num_extra_data), 1, stream);
		for(i=0; i<layer->num_extra_data; i++)
		{
			// �f�[�^�̖��O�̒����������o��
			name_length = (uint16)(strlen(layer->extra_data[i].name) + 1);
			(void)write_func(&name_length, sizeof(name_length), 1, stream);
			(void)write_func(layer->extra_data[i].name, 1, name_length, stream);
			size_t_temp = (guint32)layer->extra_data[i].data_size;
			(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
			(void)write_func(layer->extra_data[i].data, 1, layer->extra_data[i].data_size, stream);
		}

		layer = layer->next;

		// �i���󋵂��X�V
		current_progress += progress_step;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), current_progress);
		(void)sprintf(show_text, "%.0f%%", current_progress * 100);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), show_text);
#if MAJOR_VERSION == 1
		gdk_window_process_updates(progress->window, FALSE);
#else
		gdk_window_process_updates(gtk_widget_get_window(progress), FALSE);
#endif
		while(gdk_events_pending() != FALSE)
		{
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event != NULL)
			{
#if MAJOR_VERSION == 1
				if(queued_event->any.window == progress->window
#else
				if(queued_event->any.window == gtk_widget_get_window(progress)
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
	} while(layer != NULL);

	// �ǉ����̏����o��
	{
		// �f�[�^�̃^�C�v
		guint32 tag;

		// �A�N�e�B�u���C���[
		tag = 'actv';
		tag = GUINT32_TO_BE(tag);

		(void)write_func(&tag, sizeof(tag), 1, stream);
		size_t_temp = (guint32)strlen(window->active_layer->name) + 1;
		(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
		(void)write_func(window->active_layer->name, 1, size_t_temp, stream);

		// �I��͈�
		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
		{
			tag = 'slct';
			tag = GINT32_TO_BE(tag);

			(void)write_func(&tag, sizeof(tag), 1, stream);

			(void)MemSeek(image, 0, SEEK_SET);
			WritePNGStream((void*)image, (stream_func)MemWrite, NULL,
				window->selection->pixels, window->width, window->height, window->width, 1, 0, compress);
			size_t_temp = (guint32)image->data_point;
			(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
			(void)write_func(image->buff_ptr, 1, size_t_temp, stream);
		}

		// �𑜓x
		if(window->resolution > 0)
		{
			tag = 'resl';
			tag = GINT32_TO_BE(tag);

			(void)write_func(&tag, sizeof(tag), 1, stream);
			size_t_temp = sizeof(size_t_temp);
			(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
			size_t_temp = window->resolution;
			(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
		}

		// 2�߂̔w�i�F
		{
			uint8 second_bg[4];

#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			second_bg[0] = window->second_back_ground[2];
			second_bg[1] = window->second_back_ground[1];
			second_bg[2] = window->second_back_ground[0];
#else
			second_bg[0] = window->second_back_ground[0];
			second_bg[1] = window->second_back_ground[1];
			second_bg[2] = window->second_back_ground[2];
#endif
			second_bg[3] = ((window->flags & DRAW_WINDOW_SECOND_BG) == 0) ? 0 : 1;

			tag = '2ndb';
			tag = GINT32_TO_BE(tag);

			(void)write_func(&tag, sizeof(tag), 1, stream);
			size_t_temp = 4;
			(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
			(void)write_func(second_bg, 1, 4, stream);
		}

		// ICC�v���t�@�C��
		if(window->icc_profile_data != NULL)
		{
			// ICC�v���t�@�C���̓��e�������o��
			tag = 'iccp';
			tag = GINT32_TO_BE(tag);

			(void)write_func(&tag, sizeof(tag), 1, stream);
			size_t_temp = window->icc_profile_size;
			(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
			(void)write_func(window->icc_profile_data, 1, window->icc_profile_size, stream);
		}
		if(window->icc_profile_path != NULL)
		{
			// ICC�v���t�@�C���̃t�@�C���p�X�������o��
			tag = 'iccf';
			tag = GINT32_TO_BE(tag);

			(void)write_func(&tag, sizeof(tag), 1, stream);
			size_t_temp = (guint32)strlen(window->icc_profile_path) + 1;
			(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
			(void)write_func(window->icc_profile_path, 1, size_t_temp, stream);
		}
	}

	DeleteMemoryStream(image);
	DeleteMemoryStream(vector_stream);

	// �v���O���X�o�[�����Z�b�g
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), "");
}

/*************************************************
* WritePhotoShopDocument�֐�                     *
* PSD�`���ŏ�������                              *
* ����                                           *
* stream		: �������ݐ�̃X�g���[��         *
* write_func	: �������ݗp�̊֐��|�C���^       *
* seek_func		: �V�[�N�p�̊֐��|�C���^         *
* tell_func		: �V�[�N�ʒu�擾�p�̊֐��|�C���^ *
* window		: �`��̈�̏��                 *
*************************************************/
void WritePhotoShopDocument(
	void* stream,
	stream_func write_func,
	seek_func seek_func,
	long (*tell_func)(void*),
	DRAW_WINDOW* window
)
{
	// ���C���[�f�[�^�����o���p
	LAYER* layer = window->layer;
	// ���̃��C���[�Ń}�X�N���Ă��珑���o���p
	LAYER *mask;
	// ���C���[��(�V�X�e���̃R�[�h�ɕϊ�����)
	char* name;
	// �s�N�Z���f�[�^
	uint8 *byte_data = (uint8*)MEM_ALLOC_FUNC(window->pixel_buf_size);
	// ���C���[�f�[�^�̃o�C�g��
	size_t data_size;
	// 4�o�C�g�o�b�t�@
	guint32 dw;
	// 2�o�C�g�o�b�t�@
	uint16 word;
	int16 signed_word;
	// 1�o�C�g�o�b�t�@
	uint8 byte;
	// ���C���[���̒���
	uint8 name_length;
	// �p�f�B���O����o�C�g��
	uint8 padding;
	// �f�[�^���ォ�珑�����ޗp�̈ʒu�L��
	long before_point, data_size_point;
	// ExtraData���������ވʒu
	long extra_point;
	// for���p�̃J�E���^
	int i, j, k;

	// Header����������
		// Signature("8BPS")�����o��
	(void)write_func("8BPS", 1, 4, stream);
	// Version
	word = 1;
	word = GUINT16_TO_BE(word);
	(void)write_func(&word, sizeof(word), 1, stream);
	// Reserved(6�o�C�g)
	word = 0;
	for(i=0; i<3; i++)
	{
		(void)write_func(&word, sizeof(word), 1, stream);
	}
	// Channels
	word = window->channel;
	word = GUINT16_TO_BE(word);
	(void)write_func(&word, sizeof(word), 1, stream);
	// Rows(�摜�̍���)
	dw = window->height;
	dw = GUINT32_TO_BE(dw);
	(void)write_func(&dw, sizeof(dw), 1, stream);
	// Columns(�摜�̕�)
	dw = window->width;
	dw = GUINT32_TO_BE(dw);
	(void)write_func(&dw, sizeof(dw), 1, stream);
	// �s�N�Z���f�[�^�[�x
	word = 8;
	word = GUINT16_TO_BE(word);
	(void)write_func(&word, sizeof(word), 1, stream);
	// �J���[���[�h(RGB)
	word = 3;
	word = GUINT16_TO_BE(word);
	(void)write_func(&word, sizeof(word), 1, stream);

	// Color Mode Data Block
	dw = 0;
	dw = GUINT32_TO_BE(dw);
	(void)write_func(&dw, sizeof(dw), 1, stream);

	// Image Resource Block (�f�[�^����)
	dw = 0;
	dw = GUINT32_TO_BE(dw);
	(void)write_func(&dw, sizeof(dw), 1, stream);

	// Layer and Mask Information Block, Image Data
		// Block Length Layer and Mask Information Block��nSize�̗̈���J���Ă���
	data_size_point = tell_func(stream);
	dw = 0;
	(void)write_func(&dw, sizeof(dw), 1, stream);
	(void)write_func(&dw, sizeof(dw), 1, stream);

	// Layer Num�����o��
	signed_word = window->num_layer;
	if(window->channel == 4)
	{
		signed_word = - signed_word;
	}
	signed_word = GINT16_TO_BE(signed_word);
	(void)write_func(&signed_word, sizeof(signed_word), 1, stream);
	// ���C���[�f�[�^�����o��
	for(i=0; i<window->num_layer; i++)
	{
		// ���O���V�X�e���R�[�h�ɕϊ�
		name = g_convert(layer->name, -1, window->app->system_code, "UTF-8",
			NULL, NULL, NULL);

		// Top
		dw = layer->y;
		dw = GUINT32_TO_BE(dw);
		(void)write_func(&dw, sizeof(dw), 1, stream);
		// Left
		dw = layer->x;
		dw = GUINT32_TO_BE(dw);
		(void)write_func(&dw, sizeof(dw), 1, stream);
		// Bottom
		dw = layer->height - layer->y;
		dw = GUINT32_TO_BE(dw);
		(void)write_func(&dw, sizeof(dw), 1, stream);
		// Right
		dw = layer->width - layer->x;
		dw = GUINT32_TO_BE(dw);
		(void)write_func(&dw, sizeof(dw), 1, stream);
		// Channels
		word = layer->channel;
		word = GUINT16_TO_BE(word);
		(void)write_func(&word, sizeof(word), 1, stream);

		if(layer->channel == 4)
		{
			word = 0xFFFF;
			(void)write_func(&word, sizeof(word), 1, stream);
			// LengthOfChannelData
			dw = sizeof(word) + layer->width*layer->height;
			dw = GUINT32_TO_BE(dw);
			(void)write_func(&dw, sizeof(dw), 1, stream);
		}
		for(j=0; j<3; j++)
		{
			// ChannelID
			word = j;
			word = GUINT16_TO_BE(word);
			(void)write_func(&word, sizeof(word), 1, stream);
			// LengthOfChannelData
			dw = sizeof(word) + layer->width*layer->height;
			dw = GUINT32_TO_BE(dw);
			(void)write_func(&dw, sizeof(dw), 1, stream);
		}
		
		// Blend Mode Signature ("8BIM")
		(void)write_func("8BIM", 1, 4, stream);
		// Blend Mode Key
		{
			char* key;
			switch(layer->layer_mode)
			{
			case LAYER_BLEND_NORMAL:
				key = "norm";
				break;
			case LAYER_BLEND_ADD:
				key = "lddg";
				break;
			case LAYER_BLEND_MULTIPLY:
				key = "mul ";
				break;
			case LAYER_BLEND_SCREEN:
				key = "scrn";
				break;
			case LAYER_BLEND_OVERLAY:
				key = "over";
				break;
			case LAYER_BLEND_LIGHTEN:
				key = "lite";
				break;
			case LAYER_BLEND_DARKEN:
				key = "dark";
				break;
			case LAYER_BLEND_DODGE:
				key = "div ";
				break;
			case LAYER_BLEND_BURN:
				key = "idiv";
				break;
			case LAYER_BLEND_HARD_LIGHT:
				key = "hLit";
				break;
			case LAYER_BLEND_SOFT_LIGHT:
				key = "sLit";
				break;
			case LAYER_BLEND_DIFFERENCE:
				key = "diff";
				break;
			case LAYER_BLEND_HSL_HUE:
				key = "hue ";
				break;
			case LAYER_BLEND_HSL_SATURATION:
				key = "sat ";
				break;
			case LAYER_BLEND_HSL_COLOR:
				key = "colr";
				break;
			case LAYER_BLEND_HSL_LUMINOSITY:
				key = "lum ";
				break;
			default:
				key = "norm";
			}
			(void)write_func(key, 1, 4, stream);
		}

		// Opacity
		byte = (uint8)(layer->alpha * 0.01 * 255);
		(void)write_func(&byte, 1, 1, stream);
		// Clipping
		byte = 0;
		(void)write_func(&byte, 1, 1, stream);
		// Flags
		byte = 0;
		if((layer->flags & LAYER_LOCK_OPACITY) != 0)
		{
			byte |= 0x01;
		}
		if((layer->flags & LAYER_FLAG_INVISIBLE) != 0)
		{
			byte |= 0x02;
		}
		if(layer->layer_type == TYPE_LAYER_SET)
		{
			byte |= 0x10;
		}
		(void)write_func(&byte, 1, 1, stream);
		// Padding
		byte = 0;
		(void)write_func(&byte, 1, 1, stream);
		// ExtraDataSize
		padding = (uint8)strlen(name);
		name_length = ((padding + 1 + 3) & ~0x03) - 1;
		padding = name_length - padding;
		extra_point = tell_func(stream) - data_size_point;
		dw = (uint32)extra_point;
		extra_point += data_size_point;
		dw = GUINT32_TO_BE(dw);
		(void)write_func(&dw, sizeof(dw), 1, stream);

		// Layer Mask
		dw = 0;
		(void)write_func(&dw, sizeof(dw), 1, stream);

		// Blending Ranges
		dw = 0;
		(void)write_func(&dw, sizeof(dw), 1, stream);

		// Name
		byte = name_length;
		(void)write_func(&byte, 1, 1, stream);
		(void)write_func(name, 1, name_length-padding, stream);
		byte = 0;
		for(j=0; j<padding; j++)
		{
			(void)write_func(&byte, 1, 1, stream);
		}

		// �ۗ��ɂ��Ă�����Extra Data�������o��
		before_point = tell_func(stream);
		(void)seek_func(stream, extra_point, SEEK_SET);
		dw = (uint32)(before_point - extra_point - sizeof(dw));
		dw = GUINT32_TO_BE(dw);
		(void)write_func(&dw, sizeof(dw), 1, stream);
		(void)seek_func(stream, before_point, SEEK_SET);

		g_free(name);
		layer = layer->next;
	}

	word = 0;
	// �摜�f�[�^�����o��
	layer = window->layer;
	for(i=0; i<window->num_layer; i++)
	{
		// ���̃��C���[�ł̃}�X�N����
		if((layer->flags & LAYER_MASKING_WITH_UNDER_LAYER) == 0)
		{
			// �����o���`�����l���̃C���f�b�N�X
			int channel;

			if(layer->channel == 4)
			{
				// Compression
				(void)write_func(&word, sizeof(word), 1, stream);

				// �s�N�Z���f�[�^
				for(j=0; j<layer->width*layer->height; j++)
				{
					byte_data[j] = layer->pixels[j*layer->channel+3];
				}
				(void)write_func(byte_data, 1, layer->width*layer->height, stream);
			}

			(void)memcpy(window->temp_layer->pixels, window->mixed_layer->pixels, window->pixel_buf_size);
			cairo_set_source_surface(window->temp_layer->cairo_p, layer->surface_p, 0, 0);
			cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
			cairo_paint(window->temp_layer->cairo_p);

			// �`�����l���f�[�^�����o��
			for(j=0; j<3; j++)
			{
				if(j == 0)
				{
					channel = 2;
				}
				else if(j == 2)
				{
					channel = 0;
				}
				else
				{
					channel = j;
				}

				// Compression
				(void)write_func(&word, sizeof(word), 1, stream);

				// �s�N�Z���f�[�^
				for(k=0; k<layer->width*layer->height; k++)
				{
					byte_data[k] = window->temp_layer->pixels[k*layer->channel+channel];
					//byte_data[k] = layer->pixels[k*layer->channel+j];
				}
				(void)write_func(byte_data, 1, layer->width*layer->height, stream);
			}
		}
		else
		{	// �}�X�N�L��
			mask = layer;
			while((mask->flags & LAYER_MASKING_WITH_UNDER_LAYER) != 0 && mask != NULL)
			{
				mask = mask->prev;
			}

			(void)memset(window->temp_layer->pixels, 0, window->pixel_buf_size);
			cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
			cairo_set_source_surface(window->temp_layer->cairo_p, layer->surface_p, 0, 0);
			cairo_mask_surface(window->temp_layer->cairo_p, mask->surface_p, 0, 0);

			(void)memcpy(window->mask_temp->pixels, window->mixed_layer->pixels, window->pixel_buf_size);
			cairo_set_source_surface(window->mask_temp->cairo_p, layer->surface_p, 0, 0);
			cairo_set_operator(window->mask_temp->cairo_p, CAIRO_OPERATOR_OVER);
			cairo_paint(window->mask_temp->cairo_p);

			if(layer->channel == 4)
			{
				// Compression
				(void)write_func(&word, sizeof(word), 1, stream);

				// �s�N�Z���f�[�^
				for(j=0; j<layer->width*layer->height; j++)
				{
					byte_data[j] = window->temp_layer->pixels[j*layer->channel+3];
				}
				(void)write_func(byte_data, 1, layer->width*layer->height, stream);
			}
			// �`�����l���f�[�^�����o��
			for(j=0; j<3; j++)
			{
				int channel_id = j;
				if(channel_id == 0)
				{
					channel_id = 2;
				}
				else if(channel_id == 2)
				{
					channel_id = 0;
				}

				// Compression
				(void)write_func(&word, sizeof(word), 1, stream);

				// �s�N�Z���f�[�^
				for(k=0; k<layer->width*layer->height; k++)
				{
					byte_data[k] = window->mask_temp->pixels[k*layer->channel+channel_id];
				}
				(void)write_func(byte_data, 1, layer->width*layer->height, stream);
			}
		}

		//(void)write_func(&word, 1, 1, stream);
		layer = layer->next;
	}

	// Layer and Mask Information Block, Image Data�̃T�C�Y����������
	data_size = tell_func(stream);
	extra_point = (long)data_size;
	(void)seek_func(stream, data_size_point, SEEK_SET);
	dw = (uint32)(data_size - sizeof(uint32));
	dw = GUINT32_TO_BE(dw);
	(void)write_func(&dw, sizeof(dw), 1, stream);
	dw = (uint32)(data_size - sizeof(uint32)*2);
	dw = GUINT32_TO_BE(dw);
	(void)write_func(&dw, sizeof(dw), 1, stream);
	(void)seek_func(stream, extra_point, SEEK_SET);

	// �t�B���^�[�f�[�^����0�Ŗ��߂�
	(void)memset(byte_data, 0, 32);
	(void)write_func(byte_data, 1, 32, stream);

	// �S�̂̃}�X�N�͖����̂�0�������o��
	dw = 0;
	(void)write_func(&dw, sizeof(dw), 1, stream);

	// ���C���[�𓧖��������c���č���
	//layer = MixLayerForSave(window);
	layer = window->mixed_layer;
	if(layer->channel == 4)
	{
		// Compression
		word = 0;
		(void)write_func(&word, sizeof(word), 1, stream);
		for(j=0; j<layer->width*layer->height; j++)
		{
			byte_data[j] = layer->pixels[j*layer->channel+3];
		}
		(void)write_func(byte_data, 1, layer->width*layer->height, stream);
	}
	for(j=0; j<3; j++)
	{
		int channel_id = j;
		if(channel_id == 0)
		{
			channel_id = 2;
		}
		else if(channel_id == 2)
		{
			channel_id = 0;
		}

		// Compression
		word = 0;
		(void)write_func(&word, sizeof(word), 1, stream);
		for(k=0; k<layer->width*layer->height; k++)
		{
			byte_data[k] = layer->pixels[k*layer->channel+channel_id];
		}
		(void)write_func(byte_data, 1, layer->width*layer->height, stream);
	}
	//DeleteLayer(&layer);
	MEM_FREE_FUNC(byte_data);

	(void)write_func(&word, 1, 1, stream);
}

/**************************************************************
* ReadTiffTagData�֐�                                         *
* TIFF�t�@�C���̃^�O����ǂݍ���                            *
* ����                                                        *
* system_file_path	: ���sOS�ł̃t�@�C���p�X                  *
* resolution		: �𑜓x(DPI)���i�[����A�h���X           *
* icc_profile_data	: ICC�v���t�@�C���̃f�[�^���󂯂�|�C���^ *
* icc_profile_size	: ICC�v���t�@�C���̃f�[�^�̃o�C�g��       *
**************************************************************/
void ReadTiffTagData(
	const char* system_file_path,
	int* resolution,
	uint8** icc_profile_data,
	int32* icc_profile_size
)
{
	TIFF *tiff;

	tiff = TIFFOpen(system_file_path, "r");

	if(resolution != NULL)
	{
		float res_x, res_y;
		TIFFGetField(tiff, TIFFTAG_XRESOLUTION, &res_x);
		TIFFGetField(tiff, TIFFTAG_YRESOLUTION, &res_y);

		*resolution = (int)((res_x > res_y) ? res_x : res_y);
	}

	if(icc_profile_data != NULL && icc_profile_size != NULL)
	{
		guint32 length;
		void *data;

		TIFFGetField(tiff, TIFFTAG_ICCPROFILE, &length, &data);
		*icc_profile_data = (uint8*)MEM_ALLOC_FUNC(length);
		(void)memcpy(icc_profile_data, data, length);
		_TIFFfree(data);

		*icc_profile_size = length;
	}
}

/*******************************************************
* WriteTiff�֐�                                        *
* TIFF�`���ŉ摜�������o��                             *
* ����                                                 *
* system_file_path	: OS�̃t�@�C���V�X�e���ɑ������p�X *
* window			: �摜�������o���`��̈�̏��     *
* write_aplha		: �������������o�����ۂ�         *
* compress			: ���k���s�����ۂ�                 *
* profile_data		: ICC�v���t�@�C���̃f�[�^          *
*					  (�����o���Ȃ��ꍇ��NULL)         *
* profile_data_size	: ICC�v���t�@�C���̃f�[�^�o�C�g��  *
*******************************************************/
void WriteTiff(
	const char* system_file_path,
	DRAW_WINDOW* window,
	gboolean write_alpha,
	gboolean compress,
	void* profile_data,
	guint32 profile_data_size
)
{
	APPLICATION *app = window->app;
	LAYER *input_layer = window->mixed_layer;
	int default_channel = 3;
	float resolution;
	TIFF *out;
	cmsHTRANSFORM *color_transform = NULL;
	cmsColorSpaceSignature color_space = cmsSigRgbData;
	int stride;
	int i;

	if((out = TIFFOpen(system_file_path, "w")) == NULL)
	{
		return;
	}

	TIFFSetField (out, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	if((app->flags & APPLICATION_DISPLAY_SOFT_PROOF) != 0)
	{
		color_space = cmsGetColorSpace(app->output_icc);
		switch(color_space)
		{
		case cmsSigCmykData:
			default_channel = 4;
			TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_SEPARATED);
			TIFFSetField(out, TIFFTAG_INKSET, INKSET_CMYK);
			if(write_alpha == FALSE)
			{
				stride = window->width * 4;
				color_transform = cmsCreateTransform(app->input_icc, TYPE_BGRA_8,
					app->output_icc, TYPE_CMYK_8, INTENT_RELATIVE_COLORIMETRIC, 0);
			}
			else
			{
				stride = window->width * 5;
				color_transform = cmsCreateTransform(app->input_icc, TYPE_BGRA_8,
					app->output_icc, TYPE_CMYKA_8, INTENT_RELATIVE_COLORIMETRIC, 0);
			}
			break;
		case cmsSigRgbData:
			if(write_alpha == FALSE)
			{
				color_transform = cmsCreateTransform(app->input_icc, TYPE_BGRA_8,
					app->output_icc, TYPE_RGB_8, INTENT_RELATIVE_COLORIMETRIC, 0);
				stride = window->width * 3;
			}
			else
			{
				color_transform = cmsCreateTransform(app->input_icc, TYPE_BGRA_8,
					app->output_icc, TYPE_RGBA_8, INTENT_RELATIVE_COLORIMETRIC, 0);
				stride = window->width * 4;
			}
			TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
			break;
		default:
			if(write_alpha == FALSE)
			{
				stride = window->width * 3;
			}
			else
			{
				stride = window->width * 4;
			}
			stride = window->width * 4;
			TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
		}
	}
	else
	{
		if(write_alpha == FALSE)
		{
			stride = window->width * 3;
		}
		else
		{
			stride = window->width * 4;
		}
		TIFFSetField (out, TIFFTAG_PHOTOMETRIC, PHOTOMETRIC_RGB);
	}

	if(write_alpha != FALSE)
	{
		guint16 tmp[] = {EXTRASAMPLE_ASSOCALPHA};

		TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, default_channel + 1);
		TIFFSetField(out, TIFFTAG_EXTRASAMPLES, 1, tmp);

		input_layer = MixLayerForSave(window);
	}
	else
	{
		TIFFSetField(out, TIFFTAG_SAMPLESPERPIXEL, default_channel);
	}
	TIFFSetField(out, TIFFTAG_BITSPERSAMPLE, 8);

	TIFFSetField(out, TIFFTAG_IMAGEWIDTH, window->width);
	TIFFSetField(out, TIFFTAG_IMAGELENGTH, window->height);
	TIFFSetField(out, TIFFTAG_COMPRESSION, compress ? COMPRESSION_LZW : COMPRESSION_NONE);
	TIFFSetField(out, TIFFTAG_SOFTWARE, "PaintSoftKABURAGI");

	resolution = window->resolution;
	TIFFSetField(out, TIFFTAG_XRESOLUTION, &resolution);
	TIFFSetField(out, TIFFTAG_YRESOLUTION, &resolution);

	if(profile_data != NULL)
	{
		TIFFSetField(out, TIFFTAG_ICCPROFILE, profile_data_size, profile_data);
	}

	if(color_transform != NULL)
	{
		cmsDoTransform(color_transform, input_layer->pixels, window->temp_layer->pixels,
			window->width * window->height);
		cmsDeleteTransform(color_transform);
	}
	else
	{
		if(write_alpha == FALSE)
		{
			for(i=0; i<window->width*window->height; i++)
			{
				window->temp_layer->pixels[i*3+0] = window->mixed_layer->pixels[i*4+2];
				window->temp_layer->pixels[i*3+1] = window->mixed_layer->pixels[i*4+1];
				window->temp_layer->pixels[i*3+2] = window->mixed_layer->pixels[i*4+0];
			}
		}
		else
		{
			for(i=0; i<window->width*window->height; i++)
			{
				window->temp_layer->pixels[i*4+0] = window->mixed_layer->pixels[i*4+2];
				window->temp_layer->pixels[i*4+1] = window->mixed_layer->pixels[i*4+1];
				window->temp_layer->pixels[i*4+2] = window->mixed_layer->pixels[i*4+0];
				window->temp_layer->pixels[i*4+3] = window->mixed_layer->pixels[i*4+3];
			}
		}
	}

	for(i=0; i<window->height; i++)
	{
		TIFFWriteScanline(out, &window->temp_layer->pixels[i*stride], i, 0);
	}

	if(write_alpha != FALSE)
	{
		DeleteLayer(&input_layer);
	}

	TIFFWriteDirectory(out);
	TIFFClose(out);
}

static void UpdatePreviewCallBack(GtkFileChooser *file_chooser, GtkWidget *image)
{
	GdkPixbuf *pixbuf;
	gchar *file_path = gtk_file_chooser_get_preview_filename(file_chooser);
	size_t length;

	if(file_path == NULL)
	{
		gtk_file_chooser_set_preview_widget_active(file_chooser, FALSE);
		return;
	}
	else
	{
		length = strlen(file_path);
	}

	if(length >= 5 && StringCompareIgnoreCase(&file_path[length-4], ".kab") == 0)
	{
		GFile *fp = g_file_new_for_path(file_path);
		GFileInputStream *stream = g_file_read(fp, NULL, NULL);
		gint32 width, height, stride;
		uint8 has_thumbnail;
		uint32 data_size;
		MEMORY_STREAM_PTR png_data;
		// �K���f�[�^����p�̕�����
		const char format_string[] = "Paint Soft KABURAGI";
		// �K���f�[�^����p�̓ǂݍ��݃o�b�t�@
		uint8 format_read[32] = {0};

		(void)FileRead(format_read, 1, sizeof(format_string)/sizeof(*format_string), stream);
		if(memcmp(format_read, format_string, sizeof(format_string)/sizeof(*format_string)) != 0)
		{
			gtk_file_chooser_set_preview_widget_active(file_chooser, FALSE);
			return;
		}

		(void)FileSeek(stream, sizeof(data_size), SEEK_CUR);

		(void)FileRead(&has_thumbnail, 1, 1, stream);
		if(has_thumbnail != 0)
		{
			uint8 *pixels, *buf_pixels;
			int pix_stride;
			int i;

			(void)FileRead(&data_size, sizeof(data_size), 1, stream);
			png_data = CreateMemoryStream(data_size);
			(void)FileRead(png_data->buff_ptr, 1, data_size, stream);

			pixels = ReadPNGStream((void*)png_data, (stream_func)MemRead,
				&width, &height, &stride);
			pixbuf = gdk_pixbuf_new(GDK_COLORSPACE_RGB, TRUE, 8, width, height);
			buf_pixels = gdk_pixbuf_get_pixels(pixbuf);
			pix_stride = gdk_pixbuf_get_rowstride(pixbuf);

			for(i=0; i<height; i++)
			{
				(void)memcpy(&buf_pixels[i*pix_stride], &pixels[i*stride], stride);
			}

			gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
			gtk_file_chooser_set_preview_widget_active(file_chooser, TRUE);

			g_object_unref(pixbuf);
			DeleteMemoryStream(png_data);
			MEM_FREE_FUNC(pixels);
		}

		g_object_unref(stream);
		g_object_unref(fp);
	}
	else if(length >= 5 && StringCompareIgnoreCase(&file_path[length-4], ".tlg") == 0)
	{
		GFile *fp = g_file_new_for_path(file_path);
		GFileInputStream *stream = g_file_read(fp, NULL, NULL);
		GdkPixbuf *resize_buf;
		uint8 *pixels;
		uint8 *temp_pixels;
		int width, height, channel;
		int stride;
		uint8 b;
		int y;

		pixels = ReadTlgStream((void*)stream, (stream_func)FileRead, (seek_func)FileSeek,
			(long (*)(void*))FileSeekTell, &width, &height, &channel);

		if(pixels == NULL)
		{
			gtk_file_chooser_set_preview_widget_active(file_chooser, FALSE);
			return;
		}

		stride = width * channel;
		temp_pixels = (uint8*)MEM_ALLOC_FUNC(stride * height);
		for(y=0; y<height; y++)
		{
			(void)memcpy(&temp_pixels[y*stride], &pixels[(height-y-1)*stride], stride);
		}

		if(channel >= 3)
		{
			for(y=0; y<width * height; y++)
			{
				b = temp_pixels[y*channel];
				temp_pixels[y*channel] = temp_pixels[y*channel+2];
				temp_pixels[y*channel+2] = b;
			}
		}

		if(channel >= 3)
		{
			pixbuf = gdk_pixbuf_new_from_data(temp_pixels, GDK_COLORSPACE_RGB,
				channel > 3, 8, width, height, width * 4, NULL, NULL);
		}

		if(width > height)
		{
			resize_buf = gdk_pixbuf_scale_simple(pixbuf,
			THUMBNAIL_SIZE, THUMBNAIL_SIZE * height / width, GDK_INTERP_NEAREST);
		}
		else
		{
			resize_buf = gdk_pixbuf_scale_simple(pixbuf,
			THUMBNAIL_SIZE * width / height, THUMBNAIL_SIZE, GDK_INTERP_NEAREST);
		}
		gtk_image_set_from_pixbuf(GTK_IMAGE(image), resize_buf);
		gtk_file_chooser_set_preview_widget_active(file_chooser, TRUE);

		g_object_unref(pixbuf);
		g_object_unref(resize_buf);
		g_object_unref(stream);
		g_object_unref(fp);

		MEM_FREE_FUNC(pixels);
		MEM_FREE_FUNC(temp_pixels);
	}
	else
	{
		pixbuf = gdk_pixbuf_new_from_file_at_size(file_path, THUMBNAIL_SIZE, THUMBNAIL_SIZE, NULL);
		gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);

		if(pixbuf == NULL)
		{
			gtk_file_chooser_set_preview_widget_active(file_chooser, FALSE);
		}
		else
		{
			g_object_unref(pixbuf);
			gtk_file_chooser_set_preview_widget_active(file_chooser, TRUE);
		}
	}
}

/*******************************************************
* SetFileChooserPreview�֐�                            *
* �t�@�C���I���_�C�A���O�Ƀv���r���[�E�B�W�F�b�g��ݒ� *
* ����                                                 *
* file_chooser	: �t�@�C���I���_�C�A���O�E�B�W�F�b�g   *
*******************************************************/
void SetFileChooserPreview(GtkWidget *file_chooser)
{
	GtkWidget *image = gtk_image_new();

	gtk_file_chooser_set_preview_widget(GTK_FILE_CHOOSER(file_chooser), image);
	(void)g_signal_connect(G_OBJECT(file_chooser), "update-preview",
		G_CALLBACK(UpdatePreviewCallBack), image);
}

#ifdef __cplusplus
}
#endif