#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "memory_stream.h"

// C++でコンパイルする際のエラー回避
#ifdef __cplusplus
extern "C" {
#endif

/*********************************************************
* CreateMemoryStream関数                                 *
* メモリの読み書きを管理する構造体のメモリの確保と初期化 *
* 引数                                                   *
* buff_size	: 確保するバッファのサイズ                   *
* 返り値                                                 *
*	初期化された構造体のアドレス                         *
*********************************************************/
MEMORY_STREAM_PTR CreateMemoryStream(
	size_t buff_size
)
{
	// 返り値
	MEMORY_STREAM_PTR ret =
		(MEMORY_STREAM_PTR)MEM_ALLOC_FUNC(sizeof(MEMORY_STREAM));

	// メモリ確保成功のチェック
	if(ret == NULL)
	{
#ifdef _DEBUG
		(void)printf("Memory allocate error.\n(In CreateMemoryStream)\n");
#endif
		return NULL;
	}

	// バッファのメモリの確保
	ret->buff_ptr = (unsigned char*)MEM_ALLOC_FUNC(buff_size+1);

	// メモリ確保成功のチェック
	if(ret->buff_ptr == NULL)
	{
#ifdef _DEBUG
		(void)printf("Memory allocate error.\n(In CreateMemoryStream)\n");
#endif
		MEM_FREE_FUNC(ret);

		return NULL;
	}

	// バッファを0クリアしておく
	(void)memset(ret->buff_ptr, 0, buff_size+1);

	// 各種変数の設定
	ret->data_point = 0;
	ret->data_size = buff_size;
	ret->block_size = buff_size;

	return ret;
}

/*****************************************************
* DeleteMemoryStream関数                             *
* メモリの読み書きを管理する構造体のメモリを開放     *
* 引数                                               *
* mem	: メモリを読み書きを管理する構造体のアドレス *
* 返り値                                             *
*	常に0                                            *
*****************************************************/
int DeleteMemoryStream(MEMORY_STREAM_PTR mem)
{
	if(mem != NULL)
	{
		MEM_FREE_FUNC(mem->buff_ptr);
		MEM_FREE_FUNC(mem);
	}

	return 0;
}

/***************************************************************
* MemRead関数                                                  *
* メモリからデータを読み込む                                   *
* 引数                                                         *
* dst			: データの読み込み先                           *
* block_size	: データを読み込む際の1つのブロックのサイズ    *
* block_num		: 読み込むブロックの個数                       *
* mem			: 読み込み元のメモリを管理する構造体のアドレス *
* 返り値                                                       *
*	読み込んだバイト数                                         *
***************************************************************/
size_t MemRead(
	void* dst,
	size_t block_size,
	size_t block_num,
	MEMORY_STREAM_PTR mem
)
{
	// 読み込みを要求されたバイト数
	size_t required_size = block_size * block_num;

	// 実際に読み込んだバイト数、ブロック数
	size_t read_size, read_block;

	// 要求されたバイト数読み込んでもデータの終端に到達しないなら
	if(required_size + mem->data_point <= mem->data_size)
	{
		// 要求されたバイト数分読み込む(コピーする)
		read_size = required_size;
		read_block = block_num;
	}
	else
	{
		// データの終端に到達しているので
		// 読み込むバイト数を計算してから読み込み
		read_size = mem->data_size - mem->data_point;
		read_block = read_size / block_size;
	}

	(void)memcpy(dst, &mem->buff_ptr[mem->data_point], read_size);

	// データの参照位置を更新
	mem->data_point += read_size;

	return read_block;
}

/***************************************************************
* MemWrite関数                                                 *
* メモリへデータを書き込む                                     *
* 引数                                                         *
* src                                                          *
* block_size	: データを書き込む際の1つのブロックのサイズ    *
* block_num		: 書き込むブロックの数                         *
* mem			: 書き込み先のメモリを管理する構造体のアドレス *
* 返り値                                                       *
*	書き込んだバイト数                                         *
***************************************************************/
size_t MemWrite(
	const void* src,
	size_t block_size,
	size_t block_num,
	MEMORY_STREAM_PTR mem
)
{
	// 書き込みを要求されたバイト数
	size_t required_size = block_size * block_num;

	// 実際に書き込んだバイト数、ブロック数
	size_t write_size;

	// 要求されたバイト数書き込んでもバッファの終端に到達しないなら
	if(required_size + mem->data_point <= mem->data_size)
	{
		// 要求されたバイト数分書き込む(コピーする)
		write_size = required_size;
	}
	else
	{
		// バッファの終端に到達してしまうので
		// バッファを確保しなおし
		unsigned char* temp = (unsigned char*)MEM_ALLOC_FUNC(
			mem->data_size + mem->block_size + (required_size / mem->block_size) * mem->block_size);
		(void)memcpy(temp, mem->buff_ptr, mem->data_size);
		(void)memset(&temp[mem->data_size], 0, mem->block_size);
		MEM_FREE_FUNC(mem->buff_ptr);
		mem->buff_ptr = temp;
		mem->data_size += mem->block_size + (required_size / mem->block_size) * mem->block_size;
		write_size = required_size;
	}

	(void)memcpy(&mem->buff_ptr[mem->data_point], src, write_size);

	// データの参照位置を更新
	mem->data_point += write_size;

	return block_num;
}

/***************************************************************
* MemSeek関数                                                  *
* シークを行う                                                 *
* 引数                                                         *
* mem		: シークを行うメモリを管理している構造体のアドレス *
* offset	: 移動するバイト数                                 *
* origin	: 移動を開始する位置(fseekと同じ)                  *
* 返り値                                                       *
*	正常終了(0)、異常終了(0以外)                               *
***************************************************************/
int MemSeek(MEMORY_STREAM_PTR mem, long offset, int origin)
{
	switch(origin)
	{
	case SEEK_SET:
		if(offset >= 0 && (size_t)offset <= mem->data_size)
		{
			mem->data_point = offset;
			return 0;
		}
		else
		{
			return 1;
		}
	case SEEK_CUR:
		if(mem->data_point + offset <= mem->data_size)
		{
			mem->data_point += offset;
			return 0;
		}
		else
		{
			return 1;
		}
	case SEEK_END:
		{
			if(offset <= 0)
			{
				mem->data_point = mem->data_size + offset;
				return 0;
			}
			else
			{
				return 1;
			}
		}
	}	// switch(origin)

	return 1;
}

/***************************************************************
* MemSeek64関数                                                *
* シークを行う(64ビットラップ版)                               *
* 引数                                                         *
* mem		: シークを行うメモリを管理している構造体のアドレス *
* offset	: 移動するバイト数                                 *
* origin	: 移動を開始する位置(fseekと同じ)                  *
* 返り値                                                       *
*	正常終了(0)、異常終了(0以外)                               *
***************************************************************/
int MemSeek64(MEMORY_STREAM_PTR mem, long long int offset, int origin)
{
	return MemSeek(mem, (long)offset, origin);
}

/*****************************************************
* MemTell関数                                        *
* データの参照位置を返す                             *
* mem	: メモリを読み書きを管理する構造体のアドレス *
* 返り値                                             *
*	データの参照位置                                 *
*****************************************************/
long MemTell(MEMORY_STREAM_PTR mem)
{
	return (long)mem->data_point;
}

/***************************************************************
* MemGets関数                                                  *
* 1行読み込む                                                  *
* 引数                                                         *
* string	: 読み込む文字列を格納するアドレス                 *
* n			: 読み込む最大の文字数                             *
* mem		: シークを行うメモリを管理している構造体のアドレス *
* 返り値                                                       *
*	成功(string)、失敗(NULL)                                   *
***************************************************************/
char* MemGets(char* string, int n, MEMORY_STREAM_PTR mem)
{
	int i;	// for文用のカウンタ

	// データの終端に達していないかを判断
	if(mem->data_point == mem->data_size)
	{
		return NULL;
	}

	// 改行を探す
	for(i=0; i<n; i++)
	{
		// 改行を見つけた
		if(mem->buff_ptr[mem->data_point+i] == '\n' || mem->buff_ptr[mem->data_point+i] == '\r')
		{
			break;
		}

		// データの終端に達した
		if(mem->data_point + i == mem->data_size)
		{
			(void)memcpy(string, &mem->buff_ptr[mem->data_point], i);
			return NULL;
		}
	}

	// 文字数が一杯になった
	if(i == n)
	{
		(void)memcpy(string, &mem->buff_ptr[mem->data_point], i-1);
		mem->data_point += i-1;	// データの参照位置を更新
		string[i-1] = '\0';	// // 最後の文字をヌル文字にしておく
		return NULL;
	}

	(void)memcpy(string, &mem->buff_ptr[mem->data_point], i);
	mem->data_point += i;	// データの参照位置を更新
	if(mem->buff_ptr[mem->data_point] == '\r')
	{
		mem->data_point++;
	}

	if(mem->buff_ptr[mem->data_point] == '\n')
	{
		mem->data_point++;
	}
	string[i] = '\0';	// 最後の文字をヌル文字にしておく

	return string;
}

// C++でコンパイルする際のエラー回避
#ifdef __cplusplus
}
#endif
