// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <png.h>
#include <setjmp.h>
#include <zlib.h>
#include <ctype.h>
#include "libtiff/tiffio.h"
#include "libjpeg/jpeglib.h"
#include "types.h"
#include "memory.h"
#include "draw_window.h"
#include "memory_stream.h"
#include "bit_stream.h"
#include "application.h"
#include "image_read_write.h"
#include "utils.h"
#include "tlg.h"
#include "display.h"
#include "application.h"

#include "gui/GTK/utils_gtk.h"
#include "gui/GTK/gtk_widgets.h"

#ifdef __cplusplus
extern "C" {
#endif

/*****************************************************************
* DecodeImageData関数                                            *
* メモリ中の画像データをデコードする                             *
* 引数                                                           *
* data				: 画像データ                                 *
* data_size			: 画像データのバイト数                       *
* file_type			: 画像のタイプを表す文字列(例:PNGなら".png") *
* width				: 画像の幅の格納先                           *
* height			: 画像の高さの格納先                         *
* channel			: 画像チャンネル数の格納先                   *
* resolution		: 画像解像度の格納先                         *
* icc_profile_name	: ICCプロファイルの名前の格納先              *
* icc_profile_data	: ICCプロファイルのデータの格納先            *
* icc_profile_size	: ICCプロファイルのデータサイズの格納先      *
* 返り値                                                         *
*	デコードしたピクセルデータ                                   *
*****************************************************************/
uint8* DecodeImageData(
	uint8* data,
	size_t data_size,
	const char* file_type,
	int* width,
	int* height,
	int* channel,
	int* resolution,
	char** icc_profile_name,
	uint8** icc_profile_data,
	uint32* icc_profile_size
)
{
	MEMORY_STREAM stream = {data, 0, data_size, 1};
	char *type = MEM_STRDUP_FUNC(file_type);
	uint8 *result = NULL;
	int local_width = 0, local_height = 0;
	int local_channel = 0;
	int local_resolution = 0;

	{
		char *p = type;
		while(*p != '\0')
		{
			*p = tolower(*p);
			p++;
		}
	}

	if(strcmp(type, ".png") == 0)
	{
		result = ReadPNGDetailData((void*)&stream, (stream_func_t)MemRead,
			&local_width, &local_height, &local_channel, &local_resolution,
			icc_profile_name, icc_profile_data, icc_profile_size
		);
		if(result != NULL)
		{
			local_channel = local_channel / local_width;
		}
	}
	else if(strcmp(type, ".jpg") == 0 || strcmp(type, ".jpeg") == 0)
	{
		result = ReadJpegStream((void*)&stream, (stream_func_t)MemRead,
			data_size, &local_width, &local_height, &local_channel, &local_resolution,
			icc_profile_data, icc_profile_size
		);
	}
	else if(strcmp(type, ".tga") == 0)
	{
		result = ReadTgaStream((void*)&stream, (stream_func_t)MemRead, data_size,
			&local_width, &local_height, &local_channel);
	}
	else if(strcmp(type, ".tlg") == 0)
	{
		result = ReadTlgStream((void*)&stream, (stream_func_t)MemRead, (seek_func_t)MemSeek,
			(long (*)(void*))MemTell, &local_width, &local_height, &local_channel);
	}
	else if(strcmp(type, ".bmp") == 0 || strcmp(type, ".spa") == 0 || strcmp(type, ".sph") == 0)
	{
		result = ReadBmpStream((void*)&stream, (stream_func_t)MemRead, (seek_func_t)MemSeek,
			data_size, &local_width, &local_height, &local_channel, &local_resolution);
	}
	else if(strcmp(type, ".dds") == 0)
	{
		result = ReadDdsStream((void*)&stream, (stream_func_t)MemRead, (seek_func_t)MemSeek,
			data_size, &local_width, &local_height, &local_channel);
	}

	MEM_FREE_FUNC(type);

	if(width != NULL)
	{
		*width = local_width;
	}
	if(height != NULL)
	{
		*height = local_height;
	}
	if(channel != NULL)
	{
		*channel = local_channel;
	}
	if(resolution != NULL)
	{
		*resolution = local_resolution;
	}

	return result;
}

/***********************************************************
* PNG_IO構造体                                             *
* pnglibにストリームのアドレスと読み書き関数ポインタ渡す用 *
***********************************************************/
typedef struct _PNG_IO
{
	void* data;
	stream_func_t rw_func;
	void (*flush_func)(void* stream);
} PNG_IO;

/***************************************************
* PngReadWrite関数                                 *
* pnglibの要求に応じて読み書き用の関数を呼び出す   *
* 引数                                             *
* png_p		: pnglibの圧縮・展開管理構造体アドレス *
* data		: 読み書き先のアドレス                 *
* length	: 読み書きするバイト数                 *
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
* PngFlush関数                                     *
* ストリームに残ったデータをクリア                 *
* 引数                                             *
* png_p		: pnglibの圧縮・展開管理構造体アドレス *
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
* ReadPNGStream関数                                        *
* PNGイメージデータを読み込む                              *
* 引数                                                     *
* stream	: データストリーム                             *
* read_func	: 読み込み用の関数ポインタ                     *
* width		: イメージの幅を格納するアドレス               *
* height	: イメージの高さを格納するアドレス             *
* stride	: イメージの一行分のバイト数を格納するアドレス *
* 返り値                                                   *
*	ピクセルデータ                                         *
***********************************************************/
uint8* ReadPNGStream(
	void* stream,
	stream_func_t read_func,
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
	uint8 *pixels;
	uint8 **image;
	uint32 i;

	// ストリームのアドレスと関数ポインタをセット
	io.data = stream;
	io.rw_func = read_func;

	// PNG展開用のデータを生成
	png_p = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL
	);

	// 画像情報を格納するデータ領域作成
	info_p = png_create_info_struct(png_p);

	if(setjmp(png_jmpbuf(png_p)) != 0)
	{
		png_destroy_read_struct(&png_p, &info_p, NULL);

		return NULL;
	}

	// 読み込み用のストリーム、関数ポインタをセット
	png_set_read_fn(png_p, (void*)&io, PngReadWrite);

	// 画像情報の読み込み
	png_read_info(png_p, info_p);
	png_get_IHDR(png_p, info_p, &local_width, &local_height,
		&bit_depth, &color_type, &interlace_type, NULL, NULL
	);
	local_stride = (uint32)png_get_rowbytes(png_p, info_p);

	// ピクセルメモリの確保
	pixels = (uint8*)MEM_ALLOC_FUNC(local_stride*local_height);
	// pnglibでは2次元配列にする必要があるので
		// 高さ分のポインタ配列を作成
	image = (uint8**)MEM_ALLOC_FUNC(sizeof(*image)*local_height);

	// 2次元配列になるようアドレスをセット
	for(i=0; i<local_height; i++)
	{
		image[i] = &pixels[local_stride*i];
	}
	// 画像データの読み込み
	png_read_image(png_p, image);

	// メモリの開放
	png_destroy_read_struct(&png_p, &info_p, NULL);
	MEM_FREE_FUNC(image);

	// 画像データを指定アドレスにセット
	*width = (int32)local_width;
	*height = (int32)local_height;
	*stride = (int32)local_stride;

	return pixels;
}

/**************************************************************************
* ReadPNGHeader関数                                                       *
* PNGイメージのヘッダ情報を読み込む                                       *
* 引数                                                                    *
* stream	: データストリーム                                            *
* read_func	: 読み込み用の関数ポインタ                                    *
* width		: イメージの幅を格納するアドレス                              *
* height	: イメージの高さを格納するアドレス                            *
* stride	: イメージの一行分のバイト数を格納するアドレス                *
* dpi		: イメージの解像度(DPI)を格納するアドレス                     *
* icc_profile_name	: ICCプロファイルの名前を受けるポインタ               *
* icc_profile_data	: ICCプロファイルのデータを受けるポインタ             *
* icc_profile_size	: ICCプロファイルのデータのバイト数を格納するアドレス *
**************************************************************************/
void ReadPNGHeader(
	void* stream,
	stream_func_t read_func,
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

	// ストリームのアドレスと関数ポインタをセット
	io.data = stream;
	io.rw_func = read_func;

	// PNG展開用のデータを生成
	png_p = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL
	);

	// 画像情報を格納するデータ領域作成
	info_p = png_create_info_struct(png_p);

	if(setjmp(png_jmpbuf(png_p)) != 0)
	{
		png_destroy_read_struct(&png_p, &info_p, NULL);

		return;
	}

	// 読み込み用のストリーム、関数ポインタをセット
	png_set_read_fn(png_p, (void*)&io, PngReadWrite);

	// 画像情報の読み込み
	png_read_info(png_p, info_p);
	png_get_IHDR(png_p, info_p, &local_width, &local_height,
		&bit_depth, &color_type, &interlace_type, NULL, NULL
	);
	local_stride = (uint32)png_get_rowbytes(png_p, info_p);

	// 解像度の取得
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

	// ICCプロファイルの取得
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

	// メモリの開放
	png_destroy_read_struct(&png_p, &info_p, NULL);

	// 画像データを指定アドレスにセット
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
* ReadPNGDetailData関数                                                   *
* PNGイメージデータを詳細情報付きで読み込む                               *
* 引数                                                                    *
* stream	: データストリーム                                            *
* read_func	: 読み込み用の関数ポインタ                                    *
* width		: イメージの幅を格納するアドレス                              *
* height	: イメージの高さを格納するアドレス                            *
* stride	: イメージの一行分のバイト数を格納するアドレス                *
* dpi		: イメージの解像度(DPI)を格納するアドレス                     *
* icc_profile_name	: ICCプロファイルの名前を受けるポインタ               *
* icc_profile_data	: ICCプロファイルのデータを受けるポインタ             *
* icc_profile_size	: ICCプロファイルのデータのバイト数を格納するアドレス *
* 返り値                                                                  *
*	ピクセルデータ                                                        *
**************************************************************************/
uint8* ReadPNGDetailData(
	void* stream,
	stream_func_t read_func,
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

	// ストリームのアドレスと関数ポインタをセット
	io.data = stream;
	io.rw_func = read_func;

	// PNG展開用のデータを生成
	png_p = png_create_read_struct(
		PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL
	);

	// 画像情報を格納するデータ領域作成
	info_p = png_create_info_struct(png_p);

	if(setjmp(png_jmpbuf(png_p)) != 0)
	{
		png_destroy_read_struct(&png_p, &info_p, NULL);

		return NULL;
	}

	// 読み込み用のストリーム、関数ポインタをセット
	png_set_read_fn(png_p, (void*)&io, PngReadWrite);

	// 画像情報の読み込み
	png_read_info(png_p, info_p);
	png_get_IHDR(png_p, info_p, &local_width, &local_height,
		&bit_depth, &color_type, &interlace_type, NULL, NULL
	);
	local_stride = (uint32)png_get_rowbytes(png_p, info_p);

	// 解像度の取得
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

	// ICCプロファイルの取得
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

	// ピクセルメモリの確保
	pixels = (uint8*)MEM_ALLOC_FUNC(local_stride*local_height);
	// pnglibでは2次元配列にする必要があるので
		// 高さ分のポインタ配列を作成
	image = (uint8**)MEM_ALLOC_FUNC(sizeof(*image)*local_height);

	// 2次元配列になるようアドレスをセット
	for(i=0; i<local_height; i++)
	{
		image[i] = &pixels[local_stride*i];
	}
	// 画像データの読み込み
	png_read_image(png_p, image);

	// メモリの開放
	png_destroy_read_struct(&png_p, &info_p, NULL);
	MEM_FREE_FUNC(image);

	// 画像データを指定アドレスにセット
	*width = (int32)local_width;
	*height = (int32)local_height;
	*stride = (int32)local_stride;

	return pixels;
}

/*********************************************
* WritePNGStream関数                         *
* PNGのデータをストリームに書き込む          *
* 引数                                       *
* stream		: データストリーム           *
* write_func	: 書き込み用の関数ポインタ   *
* pixels		: ピクセルデータ             *
* width			: 画像の幅                   *
* height		: 画像の高さ                 *
* stride		: 画像データ一行分のバイト数 *
* channel		: 画像のチャンネル数         *
* interlace		: インターレースの有無       *
* compression	: 圧縮レベル                 *
*********************************************/
void WritePNGStream(
	void* stream,
	stream_func_t write_func,
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

	// チャンネル数に合わせてカラータイプを設定
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

	// PNG書き込み用のデータを生成
	png_p = png_create_write_struct(
		PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL
	);

	// 画像情報格納用のメモリを確保
	info_p = png_create_info_struct(png_p);

	// 書き込み用のストリームと関数ポインタをセット
	png_set_write_fn(png_p, &io, PngReadWrite, PngFlush);
	// 圧縮には全てのフィルタを使用
	png_set_filter(png_p, 0, PNG_ALL_FILTERS);
	// 圧縮レベルを設定
	png_set_compression_level(png_p, compression);

	// PNGの情報をセット
	png_set_IHDR(png_p, info_p, width, height, 8, color_type,
		interlace, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	// pnglib用に2次元配列を作成
	image = (uint8**)MEM_ALLOC_FUNC(height*sizeof(*image));
	for(i=0; i<height; i++)
	{
		image[i] = &pixels[stride*i];
	}

	// 画像データの書き込み
	png_write_info(png_p, info_p);
	png_write_image(png_p, image);
	png_write_end(png_p, info_p);

	// メモリの開放
	png_destroy_write_struct(&png_p, &info_p);

	MEM_FREE_FUNC(image);
}

/********************************************************
* WritePNGDetailData関数                                *
* PNGのデータを詳細情報付きでストリームに書き込む       *
* 引数                                                  *
* stream			: データストリーム                  *
* write_func		: 書き込み用の関数ポインタ          *
* pixels			: ピクセルデータ                    *
* width				: 画像の幅                          *
* height			: 画像の高さ                        *
* stride			: 画像データ一行分のバイト数        *
* channel			: 画像のチャンネル数                *
* interlace			: インターレースの有無              *
* compression		: 圧縮レベル                        *
* dpi				: 解像度(DPI)                       *
* icc_profile_name	: ICCプロファイルの名前             *
* icc_profile_data	: ICCプロファイルのデータ           *
* icc_profile_data	: ICCプロファイルのデータのバイト数 *
********************************************************/
void WritePNGDetailData(
	void* stream,
	stream_func_t write_func,
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

	// チャンネル数に合わせてカラータイプを設定
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

	// PNG書き込み用のデータを生成
	png_p = png_create_write_struct(
		PNG_LIBPNG_VER_STRING,
		NULL, NULL, NULL
	);

	// 画像情報格納用のメモリを確保
	info_p = png_create_info_struct(png_p);

	// 書き込み用のストリームと関数ポインタをセット
	png_set_write_fn(png_p, &io, PngReadWrite, PngFlush);
	// 圧縮には全てのフィルタを使用
	png_set_filter(png_p, 0, PNG_ALL_FILTERS);
	// 圧縮レベルを設定
	png_set_compression_level(png_p, compression);

	// PNGの情報をセット
	png_set_IHDR(png_p, info_p, width, height, 8, color_type,
		interlace, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

	// pnglib用に2次元配列を作成
	image = (uint8**)MEM_ALLOC_FUNC(height*sizeof(*image));
	for(i=0; i<height; i++)
	{
		image[i] = &pixels[stride*i];
	}

	// 解像度の情報をセット
	if(dpi > 0)
	{
		png_uint_32 dpm = (png_uint_32)((dpi * 10000 + 127) / 254);
		png_set_pHYs(png_p, info_p, dpm, dpm, PNG_RESOLUTION_METER);
	}

	// ICCプロファイルの情報をセット
	if(icc_profile_data != NULL && icc_profile_size > 0)
	{
		png_set_iCCP(png_p, info_p, icc_profile_name, PNG_COMPRESSION_TYPE_BASE,
			icc_profile_data, icc_profile_size);
	}

	// 画像データの書き込み
	png_write_info(png_p, info_p);
	png_write_image(png_p, image);
	png_write_end(png_p, info_p);

	// メモリの開放
	png_destroy_write_struct(&png_p, &info_p);

	MEM_FREE_FUNC(image);
}

typedef struct _JPEG_ERROR_MANAGER
{
	struct jpeg_error_mgr jerr;
	jmp_buf buf;
} JPEG_ERROR_MANAGER;

static void JpegErrorHandler(j_common_ptr cinfo)
{
	JPEG_ERROR_MANAGER *manager = (JPEG_ERROR_MANAGER*)(cinfo->err);

	longjmp(manager->buf, 1);
}

#define ICC_MARKER  (JPEG_APP0 + 2)			// ICCプロファイルのマークの位置
#define ICC_OVERHEAD_LEN 14					// ICCプロファイルデータのオーバーヘッドのバイト数
#define MAX_BYTES_IN_MARKER 65533			// ICCプロファイルのマークの最大バイト数
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
* ReadJpegHeader関数                                                *
* JPEGのヘッダ情報を読み込む                                        *
* 引数                                                              *
* system_file_path	: OSのファイルシステムに則したファイルパス      *
* width				: 画像の幅データの読み込み先                    *
* height			: 画像の高さデータの読み込み先                  *
* resolution		: 画像の解像度データの読み込み先                *
* icc_profile_data	: ICCプロファイルのデータを受けるポインタ       *
* icc_profile_size	: ICCプロファイルのデータのバイト数の読み込み先 *
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
	JPEG_ERROR_MANAGER error_manager;

	if(fp == NULL)
	{
		return;
	}
	
	error_manager.jerr.error_exit = JpegErrorHandler;
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

/********************************************************************
* ReadJpegStream関数                                                *
* JPEGのデータを読み込み、デコードする                              *
* 引数                                                              *
* stream			: 読み込み元                                    *
* read_func			: 読み込みに使う関数ポインタ                    *
* data_size			: JPEGのデータサイズ                            *
* width				: 画像の幅データの読み込み先                    *
* height			: 画像の高さデータの読み込み先                  *
* channel			: 画像データのチャンネル数                      *
* resolution		: 画像の解像度データの読み込み先                *
* icc_profile_data	: ICCプロファイルのデータを受けるポインタ       *
* icc_profile_size	: ICCプロファイルのデータのバイト数の読み込み先 *
* 返り値                                                            *
*	デコードしたピクセルデータ(失敗したらNULL)                      *
********************************************************************/
uint8* ReadJpegStream(
	void* stream,
	stream_func_t read_func,
	size_t data_size,
	int* width,
	int* height,
	int* channel,
	int* resolution,
	uint8** icc_profile_data,
	int32* icc_profile_size
)
{
	// JPEGデコード用のデータ
	struct jpeg_decompress_struct decode;
	// デコードしたピクセルデータ
	uint8 *pixels = NULL;
	// データを一度メモリに全て読み込む
	uint8 *jpeg_data = (uint8*)MEM_ALLOC_FUNC(data_size);
	// ピクセルデータ擬似的に2次元配列にする
	uint8 **pixel_datas;
	// 画像の幅・高さ(ローカル)
	int local_width, local_height;
	// 画像のチャンネル数(ローカル)
	int local_channel;
	// 1行分のバイト数
	int stride;
	// デコード時のエラー処理
	JPEG_ERROR_MANAGER error;
	// for文用のカウンタ
	int i;

	// データの読み込み
	(void)read_func(jpeg_data, 1, data_size, stream);

	// エラー処理の設定
	error.jerr.error_exit = (noreturn_t (*)(j_common_ptr))JpegErrorHandler;
	decode.err = jpeg_std_error(&error.jerr);
	// エラー次のジャンプ先を設定
	if(setjmp(error.buf) != 0)
	{	// エラーで戻ってきた
		MEM_FREE_FUNC(jpeg_data);
		MEM_FREE_FUNC(pixel_datas);
		MEM_FREE_FUNC(pixels);
		return NULL;
	}

	// デコードデータの初期化
	jpeg_create_decompress(&decode);
	jpeg_mem_src(&decode, jpeg_data, (unsigned long)data_size);

	// ICCプロファイル読み込み準備
	if(icc_profile_data != NULL && icc_profile_size != NULL)
	{
		jpeg_save_markers(&decode, ICC_MARKER, 0xffff);
	}

	// ヘッダの読み込み
	jpeg_read_header(&decode, FALSE);

	if(icc_profile_data != NULL)
	{
		// ICCプロファイルの読み込み
		(void)JpegReadICCProfile(&decode, icc_profile_data, icc_profile_size);
	}

	// 画像の幅、高さ、チャンネル数、解像度を取得
	local_width = decode.image_width;
	local_height = decode.image_height;
	local_channel = decode.num_components;
	stride = local_channel * local_width;

	if(width != NULL)
	{
		*width = local_width;
	}
	if(height != NULL)
	{
		*height = local_height;
	}
	if(channel != NULL)
	{
		*channel = local_channel;
	}
	if(resolution != NULL)
	{
		*resolution = decode.X_density;
	}

	// ピクセルデータのメモリを確保
	pixels = (uint8*)MEM_ALLOC_FUNC(local_height * stride);
	pixel_datas = (uint8**)MEM_ALLOC_FUNC(sizeof(*pixel_datas) * local_height);
	for(i=0; i<local_height; i++)
	{
		pixel_datas[i] = &pixels[i*stride];
	}

	// デコード開始
	(void)jpeg_start_decompress(&decode);
	while(decode.output_scanline < decode.image_height)
	{
		jpeg_read_scanlines(&decode, pixel_datas + decode.output_scanline,
			decode.image_height - decode.output_scanline);
	}

	// メモリ開放
	jpeg_finish_decompress(&decode);
	MEM_FREE_FUNC(pixel_datas);
	jpeg_destroy_decompress(&decode);
	MEM_FREE_FUNC(jpeg_data);

	return pixels;
}

/****************************************************************
* WriteJpegFile関数                                             *
* JPEG形式で画像データを書き出す                                *
* 引数                                                          *
* system_file_path	: OSのファイルシステムに則したファイルパス  *
* pixels			: ピクセルデータ                            *
* width				: 画像データの幅                            *
* height			: 画像データの高さ                          *
* channel			: 画像のチャンネル数                        *
* data_type			: ピクセルデータの形式(RGB or CMYK)         *
* quality			: JPEG画像の画質                            *
* optimize_coding	: 最適化エンコードの有無                    *
* icc_profile_data	: 埋め込むICCプロファイルのデータ           *
*						(埋め込まない時はNULL)                  *
* icc_profile_size	: 埋め込むICCプロファイルのデータのバイト数 *
* resolution		: 解像度(dpi)                               *
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
	JPEG_ERROR_MANAGER error_manager;
	FILE *fp;
	uint8 *write_array;
	int stride = width * channel;
	int i;

	if((fp = fopen(system_file_path, "wb")) == NULL)
	{
		return;
	}

	error_manager.jerr.error_exit = JpegErrorHandler;
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
* ReadOriginalFormatLayers関数                             *
* 独自形式のレイヤーデータを読み込む                       *
* 引数                                                     *
* stream		: 読み込み元のアドレス                     *
* progress_step	: 進捗状況の更新幅                         *
* window		: 描画領域の情報                           *
* app			: アプリケーションを管理する構造体アドレス *
* 返り値                                                   *
*	読み込んだレイヤーデータ                               *
***********************************************************/
LAYER* ReadOriginalFormatLayers(
	MEMORY_STREAM_PTR stream,
	FLOAT_T progress_step,
	DRAW_WINDOW* window,
	APPLICATION* app,
	uint16 num_layer
)
{
	// レイヤーの基本情報読み込み用
	LAYER_BASE_DATA base;
	// テキストレイヤーの基本情報読み込み用
	TEXT_LAYER_BASE_DATA text_base;
	// 画像データ
	uint8 *pixels;
	// 画像データのバイト数
	guint32 data_size;
	// 次のレイヤーデータが開始するポイント
	guint32 next_data_point;
	// 追加するレイヤー
	LAYER* layer = NULL;
	// ピクセルデータ展開用
	MEMORY_STREAM_PTR image;
	// ピクセルデータの幅、高さ、一行分のバイト数
	gint32 width, height, stride;
	// 32bit読み込み用
	guint32 size_t_temp;
	// レイヤー・フォントの名前とその長さ
	char *name;
	uint16 name_length;
	// レイヤーセットの階層
	int8 current_hierarchy = 0;
	int8 hierarchy[2048] = {0};
	// 現在の進捗状況
	FLOAT_T current_progress = progress_step;
	// 進捗状況のパーセンテージ表示用
	gchar show_text[16];
	// 進捗状況表示更新用
	GdkEvent *queued_event;
	// レイヤー読み込みエラー判定
	int before_error = FALSE;
	// 再表示諦めのカウンタ
	int redraw_counter = 0;
	// for文用のカウンタ
	unsigned int i, j;
	int k;

	for(i=0; i<num_layer; i++)
	{
		// レイヤーデータ読み込み
		if(before_error == FALSE)
		{	// 名前
			(void)MemRead(&name_length, sizeof(name_length), 1, stream);
			name = (char*)MEM_ALLOC_FUNC(name_length);
			(void)MemRead(name, 1, name_length, stream);
			// 基本情報
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

			if(base.layer_type >= NUM_LAYER_TYPE || base.x != 0)
			{
				goto return_layers;
			}

			// 値のセット
			layer = CreateLayer(base.x, base.y, base.width, base.height,
				base.channel, base.layer_type, layer, NULL, name, window);
			layer->alpha = base.alpha;
			layer->layer_mode = base.layer_mode;
			layer->flags = base.flags;
			hierarchy[i] = base.layer_set;
			MEM_FREE_FUNC(name);
			window->active_layer = layer;
		}
		else
		{
			name = MEM_STRDUP_FUNC("Error");
			base.layer_type = TYPE_NORMAL_LAYER;
			base.layer_mode = LAYER_BLEND_NORMAL;
			base.x = 0;
			base.y = 0;
			base.width = window->width;
			base.height = window->height;
			base.flags = 0;
			base.alpha = 100;
			base.channel = 4;
			base.layer_set = 0;
			before_error = FALSE;
			// 値のセット
			layer = CreateLayer(base.x, base.y, base.width, base.height,
				base.channel, base.layer_type, layer, NULL, name, window);
			layer->alpha = base.alpha;
			layer->layer_mode = base.layer_mode;
			layer->flags = base.flags;
			hierarchy[i] = base.layer_set;
			stream->data_point -= 4;
			MEM_FREE_FUNC(name);
			window->active_layer = layer;
		}
		// レイヤーのタイプで処理切り替え
		switch(base.layer_type)
		{
		case TYPE_NORMAL_LAYER:	// 通常レイヤー
			// PNG圧縮されたピクセルデータを展開して読み込む
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			next_data_point = (uint32)(stream->data_point + data_size);
			image = CreateMemoryStream(data_size);
			(void)MemRead(image->buff_ptr, 1, data_size, stream);
			pixels = ReadPNGStream(image, (stream_func_t)MemRead,
				&width, &height, &stride);
			if(pixels != NULL)
			{
				(void)memcpy(layer->pixels, pixels, height*stride);
			}
			else
			{
				before_error = TRUE;
			}
			DeleteMemoryStream(image);
			MEM_FREE_FUNC(pixels);
			break;
		case TYPE_VECTOR_LAYER:	// ベクトルレイヤー
			{	// 線をピクセルデータへ変更するための準備
				VECTOR_LAYER_RECTANGLE rect = {0, 0, base.width, base.height};

				layer->layer_data.vector_layer_p =
					(VECTOR_LAYER*)MEM_ALLOC_FUNC(sizeof(*layer->layer_data.vector_layer_p));
				(void)memset(layer->layer_data.vector_layer_p, 0,
					sizeof(*layer->layer_data.vector_layer_p));
				// 線情報をラスタライズしたレイヤー
				(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
				layer->layer_data.vector_layer_p->mix =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);

				// 一番下に空のレイヤー作成
				layer->layer_data.vector_layer_p->base =
					(VECTOR_DATA*)CreateVectorLine(NULL, NULL);
				(void)memset(layer->layer_data.vector_layer_p->base, 0,
					sizeof(VECTOR_LINE));
				layer->layer_data.vector_layer_p->base->line.base_data.layer =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);
				layer->layer_data.vector_layer_p->top_data =
					layer->layer_data.vector_layer_p->base;

				// データのバイト数を読み込む
				next_data_point = (uint32)(stream->data_point +
					ReadVectorLineData(&stream->buff_ptr[stream->data_point], layer));

				// ベクトルデータをラスタライズ
				layer->layer_data.vector_layer_p->flags =
					(VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ALL);
				RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
			}
			break;
		case TYPE_TEXT_LAYER:	// テキストレイヤー
			{
				// フォントサーチ用
				PangoFontFamily** search_font;
				// フォントID
				gint32 font_id;

				// データのサイズを読み込む
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);

				// 文字描画領域の座標、幅、高さ、文字サイズ
					// フォントファイル名、色等を読み込む
				(void)MemRead(&text_base.x, sizeof(text_base.x), 1, stream);
				(void)MemRead(&text_base.y, sizeof(text_base.y), 1, stream);
				(void)MemRead(&text_base.width, sizeof(text_base.width), 1, stream);
				(void)MemRead(&text_base.height, sizeof(text_base.height), 1, stream);
				(void)MemRead(&text_base.balloon_type, sizeof(text_base.balloon_type), 1, stream);
				(void)MemRead(&text_base.font_size, sizeof(text_base.font_size), 1, stream);
				(void)MemRead(text_base.color, sizeof(text_base.color), 1, stream);
				(void)MemRead(text_base.edge_position, sizeof(text_base.edge_position), 1, stream);
				(void)MemRead(&text_base.arc_start, sizeof(text_base.arc_start), 1, stream);
				(void)MemRead(&text_base.arc_end, sizeof(text_base.arc_end), 1, stream);
				(void)MemRead(text_base.back_color, sizeof(text_base.back_color), 1, stream);
				(void)MemRead(text_base.line_color, sizeof(text_base.line_color), 1, stream);
				(void)MemRead(&text_base.line_width, sizeof(text_base.line_width), 1, stream);
				(void)MemRead(&text_base.base_size, sizeof(text_base.base_size), 1, stream);
				(void)MemRead(&text_base.balloon_data.num_edge, sizeof(text_base.balloon_data.num_edge), 1, stream);
				(void)MemRead(&text_base.balloon_data.num_children, sizeof(text_base.balloon_data.num_children), 1, stream);
				(void)MemRead(&text_base.balloon_data.edge_size, sizeof(text_base.balloon_data.edge_size), 1, stream);
				(void)MemRead(&text_base.balloon_data.random_seed, sizeof(text_base.balloon_data.random_seed), 1, stream);
				(void)MemRead(&text_base.balloon_data.edge_random_size, sizeof(text_base.balloon_data.edge_random_size), 1, stream);
				(void)MemRead(&text_base.balloon_data.edge_random_distance, sizeof(text_base.balloon_data.edge_random_distance), 1, stream);
				(void)MemRead(&text_base.balloon_data.start_child_size, sizeof(text_base.balloon_data.start_child_size), 1, stream);
				(void)MemRead(&text_base.balloon_data.end_child_size, sizeof(text_base.balloon_data.end_child_size), 1, stream);
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
					CreateTextLayer(window, text_base.x, text_base.y, text_base.width, text_base.height,
						text_base.base_size, text_base.font_size, font_id, text_base.color, text_base.balloon_type,
							text_base.back_color, text_base.line_color, text_base.line_width, &text_base.balloon_data, text_base.flags
				);
				layer->layer_data.text_layer_p->edge_position[0][0] = text_base.edge_position[0][0];
				layer->layer_data.text_layer_p->edge_position[0][1] = text_base.edge_position[0][1];
				layer->layer_data.text_layer_p->edge_position[1][0] = text_base.edge_position[1][0];
				layer->layer_data.text_layer_p->edge_position[1][1] = text_base.edge_position[1][1];
				layer->layer_data.text_layer_p->edge_position[2][0] = text_base.edge_position[2][0];
				layer->layer_data.text_layer_p->edge_position[2][1] = text_base.edge_position[2][1];
				layer->layer_data.text_layer_p->arc_start = text_base.arc_start;
				layer->layer_data.text_layer_p->arc_end = text_base.arc_end;

				(void)MemRead(&name_length, sizeof(name_length), 1, stream);
				layer->layer_data.text_layer_p->text =
					(char*)MEM_ALLOC_FUNC(name_length);
				(void)MemRead(layer->layer_data.text_layer_p->text, 1, name_length, stream);
				// レイヤーをラスタライズ
				RenderTextLayer(window, layer, layer->layer_data.text_layer_p);
			}

			break;
		case TYPE_LAYER_SET:	// レイヤーセット
			{
				LAYER *target = layer->prev;
				next_data_point = (uint32)stream->data_point;
				current_hierarchy = hierarchy[i]+1;

				for(k=i-1; k>=0 && current_hierarchy == hierarchy[k]; k--)
				{
					hierarchy[k] = 0;
					target->layer_set = layer;
					target = target->prev;
				}

				if(stream->buff_ptr[stream->data_point] > 20)
				{
					before_error = TRUE;
					while(stream->buff_ptr[stream->data_point] != 0x0)
					{
						stream->data_point++;
					}
					next_data_point = (uint32)stream->data_point;
				}
			}

			break;
		case TYPE_3D_LAYER:	// 3Dモデリングレイヤー
			if(GetHas3DLayer(app) != FALSE)
			{
				// PNG圧縮されたピクセルデータを展開して読み込む
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				image = CreateMemoryStream(data_size);
				(void)MemRead(image->buff_ptr, 1, data_size, stream);
				pixels = ReadPNGStream(image, (stream_func_t)MemRead,
					&width, &height, &stride);
				if(pixels != NULL)
				{
					(void)memcpy(layer->pixels, pixels, height*stride);
				}
				DeleteMemoryStream(image);
				MEM_FREE_FUNC(pixels);

				// 3Dモデルのデータを読み込む
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				layer->modeling_data = MEM_ALLOC_FUNC(data_size);
				(void)MemRead(layer->modeling_data, 1, data_size, stream);
				layer->modeling_data_size = data_size;

				break;
			}
		case TYPE_ADJUSTMENT_LAYER:
			ReadAdjustmentLayerData(stream, (stream_func_t)MemRead, layer->layer_data.adjustment_layer_p);
			break;
		default:
			// PNG圧縮されたピクセルデータを展開して読み込む
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			next_data_point = (uint32)(stream->data_point + data_size);
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			image = CreateMemoryStream(data_size);
			(void)MemRead(image->buff_ptr, 1, data_size, stream);
			pixels = ReadPNGStream(image, (stream_func_t)MemRead,
				&width, &height, &stride);
			if(pixels != NULL)
			{
				(void)memcpy(layer->pixels, pixels, height*stride);
			}
			else
			{
				before_error = TRUE;
			}
			DeleteMemoryStream(image);
			MEM_FREE_FUNC(pixels);
		}

		// 追加情報データに移動
		(void)MemSeek(stream, (long)next_data_point, SEEK_SET);
		// 追加情報を読み込む
		(void)MemRead(&layer->num_extra_data, sizeof(layer->num_extra_data), 1, stream);

		for(j=0; j<layer->num_extra_data; j++)
		{
			// データの名前を読み込む
			(void)MemRead(&name_length, sizeof(name_length), 1, stream);
			if(name_length >= 8192)
			{
				break;
			}
			layer->extra_data[j].name = (char*)MEM_ALLOC_FUNC(name_length);
			(void)MemRead(layer->extra_data[j].name, 1, name_length, stream);
			(void)MemRead(&size_t_temp, sizeof(size_t_temp), 1, stream);
			if((int)size_t_temp >= layer->stride * layer->height * 3)
			{
				break;
			}
			layer->extra_data[j].data_size = size_t_temp;
			layer->extra_data[j].data = MEM_ALLOC_FUNC(layer->extra_data[j].data_size);
			(void)MemRead(layer->extra_data[j].data, 1, layer->extra_data[j].data_size, stream);
		}

		// 進捗状況を更新
		current_progress += progress_step;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->widgets->progress), current_progress);
		(void)sprintf(show_text, "%.0f%%", current_progress * 100);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app->widgets->progress), show_text);
