// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdio.h>
#include "memory.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include "ini_file.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

// 関数のプロトタイプ宣言
static void Delete_INI_FILE(INI_FILE_PTR ini);

/*********************************************
* Trim関数                                   *
* 文字列の前後のスペース、タブをスキップする *
* 引数                                       *
* buff	: 処理したい文字列                   *
*********************************************/
static void Trim(char* buff)
{
	char* p = buff;	// スペース、タブの探索用
	char* r = buff + strlen((const char*)buff) - 1;	// 新しい文字列の最後にヌル文字追加用

	// 前後のスペースを除いたポインタを取得
	for( ; (*p == ' ' || *p == '\t') && *p != '\0'; p++)
	{
	}

	for( ; r > p && (*r == ' ' || *r == '\t'); r--)
	{
	}

	*(r+1) = '\0';	// 文字列の最後にヌル文字を追加

	// buffの中身を更新する
	(void)strcpy(buff, p);
}

/*******************************************
* Str2Hash関数                             *
* 文字列のハッシュ値を取得する             *
* 引数                                     *
* string	: ハッシュ値を取得したい文字列 *
* 返り値                                   *
*	ハッシュ値                             *
*******************************************/
static int Str2Hash(const char* string)
{
	int hash = 0;	// 返り値

	for( ; *string != '\0'; string++)
	{
		if(*string != ' ')
		{
			hash = ((hash << 1) + tolower(*string));
		}
	}

	return hash;
}

/********************************************************
* SectionAdd関数                                        *
* セクションを追加する                                  *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
* section_name	: 追加するセクションの名前              *
* 返り値                                                *
*	正常終了(0)、異常終了(0以外)                        *
********************************************************/
static int SectionAdd(INI_FILE_PTR ini, const char* section_name)
{
	SECTION_PTR tmp_section;

	if(section_name == NULL || *section_name == '\0')
	{
		return -1;
	}

	// 新たにメモリが必要かどうか
	if(ini->section_size < ini->section_count + 1)
	{
		// 再確保
		ini->section_size += INI_ALLOC_SIZE;
		tmp_section = (SECTION_PTR)MEM_ALLOC_FUNC(sizeof(SECTION)*ini->section_size);

		// メモリ確保成功のチェック
#ifdef _DEBUG
		if(tmp_section == NULL)
		{
			(void)printf("Memory allocate error.\n(In SectionAdd)\n");

			return -2;
		}
#endif
		(void)memset(tmp_section, 0, sizeof(SECTION)*ini->section_size);

		if(ini->section != NULL)
		{
			(void)memcpy(tmp_section, ini->section, sizeof(SECTION)*ini->section_count);
			MEM_FREE_FUNC(ini->section);
		}

		ini->section = tmp_section;
	}	// if(ini->section_size < section_count + 1)

	// セクションを追加
	(void)memset((ini->section + ini->section_count), 0, sizeof(SECTION));
	(void)strcpy((ini->section + ini->section_count)->section_name, section_name);
	Trim((ini->section + ini->section_count)->section_name);
	(ini->section + ini->section_count)->hash =	// ハッシュ値を計算
		Str2Hash((ini->section + ini->section_count)->section_name);

	ini->section_count++;

	return 0;
}

/********************************************************
* SectionFind関数                                       *
* セクションを検索する                                  *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
* section_name	: 検索するセクションの名前              *
* 返り値                                                *
*	正常終了(セクションのID)、異常終了(負の値)          *
********************************************************/
static int SectionFind(INI_FILE_PTR ini, const char *section_name)
{
	int hash;	// 検索するセクションのハッシュ値
	int i;	// for文用のカウンタ

	// 検索先、検索ワードがNULLだったり、検索ワードがなかったら
		// エラーで終了
	if(ini->section == NULL || section_name == NULL
		|| *section_name == '\0')
	{
		return -1;
	}

	// 検索するセクションのハッシュ値を求める
	hash = Str2Hash(section_name);

	for(i=0; i<ini->section_count; i++)
	{
		// セクションのハッシュ値が求めるものと同じか?
		if((ini->section + i)->hash != hash)
		{	// 違ったらやり直し
			continue;
		}

		// 念のため同じ文字列かどうかを確かめておく
		if(strcmp((ini->section + i)->section_name, section_name) == 0)
		{	// 同じならID(iの値)を返す
			return i;
		}
	}	// for(i=0; i<ini->section_count; i++)

	// ここまできてしまったらエラー
	return -2;
}

