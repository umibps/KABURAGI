//---------------------------------------------------------------------------

#include <stdlib.h>
#include <string.h>
#include "slide.h"
#include "memory.h"
//---------------------------------------------------------------------------

static void CompressAddMap(SLIDE_COMPRESSOR* c, int p);
static void CompressDeleteMap(SLIDE_COMPRESSOR* c, int p);
static int CompressGetMatch(SLIDE_COMPRESSOR* c, const unsigned char*cur, int curlen, int *pos, int s);

SLIDE_COMPRESSOR* CreateSlideCompressor(void)
{
	SLIDE_COMPRESSOR* ret =
		(SLIDE_COMPRESSOR*)MEM_ALLOC_FUNC(sizeof(*ret));
	int i;

	(void)memset(ret, 0, sizeof(*ret));

	for(i=0; i < 256*256; i++)
	{
		ret->map[i] = -1;
	}

	for(i=0; i < SLIDE_N; i++)
	{
		ret->chains[i].prev = ret->chains[i].next = -1;
	}

	for(i = SLIDE_N - 1; i >= 0; i--)
	{
		CompressAddMap(ret, i);
	}

	return ret;
}

void DeleteSlideCompressor(SLIDE_COMPRESSOR** c)
{
	MEM_FREE_FUNC(*c);
	*c = NULL;
}

static void CompressAddMap(SLIDE_COMPRESSOR* c, int p)
{
	int place = c->text[p] + ((int)c->text[(p + 1) & (SLIDE_N - 1)] << 8);

	if(c->map[place] == -1)
	{
		// Å‰‚Ì‘}“ü
		c->map[place] = p;
	}
	else
	{
		// Å‰ˆÈŠO
		int old = c->map[place];
		c->map[place] = p;
		c->chains[old].prev = p;
		c->chains[p].next = old;
		c->chains[p].prev = -1;
    }
}

static void CompressDeleteMap(SLIDE_COMPRESSOR* c, int p)
{
	int n;
	if((n = c->chains[p].next) != -1)
	{
		c->chains[n].prev = c->chains[p].prev;
	}

	if((n = c->chains[p].prev) != -1)
	{
		c->chains[n].next = c->chains[p].next;
	}
	else if(c->chains[p].next != -1)
	{
		int place = c->text[p] + ((int)c->text[(p + 1) & (SLIDE_N - 1)] << 8);
		c->map[place] = c->chains[p].next;
	}
	else
	{
		int place = c->text[p] + ((int)c->text[(p + 1) & (SLIDE_N - 1)] << 8);
		c->map[place] = -1;
	}

	c->chains[p].prev = -1;
	c->chains[p].next = -1;
}

static int CompressGetMatch(SLIDE_COMPRESSOR* compress, const unsigned char*cur, int curlen, int *pos, int s)
{
	int place, maxlen = 0, matchlen;

	// get match length
	if(curlen < 3) return 0;

	place = cur[0] + ((int)cur[1] << 8);

	if((place = compress->map[place]) != -1)
	{
		int place_org;
		curlen -= 1;
		do
		{
			int lim;
			const char* c = cur + 2;
			place_org = place;
			if(s == place || s == ((place + 1) & (SLIDE_N -1))) continue;
			place += 2;
			lim = (SLIDE_M < curlen ? SLIDE_M : curlen) + place_org;
			// c = cur + 2;
			if(lim >= SLIDE_N)
			{
				if(place_org <= s && s < SLIDE_N)
					lim = s;
				else if(s < (lim&(SLIDE_N-1)))
					lim = s + SLIDE_N;
			}
			else
			{
				if(place_org <= s && s < lim)
					lim = s;
			}
			while(compress->text[place] == *(c++) && place < lim) place++;
			matchlen = place - place_org;
			if(matchlen > maxlen) *pos = place_org, maxlen = matchlen;
			if(matchlen == SLIDE_M) return maxlen;

		} while((place = compress->chains[place_org].next) != -1);
	}

	return maxlen;
}

void SlideEncode(SLIDE_COMPRESSOR* compress, const unsigned char *in, long inlen,
		unsigned char *out, long* outlen)
{
	unsigned char code[40], codeptr, mask;
	int s;
	int i;

	if(inlen == 0) return;

	*outlen = 0;
	code[0] = 0;
	codeptr = mask = 1;

	s = compress->s;
	while(inlen > 0)
	{
		int pos = 0;
		int len = CompressGetMatch(compress, in, inlen, &pos, s);
		if(len >= 3)
		{
			code[0] |= mask;
			if(len >= 18)
			{
				code[codeptr++] = pos & 0xff;
				code[codeptr++] = ((pos &0xf00)>> 8) | 0xf0;
				code[codeptr++] = len - 18;
			}
			else
			{
				code[codeptr++] = pos & 0xff;
				code[codeptr++] = ((pos&0xf00)>> 8) | ((len-3)<<4);
			}
			while(len--)
			{
				unsigned char c = 0[in++];
				CompressDeleteMap(compress, (s - 1) & (SLIDE_N - 1));
				CompressDeleteMap(compress, s);
				if(s < SLIDE_M - 1) compress->text[s + SLIDE_N] = c;
				compress->text[s] = c;
				CompressAddMap(compress, (s - 1) & (SLIDE_N - 1));
				CompressAddMap(compress, s);
				s++;
				inlen--;
				s &= (SLIDE_N - 1);
			}
		}
		else
		{
			unsigned char c = 0[in++];
			CompressDeleteMap(compress, (s - 1) & (SLIDE_N - 1));
			CompressDeleteMap(compress, s);
			if(s < SLIDE_M - 1) compress->text[s + SLIDE_N] = c;
			compress->text[s] = c;
			CompressAddMap(compress, (s - 1) & (SLIDE_N - 1));
			CompressAddMap(compress, s);
			s++;
			inlen--;
			s &= (SLIDE_N - 1);
			code[codeptr++] = c;
		}
		mask <<= 1;

		if(mask == 0)
		{
			for(i = 0; i < codeptr; i++)
			{
				out[(*outlen)++] = code[i];
			}

			mask = codeptr = 1;
			code[0] = 0;
		}
	}

	if(mask != 1)
	{
		for(i = 0; i < codeptr; i++)
		{
			out[(*outlen)++] = code[i];
		}
	}

	compress->s = s;
}
