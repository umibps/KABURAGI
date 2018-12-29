// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include <string.h>
#include <gtk/gtk.h>
#include "configure.h"
#include "pattern.h"
#include "image_read_write.h"
#include "utils.h"
#include "memory.h"

#if !defined(USE_QT) || (defined(USE_QT) && USE_QT != 0)
# include "gui/GTK/utils_gtk.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*************************************************************
* InitializePatterns関数                                     *
* パターンを初期化                                           *
* 引数                                                       *
* pattern			: パターン情報を管理する構造体のアドレス *
* directory_path	: パターンファイルのあるディレクトリパス *
* buffer_size		: 現在のバッファサイズ                   *
*************************************************************/
void InitializePattern(PATTERNS* pattern, const char* directory_path, int* buffer_size)
{
#define PATTERN_BUFFER_SIZE 16
	// パターンファイルがあるディレクトリを開く
	GDir* dir = g_dir_open(directory_path, 0, NULL);
	// ファイル名
	const gchar* file_name;
	// ファイルパス
	char file_path[4096];
	// ファイル読み込み用
	GFile* fp;
	GFileInputStream* stream;
	// 画像のバイト数
	size_t num_byte;
	// 1行分のバイト数
	int stride;
	// for文用のカウンタ
	int i;

	// ディレクトリオープン失敗
	if(dir == NULL)
	{
		pattern->patterns = NULL;
	}

	// パターンメモリをとりあえず多めに確保
	if(pattern->patterns == NULL)
	{
		*buffer_size = PATTERN_BUFFER_SIZE;
		pattern->patterns = (PATTERN*)MEM_ALLOC_FUNC(sizeof(*pattern->patterns)*(*buffer_size));
	}

	// ディレクトリのファイルを読み込む
	while((file_name = g_dir_read_name(dir)) != NULL)
	{	// GLibのライブラリ関数でファイルストリームオープン
			// サブディレクトリがあればそちらも参照
		GDir *child_dir;
		// ディレクトリパス+ファイル名でファイルパス作成
		(void)sprintf(file_path, "%s/%s", directory_path, file_name);
		child_dir = g_dir_open(file_path, 0, NULL);
		if(child_dir != NULL)
		{
			g_dir_close(child_dir);
			InitializePattern(pattern, file_path, buffer_size);
		}
			
		if((fp = g_file_new_for_path(file_path)) != NULL)
		{
			if((stream = g_file_read(fp, NULL, NULL)) != NULL)
			{	// PNGデータを読み込む
				pattern->patterns[pattern->num_pattern].pixels =
					ReadPNGStream(stream, (stream_func_t)FileRead,
						&pattern->patterns[pattern->num_pattern].width,
						&pattern->patterns[pattern->num_pattern].height,
						&pattern->patterns[pattern->num_pattern].stride
					);

				// 画像の最大バイト数を計算
				num_byte = pattern->patterns[pattern->num_pattern].width
					* pattern->patterns[pattern->num_pattern].height * 4;
				if(pattern->pattern_max_byte < num_byte)
				{
					pattern->pattern_max_byte = num_byte;
				}

				// 一行分のバイト数が4の倍数でない場合動作が保証できないので追加中止
				if(pattern->patterns[pattern->num_pattern].width > 0
					&& pattern->patterns[pattern->num_pattern].pixels != NULL)
				{
					pattern->patterns[pattern->num_pattern].channel =
						(uint8)(pattern->patterns[pattern->num_pattern].stride / pattern->patterns[pattern->num_pattern].width);
					if(pattern->patterns[pattern->num_pattern].channel == 1)
					{	// グレースケールならば使いやすいようにピクセルデータ反転
						uint8 *old_pixels = pattern->patterns[pattern->num_pattern].pixels;
						int j;

						stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, pattern->patterns[pattern->num_pattern].width);
						pattern->patterns[pattern->num_pattern].pixels = (uint8*)MEM_ALLOC_FUNC(
							stride * pattern->patterns[pattern->num_pattern].height);
						(void)memset(pattern->patterns[pattern->num_pattern].pixels, 0,
							stride * pattern->patterns[pattern->num_pattern].height);

						for(i=0;
							i<pattern->patterns[pattern->num_pattern].height; i++)
						{
							for(j=0; j<pattern->patterns[pattern->num_pattern].width; j++)
							{
								pattern->patterns[pattern->num_pattern].pixels[i*stride+j] =
									0xff - old_pixels[i*pattern->patterns[pattern->num_pattern].width+j];
							}
						}

						MEM_FREE_FUNC(old_pixels);
						pattern->patterns[pattern->num_pattern].stride = stride;
					}
					else if(pattern->patterns[pattern->num_pattern].channel == 2)
					{
						uint8 *old_pixels = pattern->patterns[pattern->num_pattern].pixels;
						stride = cairo_format_stride_for_width(
							CAIRO_FORMAT_A8, pattern->patterns[pattern->num_pattern].width);
						pattern->patterns[pattern->num_pattern].pixels = (uint8*)MEM_ALLOC_FUNC(
							stride * 2 * pattern->patterns[pattern->num_pattern].height);
						(void)memset(pattern->patterns[pattern->num_pattern].pixels, 0,
							stride * 2 * pattern->patterns[pattern->num_pattern].height);
						for(i=0; i<pattern->patterns[pattern->num_pattern].height; i++)
						{
							(void)memcpy(&pattern->patterns[pattern->num_pattern].pixels[stride*i*2],
								&old_pixels[i*pattern->patterns[pattern->num_pattern].width*2], pattern->patterns[pattern->num_pattern].width*2);
						}

						MEM_FREE_FUNC(old_pixels);
						pattern->patterns[pattern->num_pattern].stride = stride * 2;
					}
					else
					{
						stride = pattern->patterns[pattern->num_pattern].width * 4;
						if(pattern->patterns[pattern->num_pattern].channel == 3)
						{	// RGBはRGBAに変換する
							uint8* old_pixels = pattern->patterns[pattern->num_pattern].pixels;
							pattern->patterns[pattern->num_pattern].pixels = (uint8*)MEM_ALLOC_FUNC(
								pattern->patterns[pattern->num_pattern].width*pattern->patterns[pattern->num_pattern].height*4);
							for(i=0;
								i<pattern->patterns[pattern->num_pattern].width*pattern->patterns[pattern->num_pattern].height; i++)
							{
								pattern->patterns[pattern->num_pattern].pixels[i*4] = old_pixels[i*3+2];
								pattern->patterns[pattern->num_pattern].pixels[i*4+1] = old_pixels[i*3+1];
								pattern->patterns[pattern->num_pattern].pixels[i*4+2] = old_pixels[i*3];
								pattern->patterns[pattern->num_pattern].pixels[i*4+3] = 0xff;
							}
							pattern->patterns[pattern->num_pattern].channel = 4;
							pattern->patterns[pattern->num_pattern].stride =
								pattern->patterns[pattern->num_pattern].width * 4;

							MEM_FREE_FUNC(old_pixels);
						}
						else
						{
							uint8 r;
							for(i=0; i<pattern->patterns[pattern->num_pattern].width*pattern->patterns[pattern->num_pattern].height; i++)
							{
								r = pattern->patterns[pattern->num_pattern].pixels[i*4];
								pattern->patterns[pattern->num_pattern].pixels[i*4] = pattern->patterns[pattern->num_pattern].pixels[i*4+2];
								pattern->patterns[pattern->num_pattern].pixels[i*4+2] = r;
							}
						}

						pattern->patterns[pattern->num_pattern].stride = stride;
					}

					// パターン数更新
					pattern->num_pattern++;

					// バッファの残りが不足していたら最確保
					if(pattern->num_pattern >= (*buffer_size)-1)
					{
						(*buffer_size) += PATTERN_BUFFER_SIZE;
						pattern->patterns = (PATTERN*)MEM_REALLOC_FUNC(
							pattern->patterns, sizeof(*pattern->patterns)*(*buffer_size));
					}
				}

				g_object_unref(stream);
			}
			g_object_unref(fp);
		}
	}

	// パターンの最大バイト数でバッファ作成
	pattern->pattern_pixels = (uint8*)MEM_ALLOC_FUNC(pattern->pattern_max_byte);
	pattern->pattern_pixels_temp = (uint8*)MEM_ALLOC_FUNC(pattern->pattern_max_byte);
	pattern->pattern_mask = (uint8*)MEM_ALLOC_FUNC(pattern->pattern_max_byte / 4);

	// アクティブなパターンはとりあえず先頭にしておく
	pattern->active_pattern = pattern->patterns;

	g_dir_close(dir);
}