#if GTK_MAJOR_VERSION <= 2
		gdk_window_process_updates(app->widgets->progress->window, FALSE);
#else
		gdk_window_process_updates(gtk_widget_get_window(app->widgets->progress), FALSE);
#endif
		while(gdk_events_pending() != FALSE)
		{
#define MAX_REDRAW_TRY 250
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event != NULL)
			{
#if GTK_MAJOR_VERSION <= 2
				if(queued_event->any.window == app->widgets->progress->window
#else
				if(queued_event->any.window == gtk_widget_get_window(app->widgets->progress)
#endif
					&& queued_event->any.type == GDK_EXPOSE
					|| redraw_counter >= MAX_REDRAW_TRY)
				{
					redraw_counter = 0;
					gdk_event_free(queued_event);
					break;
				}
				else
				{
					redraw_counter++;
					gdk_event_free(queued_event);
				}
			}
		}
	}

return_layers:
	// 先頭のレイヤーを返す
	while(layer->prev != NULL)
	{
		layer = layer->prev;
	}

	return layer;
}

/**************************************************************
* ReadOriginalFormatLayersOldVersion5関数                     *
* 旧独自形式のレイヤーデータを読み込む(ファイルバージョン： 5 *
* 引数                                                        *
* stream		: 読み込み元のアドレス                        *
* progress_step	: 進捗状況の更新幅                            *
* window		: 描画領域の情報                              *
* app			: アプリケーションを管理する構造体アドレス    *
* 返り値                                                      *
*	読み込んだレイヤーデータ                                  *
**************************************************************/
LAYER* ReadOriginalFormatLayersOldVersion5(
	MEMORY_STREAM_PTR stream,
	FLOAT_T progress_step,
	DRAW_WINDOW* window,
	APPLICATION* app,
	uint16 num_layer
)
{
	// レイヤーの基本情報読み込み用
	LAYER_BASE_DATA base;
	// テキストレイヤーの基本情報読み込み用
	TEXT_LAYER_BASE_DATA text_base;
	// 画像データ
	uint8 *pixels;
	// 画像データのバイト数
	guint32 data_size;
	// 次のレイヤーデータが開始するポイント
	guint32 next_data_point;
	// 追加するレイヤー
	LAYER* layer = NULL;
	// ピクセルデータ展開用
	MEMORY_STREAM_PTR image;
	// ピクセルデータの幅、高さ、一行分のバイト数
	gint32 width, height, stride;
	// 32bit読み込み用
	guint32 size_t_temp;
	// レイヤー・フォントの名前とその長さ
	char *name;
	uint16 name_length;
	// レイヤーセットの階層
	int8 current_hierarchy = 0;
	int8 hierarchy[2048] = {0};
	// 現在の進捗状況
	FLOAT_T current_progress = progress_step;
	// 進捗状況のパーセンテージ表示用
	gchar show_text[16];
	// 進捗状況表示更新用
	GdkEvent *queued_event;
	// レイヤー読み込みエラー判定
	int before_error = FALSE;
	// 再表示諦めのカウンタ
	int redraw_counter = 0;
	// for文用のカウンタ
	unsigned int i, j;
	int k;

	for(i=0; i<num_layer; i++)
	{
		// レイヤーデータ読み込み
		if(before_error == FALSE)
		{	// 名前
			(void)MemRead(&name_length, sizeof(name_length), 1, stream);
			name = (char*)MEM_ALLOC_FUNC(name_length);
			(void)MemRead(name, 1, name_length, stream);
			// 基本情報
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

			if(base.layer_type >= NUM_LAYER_TYPE || base.x != 0)
			{
				goto return_layers;
			}

			// 値のセット
			layer = CreateLayer(base.x, base.y, base.width, base.height,
				base.channel, base.layer_type, layer, NULL, name, window);
			layer->alpha = base.alpha;
			layer->layer_mode = base.layer_mode;
			layer->flags = base.flags;
			hierarchy[i] = base.layer_set;
			MEM_FREE_FUNC(name);
			window->active_layer = layer;
		}
		else
		{
			name = MEM_STRDUP_FUNC("Error");
			base.layer_type = TYPE_NORMAL_LAYER;
			base.layer_mode = LAYER_BLEND_NORMAL;
			base.x = 0;
			base.y = 0;
			base.width = window->width;
			base.height = window->height;
			base.flags = 0;
			base.alpha = 100;
			base.channel = 4;
			base.layer_set = 0;
			before_error = FALSE;
			// 値のセット
			layer = CreateLayer(base.x, base.y, base.width, base.height,
				base.channel, base.layer_type, layer, NULL, name, window);
			layer->alpha = base.alpha;
			layer->layer_mode = base.layer_mode;
			layer->flags = base.flags;
			hierarchy[i] = base.layer_set;
			stream->data_point -= 4;
			MEM_FREE_FUNC(name);
			window->active_layer = layer;
		}
		// レイヤーのタイプで処理切り替え
		switch(base.layer_type)
		{
		case TYPE_NORMAL_LAYER:	// 通常レイヤー
			// PNG圧縮されたピクセルデータを展開して読み込む
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			next_data_point = (uint32)(stream->data_point + data_size);
			image = CreateMemoryStream(data_size);
			(void)MemRead(image->buff_ptr, 1, data_size, stream);
			pixels = ReadPNGStream(image, (stream_func_t)MemRead,
				&width, &height, &stride);
			if(pixels != NULL)
			{
				(void)memcpy(layer->pixels, pixels, height*stride);
			}
			else
			{
				before_error = TRUE;
			}
			DeleteMemoryStream(image);
			MEM_FREE_FUNC(pixels);
			break;
		case TYPE_VECTOR_LAYER:	// ベクトルレイヤー
			{	// 線をピクセルデータへ変更するための準備
				VECTOR_LAYER_RECTANGLE rect = {0, 0, base.width, base.height};

				layer->layer_data.vector_layer_p =
					(VECTOR_LAYER*)MEM_ALLOC_FUNC(sizeof(*layer->layer_data.vector_layer_p));
				(void)memset(layer->layer_data.vector_layer_p, 0,
					sizeof(*layer->layer_data.vector_layer_p));
				// 線情報をラスタライズしたレイヤー
				(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
				layer->layer_data.vector_layer_p->mix =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);

				// 一番下に空のレイヤー作成
				layer->layer_data.vector_layer_p->base =
					(VECTOR_DATA*)CreateVectorLine(NULL, NULL);
				(void)memset(layer->layer_data.vector_layer_p->base, 0,
					sizeof(VECTOR_LINE));
				layer->layer_data.vector_layer_p->base->line.base_data.layer =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);
				layer->layer_data.vector_layer_p->top_data =
					layer->layer_data.vector_layer_p->base;

				// データのバイト数を読み込む
				next_data_point = (uint32)(stream->data_point +
					ReadVectorLineData(&stream->buff_ptr[stream->data_point], layer));

				// ベクトルデータをラスタライズ
				layer->layer_data.vector_layer_p->flags =
					(VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ALL);
				RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
			}
			break;
		case TYPE_TEXT_LAYER:	// テキストレイヤー
			{
				// フォントサーチ用
				PangoFontFamily** search_font;
				// フォントID
				gint32 font_id;

				// データのサイズを読み込む
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);

				// 文字描画領域の座標、幅、高さ、文字サイズ
					// フォントファイル名、色等を読み込む
				(void)MemRead(&text_base.x, sizeof(text_base.x), 1, stream);
				(void)MemRead(&text_base.y, sizeof(text_base.y), 1, stream);
				(void)MemRead(&text_base.width, sizeof(text_base.width), 1, stream);
				(void)MemRead(&text_base.height, sizeof(text_base.height), 1, stream);
				(void)MemRead(&text_base.balloon_type, sizeof(text_base.balloon_type), 1, stream);
				(void)MemRead(&text_base.font_size, sizeof(text_base.font_size), 1, stream);
				(void)MemRead(text_base.color, sizeof(text_base.color), 1, stream);
				(void)MemRead(text_base.edge_position, sizeof(text_base.edge_position), 1, stream);
				(void)MemRead(&text_base.arc_start, sizeof(text_base.arc_start), 1, stream);
				(void)MemRead(&text_base.arc_end, sizeof(text_base.arc_end), 1, stream);
				(void)MemRead(text_base.back_color, sizeof(text_base.back_color), 1, stream);
				(void)MemRead(text_base.line_color, sizeof(text_base.line_color), 1, stream);
				(void)MemRead(&text_base.line_width, sizeof(text_base.line_width), 1, stream);
				(void)MemRead(&text_base.base_size, sizeof(text_base.base_size), 1, stream);
				(void)MemRead(&text_base.balloon_data.num_edge, sizeof(text_base.balloon_data.num_edge), 1, stream);
				(void)MemRead(&text_base.balloon_data.num_children, sizeof(text_base.balloon_data.num_children), 1, stream);
				(void)MemRead(&text_base.balloon_data.edge_size, sizeof(text_base.balloon_data.edge_size), 1, stream);
				(void)MemRead(&text_base.balloon_data.random_seed, sizeof(text_base.balloon_data.random_seed), 1, stream);
				(void)MemRead(&text_base.balloon_data.edge_random_size, sizeof(text_base.balloon_data.edge_random_size), 1, stream);
				(void)MemRead(&text_base.balloon_data.edge_random_distance, sizeof(text_base.balloon_data.edge_random_distance), 1, stream);
				(void)MemRead(&text_base.balloon_data.start_child_size, sizeof(text_base.balloon_data.start_child_size), 1, stream);
				(void)MemRead(&text_base.balloon_data.end_child_size, sizeof(text_base.balloon_data.end_child_size), 1, stream);
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
					CreateTextLayer(window, text_base.x, text_base.y, text_base.width, text_base.height,
						text_base.base_size, text_base.font_size, font_id, text_base.color, text_base.balloon_type,
							text_base.back_color, text_base.line_color, text_base.line_width, &text_base.balloon_data, text_base.flags
				);
				layer->layer_data.text_layer_p->edge_position[0][0] = text_base.edge_position[0][0];
				layer->layer_data.text_layer_p->edge_position[0][1] = text_base.edge_position[0][1];
				layer->layer_data.text_layer_p->edge_position[1][0] = text_base.edge_position[1][0];
				layer->layer_data.text_layer_p->edge_position[1][1] = text_base.edge_position[1][1];
				layer->layer_data.text_layer_p->edge_position[2][0] = text_base.edge_position[2][0];
				layer->layer_data.text_layer_p->edge_position[2][1] = text_base.edge_position[2][1];
				layer->layer_data.text_layer_p->arc_start = text_base.arc_start;
				layer->layer_data.text_layer_p->arc_end = text_base.arc_end;

				(void)MemRead(&name_length, sizeof(name_length), 1, stream);
				layer->layer_data.text_layer_p->text =
					(char*)MEM_ALLOC_FUNC(name_length);
				(void)MemRead(layer->layer_data.text_layer_p->text, 1, name_length, stream);
				// レイヤーをラスタライズ
				RenderTextLayer(window, layer, layer->layer_data.text_layer_p);
			}

			break;
		case TYPE_LAYER_SET:	// レイヤーセット
			{
				LAYER *target = layer->prev;
				next_data_point = (uint32)stream->data_point;
				current_hierarchy = hierarchy[i]+1;

				for(k=i-1; k>=0 && current_hierarchy == hierarchy[k]; k--)
				{
					hierarchy[k] = 0;
					target->layer_set = layer;
					target = target->prev;
				}

				if(stream->buff_ptr[stream->data_point] > 20)
				{
					before_error = TRUE;
					while(stream->buff_ptr[stream->data_point] != 0x0)
					{
						stream->data_point++;
					}
					next_data_point = (uint32)stream->data_point;
				}
			}

			break;
		case TYPE_3D_LAYER:	// 3Dモデリングレイヤー
			if(GetHas3DLayer(app) != FALSE)
			{
				// PNG圧縮されたピクセルデータを展開して読み込む
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				image = CreateMemoryStream(data_size);
				(void)MemRead(image->buff_ptr, 1, data_size, stream);
				pixels = ReadPNGStream(image, (stream_func_t)MemRead,
					&width, &height, &stride);
				if(pixels != NULL)
				{
					(void)memcpy(layer->pixels, pixels, height*stride);
				}
				DeleteMemoryStream(image);
				MEM_FREE_FUNC(pixels);

				// 3Dモデルのデータを読み込む
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				layer->modeling_data = MEM_ALLOC_FUNC(data_size);
				(void)MemRead(layer->modeling_data, 1, data_size, stream);
				layer->modeling_data_size = data_size;

				break;
			}
		default:
			// PNG圧縮されたピクセルデータを展開して読み込む
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			next_data_point = (uint32)(stream->data_point + data_size);
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			image = CreateMemoryStream(data_size);
			(void)MemRead(image->buff_ptr, 1, data_size, stream);
			pixels = ReadPNGStream(image, (stream_func_t)MemRead,
				&width, &height, &stride);
			if(pixels != NULL)
			{
				(void)memcpy(layer->pixels, pixels, height*stride);
			}
			else
			{
				before_error = TRUE;
			}
			DeleteMemoryStream(image);
			MEM_FREE_FUNC(pixels);
		}

		// 追加情報データに移動
		(void)MemSeek(stream, (long)next_data_point, SEEK_SET);
		// 追加情報を読み込む
		(void)MemRead(&layer->num_extra_data, sizeof(layer->num_extra_data), 1, stream);

		for(j=0; j<layer->num_extra_data; j++)
		{
			// データの名前を読み込む
			(void)MemRead(&name_length, sizeof(name_length), 1, stream);
			if(name_length >= 8192)
			{
				break;
			}
			layer->extra_data[j].name = (char*)MEM_ALLOC_FUNC(name_length);
			(void)MemRead(layer->extra_data[j].name, 1, name_length, stream);
			(void)MemRead(&size_t_temp, sizeof(size_t_temp), 1, stream);
			if((int)size_t_temp >= layer->stride * layer->height * 3)
			{
				break;
			}
			layer->extra_data[j].data_size = size_t_temp;
			layer->extra_data[j].data = MEM_ALLOC_FUNC(layer->extra_data[j].data_size);
			(void)MemRead(layer->extra_data[j].data, 1, layer->extra_data[j].data_size, stream);
		}

		// 進捗状況を更新
		current_progress += progress_step;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->widgets->progress), current_progress);
		(void)sprintf(show_text, "%.0f%%", current_progress * 100);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app->widgets->progress), show_text);