/*********************************************************************
* KeyAdd関数                                                         *
* キーを追加                                                         *
* 引数                                                               *
* section		: キーを追加するセクションを管理する構造体のアドレス *
* key_name		: キーの名前                                         *
* string		: キーに追加する文字列                               *
* comment_flag	: コメントかどうかのフラグ                           *
* 返り値                                                             *
*	正常終了(0)、異常終了(-1)                                        *
*********************************************************************/
static int KeyAdd(SECTION_PTR section, const char* key_name, char* string,
			char comment_flag)
{
	KEY_PTR tmp_key;	// 新たに追加するキーのメモリ領域
	int index = -1;		// 追加されたキーのインデックス

	// 追加する文字列がNULLだったり文字列がなかったらエラー
	if(key_name == NULL || *key_name == '\0' || string == NULL)
	{
		return -1;
	}

	// メモリの再確保が必要かどうか
	if(section->key_size < section->key_count + 1)
	{
		// 再確保
		section->key_size += INI_ALLOC_SIZE;
		tmp_key = (KEY_PTR)MEM_ALLOC_FUNC(sizeof(KEY)*section->key_size);
		// メモリ確保成功のチェック
#ifdef _DEBUG
		if(tmp_key == NULL)
		{
			(void)printf("Memory allocate error.\n(In Key_Add)\n");

			return -2;
		}
#endif
		if(section->key != NULL)
		{
			(void)memcpy(tmp_key, section->key, sizeof(KEY)*section->key_count);
			MEM_FREE_FUNC(section->key);
		}
		section->key = tmp_key;
	}

	// キーを追加
	// 文字列をコピー
	(void)strcpy((section->key + section->key_count)->key_name, key_name);
	Trim((section->key + section->key_count)->key_name);

	// コメントでなければハッシュ値を求める
	if(comment_flag == COMMENT_OFF)
	{
		(section->key + section->key_count)->hash =
			Str2Hash((section->key + section->key_count)->key_name);
	}

	// キーの文字列を作成する
	(section->key + section->key_count)->string =
		MEM_STRDUP_FUNC(string);

	// メモリ確保成功のチェック
#ifdef _DEBUG
	if((section->key + section->key_count)->string == NULL)
	{
		(void)printf("Memory allocate error.\n(In Key_Add)\n");

		return -3;
	}
#endif
	// コメントかどうかのフラグを設定
	(section->key + section->key_count)->comment_flag = comment_flag;

	section->key_count++;	// セクションが扱うキーの数を更新する
	return 0;
}

/*************************************************************
* KeyFind関数                                                *
* キーを検索                                                 *
* 引数                                                       *
* section	: キーを捜すセクションを管理する構造体のアドレス *
* key_name	: 検索するキーの名前                             *
* 返り値                                                     *
*	正常終了(キーのID)、異常終了(負の値)                     *
*************************************************************/
static int KeyFind(const SECTION_PTR section, const char* key_name)
{
	int hash;	// 検索する文字列のハッシュ値
	int i;		// for文用のカウンタ兼返り値

	// 検索する文字列のポインタがNULLだったり、
		// 文字列がなかったらエラー
	if(section->key == NULL || key_name == NULL
		|| *key_name == '\0')
	{
		return -1;
	}

	// ハッシュ値を求める
	hash = Str2Hash(key_name);

	// キーを検索
	for(i=0; i<section->key_count; i++)
	{
		// キーがコメントであったり、ハッシュ値が違っていたらやり直し
		if((section->key + i)->comment_flag == COMMENT_ON
			|| (section->key + i)->hash != hash)
		{
			continue;
		}

		// 念のため文字列を比較しておく
		if(strcmp((section->key + i)->key_name, key_name) == 0)
		{	// 一致したらIDを返す
			return i;
		}
	}	// for(i=0; i<section->key_count; i++)

	return -2;	// キーが見つからなかった
}

