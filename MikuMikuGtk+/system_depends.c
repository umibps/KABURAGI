// Visual Studio 2005ˆÈ~‚Å‚ÍŒÃ‚¢‚Æ‚³‚ê‚éŠÖ”‚ðŽg—p‚·‚é‚Ì‚Å
	// Œx‚ªo‚È‚¢‚æ‚¤‚É‚·‚é
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdlib.h>
#include <string.h>
#include "system_depends.h"
#include "memory.h"

#ifdef __cplusplus
extern "C" {
#endif

char* LocaleFromUTF8(const char* utf8_code)
{
	gchar *locale = g_locale_from_utf8(utf8_code, -1, NULL, NULL, NULL);
	char *ret = MEM_STRDUP_FUNC(locale);
	g_free(locale);
	return ret;
}

char* Locale2UTF8(const char* locale)
{
	gchar *utf8_code = g_locale_to_utf8(locale, -1, NULL, NULL, NULL);
	char *ret = MEM_STRDUP_FUNC(utf8_code);
	g_free(utf8_code);
	return ret;
}

char* GetDirectoryName(const char* utf8_path)
{
	gchar *dir = g_path_get_dirname(utf8_path);
	char *ret = MEM_STRDUP_FUNC(dir);
	g_free(dir);
	return ret;
}

char* NextCharUTF8(char* str)
{
	return g_utf8_next_char(str);
}

#ifdef __cplusplus
}
#endif
