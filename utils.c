// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <ctype.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include "memory.h"
#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**************************************
* StringCompareIgnoreCase関数         *
* 大文字/小文字を無視して文字列を比較 *
* 引数                                *
* str1	: 比較文字列1                 *
* str2	: 比較文字列2                 *
* 返り値                              *
*	文字列の差(同文字列なら0)         *
**************************************/
int StringCompareIgnoreCase(const char* str1, const char* str2)
{
	int ret;

	while((ret = toupper(*str1) - toupper(*str2)) == 0)
	{
		str1++, str2++;
		if(*str1 == '\0')
		{
			return 0;
		}
	}

	return ret;
}

#ifdef _MSC_VER
int strncasecmp(const char* s1, const char* s2, size_t n)
{
	if(n == 0)
	{
		return 0;
	}

	while(n-- != 0 && tolower(*s1) == tolower(*s2))
	{
		if(n == 0 || *s1 == '\0' || *s2 == '\0')
		{
			break;
		}

		s1++,	s2++;
	}

	return toupper((int)(*(unsigned char*)s1)) - tolower(*s2);
}
#endif

/**************************************************************
* StringStringIgnoreCase関数                                  *
* 大文字/小文字を無視して引数1の文字列から引数2の文字列を探す *
* str		: 検索対象の文字列                                *
* search	: 検索する文字列                                  *
* 返り値                                                      *
*	文字列発見:発見した位置のポインタ	見つからない:NULL     *
**************************************************************/
char* StringStringIgnoreCase(const char* str, const char* search)
{
	const char *ret = NULL;
	char *upper_str = MEM_STRDUP_FUNC(str);
	char *upper_search = MEM_STRDUP_FUNC(search);

	if(str != NULL && search != NULL)
	{
		char *c;
		char *upper_ret;

		c = upper_str;
		while(*c != '\0')
		{
			*c = toupper(*c);
			c++;
		}

		c = upper_search;
		while(*c != '\0')
		{
			*c = toupper(*c);
			c++;
		}

		upper_ret = strstr(upper_str, upper_search);
		if(upper_ret != NULL)
		{
			ret = &str[upper_ret - upper_str];
		}

		MEM_FREE_FUNC(upper_str);
		MEM_FREE_FUNC(upper_search);
	}

	return (char*)ret;
}

/**************************************
* memset32関数                        *
* 32bit符号なし整数でバッファを埋める *
* 引数                                *
* buff	: 埋める対象のバッファ        *
* value	: 埋める値                    *
* size	: 埋めるバイト数              *
**************************************/
void memset32(void* buff, uint32 value, size_t size)
{
	size_t i;
	for(i=0; i<(size & (~(sizeof(value) - 1))); i+=sizeof(value))
	{
		(void)memcpy(((char*)buff)+i, &value, sizeof(value));
	}
	for( ; i<size; i++)
	{
		((char*)buff)[i] = ((char*)&value)[i&(sizeof(value)-1)];
	}
}

/**************************************
* memset64関数                        *
* 64bit符号なし整数でバッファを埋める *
* 引数                                *
* buff	: 埋める対象のバッファ        *
* value	: 埋める値                    *
* size	: 埋めるバイト数              *
**************************************/
void memset64(void* buff, uint64 value, size_t size)
{
	size_t i;
	for(i=0; i<(size & (~(sizeof(value) - 1))); i+=sizeof(value))
	{
		(void)memcpy(((char*)buff)+i, &value, sizeof(value));
	}
	for( ; i<size; i++)
	{
		((char*)buff)[i] = ((char*)&value)[i&(sizeof(value)-1)];
	}
}

/***************************************
* StringReplace関数                    *
* 指定した文字列を置換する             *
* 引数                                 *
* str			: 処理をする文字列     *
* replace_from	: 置き換えられる文字列 *
* replace_to	: 置き換える文字列     *
***************************************/
void StringRepalce(
	char* str,
	char* replace_from,
	char* replace_to
)
{
	char work[1024];
	char* p = str;

	while((p = strstr(str, replace_from)) != NULL)
	{
		*p = '\0';
		p += strlen(replace_from);
		(void)strcpy(work, p);
		(void)strcat(str, replace_to);
		(void)strcat(str, work);
	}
}

int ForFontFamilySearchCompare(const char* str, PangoFontFamily** font)
{
	return StringCompareIgnoreCase(str, pango_font_family_get_name(*font));
}

/*******************************************
* FileRead関数                             *
* Gtkの関数を利用してファイル読み込み      *
* 引数                                     *
* dst			: 読み込み先のアドレス     *
* block_size	: 読み込むブロックのサイズ *
* read_num		: 読み込むブロックの数     *
* stream		: 読み込み元のストリーム   *
* 返り値                                   *
*	読み込んだブロック数                   *
*******************************************/
size_t FileRead(void* dst, size_t block_size, size_t read_num, GFileInputStream* stream)
{
	return g_input_stream_read(
		G_INPUT_STREAM(stream), dst, block_size*read_num, NULL, NULL) / block_size;
}

/*******************************************
* FileWrite関数                            *
* Gtkの関数を利用してファイル書き込み      *
* 引数                                     *
* src			: 書き込み元のアドレス     *
* block_size	: 書き込むブロックのサイズ *
* read_num		: 書き込むブロックの数     *
* stream		: 書き込み先のストリーム   *
* 返り値                                   *
*	書き込んだブロック数                   *
*******************************************/
size_t FileWrite(void* src, size_t block_size, size_t read_num, GFileOutputStream* stream)
{
	return g_output_stream_write(
		G_OUTPUT_STREAM(stream), src, block_size*read_num, NULL, NULL) / block_size;
}