/************************************************
* CreateIniFile関数                             *
* iniファイルを扱う構造体のメモリの確保と初期化 *
* 引数                                          *
* io		: iniファイルデータへのポインタ     *
* read_func	: 読み込み用の関数                  *
* data_size	: iniファイルのバイト数             *
* mode		: 読み込みモードか書き込みモードか  *
* 返り値                                        *
*	初期化された構造体のアドレス                *
************************************************/
INI_FILE_PTR CreateIniFile(
	void* io,
	size_t (*read_func)(void*, size_t, size_t, void*),
	size_t data_size,
	eINI_MODE mode
)
{
	INI_FILE_PTR ret;	// 返り値
	char tmp[KEY_BUFF_SIZE];	// 一時的に文字列を格納
	char* buff;		// iniファイルの内容を格納

	// メモリを確保する
	ret = (INI_FILE_PTR)MEM_ALLOC_FUNC(sizeof(INI_FILE));

	// メモリ確保成功のチェック
#ifdef _DEBUG
	if(ret == NULL)
	{
		(void)printf("Memory allocate error.\n(In CreateIniFile_STRUCT)\n");

		return NULL;
	}
#endif

	// メモリ開放用の関数ポインタを設定
	ret->delete_func = Delete_INI_FILE;

	// 書き込みモードか?
	if(mode == INI_WRITE)
	{	// 書き込みモードならファイルを開き、
			// セクションの情報を初期化して終了
		ret->io = io;
		ret->section = NULL;

		return ret;
	}

	// 読み取る領域の確保
	buff = (char*)MEM_ALLOC_FUNC(sizeof(char)*(data_size+1));

	// メモリ確保成功のチェック
#ifdef _DEBUG
	if(buff == NULL)
	{
		(void)printf("Memory allocate error.\n(In CreateIniFile_STRUCT)\n");

		MEM_FREE_FUNC(ret);

		return NULL;
	}
#endif

	// ファイルを読み込む
	ret->io = io;
	(void)read_func(buff, sizeof(char), data_size, ret->io);

	// バッファの最後にヌル文字を追加する
	buff[data_size] = '\0';

	// セクションの確保
	ret->section = (SECTION_PTR)MEM_ALLOC_FUNC(sizeof(SECTION)*INI_ALLOC_SIZE);

	// メモリ確保成功のチェック
#ifdef _DEBUG
	if(ret->section == NULL)
	{
		(void)printf("Memory allocate error.\n(In CreateIniFile_STRUCT)\n");

		MEM_FREE_FUNC(buff);
		MEM_FREE_FUNC(ret);

		return NULL;
	}
#endif

	// セクションの情報の初期化
	(void)memset(ret->section, 0, sizeof(SECTION)*INI_ALLOC_SIZE);
	ret->section_count = 1;
	ret->section_size = INI_ALLOC_SIZE;

	// ファイルの内容を調べるブロック
	{
		char *p, *r, *s;

		p = buff;

		// ファイルの終端までをチェック
		while((data_size > (size_t)(p - buff)) && *p != '\0')
		{
			// rを改行の次の改行に移動する
			for(r = p; (data_size > (size_t)(r - buff) && (*r != '\r') && *r != '\n'); r++)
			{
				// スペースか、タブのあとの改行ならそのまま読み込み続ける
				if((*r == ' ' || *r == '\t') && (*(r + 1) == '\n' || *(r + 1) == '\r'))
				{
					if(*(r + 1) == '\r')
					{
						r++;
					}
					r++;
				}
			}

			// pにある文字の内容で処理を切り替え
			switch(*p)
			{
			case '[':	// 「[」なら
				// セクションを追加する
				if(p == r || *(r-1) != ']')
				{	// 「[」が閉じられたら終了
					break;
				}
				*(r-1) = '\0';
				SectionAdd(ret, p+1);
				break;
			case '\r':
			case '\n':
				// 改行ならなにもしない
				break;

			default:	// それ以外ならキーかコメント
				if(ret->section == NULL || p == r)
				{	// セクションを再確保できなかったり、改行までたっしていたら終了
					break;
				}
				
				if(*p == '#')
				{	// コメント
					for(s = tmp; p < r; p++, s++)
					{	// 内容の読み込み
						*s = *p;
					}
					*s = '\0';	// 最後にヌル文字を追加
					// コメントのフラグをONにしてキーを追加
					KeyAdd((ret->section + ret->section_count - 1), tmp, "", COMMENT_ON);
				}	// if(*p == '#')
				else
				{
					// キーを追加する
					for(s = tmp; p < r; p++, s++)
					{
						if(*p == '=')
						{	// 「=」がきたら終了
							break;
						}
						*s = *p;
					}
					*s = '\0';	// 文字列の最後にヌル文字を追加

					if(*p == '=')
					{	// pの指す内容が「=」なら
						p++;	// 次の値を探す
					}
					*r = '\0';	// 最後にヌル文字を追加
					KeyAdd((ret->section + ret->section_count - 1), tmp, p, COMMENT_OFF);
				}	// if(*p == '#') else

				if(data_size > (size_t)(r - buff))
				{	// ファイルの終端まで達していなかったら次の行へ
					r++;
				}
			}	// switch(*p)

			// 次の文字へpも移動する
			p = r;

			for( ; (data_size > (size_t)(p - buff)) && (*p == '\r'
				|| *p == '\n'); p++)
			{
			}
		}	// while((data_size > (p - buff)) && *p != '\0')
	}

	MEM_FREE_FUNC(buff);
	return ret;
}

