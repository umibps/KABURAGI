//---------------------------------------------------------------------------
#ifndef SLIDE_H
#define SLIDE_H
//---------------------------------------------------------------------------
#define SLIDE_N 4096
#define SLIDE_M (18+255)

typedef struct _SLIDE_CHAIN
{
	int prev, next;
} SLIDE_CHAIN;

typedef struct _SLIDE_COMPRESSOR
{
	unsigned char text[SLIDE_N + SLIDE_M - 1];
	int map[256*256];
	SLIDE_CHAIN chains[SLIDE_N];

	unsigned char text2[SLIDE_N + SLIDE_M - 1];
	int map2[256*256];
	SLIDE_CHAIN chains2[SLIDE_N];

	int s, s2;
} SLIDE_COMPRESSOR;


extern SLIDE_COMPRESSOR* CreateSlideCompressor(void);
extern void DeleteSlideCompressor(SLIDE_COMPRESSOR** c);
extern void SlideEncode(SLIDE_COMPRESSOR* compress, const unsigned char *in, long inlen,
		unsigned char *out, long* outlen);

//---------------------------------------------------------------------------
#endif
//---------------------------------------------------------------------------