/***********************************************************
* ClipBoardImageRecieveCallBack関数                        *
* クリップボードの画像データを受けった時のコールバック関数 *
* 引数                                                     *
* clipboard	: クリップボードの情報                         *
* pixbuf	: ピクセルバッファ                             *
* data		: アプリケーションを管理する構造体の           *
***********************************************************/
static void ClipBoardImageRecieveCallBack(
	GtkClipboard *clipboard,
	GdkPixbuf* pixbuf,
	gpointer data
)
{
	// パターン情報にキャスト
	PATTERNS* patterns = (PATTERNS*)data;
	// ピクセルバッファのバイト数
	size_t num_bytes;

	if(pixbuf != NULL)
	{	// ピクセルバッファ作成に成功していたら
			// αチャンネルがなければ追加
		if(gdk_pixbuf_get_has_alpha(pixbuf) == FALSE)
		{
			pixbuf = gdk_pixbuf_add_alpha(pixbuf, FALSE, 0, 0, 0);
		}

		patterns->clip_board.width = gdk_pixbuf_get_width(pixbuf);
		patterns->clip_board.height = gdk_pixbuf_get_height(pixbuf);
		patterns->clip_board.stride = gdk_pixbuf_get_rowstride(pixbuf);
		patterns->clip_board.channel = 4;
		num_bytes = patterns->clip_board.stride * patterns->clip_board.height;
		if(patterns->pattern_max_byte < num_bytes)
		{
			patterns->pattern_max_byte = num_bytes;
			patterns->pattern_pixels = (uint8*)MEM_REALLOC_FUNC(patterns->pattern_pixels, num_bytes);
			patterns->pattern_pixels_temp = (uint8*)MEM_REALLOC_FUNC(patterns->pattern_pixels_temp, num_bytes);
			patterns->pattern_mask = (uint8*)MEM_REALLOC_FUNC(patterns->pattern_mask, num_bytes / 4);
		}

		patterns->clip_board.pixels = (uint8*)MEM_REALLOC_FUNC(patterns->clip_board.pixels, num_bytes);
		(void)memcpy(patterns->clip_board.pixels, gdk_pixbuf_get_pixels(pixbuf), num_bytes);

		patterns->has_clip_board_pattern = TRUE;

		g_object_unref(pixbuf);
	}
	else
	{
		patterns->has_clip_board_pattern = FALSE;
	}
}