/********************************************************
* WriteIniFile関数                                      *
* iniファイルに内容を書き出す                           *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
* write_func	: 書き出し用関数へのポインタ            *
* 返り値                                                *
*	正常終了(0)、以上終了(0以外)                        *
********************************************************/
int WriteIniFile(
	INI_FILE_PTR ini,
	size_t (*write_func)(void*, size_t, size_t, void*)
)
{
	char *buff;	// 書き出す内容を格納するバッファ
	char *p;	// 書き込みの文字比較用
	int len;	// 書き出すサイズ
	int i, j;	// for文用のカウンタ

	// セクションの情報がなければエラー
	if(ini->section == NULL)
	{
		return -1;
	}

	// 保存サイズを計算する
	len = 0;
	for(i=0; i<ini->section_count; i++)
	{
		// セクション名の長さと「[」「]」を足す
		len += (int)strlen((ini->section + i)->section_name) + 4;
		// キーの情報がなくなったらスキップ
		if((ini->section + i)->key == NULL)
		{
			continue;
		}

		for(j=0; j<(ini->section + i)->key_count; j++)
		{
			// キーの名前をさす文字列がなければスキップ
			if(*((ini->section + i)->key + j)->key_name == '\0')
			{
				continue;
			}

			// キー名
			len += (int)strlen(((ini->section + i)->key + j)->key_name);
			// キーがコメントでないなら文字列の長さを足す
			if(((ini->section + i)->key + j)->comment_flag == COMMENT_OFF)
			{
				len++;
				// 文字列が存在するか?
				if(((ini->section + i)->key + j)->string != NULL)
				{
					// サイズに追加
					len += (int)strlen(((ini->section + i)->key + j)->string);
				}
			}	// if(((ini->section + i)->key + j)->comment_flag == COMMENT_OFF)
			len += 2;	// 「[」と「]」の分のサイズを足す
		}	// for(j=0; j<(ini->section + i)->key_count; j++)
		len += 2;	// 「[」と「]」の分のサイズを足す
	}	// for(i=0; i<ini->section_count; i++)

	// バッファのメモリの確保
	p = buff = (char*)MEM_ALLOC_FUNC(sizeof(char)*len*2);

	// メモリ確保のチェック
#ifdef _DEBUG
	if(p == NULL)
	{
		(void)printf("Memory allocate error.\n(In Write_Ini_File)\n");

		return -2;
	}
#endif

	// 保存文字列の作成
	for(i=0; i<ini->section_count; i++)
	{
		// セクションにキーの情報がなかたらスキップ
		if((ini->section + i)->key == NULL)
		{
			continue;
		}

		// セクション名を作成
		if(i != 0)
		{
			*(p++) = '[';	// 「[」を追加
			(void)strcpy(p, (ini->section + i)->section_name);
			p += strlen(p);	// 文字列の長さ分pの位置を勧める
			*(p++) = ']';	// 「]」を追加
			//*(p++) = '\r';	// キャリッジリターンを追加
			*(p++) = '\n';	// 改行を追加
		}	// if(i != 0)

		// キーの文字列を追加
		for(j=0; j<(ini->section + i)->key_count; j++)
		{
			// キーの文字列が存在するかを判断
			if(*((ini->section + i)->key + j)->key_name == '\0')
			{
				continue;
			}

			(void)strcpy(p, ((ini->section + i)->key + j)->key_name);
			p += strlen(p);	// 文字列の長さ分pの位置を勧める
			// コメントでないならば情報を追加
			if(((ini->section + i)->key + j)->comment_flag == COMMENT_OFF)
			{
				*(p++) = '=';	// 「=」を追加
				// 文字列があるかどうかを判断してから追加する
				if(((ini->section + i)->key + j)->string != NULL)
				{
					(void)strcpy(p, ((ini->section + i)->key + j)->string);
					p += strlen(p);	// 文字列の長さ分pの位置を勧める
				}
			}	// if(((ini->section + i)->key + j)->comment_flag == COMMENT_OFF)

			//*(p++) = '\r';	// キャリッジリターンを追加
			*(p++) = '\n';	// 改行を追加
		}	// for(j=0; j<ini->section + i)->key_count; j++)

		//*(p++) = '\r';	// キャリッジリターンを追加
		*(p++) = '\n';	// 改行を追加
	}	// for(i=0; i<ini->section_count; i++)
	*p = '\0';	// 最後にヌル文字を追加しておく

	(void)write_func(buff, 1, strlen(buff), ini->io);

	// メモリの開放
	MEM_FREE_FUNC(buff);

	return 0;
}