/********************************************
* FileSeek関数                              *
* Gtkの関数を利用してファイルシーク         *
* 引数                                      *
* stream	: シークを行うストリーム        *
* offset	: 移動バイト数                  *
* origin	: 移動開始位置(fseek関数と同等) *
* 返り値                                    *
*	正常終了(0), 異常終了(0以外)            *
********************************************/
int FileSeek(void* stream, long offset, int origin)
{
	GSeekType seek;

	switch(origin)
	{
	case SEEK_SET:
		seek = G_SEEK_SET;
		break;
	case SEEK_CUR:
		seek = G_SEEK_CUR;
		break;
	case SEEK_END:
		seek = G_SEEK_END;
		break;
	default:
		return -1;
	}

	return !(g_seekable_seek(G_SEEKABLE(stream), offset, seek, NULL, NULL));
}

/************************************************
* FileSeekTell関数                              *
* Gtkの関数を利用してファイルのシーク位置を返す *
* 引数                                          *
* stream	: シーク位置を調べるストリーム      *
* 返り値                                        *
*	シーク位置                                  *
************************************************/
long FileSeekTell(void* stream)
{
	return (long)g_seekable_tell(G_SEEKABLE(stream));
}

/***********************************************
* InvertMatrix関数                             *
* 逆行列を計算する                             *
* 引数                                         *
* a	: 計算対象の行列(逆行列データはここに入る) *
* n	: 行列の次数                               *
***********************************************/
void InvertMatrix(FLOAT_T **a, int n)
{
	FLOAT_T **inv_a = (FLOAT_T**)MEM_ALLOC_FUNC(
		sizeof(*inv_a)*n);
	FLOAT_T buf;
	int maximum;
	int i, j, k;

	inv_a[0] = (FLOAT_T*)MEM_ALLOC_FUNC(
		sizeof(**inv_a)*(n*n));
	for(i=1; i<n; i++)
	{
		inv_a[i] = inv_a[i-1] + n;
	}
	(void)memset(inv_a[0], 0, sizeof(**inv_a)*(n*n));

	for(i=0; i<n; i++)
	{
		inv_a[i][i] = 1;
	}

	for(i=0; i<n; i++)
	{
		maximum = i;
		for(j=i+1; j<n; j++)
		{
			if(fabs(a[j][i]) > fabs(a[maximum][i]))
			{
				maximum = j;
			}
		}

		if(maximum != i)
		{
			for(k=0; k<n; k++)
			{
				buf = a[maximum][k];
				a[maximum][k] = a[i][k];
				a[i][k] = buf;

				buf = inv_a[maximum][k];
				inv_a[maximum][k] = inv_a[i][k];
				inv_a[i][k] = buf;
			}
		}

		buf = a[i][i];
		for(k=0; k<n; k++)
		{
			a[i][k] /= buf;
			inv_a[i][k] /= buf;
		}

		for(j=0; j<n; j++)
		{
			if(j != i)
			{
				buf = a[j][i] / a[i][i];
				for(k=0; k<n; k++)
				{
					a[j][k] = a[j][k] - a[i][k] * buf;
					inv_a[j][k] = inv_a[j][k] - inv_a[i][k] * buf;
				}
			}
		}
	}

	for(i=0; i<n; i++)
	{
		for(j=0; j<n; j++)
		{
			a[i][j] = inv_a[i][j];
		}
	}

	MEM_FREE_FUNC(inv_a[0]);
	MEM_FREE_FUNC(inv_a);
}

#ifndef _WIN32
#include <stdlib.h>

 char* utoa(unsigned val, char *buf, int radix){
     char *p = NULL;
     char *s = "0123456789abcdefghijklmnopqrstuvwxyz";
     if(radix == 0) {
         radix = 10;
     }
     if(buf == NULL) {
         return NULL;
     }
     if(val < (unsigned)radix) {
         buf[0] = s[val];
         buf[1] = '\0';
     } else {
         for(p = utoa(val / ((unsigned)radix), buf, radix); *p; p++);
         utoa(val % ((unsigned)radix), p, radix);
     }
     return buf;
 }

 char* itoa(int val, char *buf, int radix) {
     char *p;
     unsigned u;

     p = buf;
     if(radix == 0) {
         radix = 10;
     }
     if(buf == NULL) {
         return NULL;
     }
     if(val < 0) {
         *p++ = '-';
         u = -val;
     } else {
         u = val;
     }
     utoa(u, p, radix);

     return buf;
 }
#endif

void AdjustmentChangeValueCallBackInt8(GtkAdjustment* adjustment, int8* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (int8)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeValueCallBackUint8(GtkAdjustment* adjustment, uint8* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (uint8)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeValueCallBackInt16(GtkAdjustment* adjustment, int16* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (int16)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeValueCallBackUint16(GtkAdjustment* adjustment, uint16* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (uint16)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeValueCallBackInt32(GtkAdjustment* adjustment, int32* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (int32)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeValueCallBackUint32(GtkAdjustment* adjustment, uint32* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (uint32)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeVaueCallBackDouble(GtkAdjustment* adjustment, gdouble* value)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*value = gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

static void CheckButtonChangeFlags(GtkWidget* button, guint32* flags)
{
	guint32 flag_value = (guint32)GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(button), "flag-value"));
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)) == FALSE)
	{
		*flags &= ~flag_value;
	}
	else
	{
		*flags |= flag_value;
	}
}

void CheckButtonSetFlagsCallBack(GtkWidget* button, guint32* flags, guint32 flag_value)
{
	g_object_set_data(G_OBJECT(button), "flag-value", GUINT_TO_POINTER(flag_value));
	g_signal_connect(G_OBJECT(button), "toggled",
		G_CALLBACK(CheckButtonChangeFlags), flags);
}

#ifdef __cplusplus
}
#endif