#if GTK_MAJOR_VERSION <= 2
		gdk_window_process_updates(app->widgets->progress->window, FALSE);
#else
		gdk_window_process_updates(gtk_widget_get_window(app->widgets->progress), FALSE);
#endif
		while(gdk_events_pending() != FALSE)
		{
#define MAX_REDRAW_TRY 250
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event != NULL)
			{
#if GTK_MAJOR_VERSION <= 2
				if(queued_event->any.window == app->widgets->progress->window
#else
				if(queued_event->any.window == gtk_widget_get_window(app->widgets->progress)
#endif
					&& queued_event->any.type == GDK_EXPOSE
					|| redraw_counter >= MAX_REDRAW_TRY)
				{
					redraw_counter = 0;
					gdk_event_free(queued_event);
					break;
				}
				else
				{
					redraw_counter++;
					gdk_event_free(queued_event);
				}
			}
		}
	}

return_layers:
	// 先頭のレイヤーを返す
	while(layer->prev != NULL)
	{
		layer = layer->prev;
	}

	return layer;
}

/**************************************************************
* ReadOriginalFormatLayersOldVersion4関数                     *
* 旧独自形式のレイヤーデータを読み込む(ファイルバージョン： 4 *
* 引数                                                        *
* stream		: 読み込み元のアドレス                        *
* progress_step	: 進捗状況の更新幅                            *
* window		: 描画領域の情報                              *
* app			: アプリケーションを管理する構造体アドレス    *
* 返り値                                                      *
*	読み込んだレイヤーデータ                                  *
**************************************************************/
LAYER* ReadOriginalFormatLayersOldVersion4(
	MEMORY_STREAM_PTR stream,
	FLOAT_T progress_step,
	DRAW_WINDOW* window,
	APPLICATION* app,
	uint16 num_layer
)
{
	// レイヤーの基本情報読み込み用
	LAYER_BASE_DATA base;
	// テキストレイヤーの基本情報読み込み用
	TEXT_LAYER_BASE_DATA text_base = {0};
	// 画像データ
	uint8 *pixels;
	// 画像データのバイト数
	guint32 data_size;
	// 次のレイヤーデータが開始するポイント
	guint32 next_data_point;
	// 追加するレイヤー
	LAYER* layer = NULL;
	// ピクセルデータ展開用
	MEMORY_STREAM_PTR image;
	// ピクセルデータの幅、高さ、一行分のバイト数
	gint32 width, height, stride;
	// 32bit読み込み用
	guint32 size_t_temp;
	// レイヤー・フォントの名前とその長さ
	char *name;
	uint16 name_length;
	// レイヤーセットの階層
	int8 current_hierarchy = 0;
	int8 hierarchy[2048] = {0};
	// 現在の進捗状況
	FLOAT_T current_progress = progress_step;
	// 進捗状況のパーセンテージ表示用
	gchar show_text[16];
	// 進捗状況表示更新用
	GdkEvent *queued_event;
	// レイヤー読み込みエラー判定
	int before_error = FALSE;
	// 再表示諦めのカウンタ
	int redraw_counter = 0;
	// for文用のカウンタ
	unsigned int i, j;
	int k;

	for(i=0; i<num_layer; i++)
	{
		// レイヤーデータ読み込み
		if(before_error == FALSE)
		{	// 名前
			(void)MemRead(&name_length, sizeof(name_length), 1, stream);
			name = (char*)MEM_ALLOC_FUNC(name_length);
			(void)MemRead(name, 1, name_length, stream);
			// 基本情報
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

			if(base.layer_type >= NUM_LAYER_TYPE || base.x != 0)
			{
				goto return_layers;
			}

			// 値のセット
			layer = CreateLayer(base.x, base.y, base.width, base.height,
				base.channel, base.layer_type, layer, NULL, name, window);
			layer->alpha = base.alpha;
			layer->layer_mode = base.layer_mode;
			layer->flags = base.flags;
			hierarchy[i] = base.layer_set;
			MEM_FREE_FUNC(name);
			window->active_layer = layer;
		}
		else
		{
			name = MEM_STRDUP_FUNC("Error");
			base.layer_type = TYPE_NORMAL_LAYER;
			base.layer_mode = LAYER_BLEND_NORMAL;
			base.x = 0;
			base.y = 0;
			base.width = window->width;
			base.height = window->height;
			base.flags = 0;
			base.alpha = 100;
			base.channel = 4;
			base.layer_set = 0;
			before_error = FALSE;
			// 値のセット
			layer = CreateLayer(base.x, base.y, base.width, base.height,
				base.channel, base.layer_type, layer, NULL, name, window);
			layer->alpha = base.alpha;
			layer->layer_mode = base.layer_mode;
			layer->flags = base.flags;
			hierarchy[i] = base.layer_set;
			stream->data_point -= 4;
			MEM_FREE_FUNC(name);
			window->active_layer = layer;
		}
		// レイヤーのタイプで処理切り替え
		switch(base.layer_type)
		{
		case TYPE_NORMAL_LAYER:	// 通常レイヤー
			// PNG圧縮されたピクセルデータを展開して読み込む
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			next_data_point = (uint32)(stream->data_point + data_size);
			image = CreateMemoryStream(data_size);
			(void)MemRead(image->buff_ptr, 1, data_size, stream);
			pixels = ReadPNGStream(image, (stream_func_t)MemRead,
				&width, &height, &stride);
			if(pixels != NULL)
			{
				(void)memcpy(layer->pixels, pixels, height*stride);
			}
			else
			{
				before_error = TRUE;
			}
			DeleteMemoryStream(image);
			MEM_FREE_FUNC(pixels);
			break;
		case TYPE_VECTOR_LAYER:	// ベクトルレイヤー
			{	// 線をピクセルデータへ変更するための準備
				VECTOR_LAYER_RECTANGLE rect = {0, 0, base.width, base.height};

				layer->layer_data.vector_layer_p =
					(VECTOR_LAYER*)MEM_ALLOC_FUNC(sizeof(*layer->layer_data.vector_layer_p));
				(void)memset(layer->layer_data.vector_layer_p, 0,
					sizeof(*layer->layer_data.vector_layer_p));
				// 線情報をラスタライズしたレイヤー
				(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
				layer->layer_data.vector_layer_p->mix =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);

				// 一番下に空のレイヤー作成
				layer->layer_data.vector_layer_p->base =
					(VECTOR_DATA*)CreateVectorLine(NULL, NULL);
				(void)memset(layer->layer_data.vector_layer_p->base, 0,
					sizeof(*layer->layer_data.vector_layer_p->base));
				layer->layer_data.vector_layer_p->base->line.base_data.layer =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);
				layer->layer_data.vector_layer_p->top_data =
					layer->layer_data.vector_layer_p->base;

				// データのバイト数を読み込む
				next_data_point = (uint32)(stream->data_point +
					ReadVectorLineData(&stream->buff_ptr[stream->data_point], layer));

				// ベクトルデータをラスタライズ
				layer->layer_data.vector_layer_p->flags =
					(VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ALL);
				RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
			}
			break;
		case TYPE_TEXT_LAYER:	// テキストレイヤー
			{
				// フォントサーチ用
				PangoFontFamily** search_font;
				// フォントID
				gint32 font_id;
				// 吹き出しの背景色
				uint8 back_color[4] = {0xff, 0xff, 0xff, 0xff};
				// 吹き出しの線の色
				uint8 line_color[4] = {0x00, 0x00, 0x00, 0xff};

				// データのサイズを読み込む
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);

				// 文字描画領域の座標、幅、高さ、文字サイズ
					// フォントファイル名、色等を読み込む
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
					CreateTextLayer(window, text_base.x, text_base.y, text_base.width, text_base.height,
						text_base.base_size, text_base.font_size, font_id, text_base.color, TEXT_LAYER_BALLOON_TYPE_SQUARE,
						back_color, line_color, 3, NULL, text_base.flags
				);

				(void)MemRead(&name_length, sizeof(name_length), 1, stream);
				layer->layer_data.text_layer_p->text =
					(char*)MEM_ALLOC_FUNC(name_length);
				(void)MemRead(layer->layer_data.text_layer_p->text, 1, name_length, stream);
				// レイヤーをラスタライズ
				RenderTextLayer(window, layer, layer->layer_data.text_layer_p);
			}

			break;
		case TYPE_LAYER_SET:	// レイヤーセット
			{
				LAYER *target = layer->prev;
				next_data_point = (uint32)stream->data_point;
				current_hierarchy = hierarchy[i]+1;

				for(k=i-1; k>=0 && current_hierarchy == hierarchy[k]; k--)
				{
					hierarchy[k] = 0;
					target->layer_set = layer;
					target = target->prev;
				}

				if(stream->buff_ptr[stream->data_point] > 20)
				{
					before_error = TRUE;
					while(stream->buff_ptr[stream->data_point] != 0x0)
					{
						stream->data_point++;
					}
					next_data_point = (uint32)stream->data_point;
				}
			}

			break;
		case TYPE_3D_LAYER:	// 3Dモデリングレイヤー
			if(GetHas3DLayer(app) != FALSE)
			{
				// PNG圧縮されたピクセルデータを展開して読み込む
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				image = CreateMemoryStream(data_size);
				(void)MemRead(image->buff_ptr, 1, data_size, stream);
				pixels = ReadPNGStream(image, (stream_func_t)MemRead,
					&width, &height, &stride);
				if(pixels != NULL)
				{
					(void)memcpy(layer->pixels, pixels, height*stride);
				}
				DeleteMemoryStream(image);
				MEM_FREE_FUNC(pixels);

				// 3Dモデルのデータを読み込む
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				layer->modeling_data = MEM_ALLOC_FUNC(data_size);
				(void)MemRead(layer->modeling_data, 1, data_size, stream);
				layer->modeling_data_size = data_size;

				break;
			}
		default:
			// PNG圧縮されたピクセルデータを展開して読み込む
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			next_data_point = (uint32)(stream->data_point + data_size);
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			image = CreateMemoryStream(data_size);
			(void)MemRead(image->buff_ptr, 1, data_size, stream);
			pixels = ReadPNGStream(image, (stream_func_t)MemRead,
				&width, &height, &stride);
			if(pixels != NULL)
			{
				(void)memcpy(layer->pixels, pixels, height*stride);
			}
			else
			{
				before_error = TRUE;
			}
			DeleteMemoryStream(image);
			MEM_FREE_FUNC(pixels);
		}

		// 追加情報データに移動
		(void)MemSeek(stream, (long)next_data_point, SEEK_SET);
		// 追加情報を読み込む
		(void)MemRead(&layer->num_extra_data, sizeof(layer->num_extra_data), 1, stream);

		for(j=0; j<layer->num_extra_data; j++)
		{
			// データの名前を読み込む
			(void)MemRead(&name_length, sizeof(name_length), 1, stream);
			if(name_length >= 8192)
			{
				break;
			}
			layer->extra_data[j].name = (char*)MEM_ALLOC_FUNC(name_length);
			(void)MemRead(layer->extra_data[j].name, 1, name_length, stream);
			(void)MemRead(&size_t_temp, sizeof(size_t_temp), 1, stream);
			if((int)size_t_temp >= layer->stride * layer->height * 3)
			{
				break;
			}
			layer->extra_data[j].data_size = size_t_temp;
			layer->extra_data[j].data = MEM_ALLOC_FUNC(layer->extra_data[j].data_size);
			(void)MemRead(layer->extra_data[j].data, 1, layer->extra_data[j].data_size, stream);
		}

		// 進捗状況を更新
		current_progress += progress_step;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->widgets->progress), current_progress);
		(void)sprintf(show_text, "%.0f%%", current_progress * 100);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app->widgets->progress), show_text);
#if GTK_MAJOR_VERSION <= 2
		gdk_window_process_updates(app->widgets->progress->window, FALSE);
#else
		gdk_window_process_updates(gtk_widget_get_window(app->widgets->progress), FALSE);
#endif
		while(gdk_events_pending() != FALSE)
		{
#define MAX_REDRAW_TRY 250
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event != NULL)
			{
#if GTK_MAJOR_VERSION <= 2
				if(queued_event->any.window == app->widgets->progress->window
#else
				if(queued_event->any.window == gtk_widget_get_window(app->widgets->progress)
#endif
					&& queued_event->any.type == GDK_EXPOSE
					|| redraw_counter >= MAX_REDRAW_TRY)
				{
					redraw_counter = 0;
					gdk_event_free(queued_event);
					break;
				}
				else
				{
					redraw_counter++;
					gdk_event_free(queued_event);
				}
			}
		}
	}

return_layers:
	// 先頭のレイヤーを返す
	while(layer->prev != NULL)
	{
		layer = layer->prev;
	}

	return layer;
}

/**************************************************************
* ReadOriginalFormatLayersOldVersion3関数                     *
* 旧独自形式のレイヤーデータを読み込む(ファイルバージョン： 3 *
* 引数                                                        *
* stream	: 読み込み元のアドレス                            *
* window	: 描画領域の情報                                  *
* app		: アプリケーションを管理する構造体アドレス        *
* 返り値                                                      *
*	読み込んだレイヤーデータ                                  *
**************************************************************/
LAYER* ReadOriginalFormatLayersOldVersion3(
	MEMORY_STREAM_PTR stream,
	DRAW_WINDOW *window,
	APPLICATION* app,
	uint16 num_layer
)
{
	// レイヤーの基本情報読み込み用
	LAYER_BASE_DATA base;
	// テキストレイヤーの基本情報読み込み用
	TEXT_LAYER_BASE_DATA text_base;
	// 画像データ
	uint8* pixels;
	// 画像データのバイト数
	guint32 data_size;
	// 次のレイヤーデータが開始するポイント
	guint32 next_data_point;
	// 追加するレイヤー
	LAYER* layer = NULL;
	// ピクセルデータ展開用
	MEMORY_STREAM_PTR image;
	// ピクセルデータの幅、高さ、一行分のバイト数
	gint32 width, height, stride;
	// レイヤー・フォントの名前とその長さ
	char* name;
	uint16 name_length;
	// レイヤーセットの階層
	int8 current_hierarchy = 0;
	int8 hierarchy[2048] = {0};
	// BGR→RGB用
	uint8 r;
	// for文用のカウンタ
	int x;
	unsigned int i, j;

	for(i=0; i<num_layer; i++)
	{
		// レイヤーデータ読み込み
			// 名前
		(void)MemRead(&name_length, sizeof(name_length), 1, stream);
		name = (char*)MEM_ALLOC_FUNC(name_length);
		(void)MemRead(name, 1, name_length, stream);
		// 基本情報
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
		// 値のセット
		layer = CreateLayer(base.x, base.y, base.width, base.height,
			base.channel, base.layer_type, layer, NULL, name, window);
		layer->alpha = base.alpha;
		layer->layer_mode = base.layer_mode;
		layer->flags = base.flags;
		hierarchy[i] = base.layer_set;
		MEM_FREE_FUNC(name);
		window->active_layer = layer;

		// レイヤーのタイプで処理切り替え
		switch(base.layer_type)
		{
		case TYPE_NORMAL_LAYER:	// 通常レイヤー
			// PNG圧縮されたピクセルデータを展開して読み込む
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			next_data_point = (uint32)(stream->data_point + data_size);
			image = CreateMemoryStream(data_size);
			(void)MemRead(image->buff_ptr, 1, data_size, stream);
			pixels = ReadPNGStream(image, (stream_func_t)MemRead,
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
		case TYPE_VECTOR_LAYER:	// ベクトルレイヤー
			{	// 線をピクセルデータへ変更するための準備
				VECTOR_LAYER_RECTANGLE rect = {0, 0, base.width, base.height};

				layer->layer_data.vector_layer_p =
					(VECTOR_LAYER*)MEM_ALLOC_FUNC(sizeof(*layer->layer_data.vector_layer_p));
				(void)memset(layer->layer_data.vector_layer_p, 0,
					sizeof(*layer->layer_data.vector_layer_p));
				// 線情報をラスタライズしたレイヤー
				(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
				layer->layer_data.vector_layer_p->mix =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);

				// 一番下に空のレイヤー作成
				layer->layer_data.vector_layer_p->base =
					(VECTOR_DATA*)CreateVectorLine(NULL, NULL);
				(void)memset(layer->layer_data.vector_layer_p->base, 0,
					sizeof(*layer->layer_data.vector_layer_p->base));
				layer->layer_data.vector_layer_p->base->line.base_data.layer =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);
				layer->layer_data.vector_layer_p->top_data =
					layer->layer_data.vector_layer_p->base;

				// データのバイト数を読み込む
				next_data_point = (uint32)(stream->data_point + ReadVectorLineData(
					&stream->buff_ptr[stream->data_point], layer));

				// ベクトルデータをラスタライズ
				layer->layer_data.vector_layer_p->flags =
					(VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ALL);
				RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
			}
			break;
		case TYPE_TEXT_LAYER:	// テキストレイヤー
			{
				// フォントサーチ用
				PangoFontFamily** search_font;
				// フォントID
				gint32 font_id;
				// 吹き出しの背景色
				uint8 back_color[4] = {0xff, 0xff, 0xff, 0xff};
				// 吹き出しの線の色
				uint8 line_color[4] = {0x00, 0x00, 0x00, 0xff};

				// データのサイズを読み込む
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);

				// 文字描画領域の座標、幅、高さ、文字サイズ
					// フォントファイル名、色等を読み込む
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
					CreateTextLayer(window, text_base.x, text_base.y, text_base.width, text_base.height,
						text_base.base_size, text_base.font_size, font_id, text_base.color, TEXT_LAYER_BALLOON_TYPE_SQUARE,
							back_color, line_color, 3, NULL, text_base.flags
				);
				(void)MemRead(&name_length, sizeof(name_length), 1, stream);
				layer->layer_data.text_layer_p->text =
					(char*)MEM_ALLOC_FUNC(name_length);
				(void)MemRead(layer->layer_data.text_layer_p->text, 1, name_length, stream);
				// レイヤーをラスタライズ
				RenderTextLayer(window, layer, layer->layer_data.text_layer_p);
			}

			break;
		case TYPE_LAYER_SET:	// レイヤーセット
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

		// 次のレイヤーデータに移動
		(void)MemSeek(stream, (long)next_data_point, SEEK_SET);
	}

	// 先頭のレイヤーを返す
	while(layer->prev != NULL)
	{
		layer = layer->prev;
	}

	return layer;
}

