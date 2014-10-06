#ifndef _INLUCDED_INI_FILE_H_
#define _INLUCDED_INI_FILE_H_

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

#define KEY_BUFF_SIZE 256		// キーにつけられる名前の最大長
#define SECTION_BUFF_SIZE 256	// セクションにつけられる名前の最大長
#define INI_ALLOC_SIZE 10		// サイズ更新時の更新値

#define NUM_STRING_LEN 64	// 数値データの文字列の長さの最大値

#define COMMENT_ON 1	// コメントである(フラグの値)
#define COMMENT_OFF 0	// コメントでない(フラグの値)

// 読み込みか書き込みか
typedef enum _eINI_MODE
{
	INI_READ,
	INI_WRITE
} eINI_MODE;

typedef struct _KEY
{
	char key_name[KEY_BUFF_SIZE];	// キーの名前を格納
	int hash;	// ハッシュ値
	char *string;
	char comment_flag;	// コメントであるかどうかのフラグ
} KEY, *KEY_PTR;

typedef struct _SECTION
{
	// セクションの名前を格納
	char section_name[SECTION_BUFF_SIZE];
	int hash;	// ハッシュ値
	KEY_PTR key;	// キーの配列
	int key_count;	// キーの数
	int key_size;	// キーのサイズ
} SECTION, *SECTION_PTR;

typedef struct _INI_FILE
{
	SECTION_PTR section;	// iniファイルの内容を格納
	int section_size;	// セクションのサイズ
	int section_count;	// セクションの数
	void* io;			// iniファイルデータへのファイルポインタ
	char* file_path;	// iniファイルへのパス

	// メモリ開放用の関数へのポインタ
	void (*delete_func)(struct _INI_FILE *ini);
} INI_FILE, *INI_FILE_PTR;

// 関数のプロトタイプ宣言
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
EXTERN INI_FILE_PTR CreateIniFile(
	void* io,
	size_t (*read_func)(void*, size_t, size_t, void*),
	size_t data_size,
	eINI_MODE mode
);

/********************************************************
* WriteIniFile関数                                      *
* iniファイルに内容を書き出す                           *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
* write_func	: 書き出し用関数へのポインタ            *
* 返り値                                                *
*	正常終了(0)、以上終了(0以外)                        *
********************************************************/
EXTERN int WriteIniFile(
	INI_FILE_PTR ini,
	size_t (*write_func)(void*, size_t, size_t, void*)
);

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
EXTERN long IniFileGetString(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name,
	char* ret,
	const long size
);

/********************************************************
* IniFileStrdup関数                                   *
* メモリを確保して文字列を確保                          *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
* section_name	: 検索するセクションの名前              *
* key_name		: 検索するキーの名前                    *
* 返り値                                                *
*	確保したメモリのアドレス(失敗時はNULL)              *
********************************************************/
EXTERN char* IniFileStrdup(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name
);

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
EXTERN int IniFileGetInteger(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name
);

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
EXTERN double IniFileGetDouble(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name
);

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
EXTERN int IniFileGetArray(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name,
	int* array_ptr,
	int array_size
);

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
EXTERN int IniFileAddString(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name,
	const char* str
);

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
EXTERN int IniFileAddInteger(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name,
	const int num,
	const int radix
);

/********************************************************
* IniFileAddDouble関数                                  *
* iniファイルに小数データを追加する                     *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
* section_name	: 検索するセクションの名前              *
* key_name		: 検索するキーの名前                    *
* num			: 追加する数値                          *
* 返り値                                                *
*	正常終了(0)、以上終了(負の値)                       *
********************************************************/
EXTERN int IniFileAddDouble(
	INI_FILE_PTR ini,
	const char* section_name,
	const char* key_name,
	const double num
);

/********************************************************
* Change_INI_FILE_MODE関数                              *
* iniファイルの書き込み、読み込みのモードを切り替える   *
* 引数                                                  *
* ini			: iniファイルを管理する構造体のアドレス *
* 返り値                                                *
*	正常終了(0)、異常終了(負の値)                       *
********************************************************/
EXTERN int Change_INI_FILE_MODE(INI_FILE_PTR ini, eINI_MODE mode);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INLUCDED_INI_FILE_H_
