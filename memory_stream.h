#ifndef _INCLUDED_MEMORY_STREAM_H_
#define _INCLUDED_MEMORY_STREAM_H_

#include <stdio.h>	// size_t型を使うため

// C++でコンパイルする際のエラー回避
#ifdef __cplusplus
extern "C" {
#endif

#ifdef EXTERN
# undef EXTERN
#endif

#ifdef _MSC_VER
# ifdef __cplusplus
#  define EXTERN extern "C" __declspec(dllexport)
# else
#  define EXTERN extern __declspec(dllexport)
# endif
#else
# define EXTERN extern
#endif

typedef struct _MEMORY_STREAM
{
	unsigned char* buff_ptr;	// バッファ
	size_t data_point;			// データの参照位置
	size_t data_size;			// データの容量
	size_t block_size;			// 書き込み時の確保ブロック容量
} MEMORY_STREAM, *MEMORY_STREAM_PTR;

// 関数のプロトタイプ宣言
/*********************************************************
* CreateMemoryStream関数                                 *
* メモリの読み書きを管理する構造体のメモリの確保と初期化 *
* 引数                                                   *
* buff_size	: 確保するバッファのサイズ                   *
* 返り値                                                 *
*	初期化された構造体のアドレス                         *
*********************************************************/
EXTERN MEMORY_STREAM_PTR CreateMemoryStream(
	size_t buff_size
);

/*****************************************************
* DeleteMemoryStream関数                             *
* メモリの読み書きを管理する構造体のメモリを開放     *
* 引数                                               *
* mem	: メモリを読み書きを管理する構造体のアドレス *
* 返り値                                             *
*	常に0                                            *
*****************************************************/
EXTERN int DeleteMemoryStream(MEMORY_STREAM_PTR mem);

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
EXTERN size_t MemRead(
	void* dst,
	size_t block_size,
	size_t block_num,
	MEMORY_STREAM_PTR mem
);

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
EXTERN size_t MemWrite(
	const void* src,
	size_t block_size,
	size_t block_num,
	MEMORY_STREAM_PTR mem
);

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
EXTERN int MemSeek(MEMORY_STREAM_PTR mem, long offset, int origin);

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
EXTERN int MemSeek64(MEMORY_STREAM_PTR mem, long long int offset, int origin);

/*****************************************************
* MemTell関数                                        *
* データの参照位置を返す                             *
* mem	: メモリを読み書きを管理する構造体のアドレス *
* 返り値                                             *
*	データの参照位置                                 *
*****************************************************/
EXTERN long MemTell(MEMORY_STREAM_PTR mem);

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
EXTERN char* MemGets(char* string, int n, MEMORY_STREAM_PTR mem);

// C++でコンパイルする際のエラー回避
#ifdef __cplusplus
}
#endif

#endif	// _INCLUDED_MEMORY_STREAM_H_