/*******************************************************
* ReadOriginalFormatLayersOldVersion2関数              *
* 独自形式のレイヤーデータを読み込む(Ver.2)            *
* 引数                                                 *
* stream	: 読み込み元のアドレス                     *
* window	: 描画領域の情報                           *
* app		: アプリケーションを管理する構造体アドレス *
* 返り値                                               *
*	読み込んだレイヤーデータ                           *
*******************************************************/
LAYER* ReadOriginalFormatLayersOldVersion2(
	MEMORY_STREAM_PTR stream,
	DRAW_WINDOW *window,
	APPLICATION* app,
	uint16 num_layer
)
{
	// レイヤーの基本情報読み込み用
	LAYER_BASE_DATA base;
	// テキストレイヤーの基本情報読み込み用
	TEXT_LAYER_BASE_DATA text_base;
	// 画像データ
	uint8* pixels;
	// 画像データのバイト数
	uint32 data_size;
	// 次のレイヤーデータが開始するポイント
	uint32 next_data_point;
	// 追加するレイヤー
	LAYER* layer = NULL;
	// ピクセルデータ展開用
	MEMORY_STREAM_PTR image;
	// ピクセルデータの幅、高さ、一行分のバイト数
	int32 width, height, stride;
	// レイヤー・フォントの名前とその長さ
	char* name;
	uint16 name_length;
	// レイヤーセットの階層
	int8 current_hierarchy = 0;
	int8 hierarchy[2048] = {0};
	// BGR→RGB用
	uint8 r;
	// for文用のカウンタ
	int x;
	unsigned int i, j;

	for(i=0; i<num_layer; i++)
	{
		// レイヤーデータ読み込み
			// 名前
		(void)MemRead(&name_length, sizeof(name_length), 1, stream);
		name = (char*)MEM_ALLOC_FUNC(name_length);
		(void)MemRead(name, 1, name_length, stream);
		// 基本情報
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
		// 値のセット
		layer = CreateLayer(base.x, base.y, base.width, base.height,
			base.channel, base.layer_type, layer, NULL, name, window);
		layer->alpha = base.alpha;
		layer->layer_mode = base.layer_mode;
		layer->flags = base.flags;
		hierarchy[i] = base.layer_set;
		MEM_FREE_FUNC(name);
		window->active_layer = layer;

		// レイヤーのタイプで処理切り替え
		switch(base.layer_type)
		{
		case TYPE_NORMAL_LAYER:	// 通常レイヤー
			// PNG圧縮されたピクセルデータを展開して読み込む
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			next_data_point = (uint32)(stream->data_point + data_size);
			image = CreateMemoryStream(data_size);
			(void)MemRead(image->buff_ptr, 1, data_size, stream);
			pixels = ReadPNGStream(image, (stream_func_t)MemRead,
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
		case TYPE_VECTOR_LAYER:	// ベクトルレイヤー
			{	// 線をピクセルデータへ変更するための準備
				VECTOR_LAYER_RECTANGLE rect = {0, 0, base.width, base.height};

				layer->layer_data.vector_layer_p =
					(VECTOR_LAYER*)MEM_ALLOC_FUNC(sizeof(*layer->layer_data.vector_layer_p));
				(void)memset(layer->layer_data.vector_layer_p, 0,
					sizeof(*layer->layer_data.vector_layer_p));
				// 線情報をラスタライズしたレイヤー
				(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
				layer->layer_data.vector_layer_p->mix =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);

				// 一番下に空のレイヤー作成
				layer->layer_data.vector_layer_p->base =
					(VECTOR_DATA*)CreateVectorLine(NULL, NULL);
				(void)memset(layer->layer_data.vector_layer_p->base, 0,
					sizeof(*layer->layer_data.vector_layer_p->base));
				layer->layer_data.vector_layer_p->base->line.base_data.layer =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);
				layer->layer_data.vector_layer_p->top_data =
					layer->layer_data.vector_layer_p->base;

				// データのバイト数を読み込む
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point
					+ ReadVectorLineData(&stream->buff_ptr[stream->data_point], layer));

				// ベクトルデータをラスタライズ
				layer->layer_data.vector_layer_p->flags =
					(VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ALL);
				RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);
			}
			break;
		case TYPE_TEXT_LAYER:	// テキストレイヤー
			{
				// フォントサーチ用
				PangoFontFamily** search_font;
				// フォントID
				int32 font_id;
				// 吹き出しの背景色
				uint8 back_color[4] = {0xff, 0xff, 0xff, 0xff};
				// 吹き出しの線の色
				uint8 line_color[4] = {0x00, 0x00, 0x00, 0xff};

				// データのサイズを読み込む
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);

				// 文字描画領域の座標、幅、高さ、文字サイズ
					// フォントファイル名、色等を読み込む
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
					CreateTextLayer(window, text_base.x, text_base.y, text_base.width, text_base.height,
						text_base.base_size, text_base.font_size, font_id, text_base.color, TEXT_LAYER_BALLOON_TYPE_SQUARE,
							back_color, line_color, 3, NULL, text_base.flags
				);


				(void)MemRead(&name_length, sizeof(name_length), 1, stream);
				layer->layer_data.text_layer_p->text =
					(char*)MEM_ALLOC_FUNC(name_length);
				(void)MemRead(layer->layer_data.text_layer_p->text, 1, name_length, stream);
				// レイヤーをラスタライズ
				RenderTextLayer(window, layer, layer->layer_data.text_layer_p);
			}

			break;
		case TYPE_LAYER_SET:	// レイヤーセット
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

		// 次のレイヤーデータに移動
		(void)MemSeek(stream, (long)next_data_point, SEEK_SET);
	}

	// 先頭のレイヤーを返す
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
* ReadOriginalFormatLayersOldVersion1関数              *
* 旧独自形式のレイヤーデータを読み込む                 *
* 引数                                                 *
* stream	: 読み込み元のアドレス                     *
* window	: 描画領域の情報                           *
* app		: アプリケーションを管理する構造体アドレス *
* 返り値                                               *
*	読み込んだレイヤーデータ                           *
*******************************************************/
LAYER* ReadOriginalFormatLayersOldVersion1(
	MEMORY_STREAM_PTR stream,
	DRAW_WINDOW *window,
	APPLICATION* app,
	uint16 num_layer
)
{
	// レイヤーの基本情報読み込み用
	LAYER_BASE_DATA base;
	// ベクトルレイヤーの基本情報読み込み用
	VECTOR_LINE_BASE_DATA line_base;
	// テキストレイヤーの基本情報読み込み用
	TEXT_LAYER_BASE_DATA text_base;
	// 画像データ
	uint8* pixels;
	// 画像データのバイト数
	uint32 data_size;
	// 次のレイヤーデータが開始するポイント
	uint32 next_data_point;
	// 追加するレイヤー
	LAYER* layer = NULL;
	// ピクセルデータ展開用
	MEMORY_STREAM_PTR image;
	// ピクセルデータの幅、高さ、一行分のバイト数
	int32 width, height, stride;
	// レイヤー・フォントの名前とその長さ
	char* name;
	uint16 name_length;
	// レイヤーセットの階層
	int8 current_hierarchy = 0;
	int8 hierarchy[2048] = {0};
	// BGR→RGB用
	uint8 r;
	// for文用のカウンタ
	int x;
	unsigned int i, j;

	for(i=0; i<num_layer; i++)
	{
		// レイヤーデータ読み込み
			// 名前
		(void)MemRead(&name_length, sizeof(name_length), 1, stream);
		name = (char*)MEM_ALLOC_FUNC(name_length);
		(void)MemRead(name, 1, name_length, stream);
		// 基本情報
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
		// 値のセット
		layer = CreateLayer(base.x, base.y, base.width, base.height,
			base.channel, base.layer_type, layer, NULL, name, window);
		layer->alpha = base.alpha;
		layer->layer_mode = base.layer_mode;
		layer->flags = base.flags;
		hierarchy[i] = base.layer_set;
		MEM_FREE_FUNC(name);
		window->active_layer = layer;

		// レイヤーのタイプで処理切り替え
		switch(base.layer_type)
		{
		case TYPE_NORMAL_LAYER:	// 通常レイヤー
			// PNG圧縮されたピクセルデータを展開して読み込む
			(void)MemRead(&data_size, sizeof(data_size), 1, stream);
			next_data_point = (uint32)(stream->data_point + data_size);
			image = CreateMemoryStream(data_size);
			(void)MemRead(image->buff_ptr, 1, data_size, stream);
			pixels = ReadPNGStream(image, (stream_func_t)MemRead,
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
		case TYPE_VECTOR_LAYER:	// ベクトルレイヤー
			{	// 線をピクセルデータへ変更するための準備
				VECTOR_LAYER_RECTANGLE rect = {0, 0, base.width, base.height};
				VECTOR_LINE* line;
				// ZIP圧縮展開用
				z_stream decompress_stream;
				// 展開後のデータバイト数
				uint32 vector_data_size;

				layer->layer_data.vector_layer_p =
					(VECTOR_LAYER*)MEM_ALLOC_FUNC(sizeof(*layer->layer_data.vector_layer_p));
				(void)memset(layer->layer_data.vector_layer_p, 0,
					sizeof(*layer->layer_data.vector_layer_p));
				// 線情報をラスタライズしたレイヤー
				(void)memset(window->work_layer->pixels, 0, window->pixel_buf_size);
				layer->layer_data.vector_layer_p->mix =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);

				// 一番下に空のレイヤー作成
				layer->layer_data.vector_layer_p->base =
					(VECTOR_DATA*)CreateVectorLine(NULL, NULL);
				(void)memset(layer->layer_data.vector_layer_p->base, 0,
					sizeof(*layer->layer_data.vector_layer_p->base));
				layer->layer_data.vector_layer_p->base->line.base_data.layer =
					CreateVectorLineLayer(window->work_layer, NULL, &rect);
				layer->layer_data.vector_layer_p->top_data =
					layer->layer_data.vector_layer_p->base;

				// データのバイト数を読み込む
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);
				// 展開後のバイト数を読み込む
				(void)MemRead(&vector_data_size, sizeof(vector_data_size), 1, stream);

				// 展開後のデータを扱うためのストリーム作成
				image = CreateMemoryStream(vector_data_size);

				// データを展開
					// ZIPデータ展開前の準備
				decompress_stream.zalloc = Z_NULL;
				decompress_stream.zfree = Z_NULL;
				decompress_stream.opaque = Z_NULL;
				decompress_stream.avail_in = 0;
				decompress_stream.next_in = Z_NULL;
				(void)inflateInit(&decompress_stream);
				// ZIPデータ展開
				decompress_stream.avail_in = (uInt)data_size;
				decompress_stream.next_in = &stream->buff_ptr[stream->data_point];
				decompress_stream.avail_out = (uInt)vector_data_size;
				decompress_stream.next_out = image->buff_ptr;
				(void)inflate(&decompress_stream, Z_NO_FLUSH);
				(void)inflateEnd(&decompress_stream);

				// 線情報の読み込み
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
					line = CreateVectorLine((VECTOR_LINE*)layer->layer_data.vector_layer_p->top_data, NULL);
					line->base_data.vector_type = line_base.vector_type;
					line->num_points = line_base.num_points;
					line->blur = line_base.blur;
					line->outline_hardness = line_base.outline_hardness;
					// 制御点情報の読み込み
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

					layer->layer_data.vector_layer_p->top_data = (VECTOR_DATA*)line;
				}

				// ベクトルデータをラスタライズ
				layer->layer_data.vector_layer_p->flags =
					(VECTOR_LAYER_FIX_LINE | VECTOR_LAYER_RASTERIZE_ALL);
				RasterizeVectorLayer(window, layer, layer->layer_data.vector_layer_p);

				(void)DeleteMemoryStream(image);
			}
			break;
		case TYPE_TEXT_LAYER:	// テキストレイヤー
			{
				// フォントサーチ用
				PangoFontFamily** search_font;
				// フォントID
				int32 font_id;
				// 吹き出しの背景色
				uint8 back_color[4] = {0xff, 0xff, 0xff, 0xff};
				// 吹き出しの線の色
				uint8 line_color[4] = {0x00, 0x00, 0x00, 0xff};

				// データのサイズを読み込む
				(void)MemRead(&data_size, sizeof(data_size), 1, stream);
				next_data_point = (uint32)(stream->data_point + data_size);

				// 文字描画領域の座標、幅、高さ、文字サイズ
					// フォントファイル名、色等を読み込む
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
					CreateTextLayer(window, text_base.x, text_base.y, text_base.width, text_base.height,
						text_base.base_size, text_base.font_size, font_id, text_base.color, TEXT_LAYER_BALLOON_TYPE_SQUARE,
							back_color, line_color, 3, NULL, text_base.flags
				);

				(void)MemRead(&name_length, sizeof(name_length), 1, stream);
				layer->layer_data.text_layer_p->text =
					(char*)MEM_ALLOC_FUNC(name_length);
				(void)MemRead(layer->layer_data.text_layer_p->text, 1, name_length, stream);
				// レイヤーをラスタライズ
				RenderTextLayer(window, layer, layer->layer_data.text_layer_p);
			}

			break;
		case TYPE_LAYER_SET:	// レイヤーセット
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

		// 次のレイヤーデータに移動
		(void)MemSeek(stream, (long)next_data_point, SEEK_SET);
	}

	// 先頭のレイヤーを返す
	while(layer->prev != NULL)
	{
		layer = layer->prev;
	}

	return layer;
}

/***********************************
* ReadOriginalFormatExtraData関数  *
* 独自形式の追加情報を読み込む     *
* 引数                             *
* stream	: 読み込み元のアドレス *
* window	: 描画領域の情報       *
***********************************/
void ReadOriginalFormatExtraData(MEMORY_STREAM_PTR stream, DRAW_WINDOW* window)
{
	guint32 tag;		// データの種類を表すタグ
	guint32 data_size;	// 各種データのサイズ

	// タグ情報を読み取れる限りループ
	while(MemRead(&tag, sizeof(tag), 1, stream) >= 1)
	{
		// データサイズを読み込む
		if(MemRead(&data_size, sizeof(data_size), 1, stream) < 1)
		{
			break;
		}

		// タグをシステムのエンディアンに
		tag = GUINT32_FROM_BE(tag);
		// タグによって処理を切り替え
		switch(tag)
		{
		case 'actv':	// アクティブなレイヤー
			{	// レイヤーの名前を読み込んでサーチ
				char name[4096];
				(void)MemRead(name, 1, data_size, stream);
				if((window->active_layer = SearchLayer(window->layer, name)) == NULL)
				{
					window->active_layer = window->layer;
				}
			}
			break;
		case 'slct':	// 選択範囲
			{	// 選択範囲はPNG圧縮されているので展開
				MEMORY_STREAM select_stream;
				uint8 *pixels;
				int32 width, height, stride;

				select_stream.block_size = 1;
				select_stream.buff_ptr = &stream->buff_ptr[stream->data_point];
				select_stream.data_size = data_size;
				select_stream.data_point = 0;

				pixels = ReadPNGStream((void*)&select_stream, (stream_func_t)MemRead,
					&width, &height, &stride);

				if(pixels != NULL)
				{
					(void)memcpy(window->selection->pixels, pixels, window->width*window->height);

					if(UpdateSelectionArea(&window->selection_area, window->selection, window->temp_layer) != FALSE)
					{
						window->flags |= DRAW_WINDOW_HAS_SELECTION_AREA;
					}

					MEM_FREE_FUNC(pixels);
				}
			}
			break;
		case 'resl':	// 解像度
			{
				gint32 resolution;
				(void)MemRead(&resolution, sizeof(resolution), 1, stream);
				window->resolution = (int16)resolution;
			}
			break;
		case '2ndb':	// 2つめの背景色
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
		case 'iccp':	// ICCプロファイルの内容
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
* ReadOriginalFormat関数                                   *
* 独自形式のデータを読み込む                               *
* 引数                                                     *
* stram			: 読み込み元のアドレス                     *
* read_func		: 読み込み用の関数ポインタ                 *
* stream_size	: データの総バイト数                       *
* app			: アプリケーションを管理する構造体アドレス *
* data_name		: ファイル名                               *
* 返り値                                                   *
*	描画領域のデータ                                       *
***********************************************************/
DRAW_WINDOW* ReadOriginalFormat(
	void* stream,
	stream_func_t read_func,
	size_t stream_size,
	APPLICATION* app,
	const char* data_name
)
{
	// 返り値
	DRAW_WINDOW* window;
	// レイヤービューに追加するレイヤー
	LAYER* layer;
	// データは一度総てメモリに読み込んでしまう
	MEMORY_STREAM_PTR mem_stream = CreateMemoryStream(stream_size);
	// ピクセルデータ展開用
	MEMORY_STREAM_PTR image;
	// オリジナルの幅、高さ
	gint32 original_width, original_height;
	// 描画領域、レイヤの幅、高さ
	gint32 width, height;
	// 画像データ一行分のバイト数
	gint32 stride;
	// レイヤーの数
	uint16 num_layer;
	// チャンネル数、カラーモード
	uint8 channel, color_mode;
	// 画像データ
	uint8* pixels;
	// 画像データのバイト数
	guint32 data_size;
	// 適正データ判定用の文字列
	const char format_string[] = "Paint Soft KABURAGI";
	// ファイルバージョン
	guint32 file_version;
	// 進捗状況更新の幅
	FLOAT_T progress_step;
	// for文用のカウンタ
	int i;

	// メモリにデータ読み込み
	(void)read_func(mem_stream->buff_ptr, 1, stream_size, stream);

	// ファイルの適正を判定
	if(memcmp(mem_stream->buff_ptr, format_string,
		sizeof(format_string)/sizeof(*format_string)) != 0)
	{	// 不適正
		return NULL;
	}

	// バージョン情報を読み込む
	mem_stream->data_point = sizeof(format_string)/sizeof(*format_string);
	(void)MemRead(&file_version, sizeof(file_version), 1, mem_stream);

	{	// サムネイル有無の判定
		uint8 has_thumbnail;
		(void)MemRead(&has_thumbnail, 1, 1, mem_stream);

		if(has_thumbnail != 0)
		{
			guint32 skip_bytes;
			(void)MemRead(&skip_bytes, sizeof(skip_bytes), 1, mem_stream);
			(void)MemSeek(mem_stream, skip_bytes, SEEK_CUR);
		}
	}

	// チャンネル数とカラーモードを読み込む
	(void)MemRead(&channel, sizeof(channel), 1, mem_stream);
	(void)MemRead(&color_mode, sizeof(color_mode), 1, mem_stream);

	// オリジナルの幅、高さを読み込む
	(void)MemRead(&original_width, sizeof(original_width), 1, mem_stream);
	(void)MemRead(&original_height, sizeof(original_height), 1, mem_stream);

	// 描画領域の幅、高さ(4の倍数)を読み込む
	(void)MemRead(&width, sizeof(width), 1, mem_stream);
	(void)MemRead(&height, sizeof(height), 1, mem_stream);

	// レイヤーの数を読み込む
	(void)MemRead(&num_layer, sizeof(num_layer), 1, mem_stream);

	// 描画領域作成
	window = CreateDrawWindow(
		original_width, original_height, channel, data_name,
		app->widgets->note_book, app->window_num, app
	);
	if(GetHas3DLayer(app) != FALSE)
	{
		DisconnectDrawWindowCallbacks(window->gl_area, window);
	}
	gtk_widget_hide(window->window);

	// 背景画像データを読み込む
	(void)MemRead(&data_size, sizeof(data_size), 1, mem_stream);
	image = CreateMemoryStream(data_size);
	(void)MemRead(image->buff_ptr, 1, data_size, mem_stream);

	pixels = ReadPNGStream(image, (stream_func_t)MemRead, &width, &height,&stride);

	// 描画領域の背景へコピー
	(void)memcpy(window->back_ground, pixels, height*stride);

	// メモリ開放
	DeleteMemoryStream(image);
	MEM_FREE_FUNC(pixels);

	// 進捗状況を更新
	{
		GdkEvent *queued_event;
		gchar show_text[16];

		progress_step = 1.0 / (num_layer + 1);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->widgets->progress), progress_step);
		(void)sprintf(show_text, "%.0f%%", progress_step * 100);
#if GTK_MAJOR_VERSION <= 2
		gdk_window_process_updates(app->widgets->progress->window, FALSE);
#else
		gdk_window_process_updates(gtk_widget_get_window(app->widgets->progress), FALSE);
#endif
		while(gdk_events_pending() != FALSE)
		{
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event != NULL)
			{
#if GTK_MAJOR_VERSION <= 2
				if(queued_event->any.window == app->widgets->progress->window
#else
				if(queued_event->any.window == gtk_widget_get_window(app->widgets->progress)
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

	// レイヤー情報の読み込み
		// 一番下に自動で作られるレイヤーを削除
	DeleteLayer(&window->layer);
	// ファイルバージョンで処理を切り替え
	if(file_version == FILE_VERSION)
	{
		window->layer = ReadOriginalFormatLayers(mem_stream, progress_step, window, app, num_layer);
	}
	else if(file_version == 5)
	{
		window->layer = ReadOriginalFormatLayersOldVersion5(mem_stream, progress_step, window, app, num_layer);
	}
	else if(file_version == 4)
	{
		window->layer = ReadOriginalFormatLayersOldVersion4(mem_stream, progress_step, window, app, num_layer);
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

	// レイヤービューにレイヤーを追加
	layer = window->layer;
	for(i=0; i<num_layer; i++)
	{
		if(layer == NULL)
		{
			window->num_layer = i;
			break;
		}
		LayerViewAddLayer(layer, window->layer, app->layer_window.view, i+1);
		layer = layer->next;
	}

	// アクティブレイヤーは暫定的に一番下に
	window->active_layer = window->layer;

	// 追加情報の読み込み
	ReadOriginalFormatExtraData(mem_stream, window);

	ChangeActiveLayer(window, window->active_layer);
	LayerViewSetActiveLayer(window->active_layer, app->layer_window.view);

	g_object_set_data(G_OBJECT(window->active_layer->widget->box), "signal_id",
		GUINT_TO_POINTER(g_signal_connect(G_OBJECT(window->active_layer->widget->box), "size-allocate",
		G_CALLBACK(Move2ActiveLayer), app)));

	(void)DeleteMemoryStream(mem_stream);

	if(GetHas3DLayer(app) != FALSE)
	{
		SetDrawWindowCallbacks(window->gl_area, window);
	}

	// プログレスバーをリセット
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->widgets->progress), 0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app->widgets->progress), "");

	//SetDrawWindowCallbacks(window->window, window);
	gtk_widget_show(window->window);

	return window;
}

/***********************************************************
* ReadOriginalFormatMixedData関数                          *
* 独自形式の合成データを作成して返す                       *
* 引数                                                     *
* stram			: 読み込み元のアドレス                     *
* read_func		: 読み込み用の関数ポインタ                 *
* stream_size	: データの総バイト数                       *
* app			: アプリケーションを管理する構造体アドレス *
* data_name		: ファイル名                               *
* 返り値                                                   *
*	合成したレイヤー                                       *
***********************************************************/
LAYER* ReadOriginalFormatMixedData(
	void* stream,
	stream_func_t read_func,
	size_t stream_size,
	APPLICATION* app,
	const char* data_name
)
{
	// 返り値
	LAYER *mixed;
	// 描画領域の情報
	DRAW_WINDOW* window;
	// 合成するレイヤー
	LAYER* layer;
	// データは一度総てメモリに読み込んでしまう
	MEMORY_STREAM_PTR mem_stream = CreateMemoryStream(stream_size);
	// ピクセルデータ展開用
	MEMORY_STREAM_PTR image;
	// オリジナルの幅、高さ
	gint32 original_width, original_height;
	// 描画領域、レイヤの幅、高さ
	gint32 width, height;
	// 画像データ一行分のバイト数
	gint32 stride;
	// レイヤーの数
	uint16 num_layer;
	// チャンネル数、カラーモード
	uint8 channel, color_mode;
	// 画像データ
	uint8* pixels;
	// 画像データのバイト数
	guint32 data_size;
	// 適正データ判定用の文字列
	const char format_string[] = "Paint Soft KABURAGI";
	// ファイルバージョン
	guint32 file_version;
	// 進捗状況更新の幅
	FLOAT_T progress_step;

	// メモリにデータ読み込み
	(void)read_func(mem_stream->buff_ptr, 1, stream_size, stream);

	// ファイルの適正を判定
	if(memcmp(mem_stream->buff_ptr, format_string,
		sizeof(format_string)/sizeof(*format_string)) != 0)
	{	// 不適正
		return NULL;
	}

	// バージョン情報を読み込む
	mem_stream->data_point = sizeof(format_string)/sizeof(*format_string);
	(void)MemRead(&file_version, sizeof(file_version), 1, mem_stream);

	{	// サムネイル有無の判定
		uint8 has_thumbnail;
		(void)MemRead(&has_thumbnail, 1, 1, mem_stream);

		if(has_thumbnail != 0)
		{
			guint32 skip_bytes;
			(void)MemRead(&skip_bytes, sizeof(skip_bytes), 1, mem_stream);
			(void)MemSeek(mem_stream, skip_bytes, SEEK_CUR);
		}
	}

	// チャンネル数とカラーモードを読み込む
	(void)MemRead(&channel, sizeof(channel), 1, mem_stream);
	(void)MemRead(&color_mode, sizeof(color_mode), 1, mem_stream);

	// オリジナルの幅、高さを読み込む
	(void)MemRead(&original_width, sizeof(original_width), 1, mem_stream);
	(void)MemRead(&original_height, sizeof(original_height), 1, mem_stream);

	// 描画領域の幅、高さ(4の倍数)を読み込む
	(void)MemRead(&width, sizeof(width), 1, mem_stream);
	(void)MemRead(&height, sizeof(height), 1, mem_stream);

	// レイヤーの数を読み込む
	(void)MemRead(&num_layer, sizeof(num_layer), 1, mem_stream);

	// 描画領域作成
	window = CreateTempDrawWindow(
		original_width, original_height, channel, data_name,
		app->widgets->note_book, app->window_num, app
	);

	// 背景画像データを読み込む
	(void)MemRead(&data_size, sizeof(data_size), 1, mem_stream);
	image = CreateMemoryStream(data_size);
	(void)MemRead(image->buff_ptr, 1, data_size, mem_stream);

	pixels = ReadPNGStream(image, (stream_func_t)MemRead, &width, &height,&stride);

	// 描画領域の背景へコピー
	(void)memcpy(window->back_ground, pixels, height*stride);

	// メモリ開放
	DeleteMemoryStream(image);
	MEM_FREE_FUNC(pixels);

	// 進捗状況を更新
	{
		GdkEvent *queued_event;
		gchar show_text[16];

		progress_step = 1.0 / (num_layer + 1);
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->widgets->progress), progress_step);
		(void)sprintf(show_text, "%.0f%%", progress_step * 100);
#if GDK_MAJOR_VERSION <= 2
		gdk_window_process_updates(app->widgets->progress->window, FALSE);
#else
		gdk_window_process_updates(gtk_widget_get_window(app->widgets->progress), FALSE);
#endif
		while(gdk_events_pending() != FALSE)
		{
			queued_event = gdk_event_get();
			gtk_main_iteration();
			if(queued_event != NULL)
			{
#if GDK_MAJOR_VERSION <= 2
				if(queued_event->any.window == app->widgets->progress->window
#else
				if(queued_event->any.window == gtk_widget_get_window(app->widgets->progress)
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

	// レイヤー情報の読み込み
		// ファイルバージョンで処理を切り替え
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
			window->layer_blend_functions[layer->layer_mode](layer, mixed);
		}

		layer = layer->next;
	}

	(void)DeleteMemoryStream(mem_stream);

	// プログレスバーをリセット
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(app->widgets->progress), 0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(app->widgets->progress), "");

	DeleteDrawWindow(&window);

	return mixed;
}

/*******************************************************************
* ReadOriginalFormatMemoryStream関数                               *
* メモリストリームの独自フォーマットのデータを読み込む(アンドゥ用) *
* 引数                                                             *
* window	: データの読み込み先                                   *
* stream	: メモリストリーム                                     *
*******************************************************************/
void ReadOriginalFormatMemoryStream(
	DRAW_WINDOW* window,
	MEMORY_STREAM_PTR stream
)
{
	// レイヤービューに追加するレイヤー
	LAYER* layer;
	// ピクセルデータ展開用
	MEMORY_STREAM_PTR image;
	// 拡大・縮小・回転用の行列
	cairo_matrix_t matrix;
	// 画像データの幅、高さ
	gint32 width, height;
	// 画像データ一行分のバイト数
	gint32 stride;
	// チャンネル数、カラーモード
	uint8 channel, color_mode;
	// 画像データ
	uint8* pixels;
	// 画像データのバイト数
	guint32 data_size;
	// 適正データ判定用の文字列
	const char format_string[] = "Paint Soft KABURAGI";
	// 進捗状況の更新幅
	FLOAT_T progress_step;
	// for文用のカウンタ
	int i;

	// データの適正を判定
	if(memcmp(format_string, stream->buff_ptr,
		sizeof(format_string)/sizeof(*format_string)) != 0)
	{	// 不適正
		return;
	}

	// バージョン情報をスキップ
	stream->data_point = sizeof(format_string)/sizeof(*format_string) + + sizeof(data_size);

	{	// サムネイル有無の判定
		uint8 has_thumbnail;
		(void)MemRead(&has_thumbnail, 1, 1, stream);

		if(has_thumbnail != 0)
		{
			guint32 skip_bypte;
			(void)MemRead(&skip_bypte, sizeof(skip_bypte), 1, stream);
			(void)MemSeek(stream, skip_bypte, SEEK_CUR);
		}
	}

	// チャンネル数とカラーモードを読み込む
	(void)MemRead(&channel, sizeof(channel), 1, stream);
	(void)MemRead(&color_mode, sizeof(color_mode), 1, stream);

	// オリジナルの幅、高さを読み込む
	(void)MemRead(&width, sizeof(width), 1, stream);
	window->original_width = width;
	(void)MemRead(&height, sizeof(height), 1, stream);
	window->original_height = height;

	// 描画領域の幅、高さ(4の倍数)を読み込む
	(void)MemRead(&width, sizeof(width), 1, stream);
	window->width = width;
	(void)MemRead(&height, sizeof(height), 1, stream);
	window->height = height;

	// レイヤーの数を読み込む
	(void)MemRead(&window->num_layer, sizeof(window->num_layer), 1, stream);

	// 背景画像データを読み込む
	(void)MemRead(&data_size, sizeof(data_size), 1, stream);
	image = CreateMemoryStream(data_size);
	(void)MemRead(image->buff_ptr, 1, data_size, stream);

	pixels = ReadPNGStream(image, (stream_func_t)MemRead, &width, &height,&stride);

	// 描画領域の背景へコピー
	window->back_ground = (uint8*)MEM_ALLOC_FUNC(stride*height);
	(void)memcpy(window->back_ground, pixels, window->height*stride);

	// メモリ開放
	DeleteMemoryStream(image);
	MEM_FREE_FUNC(pixels);

	// ピクセルデータのバイト数を計算
	window->pixel_buf_size = width * height * window->channel;
	// 1行分のバイト数を計算
	window->stride = width * window->channel;

	// 作業用のレイヤーを作詞柄
	window->work_layer = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);

	// アクティブレイヤーと作業用レイヤーを一時的に合成するレイヤー
	window->temp_layer = CreateLayer(0, 0, width, height, 5, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);

	// レイヤーを合成したもの
	window->mixed_layer = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);
	(void)memcpy(window->mixed_layer->pixels, window->back_ground, window->pixel_buf_size);
	// 合成結果に対して拡大・縮小を設定するためのパターン
	window->mixed_pattern = cairo_pattern_create_for_surface(window->mixed_layer->surface_p);
	cairo_pattern_set_filter(window->mixed_pattern, CAIRO_FILTER_FAST);

	// 選択領域のレイヤーを作成
	window->selection = CreateLayer(0, 0, width, height, 1, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);

	// 一番上でエフェクト表示を行うレイヤーを作成
	window->effect = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);

	// テクスチャ用のレイヤーを作成
	window->texture = CreateLayer(0, 0, width, height, 1, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);

	// 下のレイヤーでマスキング、バケツツールでのマスキング用
	window->mask = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);
	window->mask_temp = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);
	// ブラシ描画時の不透明度設定用サーフェース、イメージ
	window->alpha_surface = cairo_image_surface_create_for_data(window->mask->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	window->alpha_cairo = cairo_create(window->alpha_surface);
	window->alpha_temp = cairo_image_surface_create_for_data(window->temp_layer->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	window->alpha_temp_cairo = cairo_create(window->alpha_temp);
	window->gray_mask_temp = cairo_image_surface_create_for_data(window->mask_temp->pixels,
		CAIRO_FORMAT_A8, width, height, width);
	window->gray_mask_cairo = cairo_create(window->gray_mask_temp);

	// 回転表示用の値を計算
	window->cos_value = 1;
	window->trans_x = - window->half_size + ((width / 2) * window->cos_value + (height / 2) * window->sin_value);
	window->trans_y = - window->half_size - ((width / 2) * window->sin_value - (height / 2) * window->cos_value);
	cairo_matrix_init_rotate(&matrix, 0);
	cairo_matrix_translate(&matrix, window->trans_x, window->trans_y);
	window->rotate = cairo_pattern_create_for_surface(window->disp_layer->surface_p);
	cairo_pattern_set_filter(window->rotate, CAIRO_FILTER_FAST);
	cairo_pattern_set_matrix(window->rotate, &matrix);

	// 表示用に拡大・縮小した後の一次記憶メモリ
	window->disp_temp = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);

	// アクティブレイヤーより下を合成した画像の保存用
	window->under_active = CreateLayer(0, 0, width, height, 4, TYPE_NORMAL_LAYER,
		NULL, NULL, NULL, window);
	(void)memcpy(window->under_active->pixels, window->back_ground, window->pixel_buf_size);

	progress_step = 1.0 / (window->num_layer + 1);
	// レイヤー情報の読み込み
	window->layer = ReadOriginalFormatLayers(stream, progress_step, window, window->app, window->num_layer);

	// レイヤービューにレイヤーを追加
	layer = window->layer;
	for(i=0; i<window->num_layer; i++)
	{
		LayerViewAddLayer(layer, window->layer, window->app->layer_window.view, i+1);
		layer = layer->next;
	}

	// アクティブレイヤーは暫定的に一番下に
	window->active_layer = window->layer;

	// 追加情報の読み込み
	ReadOriginalFormatExtraData(stream, window);

	ChangeActiveLayer(window, window->active_layer);
	LayerViewSetActiveLayer(window->active_layer, window->app->layer_window.view);

	g_object_set_data(G_OBJECT(window->active_layer->widget->box), "signal_id",
		GUINT_TO_POINTER(g_signal_connect(G_OBJECT(window->active_layer->widget->box), "size-allocate",
			G_CALLBACK(Move2ActiveLayer), window->app)));
}

