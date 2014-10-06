// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <ctype.h>
#include <string.h>
#include <math.h>
#include <zlib.h>
#include <gtk/gtk.h>
#include "memory.h"
#include "types.h"
#include "utils.h"

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

int StringNumCompareIgnoreCase(const char* str1, const char* str2, int num)
{
	int ret;
	int count = 0;
	while((ret = toupper(*str1) - toupper(*str2)) == 0)
	{
		str1++,	str2++;
		if(*str1 == '\0')
		{
			return 0;
		}
		count++;
		if(count >= num)
		{
			return ret;
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

/*********************************
* GetFileExtention関数           *
* ファイル名から拡張子を取得する *
* 引数                           *
* file_name	: ファイル名         *
* 返り値                         *
*	拡張子の文字列               *
*********************************/
char* GetFileExtention(char* file_name)
{
	char *extention = file_name;
	char *p = extention;

	while(*p != '\0')
	{
		if(*p == '.')
		{
			extention = p;
		}
		p++;
	}

	return extention;
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

BYTE_ARRAY* ByteArrayNew(size_t block_size)
{
	BYTE_ARRAY *ret;

	if(block_size == 0)
	{
		return NULL;
	}

	ret = (BYTE_ARRAY*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->block_size = ret->buffer_size = block_size;
	ret->buffer = (uint8*)MEM_ALLOC_FUNC(block_size);
	(void)memset(ret->buffer, 0, block_size);

	return ret;
}

void ByteArrayAppend(BYTE_ARRAY* barray, uint8 data)
{
	barray->buffer[barray->num_data] = data;
	barray->num_data++;

	if(barray->num_data >= barray->buffer_size)
	{
		size_t before_size = barray->buffer_size;
		barray->buffer_size += barray->block_size;
		barray->buffer = (uint8*)MEM_REALLOC_FUNC(barray->buffer, barray->buffer_size);
		(void)memset(&barray->buffer[before_size], 0, barray->block_size);
	}
}

void ByteArrayDestroy(BYTE_ARRAY** barray)
{
	MEM_FREE_FUNC((*barray)->buffer);
	MEM_FREE_FUNC(*barray);

	*barray = NULL;
}

void ByteArrayResize(BYTE_ARRAY* barray, size_t new_size)
{
	size_t alloc_size;

	if(barray->buffer_size == new_size)
	{
		return;
	}
	if(barray->block_size > 1)
	{
		alloc_size = ((new_size + barray->block_size - 1)
			/ barray->block_size) * barray->block_size;
	}
	else
	{
		alloc_size = new_size;
	}
	barray->buffer = (uint8*)MEM_REALLOC_FUNC(
		barray->buffer, sizeof(uint8) * alloc_size);
	if(alloc_size < barray->num_data)
	{
		barray->num_data = alloc_size-1;
	}
	barray->buffer_size = alloc_size;
}

WORD_ARRAY* WordArrayNew(size_t block_size)
{
	WORD_ARRAY *ret;

	if(block_size == 0)
	{
		return NULL;
	}

	ret = (WORD_ARRAY*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->block_size = ret->buffer_size = block_size;
	ret->buffer = (uint16*)MEM_ALLOC_FUNC(block_size * sizeof(uint16));
	(void)memset(ret->buffer, 0, sizeof(uint16)*block_size);

	return ret;
}

void WordArrayAppend(WORD_ARRAY* warray, uint16 data)
{
	warray->buffer[warray->num_data] = data;
	warray->num_data++;

	if(warray->num_data >= warray->buffer_size)
	{
		size_t before_size = warray->buffer_size;
		warray->buffer_size += warray->block_size;
		warray->buffer = (uint16*)MEM_REALLOC_FUNC(
			warray->buffer, warray->buffer_size * sizeof(uint16));
		(void)memset(&warray->buffer[before_size], 0, warray->block_size * sizeof(uint16));
	}
}

void WordArrayDestroy(WORD_ARRAY** warray)
{
	MEM_FREE_FUNC((*warray)->buffer);
	MEM_FREE_FUNC(*warray);
	*warray = NULL;
}

void WordArrayResize(WORD_ARRAY* warray, size_t new_size)
{
	size_t alloc_size;

	if(warray->buffer_size == new_size)
	{
		return;
	}
	if(warray->block_size > 1)
	{
		alloc_size = ((new_size + warray->block_size - 1)
			/ warray->block_size) * warray->block_size;
	}
	else
	{
		alloc_size = new_size;
	}
	warray->buffer = (uint16*)MEM_REALLOC_FUNC(
		warray->buffer, sizeof(uint16) * alloc_size);
	if(alloc_size < warray->num_data)
	{
		warray->num_data = alloc_size-1;
	}
	warray->buffer_size = alloc_size;
}

UINT32_ARRAY* Uint32ArrayNew(size_t block_size)
{
	UINT32_ARRAY *ret;

	if(block_size == 0)
	{
		return NULL;
	}

	ret = (UINT32_ARRAY*)MEM_ALLOC_FUNC(sizeof(*ret));
	(void)memset(ret, 0, sizeof(*ret));

	ret->block_size = ret->buffer_size = block_size;
	ret->buffer = (uint32*)MEM_ALLOC_FUNC(block_size * sizeof(uint32));

	return ret;
}

void Uint32ArrayAppend(UINT32_ARRAY* uarray, uint32 data)
{
	uarray->buffer[uarray->num_data] = data;
	uarray->num_data++;

	if(uarray->num_data >= uarray->buffer_size)
	{
		size_t before_size = uarray->buffer_size;
		uarray->buffer_size += uarray->block_size;
		uarray->buffer = (uint32*)MEM_REALLOC_FUNC(
			uarray->buffer, uarray->buffer_size * sizeof(uint32));
		(void)memset(&uarray->buffer[before_size], 0, uarray->block_size * sizeof(uint32));
	}
}

void Uint32ArrayDestroy(UINT32_ARRAY** uarray)
{
	MEM_FREE_FUNC((*uarray)->buffer);
	MEM_FREE_FUNC(*uarray);
	*uarray = NULL;
}

void Uint32ArrayResize(UINT32_ARRAY* uint32_array, size_t new_size)
{
	size_t alloc_size;

	if(uint32_array->buffer_size == new_size)
	{
		return;
	}
	if(uint32_array->block_size > 1)
	{
		alloc_size = ((new_size + uint32_array->block_size - 1)
			/ uint32_array->block_size) * uint32_array->block_size;
	}
	else
	{
		alloc_size = new_size;
	}
	uint32_array->buffer = (uint32*)MEM_REALLOC_FUNC(
		uint32_array->buffer, sizeof(uint32) * alloc_size);
	if(alloc_size < uint32_array->num_data)
	{
		uint32_array->num_data = alloc_size-1;
	}
	uint32_array->buffer_size = alloc_size;
}

POINTER_ARRAY* PointerArrayNew(size_t block_size)
{
	POINTER_ARRAY *ret;

	if(block_size == 0)
	{
		return NULL;
	}

	ret = (POINTER_ARRAY*)MEM_ALLOC_FUNC(sizeof(*ret));
	ret->num_data = 0;
	ret->block_size = ret->buffer_size = block_size;
	ret->buffer = (void**)MEM_ALLOC_FUNC(sizeof(*ret)*block_size);
	(void)memset(ret->buffer, 0, sizeof(void*)*block_size);

	return ret;
}

void PointerArrayRelease(
	POINTER_ARRAY* pointer_array,
	void (*destroy_func)(void*)
)
{
	size_t i;
	for(i=0; i<pointer_array->num_data; i++)
	{
		destroy_func(pointer_array->buffer[i]);
	}
	pointer_array->num_data = 0;
}

void PointerArrayDestroy(
	POINTER_ARRAY** pointer_array,
	void (*destroy_func)(void*)
)
{
	if(pointer_array == NULL || *pointer_array == NULL)
	{
		return;
	}

	if(destroy_func != NULL)
	{
		PointerArrayRelease(*pointer_array, destroy_func);
	}

	MEM_FREE_FUNC((*pointer_array)->buffer);
	MEM_FREE_FUNC(*pointer_array);

	*pointer_array = NULL;
}

void PointerArrayAppend(POINTER_ARRAY* pointer_array, void* data)
{
	pointer_array->buffer[pointer_array->num_data] = data;
	pointer_array->num_data++;

	if(pointer_array->num_data == pointer_array->buffer_size)
	{
		size_t before_size = pointer_array->buffer_size;
		pointer_array->buffer_size += pointer_array->block_size;
		pointer_array->buffer = (void**)MEM_REALLOC_FUNC(
			pointer_array->buffer, pointer_array->buffer_size * sizeof(void*));
		(void)memset(&pointer_array->buffer[before_size], 0,
			pointer_array->block_size * sizeof(void*));
	}
}

void PointerArrayRemoveByIndex(
	POINTER_ARRAY* pointer_array,
	size_t index,
	void (*destroy_func)(void*)
)
{
	if(index < pointer_array->num_data)
	{
		size_t i;

		if(destroy_func != NULL)
		{
			destroy_func(pointer_array->buffer[index]);
		}

		pointer_array->num_data--;
		for(i=index; i<pointer_array->num_data; i++)
		{
			pointer_array->buffer[i] = pointer_array->buffer[i+1];
		}
	}
}

void PointerArrayRemoveByData(
	POINTER_ARRAY* pointer_array,
	void* data,
	void (*destroy_func)(void*)
)
{
	size_t i, j;
	for(i=0; i<pointer_array->num_data; i++)
	{
		if(pointer_array->buffer[i] == data)
		{
			if(destroy_func != NULL)
			{
				destroy_func(data);
			}

			pointer_array->num_data--;
			for(j=i; j<pointer_array->num_data; j++)
			{
				pointer_array->buffer[j] = pointer_array->buffer[j+1];
			}

			return;
		}
	}
}

int PointerArrayDoesCointainData(POINTER_ARRAY* pointer_array, void* data)
{
	size_t i;
	for(i=0; i<pointer_array->num_data; i++)
	{
		if(pointer_array->buffer[i] == data)
		{
			return TRUE;
		}
	}

	return FALSE;
}

STRUCT_ARRAY* StructArrayNew(size_t data_size, size_t block_size)
{
	STRUCT_ARRAY *ret;

	if(block_size == 0)
	{
		return NULL;
	}

	ret = (STRUCT_ARRAY*)MEM_ALLOC_FUNC(sizeof(*ret));
	ret->num_data = 0;
	ret->data_size = data_size;
	ret->block_size = ret->buffer_size = block_size;
	ret->buffer = (uint8*)MEM_ALLOC_FUNC(data_size*block_size);
	(void)memset(ret->buffer, 0, data_size*block_size);

	return ret;
}

void StructArrayDestroy(
	STRUCT_ARRAY** struct_array,
	void (*destroy_func)(void*)
)
{
	if(destroy_func != NULL)
	{
		size_t i;
		uint8 *buffer = (uint8*)(*struct_array)->buffer;

		for(i=0; i<(*struct_array)->num_data; i++)
		{
			destroy_func(&buffer[i*(*struct_array)->data_size]);
		}
	}

	MEM_FREE_FUNC((*struct_array)->buffer);
	MEM_FREE_FUNC(*struct_array);

	*struct_array = NULL;
}

void StructArrayAppend(STRUCT_ARRAY* struct_array, void* data)
{
	uint8 *dst = struct_array->buffer;
	(void)memcpy(&dst[struct_array->num_data*struct_array->data_size],
		data, struct_array->data_size);
	struct_array->num_data++;

	if(struct_array->num_data == struct_array->buffer_size)
	{
		size_t before_size = struct_array->buffer_size;
		struct_array->buffer_size += struct_array->block_size;
		dst = struct_array->buffer = (uint8*)MEM_REALLOC_FUNC(
			struct_array->buffer, struct_array->buffer_size * struct_array->data_size);
		(void)memset(&dst[before_size*struct_array->data_size], 0,
			struct_array->block_size * struct_array->data_size);
	}
}

void* StructArrayReserve(STRUCT_ARRAY* struct_array)
{
	void *ret = (void*)&struct_array->buffer[struct_array->data_size*struct_array->num_data];
	struct_array->num_data++;

	if(struct_array->num_data == struct_array->buffer_size)
	{
		uint8 *dst;
		size_t before_size = struct_array->buffer_size;
		struct_array->buffer_size += struct_array->block_size;
		dst = struct_array->buffer = (uint8*)MEM_REALLOC_FUNC(
			struct_array->buffer, struct_array->buffer_size * struct_array->data_size);
		(void)memset(&dst[before_size*struct_array->data_size], 0,
			struct_array->block_size * struct_array->data_size);
		ret = (void*)&dst[(before_size-1)*struct_array->data_size];
	}

	return ret;
}

void StructArrayResize(STRUCT_ARRAY* struct_array, size_t new_size)
{
	size_t alloc_size;

	if(struct_array->buffer_size == new_size)
	{
		return;
	}
	if(struct_array->block_size > 1)
	{
		alloc_size = ((new_size + struct_array->block_size - 1)
			/ struct_array->block_size) * struct_array->block_size;
	}
	else
	{
		alloc_size = new_size;
	}
	struct_array->buffer = (uint8*)MEM_REALLOC_FUNC(
		struct_array->buffer, struct_array->data_size * alloc_size);
	if(alloc_size < struct_array->num_data)
	{
		struct_array->num_data = alloc_size-1;
	}
	struct_array->buffer_size = alloc_size;
}

void StructArrayRemoveByIndex(
	STRUCT_ARRAY* struct_array,
	size_t index,
	void (*destroy_func)(void*)
)
{
	if(index < struct_array->num_data)
	{
		size_t i;
		uint8 *buffer = struct_array->buffer;

		if(destroy_func != NULL)
		{
			destroy_func(&buffer[index*struct_array->data_size]);
		}

		struct_array->num_data--;
		for(i=index; i<struct_array->num_data; i++)
		{
			(void)memcpy(&buffer[i*struct_array->data_size],
				&buffer[(i+1)*struct_array->data_size], struct_array->data_size);
		}
	}
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

void AdjustmentChangeValueCallBackInt(GtkAdjustment* adjustment, int* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (int)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

void AdjustmentChangeValueCallBackUint(GtkAdjustment* adjustment, unsigned int* store)
{
	void (*func)(void*) = g_object_get_data(G_OBJECT(adjustment), "changed_callback");
	void *func_data = g_object_get_data(G_OBJECT(adjustment), "callback_data");

	*store = (unsigned int)gtk_adjustment_get_value(adjustment);

	if(func != NULL)
	{
		func(func_data);
	}
}

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

void AdjustmentChangeValueCallBackDouble(GtkAdjustment* adjustment, gdouble* value)
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
	(void)g_signal_connect(G_OBJECT(button), "toggled",
		G_CALLBACK(CheckButtonChangeFlags), flags);
}

/*******************************************************
* InflateData関数                                      *
* ZIP圧縮されたデータをデコードする                    *
* 引数                                                 *
* data				: 入力データ                       *
* out_buffer		: 出力先のバッファ                 *
* in_size			: 入力データのバイト数             *
* out_buffer_size	: 出力先のバッファのサイズ         *
* out_size			: 出力したバイト数の格納先(NULL可) *
* 返り値                                               *
*	正常終了:0、失敗:0以外                             *
*******************************************************/
int InflateData(
	uint8* data,
	uint8* out_buffer,
	size_t in_size,
	size_t out_buffer_size,
	size_t* out_size
)
{
	z_stream decompress_stream = {0};
	int result;

	(void)inflateInit(&decompress_stream);

	decompress_stream.avail_in = (uInt)in_size;
	decompress_stream.next_in = data;
	decompress_stream.avail_out = (uInt)out_buffer_size;
	decompress_stream.next_out = out_buffer;

	result = inflate(&decompress_stream, Z_NO_FLUSH);

	if(out_size != NULL)
	{
		*out_size = (size_t)(out_buffer_size - decompress_stream.avail_out);
	}

	(void)inflateEnd(&decompress_stream);

	if(result != Z_STREAM_END)
	{
		return result;
	}

	return 0;
}

/***************************************************
* DeflateData関数                                  *
* ZIP圧縮を行う                                    *
* 引数                                             *
* data					: 入力データ               *
* out_buffer			: 出力先のバッファ         *
* target_data_size		: 入力データのバイト数     *
* out_buffer_size		: 出力先のバッファのサイズ *
* compressed_data_size	: 圧縮後のバイト数格納先   *
* compress_level		: 圧縮レベル(0～9)         *
* 返り値                                           *
*	正常終了:0、失敗:0以外                         *
***************************************************/
int DeflateData(
	uint8* data,
	uint8* out_buffer,
	size_t target_data_size,
	size_t out_buffer_size,
	size_t* compressed_data_size,
	int compress_level
)
{
	z_stream compress_stream = {0};

	(void)deflateInit(&compress_stream, compress_level);
	compress_stream.avail_in = (uInt)target_data_size;
	compress_stream.next_in = data;
	compress_stream.avail_out = (uInt)out_buffer_size;
	compress_stream.next_out = out_buffer;
	(void)deflate(&compress_stream, Z_FINISH);

	if(compress_stream.avail_in > 0)
	{
		return -1;
	}

	if(compressed_data_size != NULL)
	{
		*compressed_data_size = (size_t)(out_buffer_size - compress_stream.avail_out);
	}

	(void)deflateEnd(&compress_stream);

	return 0;
}

void UpdateWidget(GtkWidget* widget)
{
#define MAX_EVENTS 500
	GdkEvent *queued_event;
	int counter = 0;
	// イベントを回してメッセージを表示

	gdk_threads_enter();
#if GTK_MAJOR_VERSION <= 2
	gdk_window_process_updates(widget->window, TRUE);
#else
	gdk_window_process_updates(gtk_widget_get_window(widget), TRUE);
#endif

	g_usleep(1);

	gdk_threads_leave();

	while(gdk_events_pending() != FALSE && counter < MAX_EVENTS)
	{
		counter++;
		queued_event = gdk_event_get();
		gtk_main_iteration();
		if(queued_event != NULL)
		{
#if GTK_MAJOR_VERSION <= 2
			if(queued_event->any.window == widget->window
#else
			if(queued_event->any.window == gtk_widget_get_window(widget)
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

#ifdef __cplusplus
}
#endif