/*********************************************************
* UpdateClipBoardPattern関数                             *
* クリップボードから画像データを取り出してパターンにする *
* 引数                                                   *
* patterns	: パターンを管理する構造体のアドレス         *
*********************************************************/
void UpdateClipBoardPattern(PATTERNS* patterns)
{
	// クリップボードに対してデータを要求
	gtk_clipboard_request_image(gtk_clipboard_get(GDK_NONE),
		ClipBoardImageRecieveCallBack, patterns);
}

/*************************************************
* CreatePatternSurface関数                       *
* パターン塗り潰し実行用のCAIROサーフェース作成  *
* 引数                                           *
* patterns	: パターンを管理する構造体のアドレス *
* scale		: パターンの拡大率                   *
* rgb		: 現在の描画色                       *
* back_rgb	: 現在の背景色                       *
* flags		: 左右反転、上下反転のフラグ         *
* mode		: チャンネル数が2のときの作成モード  *
* flow		: パターンの濃度                     *
* 返り値                                         *
*	作成したサーフェース                         *
*************************************************/
cairo_surface_t* CreatePatternSurface(
	PATTERNS* patterns,
	uint8 rgb[3],
	uint8 back_rgb[3],
	int flags,
	int mode,
	gdouble flow
)
{
	// CAIROを使ってピクセルデータをサーフェースに
	cairo_t* cairo_p;
	// 返り値
	cairo_surface_t *surface_p;
	// パターンのピクセルデータサーフェース
	cairo_surface_t *image;
	// 作成するサーフェースの幅、高さ
	int width, height;
	// ピクセルデータをコピーしない時のフラグ
	int no_copy_pixels = 0;
	// 作成するサーフェースのフォーマット
	cairo_format_t format =
		(patterns->active_pattern->channel == 4) ? CAIRO_FORMAT_ARGB32 :
			(patterns->active_pattern->channel == 3) ? CAIRO_FORMAT_RGB24 : CAIRO_FORMAT_A8;
	// for文用のカウンタ
	int i, j, k;

	// サーフェースの幅、高さを決定
	width = (int)(patterns->active_pattern->width);
	// 高さ計算
	height = (int)(patterns->active_pattern->height);

	// 幅または高さ0なら終了
	if(width == 0 || height == 0)
	{
		return NULL;
	}

	// 左右反転
	if((flags & PATTERN_FLIP_HORIZONTALLY) != 0)
	{
		int stride = cairo_format_stride_for_width(format, patterns->active_pattern->width);
		int row = (patterns->active_pattern->channel <= 2) ? stride :
					stride / patterns->active_pattern->channel;

		if(patterns->active_pattern->channel == 2)
		{
			stride *= 2;
		}

		no_copy_pixels++;
		for(i=0; i<patterns->active_pattern->height; i++)
		{
			for(j=0; j<patterns->active_pattern->width; j++)
			{
				for(k=0; k<patterns->active_pattern->channel; k++)
				{
					patterns->pattern_pixels[i*stride
						+j*patterns->active_pattern->channel+k] =
							patterns->active_pattern->pixels[i*stride
								+(patterns->active_pattern->width-j-1)*patterns->active_pattern->channel+k];
				}
			}
		}
	}
	// 上下反転
	if((flags & PATTERN_FLIP_VERTICALLY) != 0)
	{
		int stride = cairo_format_stride_for_width(format, patterns->active_pattern->width);
		int row = (patterns->active_pattern->channel <= 2) ? stride :
					stride / patterns->active_pattern->channel;
		if(patterns->active_pattern->channel == 2)
		{
			stride *= 2;
		}

		no_copy_pixels++;
		if((flags & PATTERN_FLIP_HORIZONTALLY) != 0)
		{
			for(i=0; i<patterns->active_pattern->height; i++)
			{
				for(j=0; j<row; j++)
				{
					for(k=0; k<patterns->active_pattern->channel; k++)
					{
						patterns->pattern_pixels_temp[i*stride
							+j*patterns->active_pattern->channel+k] =
								patterns->pattern_pixels[(patterns->active_pattern->height-i-1)*stride
									+j*patterns->active_pattern->channel+k];
					}
				}
			}

			(void)memcpy(patterns->pattern_pixels, patterns->pattern_pixels_temp,
				patterns->active_pattern->stride*patterns->active_pattern->height);
		}
		else
		{
			for(i=0; i<patterns->active_pattern->height; i++)
			{
				for(j=0; j<row; j++)
				{
					for(k=0; k<patterns->active_pattern->channel; k++)
					{
						patterns->pattern_pixels[i*stride
							+j*patterns->active_pattern->channel+k] =
							patterns->active_pattern->pixels[(patterns->active_pattern->height-i-1)*stride
								+j*patterns->active_pattern->channel+k];
					}
				}
			}
		}
	}
	if(no_copy_pixels == 0)
	{
		(void)memcpy(patterns->pattern_pixels, patterns->active_pattern->pixels,
			patterns->active_pattern->stride*patterns->active_pattern->height);
	}

	// 返り値作成
	surface_p = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
	cairo_p = cairo_create(surface_p);

	// アクティブなパターンのチャンネル数で処理切り替え
	switch(patterns->active_pattern->channel)
	{
	case 1:	// チャンネル数1なら描画色で塗り潰してパターンでマスク
		// 不透明度設定
		for(i=0; i<patterns->active_pattern->stride*patterns->active_pattern->height; i++)
		{
			patterns->pattern_pixels[i] = (uint8)(patterns->pattern_pixels[i]*flow);
		}
		image = cairo_image_surface_create_for_data(patterns->pattern_pixels,
			format, width, height, patterns->active_pattern->stride);
		cairo_set_source_rgb(cairo_p, rgb[0]*DIV_PIXEL, rgb[1]*DIV_PIXEL, rgb[2]*DIV_PIXEL);
		cairo_rectangle(cairo_p, 0, 0, width, height);
		cairo_mask_surface(cairo_p, image, 0, 0);
		break;
	case 2:	// チャンネル数2ならモードによって処理切り替え
		{
			// 色設定用
			HSV hsv;
			uint8 color[3];
			// 画像のαチャンネルでマスクする
			cairo_surface_t* mask;
			// 1行分のバイト数
			int stride;

			// RGBをHSVに変換
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
			if(mode == PATTERN_MODE_FORE_TO_BACK)
			{
				color[0] = rgb[0], color[1] = rgb[1], color[2] = rgb[2];
			}
			else
			{
				color[0] = rgb[2], color[1] = rgb[1], color[2] = rgb[0];
			}
#else
			color[0] = rgb[0], color[1] = rgb[1], color[2] = rgb[2];
#endif
			RGB2HSV_Pixel(color, &hsv);

			stride = cairo_format_stride_for_width(CAIRO_FORMAT_A8, patterns->active_pattern->width);

			// 不透明度設定
			for(i=0; i<patterns->active_pattern->height; i++)
			{
				for(j=0; j<patterns->active_pattern->width; j++)
				{
					patterns->pattern_mask[i*stride+j] =
						(uint8)(patterns->pattern_pixels[i*stride*2+j*2+1]*flow);
				}
			}
		
			switch(mode)
			{
			case PATTERN_MODE_SATURATION:	// 彩度でパターン作成
				for(i=0; i<patterns->active_pattern->height; i++)
				{
					for(j=0; j<patterns->active_pattern->width; j++)
					{
						hsv.s = 0xff - patterns->pattern_pixels[i*stride*2+j*2];
						HSV2RGB_Pixel(&hsv, &patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4]);
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4+3] = 0xff;
					}
				}
				break;
			case PATTERN_MODE_BRIGHTNESS:	// 明度でパターン作成
				for(i=0; i<patterns->active_pattern->height; i++)
				{
					for(j=0; j<patterns->active_pattern->width; j++)
					{
						hsv.v = 0xff - patterns->pattern_pixels[i*stride*2+j*2];
						HSV2RGB_Pixel(&hsv, &patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4]);
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4+3] = 0xff;
					}
				}
				break;
			case PATTERN_MODE_FORE_TO_BACK:	// 描画色から背景色へ
				for(i=0; i<patterns->active_pattern->height; i++)
				{
					for(j=0; j<patterns->active_pattern->width; j++)
					{
#if defined(USE_BGR_COLOR_SPACE) && USE_BGR_COLOR_SPACE != 0
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4+2] = ((patterns->pattern_pixels[i*stride*2+j*2]+1)*color[0] +
							(0xff - patterns->pattern_pixels[i*stride*2+j*2]+1)*back_rgb[0]) >> 8;
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4+1] = ((patterns->pattern_pixels[i*stride*2+j*2]+1)*color[1] +
							(0xff - patterns->pattern_pixels[i*stride*2+j*2]+1)*back_rgb[1]) >> 8;
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4] = ((patterns->pattern_pixels[i*stride*2+j*2]+1)*color[2] +
							(0xff - patterns->pattern_pixels[i*stride*2+j*2]+1)*back_rgb[2]) >> 8;