/********************************************************
* Delete_INI_FILE関数                                   *
* iniファイルを管理する構造体のメモリを開放する         *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
********************************************************/
static void Delete_INI_FILE(INI_FILE_PTR ini)
{
	int i, j;	// for文用のカウンタ

	// セクション情報があるならそれのメモリを開放する
	if(ini->section != NULL)
	{
		for(i=0; i<ini->section_count; i++)
		{	// セクションのキーメモリを開放する
			// キーの情報がないならスキップ
			if((ini->section + i)->key == NULL)
			{
				continue;
			}

			// キーのメモリを開放する
			for(j=0; j<(ini->section + i)->key_count; j++)
			{
				// 文字列情報を開放する
				MEM_FREE_FUNC(((ini->section + i)->key + j)->string);
			}

			MEM_FREE_FUNC((ini->section + i)->key);
		}	// for(i=0; i<ini->section_count; i++)

		MEM_FREE_FUNC(ini->section);
	}	// if(ini->section != NULL)

	MEM_FREE_FUNC(ini);
}

/********************************************************
* IniFileGetString関数                               *
* iniファイルから文字列を取得する                       *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
* section_name	: 検索するセクションの名前              *
* key_name		: 検索するキーの名前                    *
* ret			: 発見した文字列を格納する配列          *
* size			: retのサイズ                           *
* 返り値                                                *
*	発見した文字列の長さ(見つからなかったら0)           *
********************************************************/
long IniFileGetString(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name,
	char* ret,
	const long size
)
{
	char* buff;	// キーの文字列コピー用の一時保存
	char* p;	// キーの文字列操作用
	int section_index;	// セクションの検索結果
	int key_index;		// キーの検索結果
	int len;	// 文字列の長さ

	// セクションの検索
	if((section_index = SectionFind(ini, section_name)) < 0)
	{	// セクションが見つからなかったら終了
		return 0;
	}

	// キーの検索
	key_index = KeyFind((ini->section + section_index), key_name);
	// キーが見つからなかったり、キーに文字列がなかったら終了
	if(key_index < 0 || ((ini->section + section_index)->key + key_index)->string == NULL)
	{
		return 0;
	}

	// 内容の取得
		// 先にbuffのメモリを確保しておく
	buff = (char*)MEM_ALLOC_FUNC(sizeof(char)
		* (strlen(((ini->section + section_index)->key + key_index)->string) + 1));

	// メモリ確保成功のチェック
#ifdef _DEBUG
	if(buff == NULL)
	{
		(void)printf("Memory allocate error.\n(In IniFileGetString)\n");

		return 0;
	}
#endif
	(void)strcpy(buff, ((ini->section + section_index)->key + key_index)->string);
	Trim(buff);	// スペース、タブを削除
	// buffの内容が「"」で囲まれていたら「"」削除する
	p = (*buff == '\"') ? buff + 1 : buff;
	if((len = (int)strlen(p)) > 0 && *(p + len - 1) == '\"')
	{
		*(p + len - 1) = '\0';	// 文字列の最後をヌル文字にする
	}
	(void)strncpy(ret, p, size);
	// バッファのメモリを開放
	MEM_FREE_FUNC(buff);

	return (int)strlen(ret);
}

/********************************************************
* IniFileStrdup関数                                     *
* メモリを確保して文字列を確保                          *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
* section_name	: 検索するセクションの名前              *
* key_name		: 検索するキーの名前                    *
* 返り値                                                *
*	確保したメモリのアドレス(失敗時はNULL)              *
********************************************************/
char* IniFileStrdup(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name
)
{
	char* ret;	// 返り値
	char* p;	// 「"」(ダブルクオーテーション)削除用
	int section_index;	// セクションの検索結果
	int key_index;		// キーの検索結果
	long len;

	// セクションの検索
	if((section_index = SectionFind(ini, section_name)) < 0)
	{	// セクションが見つからなかったら終了
		return NULL;
	}

	// キーの検索
	key_index = KeyFind((ini->section + section_index), key_name);
	// キーが見つからなかったり、キーに文字列がなかったら終了
	if(key_index < 0 || ((ini->section + section_index)->key + key_index)->string == NULL)
	{
		return NULL;
	}

	// 内容の取得
		// 先にbuffのメモリを確保しておく
	ret = (char*)MEM_ALLOC_FUNC(sizeof(char)
		* (strlen(((ini->section + section_index)->key + key_index)->string) + 1));

	// メモリ確保成功のチェック
#ifdef _DEBUG
	if(ret == NULL)
	{
		(void)printf("Memory allocate error.\n(In Ini_File_Alloc_String)\n");

		return NULL;
	}
#endif

	// 一度バッファに文字列をコピーしてから操作
	(void)strcpy(ret, ((ini->section + section_index)->key + key_index)->string);
	// 文字列が「"」で囲まれていたら「"」を削除
	if(*ret == '\"')
	{
		p = ret;
		while(*(p+1) != '\0')
		{
			*p = *(p+1);
			p++;
		}

		if((len = (int)strlen(ret)) > 0 && *(ret + len - 2) == '\"')
		{
			*(ret + len - 2) = '\0';	// 最後の文字をヌル文字に変更
		}
	}

	return ret;
}