/*******************************************
* WriteOriginalFormat関数                  *
* 独自形式のデータを生成する               *
* 引数                                     *
* stream		: 書き込み先のストリーム   *
* write_func	: 書き込み用の関数ポインタ *
* window		: 描画領域の情報           *
* add_thumbnail	: サムネイルの有無         *
* compress		: 圧縮率                   *
*******************************************/
void WriteOriginalFormat(
	void* stream,
	stream_func_t write_func,
	DRAW_WINDOW* window,
	int add_thumbnail,
	int compress
)
{
	// 進捗状況表示のプログレスバー
	GtkWidget *progress;
	// ウィジェット表示イベント待機用
	GdkEvent *queued_event;
	// 進捗状況
	FLOAT_T current_progress;
	// 進捗パーセンテージ表示用のバッファ
	gchar show_text[16] = "";
	// 進捗の更新幅
	FLOAT_T progress_step = 1.0 / (window->num_layer + 1);
	// ピクセルデータ圧縮用
	MEMORY_STREAM_PTR image =
		CreateMemoryStream(window->pixel_buf_size*2);
	// ベクトルデータ圧縮用
	MEMORY_STREAM_PTR vector_stream =
		CreateMemoryStream(window->pixel_buf_size*2);
	// 書きだすレイヤーへのポインタ
	LAYER* layer = window->layer;
	// 書きだすレイヤーの基本情報
	LAYER_BASE_DATA base;
	// テキストレイヤーデータ書き出し時の基本情報
	TEXT_LAYER_BASE_DATA text_base;
	// 適正データ判定用の文字列
	char format_string[] = "Paint Soft KABURAGI";
	// 名前とその長さ
	const char* name;
	uint16 name_length;
	// 所属レイヤーセット
	LAYER *layer_set = NULL;
	// レイヤーセットの階層
	int8 hierarchy = 0;
	// 64bitの場合size_tのバイト数が変わるため変更用
	guint32 size_t_temp;
	int i;	// for文用のカウンタ

	// 進捗パーセンテージを表示
	progress = window->app->widgets->progress;
	(void)sprintf(show_text, "0%%");
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), show_text);

	// イベントを回してメッセージを表示
#if GTK_MAJOR_VERSION <= 2
	gdk_window_process_updates(window->app->widgets->status_bar->window, TRUE);
#else
	gdk_window_process_updates(gtk_widget_get_window(window->app->widgets->status_bar), TRUE);
