// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <ctype.h>
#include <png.h>
#include <setjmp.h>
#include <zlib.h>
#include "libtiff/tiffio.h"
#include "libjpeg/jpeglib.h"
#include "types.h"
#include "memory.h"
#include "memory_stream.h"
#include "bit_stream.h"
#include "image_read_write.h"
#include "utils.h"
#include "tlg.h"

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
		if(local_width > 0)
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
* png_p		: libpngの圧縮・展開管理構造体アドレス *
***************************************************/
static void PngFlush(png_structp png_p)
{
	PNG_IO* io = (PNG_IO*)png_get_io_ptr(png_p);
	if(io->flush_func != NULL)
	{
		io->flush_func(io->data);
	}
}

/***********************************************
* PngReadChunkCallback関数                     *
* チャンク読み込み用コールバック関数           *
* png_p	: libpngの圧縮・展開管理構造体アドレス *
* chunk	: チャンクデータ                       *
* 返り値                                       *
*	正常終了:正の値、不明:0、エラー:負の値     *
***********************************************/
int PngReadChunkCallback(png_structp ptr, png_unknown_chunkp chunk)
{
	return 0;
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

	// チャンク読み込み用の関数ポインタをセット
	png_set_read_user_chunk_fn(png_p, NULL,
		(png_user_chunk_ptr)PngReadChunkCallback);

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

	// ICCプロファイルの読み込み
	(void)JpegReadICCProfile(&decode, icc_profile_data, icc_profile_size);

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
	MEM_FREE_FUNC(pixels);
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

#ifdef __cplusplus
}
#endif