/********************************************************
* IniFileGetInteger関数                                 *
* iniファイルから数値を取得する                         *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
* section_name	: 検索するセクションの名前              *
* key_name		: 検索するキーの名前                    *
* 返り値                                                *
*	キーに書かれている数値(失敗時は常に0)               *
********************************************************/
int IniFileGetInteger(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name
)
{
	char buff[NUM_STRING_LEN];	// 数値の文字列を格納する
	char *p;	// 文字列操作用
	int section_index;	// セクションの検索結果
	int key_index;		// キーの検索結果
	int len;	// 文字列の長さ

	// セクションの検索
	if((section_index = SectionFind(ini, section_name)) < 0)
	{	// セクションが見つからなかったら終了
		return 0;
	}

	// キーの検索
	key_index = KeyFind((ini->section + section_index), key_name);
	// キーが見つからなかったり、キーに文字列がなかったら終了
	if(key_index < 0 || ((ini->section + section_index)->key + key_index)->string == NULL)
	{
		return 0;
	}

	// 数値がバッファをオーバーしないかをチェック
	if((len = (int)strlen(((ini->section + section_index)->key + key_index)->string)) > NUM_STRING_LEN)
	{
		(void)printf("Buffer over flow\n(In IniFileGetInteger)\n");

		return 0;
	}

	// 一度バッファに文字列をコピー
	(void)strcpy(buff, ((ini->section + section_index)->key + key_index)->string);
	// 文字列が「"」で囲まれていたら「"」を削除する
	p = (*buff == '\"') ? (len--, buff + 1) : buff;
	if(len > 0 && *(p + len - 1) == '\"')
	{
		*(p + len - 1) = '\0';	// 最後の文字をヌル文字に変える
	}

	return atoi(buff);
}

/********************************************************
* IniFileGetDouble関数                                  *
* iniファイルから小数数値を取得する                     *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
* section_name	: 検索するセクションの名前              *
* key_name		: 検索するキーの名前                    *
* 返り値                                                *
*	キーに書かれている数値(失敗時は常に0)               *
********************************************************/
double IniFileGetDouble(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name
)
{
	char buff[NUM_STRING_LEN];	// 数値の文字列を格納する
	char *p;	// 文字列操作用
	int section_index;	// セクションの検索結果
	int key_index;		// キーの検索結果
	int len;	// 文字列の長さ

	// セクションの検索
	if((section_index = SectionFind(ini, section_name)) < 0)
	{	// セクションが見つからなかったら終了
		return 0;
	}

	// キーの検索
	key_index = KeyFind((ini->section + section_index), key_name);
	// キーが見つからなかったり、キーに文字列がなかったら終了
	if(key_index < 0 || ((ini->section + section_index)->key + key_index)->string == NULL)
	{
		return 0;
	}

	// 数値がバッファをオーバーしないかをチェック
	if((len = (int)strlen(((ini->section + section_index)->key + key_index)->string)) > NUM_STRING_LEN)
	{
		(void)printf("Buffer over flow\n(In IniFileGetInteger)\n");

		return 0;
	}

	// 一度バッファに文字列をコピー
	(void)strcpy(buff, ((ini->section + section_index)->key + key_index)->string);
	// 文字列が「"」で囲まれていたら「"」を削除する
	p = (*buff == '\"') ? (len--, buff + 1) : buff;
	if(len > 0 && *(p + len - 1) == '\"')
	{
		*(p + len - 1) = '\0';	// 最後の文字をヌル文字に変える
	}

	return atof(buff);
}