#else
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4] = ((patterns->pattern_pixels[i*stride*2+j*2]+1)*color[0] +
							(0xff - patterns->pattern_pixels[i*stride*2+j*2]+1)*back_rgb[0]) >> 8;
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4+1] = ((patterns->pattern_pixels[i*stride*2+j*2]+1)*color[1] +
							(0xff - patterns->pattern_pixels[i*stride*2+j*2]+1)*back_rgb[1]) >> 8;
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4+2] = ((patterns->pattern_pixels[i*stride*2+j*2]+1)*color[2] +
							(0xff - patterns->pattern_pixels[i*stride*2+j*2]+1)*back_rgb[2]) >> 8;
#endif
						patterns->pattern_pixels_temp[i*patterns->active_pattern->width*4+j*4+3] = 0xff;
					}
				}
				break;
			}
			mask = cairo_image_surface_create_for_data(patterns->pattern_mask, format,
				patterns->active_pattern->width, patterns->active_pattern->height, stride);
			image = cairo_image_surface_create_for_data(patterns->pattern_pixels_temp, CAIRO_FORMAT_ARGB32,
				patterns->active_pattern->width, patterns->active_pattern->height, patterns->active_pattern->width*4);
			cairo_set_source_surface(cairo_p, image, 0, 0);
			cairo_mask_surface(cairo_p, mask, 0, 0);
			cairo_surface_destroy(mask);
		}
		break;
	default:
		image = cairo_image_surface_create_for_data(patterns->pattern_pixels,
			format, patterns->active_pattern->width,
			patterns->active_pattern->height, patterns->active_pattern->stride);
		cairo_set_source_surface(cairo_p, image, 0, 0);
		cairo_paint_with_alpha(cairo_p, flow);
	}

	// 不要になったデータを削除
	cairo_surface_destroy(image);
	cairo_destroy(cairo_p);
	return surface_p;
}

#ifdef __cplusplus
}
#endif
