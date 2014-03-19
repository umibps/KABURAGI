// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#define LIBICONV_PLUG

#include <string.h>
#include <ctype.h>
#include "utils.h"
#include "text_encode.h"
#include "memory.h"

void InitializeTextEncode(
	TEXT_ENCODE* encode,
	const char* from,
	const char* to
)
{
	(void)memset(encode, 0, sizeof(*encode));

	encode->convert = iconv_open(to, from);
	encode->from = MEM_STRDUP_FUNC(from);
	encode->to = MEM_STRDUP_FUNC(to);
}

void ReleaseTextEncode(TEXT_ENCODE* encode)
{
	iconv_close(encode->convert);
	MEM_FREE_FUNC(encode->from);
	MEM_FREE_FUNC(encode->to);
}

char* EncodeText(TEXT_ENCODE* encode, const char* text, size_t in_bytes)
{
	size_t length = in_bytes;
	size_t out_length = TEXT_ENCODE_BUFFER_SIZE - 1;
	size_t out_size = out_length;
	char *out_buff = encode->buff;

	if(iconv(encode->convert, &text, &length, &out_buff, &out_length) < 0)
	{
		return NULL;
	}

	encode->buff[out_size-out_length] = '\0';
	return MEM_STRDUP_FUNC(encode->buff);
}

void TextEncodeSetSource(TEXT_ENCODE* encode, const char* from)
{
	iconv_close(encode->convert);
	MEM_FREE_FUNC(encode->from);

	encode->convert = iconv_open(encode->to, from);
	encode->from = MEM_STRDUP_FUNC(from);
}

void TextEncodeSetDestination(TEXT_ENCODE* encode, const char* to)
{
	iconv_close(encode->convert);
	MEM_FREE_FUNC(encode->to);

	encode->convert = iconv_open(to, encode->from);
	encode->to = MEM_STRDUP_FUNC(to);
}

const char* GetTextTypeString(eTEXT_TYPE type)
{
	switch(type)
	{
	case TEXT_TYPE_UTF16:
		return "UTF-16LE";
	case TEXT_TYPE_UTF8:
		return "UTF-8";
	}

	return "";
}