/********************************************************
* IniFileGetArray関数                                   *
* iniファイルから複数の数値データを取得する             *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
* section_name	: 検索するセクションの名前              *
* key_name		: 検索するキーの名前                    *
* array_ptr		: データを代入する配列                  *
* array_size	: 配列のサイズ                          *
* 返り値                                                *
*	正常終了(0)、以上終了(負の値)                       *
********************************************************/
int IniFileGetArray(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name,
	int* array_ptr,
	int array_size
)
{
	char* buff;	// キーの文字列コピー用の一時保存
	char* p;	// キーの文字列操作用
	int section_index;	// セクションの検索結果
	int key_index;		// キーの検索結果
	int len;	// 文字列の長さ
	int i, j;	// for文用のカウンタ

	// セクションの検索
	if((section_index = SectionFind(ini, section_name)) < 0)
	{	// セクションが見つからなかったら終了
		return -1;
	}

	// キーの検索
	key_index = KeyFind((ini->section + section_index), key_name);
	// キーが見つからなかったり、キーに文字列がなかったら終了
	if(key_index < 0 || ((ini->section + section_index)->key + key_index)->string == NULL)
	{
		return -1;
	}

	// 内容の取得
		// 先にbuffのメモリを確保しておく
	buff = (char*)MEM_ALLOC_FUNC(sizeof(char)
		* (strlen(((ini->section + section_index)->key + key_index)->string) + 1));

	// メモリ確保成功のチェック
#ifdef _DEBUG
	if(buff == NULL)
	{
		(void)printf("Memory allocate error.\n(In IniFileGetString)\n");

		return 0;
	}
#endif
	(void)strcpy(buff, ((ini->section + section_index)->key + key_index)->string);
	Trim(buff);	// スペース、タブを削除
	// buffの内容が「"」で囲まれていたら「"」削除する
	p = (*buff == '\"') ? buff + 1 : buff;
	if((len = (int)strlen(p)) > 0 && *(p + len - 1) == '\"')
	{
		*(p + len - 1) = '\0';	// 文字列の最後をヌル文字にする
	}

	// 要求された数だけデータを読み込む
	for(i=0; (i<array_size && p != '\0'); i++)
	{
		// 文字列を数値に変換するためのバッファ
		char num_str[NUM_STRING_LEN];

		// pがさすものが数字になるまでpを勧める
		while(isdigit(*p) == 0)
		{
			if(*p == '\0')
			{	// 文字列の終端にきてしまったらループから脱出
				break;
			}

			p++;
		}

		// 数値の文字列をnum_strに入れる
		for(j=0; (isalnum(*p)!=0 && j < NUM_STRING_LEN); j++, p++)
		{
			num_str[j] = *p;
		}
		num_str[j] = '\0';	// atoiを使うために最後の文字をヌル文字にする

		// 配列にデータを入れる
		array_ptr[i] = atoi(num_str);
	}	// for(i=0; (i<array_size && p != '\0'); i++)

	// バッファのメモリを開放
	MEM_FREE_FUNC(buff);

	return 0;
}

/********************************************************
* IniFileWriteAdd関数                                   *
* iniファイルにデータを追加する                         *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
* section_name	: 検索するセクションの名前              *
* key_name		: 検索するキーの名前                    *
* str			: 追加するデータの文字列                *
* 返り値                                                *
*	正常終了(0)、以上終了(-1)                           *
********************************************************/
static int IniFileAddData(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name,
	char* str
)
{
	int section_index;	// セクションの検索結果
	int key_index;		// キーの検索結果
	int i;	// for文用のカウンタ

	// セクション名が指定されていなかったらエラー
	if(section_name == NULL)
	{
		return -1;
	}

	// セクションの情報がなかったらメモリを確保する
	if(ini->section == NULL)
	{
		ini->section = (SECTION_PTR)MEM_ALLOC_FUNC(sizeof(SECTION) * INI_ALLOC_SIZE);

		// メモリ確保成功のチェック
#ifdef _DEBUG
		if(ini->section == NULL)
		{
			return -2;
		}
#endif
		ini->section_count = 1;
		ini->section_size = INI_ALLOC_SIZE;
		(void)memset(ini->section, 0, sizeof(SECTION) * INI_ALLOC_SIZE);
	}

	// セクションの検索
	if((section_index = SectionFind(ini, section_name)) < 0)
	{	// 見つからなかったらセクションを追加する
		if(SectionAdd(ini, section_name) != 0)
		{
			return -3;
		}
		// セクションのインデックスを設定
		section_index = ini->section_count - 1;
	}

	// キーの名前がなく、キーが存在していたらキーを削除する
		// (このセクションにはキーがないことになるため)
	if(key_name == NULL)
	{
		if((ini->section + section_index)->key != NULL)
		{	// キーの削除
			for(i=0; i<(ini->section + section_index)->key_count; i++)
			{
				// キーに文字列が存在していたらそのメモリを開放する
				MEM_FREE_FUNC(((ini->section + section_index)->key + i)->string);
			}

			// キーに使われていたメモリを開放し、値をリセット
			MEM_FREE_FUNC((ini->section + section_index)->key);
			(ini->section + section_index)->key = NULL;
			(ini->section + section_index)->key_count = 0;
			(ini->section + section_index)->key_size = 0;
		}	// if((ini->section + section_index)->key != NULL)

		return 0;	// 以下の処理はいらないので終了
	}	// if(key_name == NULL)

	// キーの検索
	if((key_index = KeyFind((ini->section + section_index), key_name)) < 0)
	{	// 見つからなかったらキーを追加
		return KeyAdd((ini->section + section_index), key_name, str, COMMENT_OFF);
	}
	else
	{	// 見つかったら内容を変更する
		// 文字列が存在したらそのメモリを先に開放
		MEM_FREE_FUNC(((ini->section + section_index)->key + key_index)->string);

		if(str == NULL)
		{	// キーに入れる情報がなけばその用に設定
			*((ini->section + section_index)->key + key_index)->key_name = '\0';
			((ini->section + section_index)->key + key_index)->string = NULL;
			return 0;
		}

		// キーに文字列を追加する
		((ini->section + section_index)->key + key_index)->string =
			MEM_STRDUP_FUNC(str);
		// メモリ確保成功のチェック
#ifdef _DEBUG
		if(((ini->section + section_index)->key + key_index)->string == NULL)
		{
			(void)printf("Memory allocate error.\n(In Ini_File_Write_Data)\n");

			return -4;
		}
#endif
	}	// if((key_index = KeyFind((ini->section + section_index), key_name)) < 0) else

	return 0;
}