#endif
	while(gdk_events_pending() != FALSE)
	{
		queued_event = gdk_event_get();
		gtk_main_iteration();
		if(queued_event != NULL)
		{
#if GTK_MAJOR_VERSION <= 2
			if(queued_event->any.window == window->app->widgets->status_bar->window
#else
			if(queued_event->any.window == gtk_widget_get_window(window->app->widgets->status_bar)
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

	// ファイル判定用の文字列を書き出す
	(void)write_func(format_string, 1, sizeof(format_string)/sizeof(*format_string), stream);
	size_t_temp = 0;
	// ファイルバージョンを書き出す
	size_t_temp = FILE_VERSION;
	(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);

	// サムネイルの作成と書き込み
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

			WritePNGStream(image, (stream_func_t)MemWrite, NULL, pixels,
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

	// チャンネル数、カラーモードを書き出す
	(void)write_func(&window->channel, sizeof(window->channel), 1, stream);
	(void)write_func(&window->color_mode, sizeof(window->color_mode), 1, stream);

	// オリジナルの幅、高さを書き出す
	size_t_temp = window->original_width;
	(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
	size_t_temp = window->original_height;
	(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);

	// 幅、高さ(4の倍数)を書き出す
	size_t_temp = window->width;
	(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
	size_t_temp = window->height;
	(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);

	// レイヤーの数を書き出す
	(void)write_func(&window->num_layer, sizeof(window->num_layer), 1, stream);

	// 背景画像のピクセルデータを書き出す
	WritePNGStream(
		image, (stream_func_t)MemWrite, NULL, window->back_ground,
		window->width, window->height, window->stride, window->channel,
		0, compress
	);
	// データバイト数書き出し
	size_t_temp = (uint32)image->data_point;
	(void)write_func(&size_t_temp, sizeof(size_t_temp),
		1, stream);
	// データ書き出し
	(void)write_func(image->buff_ptr, 1, image->data_point, stream);

	// 進捗状況を更新
	current_progress = progress_step;
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), current_progress);
	(void)sprintf(show_text, "%.0f%%", current_progress * 100);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), show_text);
#if GTK_MAJOR_VERSION <= 2
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
#if GTK_MAJOR_VERSION <= 2
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

	// レイヤーのタイプに合わせて書きだすデータを変更
	do
	{
		// レイヤー名を書き出し
		name_length = (uint16)strlen(layer->name) + 1;
		(void)write_func(&name_length, sizeof(name_length), 1, stream);
		(void)write_func(layer->name, 1, name_length, stream);

		// レイヤーセットの階層を調べる
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

		// レイヤー基本情報を書き出し
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

		// レイヤーのタイプで処理切り替え
		switch(layer->layer_type)
		{
		case TYPE_NORMAL_LAYER:	// 通常レイヤー
		default:
			// ピクセルデータをPNG圧縮して書きだす
			(void)MemSeek(image, 0, SEEK_SET);
			WritePNGStream(image, (stream_func_t)MemWrite, NULL, layer->pixels,
				layer->width, layer->height, layer->stride, layer->channel,
				0, compress
			);
			size_t_temp = (guint32)image->data_point;
			(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
			(void)write_func(image->buff_ptr, 1, image->data_point, stream);
			break;
		case TYPE_VECTOR_LAYER:	// ベクトルレイヤー
			WriteVectorLineData(layer, stream, write_func, image, vector_stream, compress);
			break;
		case TYPE_TEXT_LAYER:	// テキストレイヤー
			// 文字描画領域の座標、幅、高さ、文字サイズ
				// フォントファイル名、色等を書き出す
			// データは一度メモリに溜める
			(void)MemSeek(image, 0, SEEK_SET);

			// テキストレイヤーの基本情報(座標、縦書き等)を書き込む
			text_base.x = layer->layer_data.text_layer_p->x;
			text_base.y = layer->layer_data.text_layer_p->y;
			text_base.width = layer->layer_data.text_layer_p->width;
			text_base.height = layer->layer_data.text_layer_p->height;
			text_base.font_size = layer->layer_data.text_layer_p->font_size;
			text_base.balloon_type = layer->layer_data.text_layer_p->balloon_type;
			(void)memcpy(text_base.color, layer->layer_data.text_layer_p->color, 3);
			text_base.edge_position[0][0] = layer->layer_data.text_layer_p->edge_position[0][0];
			text_base.edge_position[0][1] = layer->layer_data.text_layer_p->edge_position[0][1];
			text_base.edge_position[1][0] = layer->layer_data.text_layer_p->edge_position[1][0];
			text_base.edge_position[1][1] = layer->layer_data.text_layer_p->edge_position[1][1];
			text_base.edge_position[2][0] = layer->layer_data.text_layer_p->edge_position[2][0];
			text_base.edge_position[2][1] = layer->layer_data.text_layer_p->edge_position[2][1];
			text_base.arc_start = layer->layer_data.text_layer_p->arc_start;
			text_base.arc_end = layer->layer_data.text_layer_p->arc_end;
			text_base.balloon_data = layer->layer_data.text_layer_p->balloon_data;
			(void)memcpy(text_base.back_color, layer->layer_data.text_layer_p->back_color, 4);
			(void)memcpy(text_base.line_color, layer->layer_data.text_layer_p->line_color, 4);
			text_base.line_width = layer->layer_data.text_layer_p->line_width;
			text_base.base_size = layer->layer_data.text_layer_p->base_size;
			text_base.flags = layer->layer_data.text_layer_p->flags;

			(void)MemWrite(&text_base.x, sizeof(text_base.x), 1, image);
			(void)MemWrite(&text_base.y, sizeof(text_base.y), 1, image);
			(void)MemWrite(&text_base.width, sizeof(text_base.width), 1, image);
			(void)MemWrite(&text_base.height, sizeof(text_base.height), 1, image);
			(void)MemWrite(&text_base.balloon_type, sizeof(text_base.balloon_type), 1, image);
			(void)MemWrite(&text_base.font_size, sizeof(text_base.font_size), 1, image);
			(void)MemWrite(text_base.color, sizeof(text_base.color), 1, image);
			(void)MemWrite(text_base.edge_position, sizeof(text_base.edge_position), 1, image);
			(void)MemWrite(&text_base.arc_start, sizeof(text_base.arc_start), 1, image);
			(void)MemWrite(&text_base.arc_end, sizeof(text_base.arc_end), 1, image);
			(void)MemWrite(text_base.back_color, sizeof(text_base.back_color), 1, image);
			(void)MemWrite(text_base.line_color, sizeof(text_base.line_color), 1, image);
			(void)MemWrite(&text_base.line_width, sizeof(text_base.line_width), 1, image);
			(void)MemWrite(&text_base.base_size, sizeof(text_base.base_size), 1, image);
			(void)MemWrite(&text_base.balloon_data.num_edge, sizeof(text_base.balloon_data.num_edge), 1, image);
			(void)MemWrite(&text_base.balloon_data.num_children, sizeof(text_base.balloon_data.num_children), 1, image);
			(void)MemWrite(&text_base.balloon_data.edge_size, sizeof(text_base.balloon_data.edge_size), 1, image);
			(void)MemWrite(&text_base.balloon_data.random_seed, sizeof(text_base.balloon_data.random_seed), 1, image);
			(void)MemWrite(&text_base.balloon_data.edge_random_size, sizeof(text_base.balloon_data.edge_random_size), 1, image);
			(void)MemWrite(&text_base.balloon_data.edge_random_distance, sizeof(text_base.balloon_data.edge_random_distance), 1, image);
			(void)MemWrite(&text_base.balloon_data.start_child_size, sizeof(text_base.balloon_data.start_child_size), 1, image);
			(void)MemWrite(&text_base.balloon_data.end_child_size, sizeof(text_base.balloon_data.end_child_size), 1, image);
			(void)MemWrite(&text_base.flags, sizeof(text_base.flags), 1, image);
			// フォントの名前を書き込む
			name = pango_font_family_get_name(
				window->app->font_list[layer->layer_data.text_layer_p->font_id]);
			name_length = (uint16)strlen(name) + 1;
			(void)MemWrite(&name_length, sizeof(name_length), 1, image);
			(void)MemWrite((void*)name, 1, name_length, image);

			// テキストデータを書き込む
			if(layer->layer_data.text_layer_p->text != NULL)
			{
				name_length = (uint16)strlen(layer->layer_data.text_layer_p->text) + 1;
				(void)MemWrite(&name_length, sizeof(name_length), 1, image);
				(void)MemWrite(layer->layer_data.text_layer_p->text,
					1, name_length, image);
			}
			else
			{
				name_length = 1;
				(void)MemWrite(&name_length, sizeof(name_length), 1, image);
				name_length = 0;
				(void)MemWrite(&name_length, 1, 1, image);
			}

			// バイト数を書き込んでから
			size_t_temp = (guint32)image->data_point;
			(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
			// データを書き出す
			(void)write_func(image->buff_ptr, 1, image->data_point, stream);

			break;
#if defined(USE_3D_LAYER) && USE_3D_LAYER != 0
		case TYPE_3D_LAYER:
			{
				MEMORY_STREAM *modeling_stream = CreateMemoryStream(1024 * 1024 * 1024);
				// ピクセルデータをPNG圧縮して書きだす
				(void)MemSeek(image, 0, SEEK_SET);
				WritePNGStream(image, (stream_func_t)MemWrite, NULL, layer->pixels,
					layer->width, layer->height, layer->stride, layer->channel,
					0, compress
				);
				SaveProjectContextData(layer->layer_data.project, (void*)modeling_stream,
					(size_t (*)(void*, size_t, size_t, void*))MemWrite, (int (*)(void*, long, int))MemSeek, (long (*)(void*))MemTell);
				size_t_temp = (guint32)(image->data_point + modeling_stream->data_point + sizeof(guint32) + sizeof(guint32));
				(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
				size_t_temp = (guint32)image->data_point;
				(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
				(void)write_func(image->buff_ptr, 1, image->data_point, stream);
				size_t_temp = (guint32)modeling_stream->data_point;
				(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
				(void)write_func(modeling_stream->buff_ptr, 1, modeling_stream->data_point, stream);
				(void)DeleteMemoryStream(modeling_stream);
			}
			break;
#endif
		case TYPE_LAYER_SET:	// レイヤーセット
			break;
		case TYPE_ADJUSTMENT_LAYER:	// 調整レイヤー
			WriteAdjustmentLayerData(stream, write_func, layer->layer_data.adjustment_layer_p);
			break;
		}

		// 追加情報を書き込む
			// 追加情報の数を書き出し
		(void)write_func(&layer->num_extra_data, sizeof(layer->num_extra_data), 1, stream);
		for(i=0; i<layer->num_extra_data; i++)
		{
			// データの名前の長さを書き出す
			name_length = (uint16)(strlen(layer->extra_data[i].name) + 1);
			(void)write_func(&name_length, sizeof(name_length), 1, stream);
			(void)write_func(layer->extra_data[i].name, 1, name_length, stream);
			size_t_temp = (guint32)layer->extra_data[i].data_size;
			(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
			(void)write_func(layer->extra_data[i].data, 1, layer->extra_data[i].data_size, stream);
		}

		layer = layer->next;

		// 進捗状況を更新
		current_progress += progress_step;
		gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), current_progress);
		(void)sprintf(show_text, "%.0f%%", current_progress * 100);
		gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), show_text);
#if GTK_MAJOR_VERSION <= 2
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
#if GTK_MAJOR_VERSION <= 2
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

	// 追加情報の書き出し
	{
		// データのタイプ
		guint32 tag;

		// アクティブレイヤー
		tag = 'actv';
		tag = GUINT32_TO_BE(tag);

		(void)write_func(&tag, sizeof(tag), 1, stream);
		size_t_temp = (guint32)strlen(window->active_layer->name) + 1;
		(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
		(void)write_func(window->active_layer->name, 1, size_t_temp, stream);

		// 選択範囲
		if((window->flags & DRAW_WINDOW_HAS_SELECTION_AREA) != 0)
		{
			tag = 'slct';
			tag = GINT32_TO_BE(tag);

			(void)write_func(&tag, sizeof(tag), 1, stream);

			(void)MemSeek(image, 0, SEEK_SET);
			WritePNGStream((void*)image, (stream_func_t)MemWrite, NULL,
				window->selection->pixels, window->width, window->height, window->width, 1, 0, compress);
			size_t_temp = (guint32)image->data_point;
			(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
			(void)write_func(image->buff_ptr, 1, size_t_temp, stream);
		}

		// 解像度
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

		// 2つめの背景色
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

		// ICCプロファイル
		if(window->icc_profile_data != NULL)
		{
			// ICCプロファイルの内容を書き出す
			tag = 'iccp';
			tag = GINT32_TO_BE(tag);

			(void)write_func(&tag, sizeof(tag), 1, stream);
			size_t_temp = window->icc_profile_size;
			(void)write_func(&size_t_temp, sizeof(size_t_temp), 1, stream);
			(void)write_func(window->icc_profile_data, 1, window->icc_profile_size, stream);
		}
		if(window->icc_profile_path != NULL)
		{
			// ICCプロファイルのファイルパスを書き出す
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

	// プログレスバーをリセット
	gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(progress), 0);
	gtk_progress_bar_set_text(GTK_PROGRESS_BAR(progress), "");
}

/************************************************
* PackLine関数                                  *
* 一行分のデータをRun Length符号化する          *
* 引数                                          *
* start		: 符号化するデータの開始位置        *
* length	: データの長さ                      *
* stride	: 1ピクセルのチャンネルデータの間隔 *
* dst		: 符号化したデータの書き込み先      *
* 返り値                                        *
*	書き出したデータのバイト数                  *
************************************************/
static int32 PackLine(
	uint8* start,
	int32 length,
	int32 stride,
	uint8* dst
)
{
	int32 remain = length;
	int i, j;

	length = 0;
	while(remain > 0)
	{
		// 最初のデータと同じものの個数を調べる
		i = 0;
		while((i < 128) && (remain - i > 0) && (start[0] == start[i*stride]))
		{
			i++;
		}
		// データ有
		if(i > 1)
		{
			*dst++ = - (i - 1);
			*dst++ = *start;

			start += i * stride;
			remain -= i;
			length += 2;
		}
		else
		{
			i = 0;
			while((i < 128) && (remain - (i + 1) > 0)
				&& (start[i*stride] != start[(i + 1)*stride]
				|| remain - (i + 2) <= 0 || start[i*stride] != start[(i + 2)*stride]))
			{
				i++;
			}

			if(remain == 1)
			{
				i = 1;
			}
			if(i > 0)
			{
				*dst++ = i - 1;
				for(j=0; j<i; j++)
				{
					*dst++ = start[j*stride];
				}
				start += i*stride;
				remain -= i;
				length += i + 1;
			}
		}
	}

	return length;
}

/*******************************************************
* GetCompressChannelData関数                           *
* Run Length符号化したデータテーブルを作成する         *
* 引数                                                 *
* channel_data		: 符号化するチャンネルデータ       *
* channel_columns	: 符号化する画像の幅               *
* channel_rows		: 符号化する画像の高さ             *
* stride			: 符号化する画像の一行分のバイト数 *
* length_table		: 一行分のテーブルのバイト数記録先 *
* remain_data		: 符号化後のデータ保存先           *
* 返り値                                               *
*	符号化後のバイト数テーブルのバイト数               *
*******************************************************/
static int GetCompressChannelData(
	uint8* channel_data,
	int32 channel_columns,
	int32 channel_rows,
	int32 stride,
	uint16* length_table,
	uint8* remain_data
)
{
	uint8 *start;
	int32 channel_length;
	int32 length;
	int i;

	channel_length = channel_columns * channel_rows;

	length = 0;
	for(i=0; i<channel_rows; i++)
	{
		start = channel_data + (i * stride);

		length_table[i] = (uint16)PackLine(start, channel_columns, 1, &remain_data[length]);

		length += length_table[i];
	}

	return length;
}

/*************************************************
* DecodeRunLengthData関数                        *
* Run Length符号化されたデータを復号化する       *
* 引数                                           *
* src			: 復号元のデータ                 *
* length_table	: 一行分のバイト数を格納した配列 *
* height		: レイヤーの高さ                 *
* dst			: 復号したデータの書き込み先     *
*************************************************/
static void DecodeRunLengthData(
	uint8* src,
	uint16* length_table,
	int32 height,
	int32 stride,
	uint8* dst
)
{
	uint8 *data;
	uint8 *start = dst;
	uint8 channel_value;
	int next_pointer = 0;
	int packed_size;
	int unpacked_size;
	int n;
	int i;
	for(i=0; i<height; i++)
	{
		data = src + next_pointer;
		dst = start + (i*stride);
		next_pointer += packed_size = length_table[i];
		unpacked_size = stride;
		while(packed_size > 0 && unpacked_size > 0)
		{
			n = *data;
			data++;
			packed_size--;
			if(n == 128)
			{
				continue;
			}
			else if(n > 128)
			{
				n -= 256;
			}

			if(n < 0)
			{
				n = 1 - n;
				if(packed_size == 0)
				{
					break;
				}
				if(n > unpacked_size)
				{
					break;
				}
				channel_value = *data;
				for( ; n > 0; --n)
				{
					if(unpacked_size == 0)
					{
						break;
					}
					*dst = channel_value;
					dst++;
					unpacked_size--;
				}
				if(unpacked_size != 0)
				{
					data++;
					packed_size--;
				}
			}
			else
			{
				n++;
				for( ; n > 0; --n)
				{
					if(packed_size == 0)
					{
						break;
					}
					if(unpacked_size == 0)
					{
						break;
					}
					*dst = *data;
					dst++;
					unpacked_size--;
					data++;
					packed_size--;
				}
			}
		}

		if(unpacked_size > 0)
		{
			for(n = 0; n < unpacked_size; ++n)
			{
				*dst = 0;
				dst++;
			}
		}
	}
}

/**********************************************************************
* PSDのレイヤー合成モードからKABURAGI用のレイヤー合成モードに変換する *
* 引数                                                                *
* key	: 合成モードを表すデータ                                      *
* 返り値                                                              *
*	合成モード定数                                                    *
**********************************************************************/
static eLAYER_BLEND_MODE GetLayerBlendModeFromPhotoShopDocument(const uint8* key)
{
	if(memcmp(key, "norm", 4) == 0)
	{
		return LAYER_BLEND_NORMAL;
	}
	else if(memcmp(key, "lddg", 4) == 0)
	{
		return LAYER_BLEND_ADD;
	}
	else if(memcmp(key, "mul ", 4) == 0)
	{
		return LAYER_BLEND_MULTIPLY;
	}
	else if(memcmp(key, "scrn", 4) == 0)
	{
		return LAYER_BLEND_SCREEN;
	}
	else if(memcmp(key, "over", 4) == 0)
	{
		return LAYER_BLEND_OVERLAY;
	}
	else if(memcmp(key, "lite", 4) == 0)
	{
		return LAYER_BLEND_LIGHTEN;
	}
	else if(memcmp(key, "dark", 4) == 0)
	{
		return LAYER_BLEND_DARKEN;
	}
	else if(memcmp(key, "div ", 4) == 0)
	{
		return LAYER_BLEND_DODGE;
	}
	else if(memcmp(key, "idiv", 4) == 0)
	{
		return LAYER_BLEND_BURN;
	}
	else if(memcmp(key, "hLit", 4) == 0)
	{
		return LAYER_BLEND_HARD_LIGHT;
	}
	else if(memcmp(key, "sLit", 4) == 0)
	{
		return LAYER_BLEND_SOFT_LIGHT;
	}
	else if(memcmp(key, "diff", 4) == 0)
	{
		return LAYER_BLEND_DIFFERENCE;
	}
	else if(memcmp(key, "hue ", 4) == 0)
	{
		return LAYER_BLEND_HSL_HUE;
	}
	else if(memcmp(key, "sat ", 4) == 0)
	{
		return LAYER_BLEND_HSL_SATURATION;
	}
	else if(memcmp(key, "colr", 4) == 0)
	{
		return LAYER_BLEND_HSL_COLOR;
	}
	else if(memcmp(key, "lum ", 4) == 0)
	{
		return LAYER_BLEND_HSL_LUMINOSITY;
	}
	return LAYER_BLEND_NORMAL;
}

/*************************************************
* ReadPhotoShopDocument関数                      *
* PSD形式を読み込む                              *
* 引数                                           *
* stream		: 書き込み先のストリーム         *
* write_func	: 書き込み用の関数ポインタ       *
* seek_func		: シーク用の関数ポインタ         *
* tell_func		: シーク位置取得用の関数ポインタ *
* 返り値                                         *
*	正常終了時:一番下のレイヤー, 異常終了時:NULL *
*************************************************/
LAYER* ReadPhotoShopDocument(
	void* stream,
	stream_func_t read_func,
	seek_func_t seek_func,
	long (*tell_func)(void*)
)
{
	// 一番下のレイヤー
	LAYER *bottom = NULL;
	// 処理中のレイヤー
	LAYER *layer = NULL;
	// 最も大きいレイヤーの幅・高さ
	int max_width = 0,	max_height = 0;
	// Run Lengthのバイトサイズテーブル情報
	uint16 *length_table;
	// 画像データのバイト数
	size_t data_size;
	// 画像データの開始位置
	long data_start;
	// キャンバスの幅、高さ
	int canvas_width, canvas_height;
	// セクションの長さ
	unsigned long block_length;
	// セクションの終了位置
	unsigned long block_end;
	// カラーモード
	uint16 color_mode;
	// チャンネル数
	uint16 channels;
	// チャンネルインデックス
	int (*channel_indices)[4];
	// チャンネルデータのバイト数
	guint32 (*channel_bytes)[4];
	// ピクセルデータ
	uint8 *pixels = NULL;
	// 圧縮されたデータ
	uint8 *rle_data = NULL;
	// レイヤーの名前
	char layer_name[1024];
	// レイヤーの数
	int16 num_layers;
	// 4バイトバッファ
	guint32 dw;
	gint32 signed_dw;
	// 2バイトバッファ
	uint16 word;
	int16 signed_word;
	// 1バイトバッファ
	uint8 byte;
	// 比較用のキー
	char *key;
	// Signature格納バッファ
	uint8 signature[16];
	// for文用のカウンタ
	int i, j, k;

	// Headerを読み込む
	(void)read_func(signature, 1, 4, stream);
	key = "8BPS";
	if(memcmp(signature, key, 4) != 0)
	{
		return NULL;
	}
	// Version
	(void)read_func(&word, sizeof(word), 1, stream);
	// Reserved
	for(i=0; i<3; i++)
	{
		(void)read_func(&word, sizeof(word), 1, stream);
	}
	// Channels
	(void)read_func(&channels, sizeof(channels), 1, stream);
	channels = GUINT16_FROM_BE(channels);
	// Rows(キャンバスの高さ)
	(void)read_func(&dw, sizeof(dw), 1, stream);
	canvas_height = (int)(GUINT32_FROM_BE(dw));
	// Columns(キャンバスの幅)
	(void)read_func(&dw, sizeof(dw), 1, stream);
	canvas_width = (int)(GUINT32_FROM_BE(dw));
	// ピクセルデータ深度
	(void)read_func(&word, sizeof(word), 1, stream);
	word = GUINT16_FROM_BE(word);
	if(word != 8)
	{
		return NULL;
	}
	// カラーモード
	(void)read_func(&color_mode, sizeof(color_mode), 1, stream);
	color_mode = GUINT16_FROM_BE(color_mode);
	if(!(
		color_mode == 1	// グレースケール
		|| color_mode == 3	// RGB
		|| color_mode == 4	// CMYK
	))
	{
		return NULL;
	}

	// Color Mode Data Block
	(void)read_func(&dw, sizeof(dw), 1, stream);
	block_length = (unsigned int)(GUINT32_FROM_BE(dw));
	block_end = (unsigned int)(tell_func(stream) + block_length);

	// 現在、インデックスカラー等には非対応
	if(seek_func(stream, block_end, SEEK_SET) < 0)
	{
		return NULL;
	}

	// Image Resource Block
	(void)read_func(&dw, sizeof(dw), 1, stream);
	block_length = (unsigned int)(GUINT32_FROM_BE(dw));
	block_end = (unsigned int)(tell_func(stream) + block_length);
	if(seek_func(stream, block_end, SEEK_SET) < 0)
	{
		return NULL;
	}

	// Layer and Mask Information Block
	(void)read_func(&dw, sizeof(dw), 1, stream);
	data_size = GUINT32_FROM_BE(dw);
	(void)read_func(&dw, sizeof(dw), 1, stream);
	block_length = (unsigned int)(GUINT32_FROM_BE(dw));
	block_end = (unsigned int)(tell_func(stream) + block_length);

	// レイヤーの数
	(void)read_func(&signed_word, sizeof(signed_word), 1, stream);
	num_layers = GINT16_FROM_BE(signed_word);
	if(num_layers < 0)
	{
		num_layers = - num_layers;
	}
	channel_indices = (int (*)[4])MEM_ALLOC_FUNC(sizeof(*channel_indices)*num_layers);
	channel_bytes = (guint32 (*)[4])MEM_ALLOC_FUNC(sizeof(*channel_bytes)*num_layers);
	// レイヤーの情報を読み込む
	for(i=0; i<num_layers; i++)
	{
		// レイヤーの座標
		int32 layer_x,	layer_y;
		// レイヤーの幅、高さ
		int32 layer_width,	layer_height;
		// レイヤーのチャンネル数
		uint8 channels;
		// レイヤーの合成モード
		eLAYER_BLEND_MODE blend_mode;
		// レイヤーの不透明度
		uint8 opacity;
		// 非表示、透明保護等のフラグ
		uint8 layer_flags;
		// 追加情報のバイト数
		unsigned int extra_data_size;
		unsigned int extra_data_end;
		// レイヤー名の長さ
		uint16 name_length;
		// UTF8への変換用
		char *utf8_name = NULL;
		// レイヤー名のエンコード方式
		int is_unicode = FALSE;
		// Top
		(void)read_func(&signed_dw, sizeof(signed_dw), 1, stream);
		signed_dw = GINT32_FROM_BE(signed_dw);
		layer_y = signed_dw;
		// Left
		(void)read_func(&signed_dw, sizeof(signed_dw), 1, stream);
		signed_dw = GINT32_FROM_BE(signed_dw);
		layer_x = signed_dw;
		// Bottom
		(void)read_func(&signed_dw, sizeof(signed_dw), 1, stream);
		signed_dw = GINT32_FROM_BE(signed_dw);
		layer_height = signed_dw - layer_y;
		// Right
		(void)read_func(&signed_dw, sizeof(signed_dw), 1, stream);
		signed_dw = GINT32_FROM_BE(signed_dw);
		layer_width = signed_dw - layer_x;

		if(max_width < layer_width)
		{
			max_width = layer_width;
		}
		if(max_height < layer_height)
		{
			max_height = layer_height;
		}

		// Channels
		(void)read_func(&word, sizeof(word), 1, stream);
		word = GUINT16_FROM_BE(word);
		channels = (uint8)word;

		for(j=0; j<channels; j++)
		{
			(void)read_func(&word, sizeof(word), 1, stream);
			word = GUINT16_FROM_BE(word);
			if(word == 0xFFFF)
			{
				channel_indices[i][j] = 3;
			}
			else
			{
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
				if(word == 0)
				{
					channel_indices[i][j] = 2;
				}
				else if(word == 2)
				{
					channel_indices[i][j] = 0;
				}
				else
#endif
				{
					channel_indices[i][j] = word;
				}
			}
			(void)read_func(&dw, sizeof(dw), 1, stream);
			dw = GUINT32_FROM_BE(dw);
			channel_bytes[i][j] = dw;
		}

		// 合成モード
		(void)read_func(signature, 1, 4, stream);
		key = "8BIM";
		if(memcmp(signature, key, 4) != 0)
		{
		}
		(void)read_func(signature, 1, 4, stream);
		blend_mode = GetLayerBlendModeFromPhotoShopDocument(signature);
		// 不透明度
		(void)read_func(&opacity, sizeof(opacity), 1, stream);
		// クリッピング(読み飛ばす)
		(void)read_func(&byte, sizeof(byte), 1, stream);
		// フラグ
		layer_flags = 0;
		(void)read_func(&byte, sizeof(byte), 1, stream);
		if((byte & 0x01) != 0)
		{
			layer_flags |= LAYER_LOCK_OPACITY;
		}
		if((byte & 0x02) != 0)
		{
			layer_flags |= LAYER_FLAG_INVISIBLE;
		}
		// Padding
		(void)read_func(&byte, sizeof(byte), 1, stream);
		// Extra Data Size
		(void)read_func(&dw, sizeof(dw), 1, stream);
		extra_data_size = GUINT32_FROM_BE(dw);
		extra_data_end = (unsigned int)(tell_func(stream) + extra_data_size);
		// レイヤーマスク(読み飛ばす)
		(void)read_func(&dw, sizeof(dw), 1, stream);
		// Blending Ranges(読み飛ばす)
		(void)read_func(&dw, sizeof(dw), 1, stream);
		// レイヤー名
		(void)read_func(&byte, sizeof(byte), 1, stream);
		name_length = byte;
		(void)read_func(layer_name, 1, name_length, stream);
		layer_name[name_length] = '\0';

		while(extra_data_size >= 4 && ftell(stream) < (long)(extra_data_end))
		{
			long key_data_size;
			(void)read_func(signature, 1, 4, stream);
			if(memcmp(signature, "8BIM", 4) == 0)
			{
				(void)read_func(signature, 1, 4, stream);
				if(memcmp(signature, "luni", 4) == 0)
				{
					is_unicode = TRUE;
					(void)read_func(&dw, sizeof(dw), 1, stream);
					(void)read_func(&dw, sizeof(dw), 1, stream);
					dw = GUINT32_FROM_BE(dw);
					name_length = (uint16)(dw * 2);
					(void)read_func(layer_name, 1, name_length, stream);
				}
				else
				{
					(void)read_func(&dw, sizeof(dw), 1, stream);
					key_data_size = (long)(GUINT32_FROM_BE(dw));
					(void)seek_func(stream, key_data_size, SEEK_CUR);
				}
			}
		}

		(void)seek_func(stream, extra_data_end, SEEK_SET);

		if(is_unicode != FALSE)
		{
			utf8_name = g_convert(layer_name, name_length, "UTF-8", "UTF-16BE",
				NULL, NULL, NULL);
		}
		else
		{
			utf8_name = g_locale_to_utf8(layer_name, -1, NULL, NULL, NULL);
		}
		layer = CreateLayer(layer_x, layer_y, layer_width, layer_height,
			4, TYPE_NORMAL_LAYER, layer, NULL, utf8_name, NULL);
		(void)memset(layer->pixels, 0, layer->height * layer->stride);
		if(channels == 3)
		{
			for(j=0; j<layer->width * layer->height; j++)
			{
				layer->pixels[j*4+3] = 0xff;
			}
		}
		layer->channel = channels;
		layer->flags = layer_flags;
		layer->alpha = (opacity * 100) / 255;
		g_free(utf8_name);
		if(bottom == NULL)
		{
			bottom = layer;
		}
	}
	//(void)seek_func(stream, block_end, SEEK_SET);
	// 画像データ読み込み
	data_start = tell_func(stream);
	layer = bottom;
	pixels = (uint8*)MEM_ALLOC_FUNC(max_width * max_height * 2);
	rle_data = (uint8*)MEM_ALLOC_FUNC(max_width * max_height * 2);
	length_table = (uint16*)MEM_ALLOC_FUNC(max_height * 2);
	for(i=0; i<num_layers; i++)
	{
		for(j=0; j<layer->channel; j++)
		{
			int channel_index = channel_indices[i][j];
			block_end = data_start + channel_bytes[i][j];
			data_start = block_end;
			(void)read_func(&word, sizeof(word), 1, stream);
			word = GUINT16_FROM_BE(word);
			if(word == 0)
			{	// 圧縮無し
				(void)read_func(pixels, 1, layer->width * layer->height, stream);
				for(k=0; k<layer->width * layer->height; k++)
				{
					layer->pixels[k*4+channel_index] = pixels[k];
				}
			}
			else if(word == 1)
			{	// Run Length圧縮
				size_t point = 0;
				(void)read_func(length_table, sizeof(*length_table), layer->height, stream);
				for(k=0; k<layer->height; k++)
				{
					length_table[k] = GUINT16_FROM_BE(length_table[k]);
					(void)read_func(&rle_data[point], 1, length_table[k], stream);
					point += length_table[k];
				}
				DecodeRunLengthData(rle_data, length_table, layer->height, layer->width, pixels);
				for(k=0; k<layer->width * layer->height; k++)
				{
					layer->pixels[k*4+channel_index] = pixels[k];
				}
			}
			else
			{
				j--;
				continue;
			}
			//(void)seek_func(stream, block_end, SEEK_SET);
		}
		layer = layer->next;
	}

	MEM_FREE_FUNC(rle_data);
	MEM_FREE_FUNC(pixels);
	MEM_FREE_FUNC(length_table);
	MEM_FREE_FUNC(channel_indices);
	MEM_FREE_FUNC(channel_bytes);

	return bottom;
}

/*************************************************
* WritePhotoShopDocument関数                     *
* PSD形式で書きだす                              *
* 引数                                           *
* stream		: 書き込み先のストリーム         *
* write_func	: 書き込み用の関数ポインタ       *
* seek_func		: シーク用の関数ポインタ         *
* tell_func		: シーク位置取得用の関数ポインタ *
* window		: 描画領域の情報                 *
*************************************************/
void WritePhotoShopDocument(
	void* stream,
	stream_func_t write_func,
	seek_func_t seek_func,
	long (*tell_func)(void*),
	DRAW_WINDOW* window
)
{
	// レイヤーデータ書き出し用
	LAYER* layer = window->layer;
	// 下のレイヤーでマスクしてから書き出す用
	LAYER *mask;
	// レイヤー名(システムのコードに変換する)
	char* name;
	// ピクセルデータ
	uint8 *byte_data = (uint8*)MEM_ALLOC_FUNC(window->pixel_buf_size);
	// RLE圧縮後のデータ
	uint8 *rle_data = (uint8*)MEM_ALLOC_FUNC(window->height * (window->width + 10 + (window->width/100)));
	// RLE圧縮後のデータバイト数
	int compressed_data_length;
	// チャンネルのデータのバイト数書き込み位置
	long (*channel_length_position)[4] =
		(long (*)[4])MEM_ALLOC_FUNC(sizeof(*channel_length_position)*window->num_layer);
	// RLEテーブルのデータ
	uint16 *length_table = (uint16*)MEM_ALLOC_FUNC(sizeof(*length_table)*window->height);
	// チャンネルデータのバイト数書き込み位置
	long length_table_position;
	// チャンネルデータのバイト数
	int channel_data_length;
	// レイヤーデータのバイト数
	size_t data_size;
	// 4バイトバッファ
	guint32 dw;
	// 2バイトバッファ
	uint16 word;
	int16 signed_word;
	// 1バイトバッファ
	uint8 byte;
	// レイヤー名の長さ
	uint8 name_length;
	// パディングするバイト数
	uint8 padding;
	// データを後から書き込む用の位置記憶
	long before_point, data_size_point;
	// ExtraDataを書き込む位置
	long extra_point;
	// for文用のカウンタ
	int i, j, k;

	// Headerを書き込む
		// Signature("8BPS")書き出し
	(void)write_func("8BPS", 1, 4, stream);
	// Version
	word = 1;
	word = GUINT16_TO_BE(word);
	(void)write_func(&word, sizeof(word), 1, stream);
	// Reserved(6バイト)
	word = 0;
	for(i=0; i<3; i++)
	{
		(void)write_func(&word, sizeof(word), 1, stream);
	}
	// Channels
	word = window->channel;
	word = GUINT16_TO_BE(word);
	(void)write_func(&word, sizeof(word), 1, stream);
	// Rows(画像の高さ)
	dw = window->height;
	dw = GUINT32_TO_BE(dw);
	(void)write_func(&dw, sizeof(dw), 1, stream);
	// Columns(画像の幅)
	dw = window->width;
	dw = GUINT32_TO_BE(dw);
	(void)write_func(&dw, sizeof(dw), 1, stream);
	// ピクセルデータ深度
	word = 8;
	word = GUINT16_TO_BE(word);
	(void)write_func(&word, sizeof(word), 1, stream);
	// カラーモード(RGB)
	word = 3;
	word = GUINT16_TO_BE(word);
	(void)write_func(&word, sizeof(word), 1, stream);

	// Color Mode Data Block
	dw = 0;
	dw = GUINT32_TO_BE(dw);
	(void)write_func(&dw, sizeof(dw), 1, stream);

	// Image Resource Block (データ無し)
	dw = 0;
	dw = GUINT32_TO_BE(dw);
	(void)write_func(&dw, sizeof(dw), 1, stream);

	// Layer and Mask Information Block, Image Data
		// Block Length Layer and Mask Information BlockとnSizeの領域を開けておく
	data_size_point = tell_func(stream);
	dw = 0;
	(void)write_func(&dw, sizeof(dw), 1, stream);
	(void)write_func(&dw, sizeof(dw), 1, stream);

	// Layer Num書き出し
	signed_word = window->num_layer;
	if(window->channel == 4)
	{
		signed_word = - signed_word;
	}
	signed_word = GINT16_TO_BE(signed_word);
	(void)write_func(&signed_word, sizeof(signed_word), 1, stream);
	// レイヤーデータ書き出し
	for(i=0; i<window->num_layer; i++)
	{
		// 書き出すチャンネルのインデックス
		int channel;

		// 名前をシステムコードに変換
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
			channel_length_position[i][3] = tell_func(stream);
			dw = sizeof(word) + layer->width*layer->height;
			dw = GUINT32_TO_BE(dw);
			(void)write_func(&dw, sizeof(dw), 1, stream);
		}
		for(j=0; j<3; j++)
		{
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			if(j == 0)
			{
				channel = 2;
			}
			else if(j == 2)
			{
				channel = 0;
			}
			else
#endif
			{
				channel = j;
			}
			// ChannelID
			word = j;
			word = GUINT16_TO_BE(word);
			(void)write_func(&word, sizeof(word), 1, stream);
			// LengthOfChannelData
			channel_length_position[i][channel] = tell_func(stream);
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

		// 保留にしておいたExtra Dataを書き出す
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
	// 画像データ書き出し
	layer = window->layer;
	for(i=0; i<window->num_layer; i++)
	{
		// 下のレイヤーでのマスク無し
		if((layer->flags & LAYER_MASKING_WITH_UNDER_LAYER) == 0)
		{
			// 書き出すチャンネルのインデックス
			int channel;
 
			if(layer->channel == 4)
			{
				channel_data_length = sizeof(uint16);
				// Compression
				word = 1;
				word = GUINT16_TO_BE(word);
				(void)write_func(&word, sizeof(word), 1, stream);

				length_table_position = tell_func(stream);
				// 一度ダミーのテーブルを書き出しておく
				(void)write_func(length_table, sizeof(uint16), layer->height, stream);
				channel_data_length += layer->height * sizeof(uint16);

				// ピクセルデータ
				for(j=0; j<layer->width*layer->height; j++)
				{
					byte_data[j] = layer->pixels[j*layer->channel+3];
				}
				compressed_data_length = GetCompressChannelData(byte_data,
					layer->width, layer->height, layer->width, length_table, rle_data);
				channel_data_length += compressed_data_length;
				(void)write_func(rle_data, 1, compressed_data_length, stream);
				(void)seek_func(stream, length_table_position, SEEK_SET);
				for(j=0; j<layer->height; j++)
				{
					word = length_table[j];
					word = GUINT16_TO_BE(word);
					(void)write_func(&word, sizeof(word), 1, stream);
				}
				(void)seek_func(stream, channel_length_position[i][3], SEEK_SET);
				dw = channel_data_length;
				dw = GUINT32_TO_BE(dw);
				(void)write_func(&dw, sizeof(dw), 1, stream);
				(void)seek_func(stream, 0, SEEK_END);
				//(void)write_func(byte_data, 1, layer->width*layer->height, stream);
			}

			(void)memcpy(window->temp_layer->pixels, window->mixed_layer->pixels, window->pixel_buf_size);
			cairo_set_source_surface(window->temp_layer->cairo_p, layer->surface_p, 0, 0);
			cairo_set_operator(window->temp_layer->cairo_p, CAIRO_OPERATOR_OVER);
			cairo_paint(window->temp_layer->cairo_p);

			// チャンネルデータ書き出し
			for(j=0; j<3; j++)
			{
				channel_data_length = sizeof(uint16);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
				if(j == 0)
				{
					channel = 2;
				}
				else if(j == 2)
				{
					channel = 0;
				}
				else
#endif
				{
					channel = j;
				}

				// Compression
				word = 1;
				word = GUINT16_TO_BE(word);
				(void)write_func(&word, sizeof(word), 1, stream);

				length_table_position = tell_func(stream);
				// 一度ダミーのテーブルを書き出しておく
				(void)write_func(length_table, sizeof(uint16), layer->height, stream);
				channel_data_length += layer->height * sizeof(uint16);

				// ピクセルデータ
				for(k=0; k<layer->width*layer->height; k++)
				{
					byte_data[k] = window->temp_layer->pixels[k*layer->channel+channel];
					//byte_data[k] = layer->pixels[k*layer->channel+j];
				}
				compressed_data_length = GetCompressChannelData(byte_data,
					layer->width, layer->height, layer->width, length_table, rle_data);
				channel_data_length += compressed_data_length;
				(void)write_func(rle_data, 1, compressed_data_length, stream);
				(void)seek_func(stream, length_table_position, SEEK_SET);
				for(k=0; k<layer->height; k++)
				{
					word = length_table[k];
					word = GUINT16_TO_BE(word);
					(void)write_func(&word, sizeof(word), 1, stream);
				}
				(void)seek_func(stream, channel_length_position[i][channel], SEEK_SET);
				dw = channel_data_length;
				dw = GUINT32_TO_BE(dw);
				(void)write_func(&dw, sizeof(dw), 1, stream);
				(void)seek_func(stream, 0, SEEK_END);
				//(void)write_func(byte_data, 1, layer->width*layer->height, stream);
			}
		}
		else
		{	// マスク有り
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
				channel_data_length = sizeof(uint16);
				// Compression
				word = 1;
				word = GUINT16_TO_BE(word);
				(void)write_func(&word, sizeof(word), 1, stream);

				length_table_position = tell_func(stream);
				// 一度ダミーのテーブルを書き出しておく
				(void)write_func(length_table, sizeof(uint16), layer->height, stream);
				channel_data_length += layer->height * sizeof(uint16);

				// ピクセルデータ
				for(j=0; j<layer->width*layer->height; j++)
				{
					byte_data[j] = window->temp_layer->pixels[j*layer->channel+3];
				}
				compressed_data_length = GetCompressChannelData(byte_data,
					layer->width, layer->height, layer->width, length_table, rle_data);
				channel_data_length += compressed_data_length;
				(void)write_func(rle_data, 1, compressed_data_length, stream);
				(void)seek_func(stream, length_table_position, SEEK_SET);
				for(j=0; j<layer->height; j++)
				{
					word = length_table[j];
					word = GUINT16_TO_BE(word);
					(void)write_func(&word, sizeof(word), 1, stream);
				}
				(void)seek_func(stream, channel_length_position[i][3], SEEK_SET);
				dw = channel_data_length;
				dw = GUINT32_TO_BE(dw);
				(void)write_func(&dw, sizeof(dw), 1, stream);
				(void)seek_func(stream, 0, SEEK_END);
				//(void)write_func(byte_data, 1, layer->width*layer->height, stream);
			}
			// チャンネルデータ書き出し
			for(j=0; j<3; j++)
			{
				int channel_id = j;
				channel_data_length = sizeof(uint16);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
				if(channel_id == 0)
				{
					channel_id = 2;
				}
				else if(channel_id == 2)
				{
					channel_id = 0;
				}
#endif

				// Compression
				word = 1;
				word = GUINT16_TO_BE(word);
				(void)write_func(&word, sizeof(word), 1, stream);

				length_table_position = tell_func(stream);
				// 一度ダミーのテーブルを書き出しておく
				(void)write_func(length_table, sizeof(uint16), layer->height, stream);
				channel_data_length += layer->height * sizeof(uint16);

				// ピクセルデータ
				for(k=0; k<layer->width*layer->height; k++)
				{
					byte_data[k] = window->mask_temp->pixels[k*layer->channel+channel_id];
				}
				compressed_data_length = GetCompressChannelData(byte_data,
					layer->width, layer->height, layer->width, length_table, rle_data);
				channel_data_length += compressed_data_length;
				(void)write_func(rle_data, 1, compressed_data_length, stream);
				(void)seek_func(stream, length_table_position, SEEK_SET);
				for(k=0; k<layer->height; k++)
				{
					word = length_table[k];
					word = GUINT16_TO_BE(word);
					(void)write_func(&word, sizeof(word), 1, stream);
				}
				(void)seek_func(stream, channel_length_position[i][channel_id], SEEK_SET);
				dw = channel_data_length;
				dw = GUINT32_TO_BE(dw);
				(void)write_func(&dw, sizeof(dw), 1, stream);
				(void)seek_func(stream, 0, SEEK_END);
				//(void)write_func(byte_data, 1, layer->width*layer->height, stream);
			}
		}

		//(void)write_func(&word, 1, 1, stream);
		layer = layer->next;
	}

	// Layer and Mask Information Block, Image Dataのサイズを書きだす
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

	// フィルターデータ分を0で埋める
	(void)memset(byte_data, 0, 32);
	(void)write_func(byte_data, 1, 32, stream);

	// 全体のマスクは無いので0を書き出す
	dw = 0;
	(void)write_func(&dw, sizeof(dw), 1, stream);

	// レイヤーを透明部分を残して合成
	//layer = MixLayerForSave(window);
	layer = window->mixed_layer;
	if(layer->channel == 4)
	{
		channel_data_length = sizeof(uint16);
		// Compression
		word = 1;
		word = GUINT16_TO_BE(word);
		(void)write_func(&word, sizeof(word), 1, stream);

		length_table_position = tell_func(stream);
		// 一度ダミーのテーブルを書き出しておく
		(void)write_func(length_table, sizeof(uint16), layer->height, stream);
		channel_data_length += layer->height * sizeof(uint16);

		for(j=0; j<layer->width*layer->height; j++)
		{
			byte_data[j] = layer->pixels[j*layer->channel+3];
		}
		compressed_data_length = GetCompressChannelData(byte_data,
			layer->width, layer->height, layer->width, length_table, rle_data);
		channel_data_length += compressed_data_length;
		(void)write_func(rle_data, 1, compressed_data_length, stream);
		(void)seek_func(stream, length_table_position, SEEK_SET);
		for(j=0; j<layer->height; j++)
		{
			word = length_table[j];
			word = GUINT16_TO_BE(word);
			(void)write_func(&word, sizeof(word), 1, stream);
		}
		(void)seek_func(stream, channel_length_position[i][3], SEEK_SET);
		dw = channel_data_length;
		dw = GUINT32_TO_BE(dw);
		(void)write_func(&dw, sizeof(dw), 1, stream);
		(void)seek_func(stream, 0, SEEK_END);
		//(void)write_func(byte_data, 1, layer->width*layer->height, stream);
	}
	for(j=0; j<3; j++)
	{
		int channel_id = j;
		channel_data_length = sizeof(uint16);
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
		if(channel_id == 0)
		{
			channel_id = 2;
		}
		else if(channel_id == 2)
		{
			channel_id = 0;
		}
#endif

		// Compression
		word = 1;
		word = GUINT16_TO_BE(word);
		(void)write_func(&word, sizeof(word), 1, stream);
		for(k=0; k<layer->width*layer->height; k++)
		{
			byte_data[k] = layer->pixels[k*layer->channel+channel_id];
		}
		compressed_data_length = GetCompressChannelData(byte_data,
			layer->width, layer->height, layer->width, length_table, rle_data);
		channel_data_length += compressed_data_length;
		(void)write_func(rle_data, 1, compressed_data_length, stream);
		(void)seek_func(stream, length_table_position, SEEK_SET);
		for(k=0; k<layer->height; k++)
		{
			word = length_table[k];
			word = GUINT16_TO_BE(word);
			(void)write_func(&word, sizeof(word), 1, stream);
		}
		(void)seek_func(stream, channel_length_position[i][channel_id], SEEK_SET);
		dw = channel_data_length;
		dw = GUINT32_TO_BE(dw);
		(void)write_func(&dw, sizeof(dw), 1, stream);
		(void)seek_func(stream, 0, SEEK_END);
		//(void)write_func(byte_data, 1, layer->width*layer->height, stream);
	}
	//DeleteLayer(&layer);
	MEM_FREE_FUNC(length_table);
	MEM_FREE_FUNC(rle_data);
	MEM_FREE_FUNC(channel_length_position);
	MEM_FREE_FUNC(byte_data);

	(void)write_func(&word, 1, 1, stream);
}

/**************************************************************
* ReadTiffTagData関数                                         *
* TIFFファイルのタグ情報を読み込む                            *
* 引数                                                        *
* system_file_path	: 実行OSでのファイルパス                  *
* resolution		: 解像度(DPI)を格納するアドレス           *
* icc_profile_data	: ICCプロファイルのデータを受けるポインタ *
* icc_profile_size	: ICCプロファイルのデータのバイト数       *
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
* WriteTiff関数                                        *
* TIFF形式で画像を書き出す                             *
* 引数                                                 *
* system_file_path	: OSのファイルシステムに即したパス *
* window			: 画像を書き出す描画領域の情報     *
* write_aplha		: 透明情報を書き出すか否か         *
* compress			: 圧縮を行うか否か                 *
* profile_data		: ICCプロファイルのデータ          *
*					  (書き出さない場合はNULL)         *
* profile_data_size	: ICCプロファイルのデータバイト数  *
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
	{
		char application_name[256];
		(void)sprintf(application_name, "Paint Soft %s",
			(GetHas3DLayer(app) == FALSE) ? "KABURAGI" : "MIKADO");
		TIFFSetField(out, TIFFTAG_SOFTWARE, application_name);
	}

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

/***************************************
* DecodeTgaRle関数                     *
* TGA画像のRLE圧縮をデコードする       *
* 引数                                 *
* src		: 圧縮されたデータ         *
* dst		: デコード後のデータ保存先 *
* width		: 画像の幅                 *
* height	: 画像の高さ               *
* channel	: 画像のチャンネル数       *
***************************************/
static void DecodeTgaRle(
	uint8* src,
	uint8* dst,
	int width,
	int height,
	int channel
)
{
	// RLE圧縮展開後のデータ
	uint8 data[4];
	// BGR→RGB用
	uint8 *swap;
	uint8 b;
	// 繰り返し回数
	int num;
	// 圧縮されているか否か
	int compressed;
	// for文用のカウンタ
	int i, j, k;

	for(i=0; i<width*height; i++)
	{
		// 先頭の1バイトから圧縮/無圧縮を判定
		compressed = ((*src & (1 << 7)) == 0) ? FALSE : TRUE;
		num = *src;
		src++;

		if(compressed != FALSE)
		{	// 圧縮データ
			num = (num & 127) + 1;
			for(j=0; j<channel; j++)
			{
				data[j] = *src;
				src++;
			}

			// BGR→RGBしながらデコード
			for(j=0; j<num; j++)
			{
				swap = dst;
				for(k=0; k<channel; k++)
				{
					*dst = data[k];
					dst++;
				}
				b = swap[0];
				swap[0] = swap[2];
				swap[2] = b;
			}
		}
		else
		{	// 無圧縮データ
			num++;
			for(j=0; j<num; j++)
			{
				swap = dst;
				for(k=0; k<channel; k++)
				{
					*dst = *src;
					src++,	dst++;
				}
				// BGR→RGB
				b = swap[0];
				swap[0] = swap[2];
				swap[2] = b;
			}
		}
	}
}

/**********************************************
* ReadTgaStream関数                           *
* TGAファイルの読み込み(必要に応じてデコード) *
* 引数                                        *
* stream	: 読み込み元                      *
* read_func	: 読み込みに使う関数ポインタ      *
* data_size	: 画像データのバイト数            *
* width		: 画像の幅                        *
* height	: 画像の高さ                      *
* channel	: 画像のチャンネル数              *
* 返り値                                      *
*	ピクセルデータ(失敗時はNULL)              *
**********************************************/
uint8* ReadTgaStream(
	void* stream,
	stream_func_t read_func,
	size_t data_size,
	int* width,
	int* height,
	int* channel
)
{
#define TGA_HEADER_SIZE 18
	// ピクセルデータ
	uint8 *pixels = NULL;
	// ヘッダデータ
	uint8 header_data[TGA_HEADER_SIZE] = {0};
	// 画像の幅・高さ(ローカル)
	int local_width, local_height;
	// 画像のチャンネル数(ローカル)
	int local_channel = 0;
	// ビット深さ
	int depth;
	// 1行分のバイト数
	int stride;
	// 圧縮の有無
	int compressed = FALSE;
	// インデックスか否か
	int indexed = FALSE;
	// for文用のカウンタ
	int i;

	// ヘッダ情報の読み込み
	(void)read_func(header_data, 1, TGA_HEADER_SIZE, stream);

	// 幅と高さを計算
	local_width = (header_data[13] << 8) + header_data[12];
	local_height = (header_data[15] << 8) + header_data[14];

	// インデックス・圧縮の情報を読み込み
	switch(header_data[2])
	{
	case 0x00:
		return NULL;
	case 0x01:
		indexed = TRUE;
		local_channel = 3;
		break;
	case 0x02:
		local_channel = 3;
		break;
	case 0x03:
		local_channel = 1;
		break;
	case 0x09:
		compressed = TRUE;
		indexed = TRUE;
		local_channel = 3;
		break;
	case 0x0A:
		compressed = TRUE;
		local_channel = 3;
		break;
	case 0x0B:
		compressed = TRUE;
		local_channel = 1;
		break;
	}

	depth = header_data[16];

	if(depth == 32)
	{
		local_channel = 4;
	}

	if(width != NULL)
	{
		*width = local_width;
	}
	if(height != NULL)
	{
		*height = local_height;
	}
	if(channel != NULL)
	{
		*channel = local_channel;
	}
	stride = local_channel * local_width;

	// ピクセルデータのメモリ確保
	pixels = (uint8*)MEM_ALLOC_FUNC(stride * local_height);

	// ピクセルデータの読み込み
	if(compressed == FALSE)
	{	// 無圧縮
		(void)read_func(pixels, 1, stride * local_height, stream);
	}
	else
	{	// 圧縮
		uint8 *tga_data;	// 圧縮されたデータ
		tga_data = (uint8*)MEM_ALLOC_FUNC(data_size - TGA_HEADER_SIZE);
		(void)read_func(tga_data, 1, data_size - TGA_HEADER_SIZE, stream);
		DecodeTgaRle(tga_data, pixels, local_width, local_height, local_channel);
		MEM_FREE_FUNC(tga_data);
	}

	// BBR→RGB
	if(local_channel >= 3)
	{
		uint8 b;
		for(i=0; i<local_width*local_height; i++)
		{
			b = pixels[i*local_channel];
			pixels[i*local_channel] = pixels[i*local_channel+2];
			pixels[i*local_channel+2] = b;
		}
	}

	return pixels;
}

typedef enum _eBMP_TYPE
{
	BMP_TYPE_UNKNOWN,
	BMP_TYPE_OS2,
	BMP_TYPE_MS
} eBMP_TYPE;

typedef enum _eBMP_COMPRESS_TYPE
{
	BMP_NO_COMPRESS,
	BMP_COMPRESS_RLE8,
	BMP_COMPRESS_RLE4,
	BMP_COMPRESS_BIT_FIELDS
} eBMP_COMPRESS_TYPE;

/*****************************************
* DecodeBmpRLE8関数                      *
* 8ビットのRLE圧縮データをデコードする   *
* 引数                                   *
* src		: 圧縮データ                 *
* dst		: デコード後のデータの格納先 *
* width		: 画像データの幅             *
* height	: 画像データの高さ           *
* stride	: 1行分のバイト数            *
* 返り値                                 *
*	デコード後のバイト数                 *
*****************************************/
static unsigned int DecodeBmpRLE8(
	void* src,
	void* dst,
	int width,
	int height,
	int stride
)
{
	uint8 *current_src = src;
	// for文用のカウンタ
	int x, y;
	int i;

	if(dst == NULL || src == NULL || width <= 0 || height <= 0)
	{
		return 0;
	}

	(void)memset(dst, 0, stride * height);

	for(y=0; y<height; y++)
	{
		uint8 *current_dst = (uint8*)dst + stride * y;
		// EOB、EOL出現フラグ
		int eob = FALSE, eol;
		uint8 code[2];

		// EOBに到達するまでループ
		for(x=0; ; )
		{
			// 2バイト取得
			code[0] = *current_src++;
			code[1] = *current_src++;

			if(code[0] == 0)
			{
				eol = FALSE;

				switch(code[1])
				{
				case 0x00:	// EOL
					eol = TRUE;
					break;
				case 0x01:	// EOB
					eob = TRUE;
					break;
				case 0x02:	// 位置移動情報
					code[0] = *current_src++;
					code[1] = *current_src++;
					current_dst += code[0] + stride * code[1];
					break;
				default:	// 絶対モードデータ
					x += code[1];
					for(i=0; i<code[1]; i++)
					{
						*current_dst++ = *current_src++;
					}

					// パディング
					if((code[1] & 1) != 0)
					{
						current_src++;
					}
				}

				if(eol != FALSE || eob != FALSE)
				{
					break;
				}
			}
			else if(x < width)
			{
				// エンコードデータ
				x += code[0];
				for(i=0; i<code[0]; i++)
				{
					*current_dst++ = code[1];
				}
			}
			else
			{
				return 0;
			}
		}

		if(eob != FALSE)
		{
			break;
		}
	}

	return stride * height;
}

/*****************************************
* DecodeBmpRLE4関数                      *
* 4ビットのRLE圧縮データをデコードする   *
* 引数                                   *
* src		: 圧縮データ                 *
* dst		: デコード後のデータの格納先 *
* width		: 画像データの幅             *
* height	: 画像データの高さ           *
* stride	: 1行分のバイト数            *
* 返り値                                 *
*	デコード後のバイト数                 *
*****************************************/
static unsigned int DecodeBmpRLE4(
	void* src,
	void* dst,
	int width,
	int height,
	int stride
)
{
	uint8 *current_src = src;
	// for文用のカウンタ
	int x, y;
	int i;

	if(dst == NULL || src == NULL || width <= 0 || height <= 0)
	{
		return 0;
	}

	(void)memset(dst, 0, stride * height);

	for(y=0; y<height; y++)
	{
		uint8 *current_dst = (uint8*)dst + stride * y;
		// 次の書き出し位置判定用
		int is_hi = TRUE;
		// EOB、EOL出現フラグ
		int eob = FALSE, eol;
		uint8 code[2];

		// EOLに到達するまでループ
		for(x=0; ; )
		{
			code[0] = *current_src++;
			code[1] = *current_src++;

			// 1バイト目が0なら非圧縮
			if(code[0] == 0)
			{
				eol = FALSE;

				// 識別コードで処理を切り替え
				switch(code[1])
				{
				case 0x00:	// EOL
					eol = TRUE;
					break;
				case 0x01:	// EOB
					eob = TRUE;
					break;
				case 0x02:	// 位置移動情報
					code[0] = *current_src++;
					code[1] = *current_src++;
					x += code[0];
					y += code[1];
					current_dst += (int)code[0] / 2 + stride * code[1];

					// 奇数ピクセルなら半バイト位置を進める
					if((code[0] & 1) != 0)
					{
						is_hi = !is_hi;
						if(is_hi != FALSE)
						{
							current_dst++;
						}
					}
					break;
				default:	// 絶対モード
					{
						int dst_byte = ((int)code[1] + 1) / 2;

						if(is_hi != FALSE)
						{
							for(i=0; i<dst_byte; i++)
							{
								*current_dst++ = *current_src++;
							}

							// 奇数ピクセルなら半バイト戻る
							if((code[1] & 1) != 0)
							{
								*(--current_dst) &= 0xf0;
								is_hi = FALSE;
							}
						}
						else
						{
							for(i=0; i<dst_byte; i++)
							{
								*current_dst++ |= (*current_src >> 4) & 0x0f;
								*current_dst |= (*current_src++ << 4) & 0xf0;
							}

							// 奇数ピクセルなら半バイト戻る
							if((code[1] & 1) != 0)
							{
								*current_dst = 0x00;
								is_hi = TRUE;
							}
						}

						// パディング
						if((dst_byte & 1) != 0)
						{
							current_src++;
						}
					}
					break;
				}

				if(eol != FALSE || eob != FALSE)
				{
					break;
				}
			}
			else if(x < width)
			{
				int dst_byte;

				// 圧縮データ
				x += code[0];

				// 次の書き出しが上位4ビットでない場合
				if(is_hi == FALSE)
				{
					*current_dst++ = (code[1] >> 4) & 0x0f;
					code[1] = ((code[1] >> 4) & 0x0f) | ((code[1] << 4) & 0xf0);
					code[0]--;
					is_hi = TRUE;
				}

				// 書き出し
				dst_byte = ((int)code[0] + 1) / 2;
				for(i=0; i<dst_byte; i++)
				{
					*current_dst++ = code[1];
				}

				// 奇数ピクセルなら半バイト戻る
				if((code[0] & 1) != 0)
				{
					*(--current_dst) &= 0xf0;
					is_hi = FALSE;
				}
			}
			else
			{
				return 0;
			}
		}

		if(eob != FALSE)
		{
			break;
		}
	}

	return stride * height;
}

/*******************************************************
* ReadBmpStream関数                                    *
* BMPファイルのピクセルデータを読み込む                *
* 引数                                                 *
* stream		: 画像データのストリーム               *
* read_func		: ストリーム読み込み用の関数ポインタ   *
* seek_func		: ストリームのシークに使う関数ポインタ *
* data_size		: 画像データのバイト数                 *
* width			: 画像の幅の格納先                     *
* height		: 画像の高さの格納先                   *
* channel		: 画像のチャンネル数の格納先           *
* resolution	: 画像の解像度(dpi)の格納先            *
* 返り値                                               *
*	ピクセルデータ(失敗時はNULL)                       *
*******************************************************/
uint8* ReadBmpStream(
	void* stream,
	stream_func_t read_func,
	seek_func_t seek_func,
	size_t data_size,
	int* width,
	int* height,
	int* channel,
	int* resolution
)
{
#define BMP_FILE_HEADER_SIZE 14
#define OS2_BMP_INFO_HEADER_SIZE 12
#define MS_BMP_INFO_HEADER_SIZE 40
	uint8 *pixels = NULL;
	int local_width, local_height;
	int local_channel;
	int stride;
	int file_size;
	int data_offset;
	int info_header_size;
	int reverse_y = FALSE;
	int bit_per_pixel = 0;
	eBMP_TYPE bmp_type = BMP_TYPE_UNKNOWN;
	eBMP_COMPRESS_TYPE compress_type = BMP_NO_COMPRESS;
	uint8 header[64] = {0};
	int i;

	// ファイルヘッダの読み込み
	(void)read_func(header, 1, BMP_FILE_HEADER_SIZE, stream);
	// ファイルタイプのチェック
	if(header[0] != 'B' || header[1] != 'M')
	{
		return NULL;
	}
	// ファイルサイズの取得
	file_size = (header[5] << 24) + (header[4] << 16)
		+ (header[3] << 8) + header[2];
	// ピクセルデータまでのオフセット
	data_offset = (header[13] << 24) + (header[12] << 16)
		+ (header[11] << 8) + header[10];

	// インフォヘッダのバイト数を取得
	(void)read_func(header, 1, 4, stream);
	info_header_size = (header[3] << 24) + (header[2] << 16)
		+ (header[1] << 8) + header[0];
	// ヘッダのバイト数でOS/2 or Windowsを判別
	if(info_header_size == OS2_BMP_INFO_HEADER_SIZE)
	{
		bmp_type = BMP_TYPE_OS2;
		(void)read_func(header, 1, OS2_BMP_INFO_HEADER_SIZE - 4, stream);
		local_width = (header[1] << 8) + header[0];
		local_height = (header[3] << 8) + header[2];
		if(header[4] != 1)
		{
			return NULL;
		}
		bit_per_pixel = (header[7] << 8) + header[6];
	}
	else if(info_header_size == MS_BMP_INFO_HEADER_SIZE)
	{
		bmp_type = BMP_TYPE_MS;
		(void)read_func(header, 1, MS_BMP_INFO_HEADER_SIZE - 4, stream);
		local_width = (header[3] << 24) + (header[2] << 16)
			+ (header[1] << 8) + header[0];
		local_height = (header[7] << 24) + (header[6] << 16)
			+ (header[5] << 8) + header[4];
		if(local_height < 0)
		{
			reverse_y = TRUE;
			local_height = - local_height;
		}

		if(header[8] != 1)
		{
			return NULL;
		}

		bit_per_pixel = (header[11] << 8) + header[10];
		compress_type = (eBMP_COMPRESS_TYPE)((header[15] << 24) + (header[14] << 16)
			+ (header[13] << 8) + header[12]);

		if(resolution != NULL)
		{
			*resolution = (header[23] << 24) + (header[22] << 16)
				+ (header[21] << 8) + header[20];
			*resolution = (int)(*resolution * 0.0254 + 0.5);
		}
	}
	else
	{
		return NULL;
	}

	// チャンネル数の設定
	switch(bit_per_pixel)
	{
	case 1:
		local_channel = 1;
		break;
	case 4:
	case 8:
	case 24:
		local_channel = 3;
		break;
	case 32:
		local_channel = 4;
		break;
	default:
		return NULL;
	}

	if(width != NULL)
	{
		*width = local_width;
	}
	if(height != NULL)
	{
		*height = local_height;
	}
	if(channel != NULL)
	{
		*channel = local_channel;
	}
	if(compress_type == BMP_NO_COMPRESS)
	{
		stride = local_channel * local_width;
	}
	else
	{
		stride = local_channel * ((local_width * 8 + 31) / 32 * 4);
	}

	(void)seek_func(stream, data_offset, SEEK_SET);

	// ピクセルデータのメモリ確保
	pixels = (uint8*)MEM_ALLOC_FUNC(stride * local_height);

	// ピクセルデータの読み込み
	switch(compress_type)
	{
	case BMP_NO_COMPRESS:
		if(bit_per_pixel == 24 || bit_per_pixel == 32)
		{
			(void)read_func(pixels, 1, stride * local_height, stream);
		}
		else if(bit_per_pixel == 1)
		{
			BIT_STREAM bit_stream = {0};
			uint8 *bmp_data = (uint8*)MEM_ALLOC_FUNC(data_size - data_offset);
			(void)read_func(bmp_data, 1, data_size - data_offset, stream);

			bit_stream.bytes = bmp_data;
			bit_stream.num_bytes = data_size - data_offset;

			(void)memset(pixels, 0, stride * local_height);
			for(i=0; i<local_width * local_height; i++)
			{
				if(BitsRead(&bit_stream, 1) != 0)
				{
					pixels[i] = 0xff;
				}
			}

			MEM_FREE_FUNC(bmp_data);
		}
		else
		{
			MEM_FREE_FUNC(pixels);
			return NULL;
		}
		break;
	case BMP_COMPRESS_RLE8:
		{
			uint8 *bmp_data = (uint8*)MEM_ALLOC_FUNC(data_size - data_offset);
			(void)read_func(bmp_data, 1, data_size - data_offset, stream);

			DecodeBmpRLE8(bmp_data, pixels, local_width, local_height, stride);
			MEM_FREE_FUNC(bmp_data);
		}
		break;
	case BMP_COMPRESS_RLE4:
		{
			uint8 *bmp_data = (uint8*)MEM_ALLOC_FUNC(data_size - data_offset);
			(void)read_func(bmp_data, 1, data_size - data_offset, stream);

			DecodeBmpRLE4(bmp_data, pixels, local_width, local_height, stride);
			MEM_FREE_FUNC(bmp_data);
		}
		break;
	}

	if(reverse_y != FALSE)
	{
		int copy_stride = local_width * local_channel;
		uint8 *copy_pixels = (uint8*)MEM_ALLOC_FUNC(copy_stride * local_height);

		for(i=0; i<local_height; i++)
		{
			(void)memcpy(&copy_pixels[i*copy_stride], &pixels[(local_height-i-1)*stride], copy_stride);
		}

		MEM_FREE_FUNC(pixels);
		pixels = copy_pixels;
	}
	else
	{
		int copy_stride = local_width * local_channel;
		uint8 *copy_pixels = (uint8*)MEM_ALLOC_FUNC(copy_stride * local_height);

		for(i=0; i<local_height; i++)
		{
			(void)memcpy(&copy_pixels[i*copy_stride], &pixels[i*stride], copy_stride);
		}

		MEM_FREE_FUNC(pixels);
		pixels = copy_pixels;
	}

	if(local_channel >= 3)
	{
		uint8 r;
		for(i=0; i<local_width*local_height; i++)
		{
			r = pixels[i*local_channel];
			pixels[i*local_channel] = pixels[i*local_channel+2];
			pixels[i*local_channel+2] = r;
		}
	}

	return pixels;
}

typedef struct _DDS_PIXEL_FORMAT
{
	uint32 size;
	uint32 flags;
	uint32 four_cc;
	uint32 bpp;
	uint32 red_mask;
	uint32 green_mask;
	uint32 blue_mask;
	uint32 alpha_mask;
} DDS_PIXEL_FORMAT;

typedef struct _DDS_CAPS
{
	uint32 caps1;
	uint32 caps2;
	uint32 caps3;
	uint32 caps4;
} DDS_CAPS;

typedef struct _DDS_COLOR_KEY
{
	uint32 low_value;
	uint32 high_value;
} DDS_COLOR_KEY;

typedef struct _DDS_SURFACE_DESCRIPTION
{
	uint32 size;
	uint32 flags;
	uint32 height;
	uint32 width;
	uint32 pitch;
	uint32 depth;
	uint32 mip_map_levels;
	uint32 alpha_bit_depth;
	uint32 reserved;
	uint32 surface;

	DDS_COLOR_KEY color_key_dest_overlay;
	DDS_COLOR_KEY color_key_dest_bitblt;
	DDS_COLOR_KEY color_key_source_overlay;
	DDS_COLOR_KEY color_key_source_bitblt;

	DDS_PIXEL_FORMAT format;
	DDS_CAPS caps;

	uint32 texture_stage;
} DDS_SURFACE_DESCRIPTION;

static size_t ReadDdsPixelFormat(
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	DDS_PIXEL_FORMAT* format
)
{
	size_t read_size = 0;
	read_size += read_func(&format->size, 1, sizeof(format->size), src);
	read_size += read_func(&format->flags, 1, sizeof(format->flags), src);
	read_size += read_func(&format->four_cc, 1, sizeof(format->four_cc), src);
	read_size += read_func(&format->bpp, 1, sizeof(format->bpp), src);
	read_size += read_func(&format->red_mask, 1, sizeof(format->red_mask), src);
	read_size += read_func(&format->green_mask, 1, sizeof(format->green_mask), src);
	read_size += read_func(&format->blue_mask, 1, sizeof(format->blue_mask), src);
	read_size += read_func(&format->alpha_mask, 1, sizeof(format->alpha_mask), src);

	return read_size;
}

static size_t ReadDdsCaps(
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	DDS_CAPS* caps
)
{
	size_t read_size = 0;
	read_size += read_func(&caps->caps1, 1, sizeof(caps->caps1), src);
	read_size += read_func(&caps->caps2, 1, sizeof(caps->caps2), src);
	read_size += read_func(&caps->caps3, 1, sizeof(caps->caps3), src);
	read_size += read_func(&caps->caps4, 1, sizeof(caps->caps4), src);

	return read_size;
}

static size_t ReadDdsColorKey(
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	DDS_COLOR_KEY* color_key
)
{
	size_t read_size = 0;
	read_size += read_func(&color_key->low_value, 1, sizeof(color_key->low_value), src);
	read_size += read_func(&color_key->high_value, 1, sizeof(color_key->high_value), src);

	return read_size;
}

static size_t ReadDdsSurfaceDescription(
	void* stream,
	size_t (*read_func)(void*, size_t, size_t, void*),
	DDS_SURFACE_DESCRIPTION* description
)
{
	size_t read_size = 0;
	read_size += read_func(&description->size, 1, sizeof(description->size), stream);
	read_size += read_func(&description->flags, 1, sizeof(description->flags), stream);
	read_size += read_func(&description->height, 1, sizeof(description->height), stream);
	read_size += read_func(&description->width, 1, sizeof(description->width), stream);
	read_size += read_func(&description->pitch, 1, sizeof(description->pitch), stream);
	read_size += read_func(&description->depth, 1, sizeof(description->depth), stream);
	read_size += read_func(&description->mip_map_levels, 1, sizeof(description->mip_map_levels), stream);
	read_size += read_func(&description->alpha_bit_depth, 1, sizeof(description->alpha_bit_depth), stream);
	read_size += read_func(&description->reserved, 1, sizeof(description->reserved), stream);
	read_size += read_func(&description->surface, 1, sizeof(description->surface), stream);

	read_size += ReadDdsColorKey(stream, read_func, &description->color_key_dest_overlay);
	read_size += ReadDdsColorKey(stream, read_func, &description->color_key_dest_bitblt);
	read_size += ReadDdsColorKey(stream, read_func, &description->color_key_source_overlay);
	read_size += ReadDdsColorKey(stream, read_func, &description->color_key_source_bitblt);

	read_size += ReadDdsPixelFormat(stream, read_func, &description->format);
	read_size += ReadDdsCaps(stream, read_func, &description->caps);

	read_size += read_func(&description->texture_stage, 1, sizeof(description->texture_stage), stream);

	return read_size;
}

static void DecompressBlockDXT_Internal(
	const uint8* block,
	uint8* output,
	unsigned int stride,
	const uint8* alpha_values
)
{
	uint32 code;
	uint32 temp;
	uint16 color0, color1;
	uint8 r0, g0, b0, r1, g1, b1;
	uint32 position_code;
	uint8 alpha;
	uint8 *pixel;

	unsigned int i, j;

	color0 = *(const uint16*)(block);
	color1 = *(const uint16*)(block + 2);

	temp = (color0 >> 11) * 255 + 16;
	r0 = (uint8)((temp/32 + temp)/32);
	temp = ((color0 & 0x7E0) >> 5) * 255 + 32;
	g0 = (uint8)((temp/64 + temp)/64);
	temp = (color0 & 0x001F) * 255 + 16;
	b0 = (uint8)((temp/32 + temp)/32);

	temp = (color1 >> 11) * 255 + 16;
	r1 = (uint8)((temp/32 + temp)/32);
	temp = ((color1 & 0x7E0) >> 5) * 255 + 32;
	g1 = (uint8)((temp/64 + temp)/64);
	temp = (color1 & 0x001F) * 255 + 16;
	b1 = (uint8)((temp/32 + temp)/32);

	code = *(const uint32*)(block + 4);

	if(color0 > color1)
	{
		for(j=0; j<4; ++j)
		{
			for(i=0; i<4; ++i)
			{
				pixel = &output[j*stride + i*4];
				alpha = alpha_values[j*4+i];

				position_code = (code >> 2*(4*j+i)) & 0x03;

				switch(position_code)
				{
				case 0:
					pixel[0] = r0;
					pixel[1] = g0;
					pixel[2] = b0;
					pixel[3] = alpha;
					break;
				case 1:
					pixel[0] = r1;
					pixel[1] = g1;
					pixel[2] = b1;
					pixel[3] = alpha;
					break;
				case 2:
					pixel[0] = (2*r0+r1)/3;
					pixel[1] = (2*g0+g1)/3;
					pixel[2] = (2*b0+b1)/3;
					pixel[3] = alpha;
				case 3:
					pixel[0] = (r0+2*r1)/3;
					pixel[1] = (g0+2*g1)/3;
					pixel[2] = (b0+2*b1)/3;
					pixel[3] = alpha;
					break;
				}
			}
		}
	}
	else
	{
		for(j=0; j<4; ++j)
		{
			for(i=0; i<4; ++i)
			{
				pixel = &output[j*stride + i*4];
				alpha = alpha_values[j*4+i];

				position_code = (code >> 2*(4*j+i)) & 0x03;

				switch(position_code)
				{
				case 0:
					pixel[0] = r0;
					pixel[1] = g0;
					pixel[2] = b0;
					pixel[3] = alpha;
					break;
				case 1:
					pixel[0] = r1;
					pixel[1] = g1;
					pixel[2] = b1;
					pixel[3] = alpha;
					break;
				case 2:
					pixel[0] = (r0+r1)/3;
					pixel[1] = (g0+g1)/3;
					pixel[2] = (b0+b1)/3;
					pixel[3] = alpha;
				case 3:
					pixel[0] = 0;
					pixel[1] = 0;
					pixel[2] = 0;
					pixel[3] = alpha;
					break;
				}
			}
		}
	}
}

void DecompressBlockDXT1(
	uint32 x,
	uint32 y,
	uint32 width,
	const uint8* block_storage,
	uint8* image
)
{
	const uint8 const_alpha[] =
	{
		0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff,
		0xff, 0xff, 0xff, 0xff
	};
	DecompressBlockDXT_Internal(block_storage,
		image + x*4 + (y*width*4), width*4, const_alpha);
}

void DecompressBlockDXT3(
	uint32 x,
	uint32 y,
	uint32 width,
	const uint8* block_storage,
	uint8* image
)
{
	uint8 alpha_values[16] = {0};
	int i;

	for(i=0; i<4; ++i)
	{
		const uint16* alpha_data = (const uint16*)(block_storage);
		alpha_values[i*4 + 0] = (((*alpha_data) >> 0) & 0xF) * 17;
		alpha_values[i*4 + 1] = (((*alpha_data) >> 4) & 0xF) * 17;
		alpha_values[i*4 + 2] = (((*alpha_data) >> 8) & 0xF) * 17;
		alpha_values[i*4 + 3] = (((*alpha_data) >> 12) & 0xF) * 17;

		block_storage += 2;
	}

	DecompressBlockDXT_Internal(block_storage,
		image + x*4 + (y*width*4), width*4, alpha_values);
}

void DecompressBlockDXT5(
	uint32 x,
	uint32 y,
	uint32 width,
	const uint8* block_storage,
	uint8* image
)
{
	uint32 temp, code;
	uint8 alpha0, alpha1;
	const uint8* bits;
	uint32 alpha_code1;
	uint16 alpha_code2;

	uint16 color0, color1;
	uint8 r0, g0, b0, r1, g1, b1;

	int i, j;


	alpha0 = *(block_storage);
	alpha1 = *(block_storage + 1);

	bits = block_storage + 2;
	alpha_code1 = bits[2] | (bits[3] << 8) | (bits[4] << 16) | (bits[5] << 24);
	alpha_code2 = bits[0] | (bits[1] << 8);

	color0 = *(const uint16*)(block_storage + 8);
	color1 = *(const uint16*)(block_storage + 10);	

	temp = (color0 >> 11) * 255 + 16;
	r0 = (uint8)((temp/32 + temp)/32);
	temp = ((color0 & 0x07E0) >> 5) * 255 + 32;
	g0 = (uint8)((temp/64 + temp)/64);
	temp = (color0 & 0x001F) * 255 + 16;
	b0 = (uint8)((temp/32 + temp)/32);

	temp = (color1 >> 11) * 255 + 16;
	r1 = (uint8)((temp/32 + temp)/32);
	temp = ((color1 & 0x07E0) >> 5) * 255 + 32;
	g1 = (uint8)((temp/64 + temp)/64);
	temp = (color1 & 0x001F) * 255 + 16;
	b1 = (uint8)((temp/32 + temp)/32);

	code = *(const uint32*)(block_storage + 12);

	for(j=0; j<4; j++)
	{
		for(i=0; i<4; i++)
		{
			uint8 final_alpha;
			int alpha_code, alpha_code_index;
			uint8 color_code;
			uint8 *pixel;

			pixel = &image[(i+x)*4 + (width*4 * (y+j))];
			alpha_code_index = 3*(4*j+i);
			if(alpha_code_index <= 12)
			{
				alpha_code = (alpha_code2 >> alpha_code_index) & 0x07;
			}
			else if(alpha_code_index == 15)
			{
				alpha_code = (alpha_code2 >> 15) | ((alpha_code1 << 1) & 0x06);
			} else // alphaCodeIndex >= 18 && alphaCodeIndex <= 45
			{
				alpha_code = (alpha_code1 >> (alpha_code_index - 16)) & 0x07;
			}

			if (alpha_code == 0)
			{
				final_alpha = alpha0;
			}
			else if(alpha_code == 1)
			{
				final_alpha = alpha1;
			}
			else
			{
				if(alpha0 > alpha1)
				{
					final_alpha = (uint8)(((8-alpha_code)*alpha0 + (alpha_code-1)*alpha1)/7);
				}
				else
				{
					if (alpha_code == 6)
					{
						final_alpha = 0;
					}
					else if(alpha_code == 7)
					{
						final_alpha = 255;
					}
					else
					{
						final_alpha = (uint8)(((6-alpha_code)*alpha0 + (alpha_code-1)*alpha1)/5);
					}
				}
			}

			color_code = (code >> 2*(4*j+i)) & 0x03; 

			switch (color_code)
			{
			case 0:
				pixel[0] = r0;
				pixel[1] = g0;
				pixel[2] = b0;
				pixel[3] = final_alpha;
				break;
			case 1:
				pixel[0] = r1;
				pixel[1] = g1;
				pixel[2] = b1;
				pixel[3] = final_alpha;
				break;
			case 2:
				pixel[0] = (2*r0+r1)/3;
				pixel[1] = (2*g0+g1)/3;
				pixel[2] = (2*b0+b1)/3;
				pixel[3] = final_alpha;
				break;
			case 3:
				pixel[0] = (r0+2*r1)/3;
				pixel[1] = (g0+2*g1)/3;
				pixel[2] = (b0+2*b1)/3;
				pixel[3] = final_alpha;
				break;
			}
		}
	}
}

/*****************************************
* ReadDdsStream関数                      *
* DDSファイルの読み込み                  *
* 引数                                   *
* stream	: 読み込み元                 *
* read_func	: 読み込みに使う関数ポインタ *
* data_size	: 画像データのバイト数       *
* width		: 画像の幅                   *
* height	: 画像の高さ                 *
* channel	: 画像のチャンネル数         *
* 返り値                                 *
*	ピクセルデータ(失敗時はNULL)         *
*****************************************/
uint8* ReadDdsStream(
	void* stream,
	stream_func_t read_func,
	seek_func_t seek_func,
	size_t data_size,
	int* width,
	int* height,
	int* channel
)
{
	DDS_SURFACE_DESCRIPTION description;
	size_t read_size = 0;
	char magic[5] = {0};
	uint8 *pixels;
	uint8 *data;
	uint8 *block_storage;
	size_t compressed_data_size;
	uint32 four_cc;
	unsigned int block_count_x, block_count_y;
	unsigned int i, j;

	read_size += read_func(magic, 1, 4, stream);
	if(strcmp(magic, "DDS ") != 0)
	{
		return NULL;
	}

	read_size += ReadDdsSurfaceDescription(stream, read_func, &description);

	compressed_data_size = data_size - read_size;
	data = (uint8*)MEM_ALLOC_FUNC(compressed_data_size);
	(void)read_func(data, 1, compressed_data_size, stream);
	pixels = (uint8*)MEM_ALLOC_FUNC(description.width * description.height * 4);

	four_cc = UINT32_FROM_BE(description.format.four_cc);

	block_count_x = (description.width + 3) / 4;
	block_count_y = (description.height + 3) / 4;
	block_storage = data;
	switch(four_cc)
	{
	case 'DXT1':
		for(i=0; i<block_count_y; i++)
		{
			for(j=0; j<block_count_x; j++)
			{
				DecompressBlockDXT1(j*4, i*4, description.width,
					block_storage + j * 8, pixels);
			}
			block_storage += block_count_x * 8;
		}
		break;
	case 'DXT3':
		for(i=0; i<block_count_y; i++)
		{
			for(j=0; j<block_count_x; j++)
			{
				DecompressBlockDXT3(j*4, i*4, description.width,
					block_storage + j * 16, pixels);
			}
			block_storage += block_count_x * 16;
		}
		break;
	case 'DXT5':
		for(i=0; i<block_count_y; i++)
		{
			for(j=0; j<block_count_x; j++)
			{
				DecompressBlockDXT5(j*4, i*4, description.width,
					block_storage + j * 8, pixels);
			}
			block_storage += block_count_x * 8;
		}
		break;
	default:
		return NULL;
	}

	if(width != NULL)
	{
		*width = description.width;
	}
	if(height != NULL)
	{
		*height = description.height;
	}
	if(channel != NULL)
	{
		*channel = 4;
	}

	MEM_FREE_FUNC(data);

	return pixels;
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
		// 適正データ判定用の文字列
		const char format_string[] = "Paint Soft KABURAGI";
		// 適正データ判定用の読み込みバッファ
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

			pixels = ReadPNGStream((void*)png_data, (stream_func_t)MemRead,
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

		pixels = ReadTlgStream((void*)stream, (stream_func_t)FileRead, (seek_func_t)FileSeek,
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
* SetFileChooserPreview関数                            *
* ファイル選択ダイアログにプレビューウィジェットを設定 *
* 引数                                                 *
* file_chooser	: ファイル選択ダイアログウィジェット   *
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
