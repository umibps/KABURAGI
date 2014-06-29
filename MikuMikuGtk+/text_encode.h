#ifndef _INCLUDED_TEXT_ENCODE_H_
#define _INCLUDED_TEXT_ENCODE_H_

#include <iconv.h>

#define TEXT_ENCODE_BUFFER_SIZE 4096

typedef enum _eTEXT_TYPE
{
	TEXT_TYPE_UTF16,
	TEXT_TYPE_UTF8
} eTEXT_TYPE;

typedef enum _eTEXT_CONSTANT_TYPE
{
	TEXT_CONSTANT_TYPE_LEFT,
	TEXT_CONSTANT_TYPE_RIGHT,
	TEXT_CONSTANT_TYPE_FINGER,
	TEXT_CONSTANT_TYPE_ELBOW,
	TEXT_CONSTANT_TYPE_ARM,
	TEXT_CONSTANT_TYPE_WRIST,
	TEXT_CONSTANT_TYPE_CENTER,
	TEXT_CONSTANT_TYPE_ASTERISK,
	TEXT_CONSTANT_TYPE_SPH_EXTENSION,
	TEXT_CONSTANT_TYPE_SPA_EXTENSION,
	TEXT_CONSTANT_TYPE_RIGHT_KNEE,
	TEXT_CONSTANT_TYPE_LEFT_KNEE,
	TEXT_CONSTANT_TYPE_ROOT_BONE,
	TEXT_CONSTANT_TYPE_SCALE_BONE_ASSET,
	TEXT_CONSTANT_TYPE_OPACITY_MORPH_ASSET,
	MAX_TEXT_CONSTANT_TYPE
} eTEXT_CONSTANT_TYPE;

typedef enum _eTEXT_ENCODE_TYPE
{
	TEXT_ENCODE_TYPE_UNKOWN,
	TEXT_ENCODE_TYPE_ASCII,
	TEXT_ENCODE_TYPE_JIS,
	TEXT_ENCODE_TYPE_SJIS,
	TEXT_ENCODE_TYPE_EUC,
	TEXT_ENCODE_TYPE_BINARY = -1,
} eTEXT_ENCODE_TYPE;

typedef struct _TEXT_ENCODE
{
	iconv_t convert;
	char buff[TEXT_ENCODE_BUFFER_SIZE];
	char *from;
	char *to;
} TEXT_ENCODE;

#ifdef __cplusplus
extern "C" {
#endif

extern void InitializeTextEncode(
	TEXT_ENCODE* encode,
	const char* from,
	const char* to
);

extern void ReleaseTextEncode(TEXT_ENCODE* encode);

extern char* EncodeText(TEXT_ENCODE* encode, const char* text, size_t in_bytes);

extern void TextEncodeSetSource(TEXT_ENCODE* encode, const char* form);

extern void TextEncodeSetDestination(TEXT_ENCODE* encode, const char* to);

extern const char* GetTextTypeString(eTEXT_TYPE type);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_TEXT_ENCODE_H_