/********************************************************
* IniFileAddString関数                                  *
* iniファイルに文字列を追加する                         *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
* section_name	: 検索するセクションの名前              *
* key_name		: 検索するキーの名前                    *
* str			: 追加する文字列                        *
* 返り値                                                *
*	正常終了(0)、以上終了(負の値)                       *
********************************************************/
int IniFileAddString(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name,
	const char* str
)
{
	char* buff;	// 書き込む文字列を格納する
	char* p;	// 文字列操作用
	int ret;	// 返り値

	// 文字列がなかったらそのまま書き出し
	if(str == NULL || *str == '\0')
	{
		return IniFileAddData(ini, section_name, key_name, "");
	}

	// バッファのメモリを確保する
		// 「"」で囲む分のメモリも含む
	buff = (char*)MEM_ALLOC_FUNC(sizeof(char)*(strlen(str)+3));

	// メモリ確保成功のチェック
#ifdef _DEBUG
	if(buff == NULL)
	{
		(void)printf("Memory allocate error.\n(In Ini_File_Write_String)\n");

		return -1;
	}
#endif

	// 「"」で文字列を囲む
	p = buff;
	*(p++) = '\"';
	(void)strcpy(p, str);
	p += strlen(p);
	*(p++) = '\"';
	*(p++) = '\0';	// 文字列の最後にヌル文字を追加
	ret = IniFileAddData(ini, section_name, key_name, buff);

	// バッファのメモリを開放する
	MEM_FREE_FUNC(buff);

	return ret;
}

/*********************************************************
* IniFileAddInteger関数                                  *
* iniファイルに数値を追加する                            *
* 引数                                                   *
* ini			: iniファイルを管理する構造体のアドレス  *
* section_name	: 検索するセクションの名前               *
* key_name		: 検索するキーの名前                     *
* num			: 追加する数値                           *
* radix			: 数値のフォーマット(10進数、16進数など) *
* 返り値                                                 *
*	正常終了(0)、以上終了(負の値)                        *
*********************************************************/
extern int IniFileAddInteger(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name,
	const int num,
	const int radix
)
{
	// 書き込むデータを格納
	char buff[NUM_STRING_LEN];

	// 数値を文字列に変換する
	itoa(num, buff, radix);

	// データを追加
	return IniFileAddData(ini, section_name, key_name, buff);
}

/********************************************************
* IniFileAddDouble関数                                  *
* iniファイルに小数データを追加する                     *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
* section_name	: 検索するセクションの名前              *
* key_name		: 検索するキーの名前                    *
* num			: 追加する数値                          *
* digigt		: 小数点以下の桁数(-1以下で設定無し)    *
* 返り値                                                *
*	正常終了(0)、以上終了(負の値)                       *
********************************************************/
int IniFileAddDouble(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name,
	const double num,
	const int digit
)
{
	// 書き込むデータを格納
	char buff[NUM_STRING_LEN];

	// 小数点以下の設定があれば
	if(digit >= 0)
	{
		// 桁数作成用
		char format[16];

		// フォーマット用文字列作成
		(void)sprintf(format, "%%.%df", digit);
		// 数値を文字列に変換する
		(void)sprintf(buff, format, num);
	}
	else
	{
		// 数値を文字列に変換する
		(void)sprintf(buff, "%.2f", num);
	}

	// データを追加
	return IniFileAddData(ini, section_name, key_name, buff);
}

#ifdef __cplusplus
}
#endif
