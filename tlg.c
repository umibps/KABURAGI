// Visual Studio 2005以降では古いとされる関数を使用するので
	// 警告が出ないようにする
#if defined _MSC_VER && _MSC_VER >= 1400
# define _CRT_SECURE_NO_DEPRECATE
#endif

#include "tlg.h"

#ifdef _MSC_VER
# if defined(USE_SIMD_DECODE) && USE_SIMD_DECODE != 0
#  pragma comment(lib, "tlg.lib")
# endif
#endif

#ifdef __BORLANDC__
# pragma link "tlg.lib"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"

/* Functions that needs to be exported ( for non-class-member functions ) */
/* This should only be applyed for function declaration in headers ( not body ) */
#define TJS_EXP_FUNC_DEF(rettype, name, arg) extern rettype name arg


/* Functions that needs to be exported ( for class-member functions ) */
#define TJS_METHOD_DEF(rettype, name, arg) rettype name arg
#define TJS_CONST_METHOD_DEF(rettype, name, arg) rettype name arg const
#define TJS_STATIC_METHOD_DEF(rettype, name, arg) static rettype name arg
#define TJS_STATIC_CONST_METHOD_DEF(rettype, name, arg) static rettype name arg const
#define TJS_METHOD_RET_EMPTY
#define TJS_METHOD_RET(type)



#if defined(_WIN32)  && !defined(__GNUC__)

/* VC++/BCC */

#include <windows.h>

/*[*/
typedef __int8 tjs_int8;
typedef unsigned __int8 tjs_uint8;
typedef __int16 tjs_int16;
typedef unsigned __int16 tjs_uint16;
typedef __int32 tjs_int32;
typedef unsigned __int32 tjs_uint32;
typedef __int64 tjs_int64;
typedef unsigned __int64 tjs_uint64;
typedef int tjs_int;    /* at least 32bits */
typedef unsigned int tjs_uint;    /* at least 32bits */

#ifdef __cplusplus
typedef wchar_t tjs_char;
#else
typedef unsigned short tjs_char;
#endif

typedef char tjs_nchar;
typedef double tjs_real;

typedef unsigned char uint8;

#define TJS_HOST_IS_BIG_ENDIAN 0
#define TJS_HOST_IS_LITTLE_ENDIAN 1

#ifndef TJS_INTF_METHOD
#define TJS_INTF_METHOD __cdecl
	/* TJS_INTF_METHOD is "cdecl" (by default)
		since TJS2 2.4.14 (kirikir2 2.25 beta 1) */
#endif

#define TJS_USERENTRY __cdecl

#define TJS_I64_VAL(x) ((tjs_int64)(x##i64))
#define TJS_UI64_VAL(x) ((tjs_uint64)(x##i64))

/*]*/

#else

/* gcc ? */

#ifndef __GNUC__
 #error "GNU C++ required."
#endif
/*
#ifndef HAVE_CONFIG_H
 #error "-DHAVE_CONFIG_H and config.h required."
#endif
*/
#include <sys/types.h>
#include <stdint.h>


#if defined(__linux__)
	typedef int8_t tjs_int8;
	typedef u_int8_t tjs_uint8;
	typedef int16_t tjs_int16;
	typedef u_int16_t tjs_uint16;
	typedef int32_t tjs_int32;
	typedef u_int32_t tjs_uint32;
	typedef int64_t tjs_int64;
	typedef u_int64_t tjs_uint64;
#elif defined(__GNUC__)
	typedef int8_t tjs_int8;
	typedef uint8_t tjs_uint8;
	typedef int16_t tjs_int16;
	typedef uint16_t tjs_uint16;
	typedef int32_t tjs_int32;
	typedef uint32_t tjs_uint32;
	typedef int64_t tjs_int64;
	typedef uint64_t tjs_uint64;
#endif

typedef tjs_uint32 DWORD;
typedef tjs_uint16 WORD;
typedef tjs_uint8 BYTE;
typedef int BOOL;
# define BI_RGB (0)
# ifndef FALSE
#  define FALSE (0)
# endif
# ifndef TRUE
#  define TRUE (!(FALSE))
# endif

typedef wchar_t tjs_char;

typedef char tjs_nchar;
typedef double tjs_real;

typedef int tjs_int;
typedef unsigned int tjs_uint;

typedef unsigned char uint8;

#define TJS_I64_VAL(x) ((tjs_int64)(x##LL))
#define TJS_UI64_VAL(x) ((tjs_uint64)(x##LL))

#ifdef WORDS_BIGENDIAN
	#define TJS_HOST_IS_BIG_ENDIAN 1
	#define TJS_HOST_IS_LITTLE_ENDIAN 0
#else
	#define TJS_HOST_IS_BIG_ENDIAN 0
	#define TJS_HOST_IS_LITTLE_ENDIAN 1
#endif

#define TJS_INTF_METHOD
#define TJS_USERENTRY


#endif /* end of defined(_WIN32) && !defined(__GNUC__) */

/*[*/
#define TJS_W(X) L##X
#define TJS_N(X) X

typedef tjs_int32 tjs_error;

typedef tjs_int64 tTVInteger;
typedef tjs_real tTVReal;


/* IEEE double manipulation support
 (TJS requires IEEE double(64-bit float) native support on machine or C++ compiler) */

/*

63 62       52 51                         0
+-+-----------+---------------------------+
|s|    exp    |         significand       |
+-+-----------+---------------------------+

s = sign,  negative if this is 1, otherwise positive.



*/

#define TJS_BS_SEEK_SET SEEK_SET
#define TJS_BS_SEEK_CUR SEEK_CUR
#define TJS_BS_SEEK_END SEEK_END

#define TVP_TLG6_H_BLOCK_SIZE 8
#define TVP_TLG6_W_BLOCK_SIZE 8

#define TVP_TLG6_GOLOMB_N_COUNT  4

#if TJS_HOST_IS_BIG_ENDIAN
	#define TVP_TLG6_BYTEOF(a, x) (((tjs_uint8*)(a))[(x)])

	#define TVP_TLG6_FETCH_32BITS(addr) ((tjs_uint32)TVP_TLG6_BYTEOF((addr), 0) +  \
									((tjs_uint32)TVP_TLG6_BYTEOF((addr), 1) << 8) + \
									((tjs_uint32)TVP_TLG6_BYTEOF((addr), 2) << 16) + \
									((tjs_uint32)TVP_TLG6_BYTEOF((addr), 3) << 24) )
#else
	#define TVP_TLG6_FETCH_32BITS(addr) (*(tjs_uint32*)addr)
#endif

#define TVP_TLG6_LeadingZeroTable_BITS 12
#define TVP_TLG6_LeadingZeroTable_SIZE  (1<<TVP_TLG6_LeadingZeroTable_BITS)

#define TVP_TLG6_DO_CHROMA_DECODE_PROTO(B, G, R, A, POST_INCREMENT) do \
			{ \
				tjs_uint32 u = *prevline; \
				p = med(p, u, up, \
					(0xff0000 & ((R)<<16)) + (0xff00 & ((G)<<8)) + (0xff & (B)) + ((A) << 24) ); \
				up = u; \
				*curline = p; \
				curline ++; \
				prevline ++; \
				POST_INCREMENT \
			} while(--w);
#define TVP_TLG6_DO_CHROMA_DECODE_PROTO2(B, G, R, A, POST_INCREMENT) do \
			{ \
				tjs_uint32 u = *prevline; \
				p = avg(p, u, up, \
					(0xff0000 & ((R)<<16)) + (0xff00 & ((G)<<8)) + (0xff & (B)) + ((A) << 24) ); \
				up = u; \
				*curline = p; \
				curline ++; \
				prevline ++; \
				POST_INCREMENT \
			} while(--w);
#define TVP_TLG6_DO_CHROMA_DECODE(N, R, G, B) case (N<<1): \
	TVP_TLG6_DO_CHROMA_DECODE_PROTO(R, G, B, IA, {in+=step;}) break; \
	case (N<<1)+1: \
	TVP_TLG6_DO_CHROMA_DECODE_PROTO2(R, G, B, IA, {in+=step;}) break;

#define TJS_VS_int16_LEN 21

typedef enum _tTVPLoadGraphicType
{
	lgtFullColor, // full 32bit color
	lgtPalGray, // palettized or grayscale
	lgtMask // mask
} tTVPLoadGraphicType;

#ifndef _WIN32
typedef struct tagBITMAPINFOHEADER{
  DWORD  biSize;
  tjs_int32   biWidth;
  tjs_int32   biHeight;
  WORD   biPlanes;
  WORD   biBitCount;
  DWORD  biCompression;
  DWORD  biSizeImage;
  tjs_int32   biXPelsPerMeter;
  tjs_int32   biYPelsPerMeter;
  DWORD  biClrUsed;
  DWORD  biClrImportant;
} BITMAPINFOHEADER, *PBITMAPINFOHEADER;

typedef struct tagRGBQUAD {
  BYTE    rgbBlue;
  BYTE    rgbGreen;
  BYTE    rgbRed;
  BYTE    rgbReserved;
} RGBQUAD;

typedef struct tagBITMAPINFO {
  BITMAPINFOHEADER bmiHeader;
  RGBQUAD          bmiColors[1];
} BITMAPINFO, *PBITMAPINFO;
#endif /* _WIN32 */

typedef struct _tTVPLayerBitmapMemoryRecord
{
	void * alloc_ptr; // allocated pointer
	tjs_uint size; // original bmp bits size, in bytes
	tjs_uint32 sentinel_backup1; // sentinel value 1
	tjs_uint32 sentinel_backup2; // sentinel value 2
} tTVPLayerBitmapMemoryRecord;

typedef struct _tTVPBitmap
{
	PBITMAPINFO BitmapInfo;
	void * Bits; // pointer to bitmap bits
	tjs_int PitchBytes; // bytes required in a line
	tjs_int PitchStep; // step bytes to next(below) line
	tjs_int BitmapInfoSize;
	struct _tTVPBitmap* bitmap;
	tjs_int RefCount;
	tjs_int height;
	tjs_int width;
	void (*Release)(struct _tTVPBitmap *bitmap);
} tTVPBitmap;

typedef struct _tTVPBaseBitmap
{
	struct _tTVPBaseBitmap *base_bitmap;
	tTVPBitmap *bitmap;
	BOOL FontChanged;
	void (*Recreate)(struct _tTVPBaseBitmap *bitmap, tjs_uint w, tjs_uint h, tjs_uint bpp);
	void (*Allocate)(tTVPBitmap *bitmap, tjs_uint width, tjs_uint height, tjs_uint bpp);
	void (*Independ)(struct _tTVPBaseBitmap *bitmap);
	BOOL (*Isindependent)(struct _tTVPBaseBitmap *bitmap);
	void* (*GetScanLineForWrite)(struct _tTVPBaseBitmap *bitmap, tjs_uint l);
	void* (*GetScanLine)(tTVPBitmap *bitmap, tjs_uint l);
} tTVPBaseBitmap;

typedef struct
{
	char* Name;
	tTVPBaseBitmap *Dest;
	tTVPLoadGraphicType Type;
	tjs_int ColorKey;
	tjs_uint8 *Buffer;
	tjs_uint ScanLineNum;
	tjs_uint DesW;
	tjs_uint DesH;
	tjs_uint OrgW;
	tjs_uint OrgH;
	tjs_uint BufW;
	tjs_uint BufH;
	char *NeedMetaInfo;
} tTVPLoadGraphicData;

typedef void (*tTVPGraphicSizeCallback)
	(void *callbackdata, tjs_uint w, tjs_uint h);
typedef void * (*tTVPGraphicScanLineCallback)
	(void *callbackdata, tjs_int y, tjs_uint color_key);
typedef void (*tTVPMetaInfoPushCallback)
	(void *callbackdata, const char* name, const char* value);

typedef struct _tTJSBinaryStream
{
	size_t (*read)(void* buffer, size_t block_size, size_t block_num, void* src);
	int (*seek)(void* src, long offset, int origin);
	long (*tell)(void* src);
	void* pData;
} tTJSBinaryStream;

typedef enum _tTVPGraphicLoadMode
{
	glmNormal, // normal, ie. 32bit ARGB graphic
	glmPalettized, // palettized 8bit mode
	glmGrayscale // grayscale 8bit mode
} tTVPGraphicLoadMode;

#ifdef __cplusplus
extern "C" {
#endif

tjs_uint32 TJS_ReadI32LE(struct _tTJSBinaryStream* info)
{
#if TJS_HOST_IS_BIG_ENDIAN
	tjs_uint8 buffer[4];
	tjs_uint32 ret = 0;
	tjs_int i;
	info->ReadBuffer(info, buffer, 4);
	for(i=0; i<4; i++)
	{
		ret += (tjs_uint32)buffer[i]<<(i*8);
	}
	return ret;
#else
	tjs_uint32 temp;
	(void)info->read(&temp, 4, 1, info->pData);

	return temp;
#endif
}

void tTVPBaseBitmapRelease(tTVPBitmap *bitmap)
{
	if(bitmap->RefCount == 1)
	{
		MEM_FREE_FUNC(bitmap);
	}
	else
	{
		bitmap->RefCount--;
	}
}

#define TVP_ALLOC_GBUF_MEM_ALLOC_FUNC

void * TVPAllocBitmapBits(tjs_uint size, tjs_uint width, tjs_uint height)
{
	tjs_uint8 *ptrorg, *ptr;
	tjs_uint allocbytes;
	tTVPLayerBitmapMemoryRecord * record;
	if(size == 0) return NULL;
	allocbytes = 16 + size + sizeof(tTVPLayerBitmapMemoryRecord) + sizeof(tjs_uint32)*2;
#ifdef TVP_ALLOC_GBUF_MEM_ALLOC_FUNC
	ptr = ptrorg = (tjs_uint8*)MEM_ALLOC_FUNC(allocbytes);
#else
	ptr = ptrorg = (tjs_uint8*)GlobalAlloc(GMEM_FIXED, allocbytes);
#endif
	if(!ptr) exit(1);

	// align to a paragraph ( 16-bytes )
	ptr += 16 + sizeof(tTVPLayerBitmapMemoryRecord);
	*ptr >>= 4;
	*ptr <<= 4;

	record =
		(tTVPLayerBitmapMemoryRecord*)
		(ptr - sizeof(tTVPLayerBitmapMemoryRecord) - sizeof(tjs_uint32));

	// fill memory allocation record
	record->alloc_ptr = (void *)ptrorg;
	record->size = size;
	record->sentinel_backup1 = rand() + (rand() << 16);
	record->sentinel_backup2 = rand() + (rand() << 16);

	// set sentinel
	*(tjs_uint32*)(ptr - sizeof(tjs_uint32)) = ~record->sentinel_backup1;
	*(tjs_uint32*)(ptr + size              ) = ~record->sentinel_backup2;
		// Stored sentinels are nagated, to avoid that the sentinel backups in
		// tTVPLayerBitmapMemoryRecord becomes the same value as the sentinels.
		// This trick will make the detection of the memory corruption easier.
		// Because on some occasions, running memory writing will write the same
		// values at first sentinel and the tTVPLayerBitmapMemoryRecord.

	// return buffer pointer
	return ptr;
}

void tTVPBitmapAllocate(tTVPBitmap *bitmap, tjs_uint width, tjs_uint height, tjs_uint bpp)
{
	// allocate bitmap bits
	// bpp must be 8 or 32

	// create BITMAPINFO
	tjs_int i;
	tjs_uint bitmap_width;

	bitmap->BitmapInfoSize = sizeof(BITMAPINFOHEADER) +
			((bpp==8)?sizeof(RGBQUAD)*256 : 0);
#ifdef _WIN32
	bitmap->BitmapInfo = (BITMAPINFO*)
		GlobalAlloc(GPTR, bitmap->BitmapInfoSize);
#else
	bitmap->BitmapInfo = (BITMAPINFO*)
		MEM_ALLOC_FUNC(bitmap->BitmapInfoSize);
#endif
	bitmap->width = width;
	bitmap->height = height;

	bitmap_width = width;
		// note that the allocated bitmap size can be bigger than the
		// original size because the horizontal pitch of the bitmap
		// is aligned to a paragraph (16bytes)

	if(bpp == 8)
	{
		bitmap_width = (((bitmap_width-1) / 16)+1) *16; // align to a paragraph
		//bitmap->PitchBytes = (((bitmap_width-1) >> 2)+1) <<2;
		bitmap->PitchBytes = bitmap_width * 3;
	}
	else
	{
		bitmap_width = (((bitmap_width-1) / 4)+1) *4; // align to a paragraph
		bitmap->PitchBytes = bitmap_width * 4;
	}

	bitmap->PitchStep = -bitmap->PitchBytes;
	if(bpp == 8) bpp = 24;

	bitmap->BitmapInfo->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
	bitmap->BitmapInfo->bmiHeader.biWidth = bitmap_width;
	bitmap->BitmapInfo->bmiHeader.biHeight = height;
	bitmap->BitmapInfo->bmiHeader.biPlanes = 1;
	bitmap->BitmapInfo->bmiHeader.biBitCount = bpp;
	bitmap->BitmapInfo->bmiHeader.biCompression = BI_RGB;
	bitmap->BitmapInfo->bmiHeader.biSizeImage = bitmap->PitchBytes * height;
	bitmap->BitmapInfo->bmiHeader.biXPelsPerMeter = 0;
	bitmap->BitmapInfo->bmiHeader.biYPelsPerMeter = 0;
	bitmap->BitmapInfo->bmiHeader.biClrUsed = 0;
	bitmap->BitmapInfo->bmiHeader.biClrImportant = 0;

	// create grayscale palette
	if(bpp == 8)
	{
		RGBQUAD *pal = (RGBQUAD*)((tjs_uint8*)bitmap->BitmapInfo + sizeof(BITMAPINFOHEADER));

		for(i=0; i<256; i++)
		{
			pal[i].rgbBlue = pal[i].rgbGreen = pal[i].rgbRed = (BYTE)i;
			pal[i].rgbReserved = 0;
		}
	}

	// allocate bitmap bits
	bitmap->Bits = TVPAllocBitmapBits(bitmap->BitmapInfo->bmiHeader.biSizeImage,
								width, height);
}

BOOL tTVPBaseBitmapIsindependent(tTVPBaseBitmap *bitmap)
{
	return (bitmap->bitmap->RefCount == 1);
}

void tTVPBaseBitmapIndepend(tTVPBaseBitmap *bitmap)
{
	tTVPBitmap *newb;
	// sever Bitmap's image sharing
	if(bitmap->Isindependent(bitmap)) return;
	newb = (tTVPBitmap*)MEM_ALLOC_FUNC(sizeof(tTVPBitmap));
	bitmap->bitmap->Release(bitmap->bitmap);
	bitmap->bitmap = newb;
	bitmap->FontChanged = TRUE; // informs internal font information is invalidated
}

void tTVPBaseBitmapRecreate(tTVPBaseBitmap *bitmap, tjs_uint w, tjs_uint h, tjs_uint bpp)
{
	bitmap->bitmap->Release(bitmap->bitmap);
	bitmap->bitmap = (tTVPBitmap*)MEM_ALLOC_FUNC(sizeof(tTVPBitmap));
	bitmap->Allocate(bitmap->bitmap, w, h, bpp);
}

void* tTVPBitmapGetScanLine(tTVPBitmap *bitmap, tjs_uint l)
{
	if((tjs_int)l>=bitmap->BitmapInfo->bmiHeader.biHeight)
	{
		exit(1);
	}

	return (bitmap->BitmapInfo->bmiHeader.biHeight - l -1 ) * bitmap->PitchBytes + (tjs_uint8*)bitmap->Bits;
}

void* tTVPBitmapGetScanLineForWrite(tTVPBaseBitmap *bitmap, tjs_uint l)
{
	bitmap->Independ(bitmap);
	return bitmap->GetScanLine(bitmap->bitmap, l);
}

void Create_tTVPBaseBitmap(tTVPBaseBitmap *bitmap)
{
	if(!bitmap)
	{
		bitmap = (tTVPBaseBitmap*)MEM_ALLOC_FUNC(sizeof(tTVPBaseBitmap));
	}

	bitmap->Recreate = tTVPBaseBitmapRecreate;
	bitmap->Allocate = tTVPBitmapAllocate;
	bitmap->Independ = tTVPBaseBitmapIndepend;
	bitmap->Isindependent = tTVPBaseBitmapIsindependent;
	bitmap->GetScanLineForWrite = tTVPBitmapGetScanLineForWrite;
	bitmap->GetScanLine = tTVPBitmapGetScanLine;
	bitmap->bitmap->Release = tTVPBaseBitmapRelease;
	bitmap->bitmap->RefCount = 1;
}

void* TJSAlignedAlloc(tjs_uint bytes, tjs_uint align_bits)
{
	tjs_int align = 1 << align_bits;

	return MEM_ALLOC_FUNC(sizeof(tjs_uint8)+(bytes+align+sizeof(void*)));
}

tjs_int (*TVPTLG5DecompressSlide)(tjs_uint8 *out, const tjs_uint8 *in, tjs_int insize, tjs_uint8 *text, tjs_int initialr);
void (*TVPTLG5ComposeColors3To4)(tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const * buf, tjs_int width);
void (*TVPTLG5ComposeColors4To4)(tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const* buf, tjs_int width);
void (*TVPMakeAlphaFromKey)(tjs_uint32 *dest, tjs_int len, tjs_uint32 key);
void (*TVPFillARGB)(tjs_uint32 *dest, tjs_int len, tjs_uint32 value);
void (*TVPTLG6DecodeGolombValuesForFirst)(tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool);
void (*TVPTLG6DecodeGolombValues)(tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool);
void (*TVPTLG6DecodeLine)(tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int block_count, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *in, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir);
extern tjs_uint32 TVPCPUType;

#ifdef _WIN32
# define TVP_GL_IA32_FUNC_EXTERN_DECL(rettype, funcname, arg)  extern rettype __cdecl funcname arg
#else
# define TVP_GL_IA32_FUNC_EXTERN_DECL(rettype, funcname, arg)  extern rettype funcname arg
#endif

TVP_GL_IA32_FUNC_EXTERN_DECL(tjs_int,  TVPTLG5DecompressSlide_a,  (tjs_uint8 *out, const tjs_uint8 *in, tjs_int insize, tjs_uint8 *text, tjs_int initialr));
TVP_GL_IA32_FUNC_EXTERN_DECL(void,  TVPTLG5ComposeColors3To4_mmx_a,  (tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const * buf, tjs_int width));
TVP_GL_IA32_FUNC_EXTERN_DECL(void,  TVPTLG5ComposeColors4To4_mmx_a,  (tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const * buf, tjs_int width));

TVP_GL_IA32_FUNC_EXTERN_DECL(void,  TVPTLG6DecodeLine_mmx_a,  (tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int block_count, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *input, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir));
TVP_GL_IA32_FUNC_EXTERN_DECL(void,  TVPTLG6DecodeLine_sse_a,  (tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int block_count, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *input, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir));

TVP_GL_IA32_FUNC_EXTERN_DECL(void,  TVPTLG6DecodeGolombValuesForFirst_a,  (tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool));
TVP_GL_IA32_FUNC_EXTERN_DECL(void,  TVPTLG6DecodeGolombValues_a,  (tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool));
TVP_GL_IA32_FUNC_EXTERN_DECL(void,  TVPTLG6DecodeGolombValuesForFirst_mmx_a,  (tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool));
TVP_GL_IA32_FUNC_EXTERN_DECL(void,  TVPTLG6DecodeGolombValues_mmx_a,  (tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool));
TVP_GL_IA32_FUNC_EXTERN_DECL(void,  TVPTLG6DecodeGolombValuesForFirst_emmx_a,  (tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool));
TVP_GL_IA32_FUNC_EXTERN_DECL(void,  TVPTLG6DecodeGolombValues_emmx_a,  (tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool));

TVP_GL_IA32_FUNC_EXTERN_DECL(void,  TVPFillARGB_sse_a, (tjs_uint32 *dest, tjs_int len, tjs_uint32 value));
TVP_GL_IA32_FUNC_EXTERN_DECL(void,  TVPFillARGB_mmx_a, (tjs_uint32 *dest, tjs_int len, tjs_uint32 value));
TVP_GL_IA32_FUNC_EXTERN_DECL(void,  TVPMakeAlphaFromKey_cmovcc_a, (tjs_uint32 *dest, tjs_int len, tjs_uint32 key));

tjs_int TVPTLG5DecompressSlide_c(tjs_uint8 *out, const tjs_uint8 *in, tjs_int insize, tjs_uint8 *text, tjs_int initialr)
{
	tjs_int r = initialr;
	tjs_uint flags = 0;
	const tjs_uint8 *inlim = in + insize;
	while(in < inlim)
	{
		if(((flags >>= 1) & 256) == 0)
		{
			flags = 0[in++] | 0xff00;
		}
		if(flags & 1)
		{
			tjs_int mpos = in[0] | ((in[1] & 0xf) << 8);
			tjs_int mlen = (in[1] & 0xf0) >> 4;
			in += 2;
			mlen += 3;
			if(mlen == 18) mlen += 0[in++];

			while(mlen--)
			{
				0[out++] = text[r++] = text[mpos++];
				mpos &= (4096 - 1);
				r &= (4096 - 1);
			}
		}
		else
		{
			uint8 c = 0[in++];
			0[out++] = c;
			text[r++] = c;
/*			0[out++] = text[r++] = 0[in++];*/
			r &= (4096 - 1);
		}
	}
	return r;
}

void TVPTLG5ComposeColors3To4_c(tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const * buf, tjs_int width)
{
	tjs_int x;
	tjs_uint8 pc[3];
	tjs_uint8 c[3];
	pc[0] = pc[1] = pc[2] = 0;
	for(x = 0; x < width; x++)
	{
		c[0] = buf[0][x];
		c[1] = buf[1][x];
		c[2] = buf[2][x];
		c[0] += c[1]; c[2] += c[1];
		*(tjs_uint32 *)outp =
								((((pc[0] += c[0]) + upper[0]) & 0xff)      ) +
								((((pc[1] += c[1]) + upper[1]) & 0xff) <<  8) +
								((((pc[2] += c[2]) + upper[2]) & 0xff) << 16) +
								0xff000000;
		outp += 4;
		upper += 4;
	}
}

void TVPTLG5ComposeColors4To4_c(tjs_uint8 *outp, const tjs_uint8 *upper, tjs_uint8 * const* buf, tjs_int width)
{
	tjs_int x;
	tjs_uint8 pc[4];
	tjs_uint8 c[4];
	pc[0] = pc[1] = pc[2] = pc[3] = 0;
	for(x = 0; x < width; x++)
	{
		c[0] = buf[0][x];
		c[1] = buf[1][x];
		c[2] = buf[2][x];
		c[3] = buf[3][x];
		c[0] += c[1]; c[2] += c[1];
		*(tjs_uint32 *)outp =
								((((pc[0] += c[0]) + upper[0]) & 0xff)      ) +
								((((pc[1] += c[1]) + upper[1]) & 0xff) <<  8) +
								((((pc[2] += c[2]) + upper[2]) & 0xff) << 16) +
								((((pc[3] += c[3]) + upper[3]) & 0xff) << 24);
		outp += 4;
		upper += 4;
	}
}

void TVPBindMaskToMain(tjs_uint32 *main, const tjs_uint8 *mask, tjs_int len)
{
	{
		int ___index = 0;
		len -= (8-1);

		while(___index < len)
		{
{
	main[(___index+0)] = (main[(___index+0)] & 0xffffff) + (mask[(___index+0)] << 24);
}
{
	main[(___index+1)] = (main[(___index+1)] & 0xffffff) + (mask[(___index+1)] << 24);
}
{
	main[(___index+2)] = (main[(___index+2)] & 0xffffff) + (mask[(___index+2)] << 24);
}
{
	main[(___index+3)] = (main[(___index+3)] & 0xffffff) + (mask[(___index+3)] << 24);
}
{
	main[(___index+4)] = (main[(___index+4)] & 0xffffff) + (mask[(___index+4)] << 24);
}
{
	main[(___index+5)] = (main[(___index+5)] & 0xffffff) + (mask[(___index+5)] << 24);
}
{
	main[(___index+6)] = (main[(___index+6)] & 0xffffff) + (mask[(___index+6)] << 24);
}
{
	main[(___index+7)] = (main[(___index+7)] & 0xffffff) + (mask[(___index+7)] << 24);
}
			___index += 8;
		}

		len += (8-1);

		while(___index < len)
		{
{
	main[___index] = (main[___index] & 0xffffff) + (mask[___index] << 24);
}
			___index ++;
		}
	}
}

void TVPMakeAlphaFromKey_c(tjs_uint32 *dest, tjs_int len, tjs_uint32 key)
{
	tjs_uint32 a, b;
	{
		int ___index = 0;
		len -= (8-1);

		while(___index < len)
		{
	a = dest[(___index+(0*2))] & 0xffffff;
	b = dest[(___index+(0*2+1))] & 0xffffff;
	if(a != key) a |= 0xff000000;
	if(b != key) b |= 0xff000000;
	dest[(___index+(0*2))] = a;
	dest[(___index+(0*2+1))] = b;
	a = dest[(___index+(1*2))] & 0xffffff;
	b = dest[(___index+(1*2+1))] & 0xffffff;
	if(a != key) a |= 0xff000000;
	if(b != key) b |= 0xff000000;
	dest[(___index+(1*2))] = a;
	dest[(___index+(1*2+1))] = b;
	a = dest[(___index+(2*2))] & 0xffffff;
	b = dest[(___index+(2*2+1))] & 0xffffff;
	if(a != key) a |= 0xff000000;
	if(b != key) b |= 0xff000000;
	dest[(___index+(2*2))] = a;
	dest[(___index+(2*2+1))] = b;
	a = dest[(___index+(3*2))] & 0xffffff;
	b = dest[(___index+(3*2+1))] & 0xffffff;
	if(a != key) a |= 0xff000000;
	if(b != key) b |= 0xff000000;
	dest[(___index+(3*2))] = a;
	dest[(___index+(3*2+1))] = b;
			___index += 8;
		}

		len += (8-1);

		while(___index < len)
		{
	a = dest[___index] & 0xffffff;;
	if(a != key) a |= 0xff000000;;
	dest[___index] = a;;
			___index ++;
		}
	}
}


static tjs_uint8 TVPTLG6LeadingZeroTable[TVP_TLG6_LeadingZeroTable_SIZE];

static short TVPTLG6GolombCompressed[TVP_TLG6_GOLOMB_N_COUNT][9] = {
		{3,7,15,27,63,108,223,448,130,},
		{3,5,13,24,51,95,192,384,257,},
		{2,5,12,21,39,86,155,320,384,},
		{2,3,9,18,33,61,129,258,511,},
	/* Tuned by W.Dee, 2004/03/25 */
};

char TVPTLG6GolombBitLengthTable
	[TVP_TLG6_GOLOMB_N_COUNT*2*128][TVP_TLG6_GOLOMB_N_COUNT] =
	{ { 0 } };

void TVPFillARGB_c(tjs_uint32 *dest, tjs_int len, tjs_uint32 value)
{
	/* non-cached version of TVPFillARGB */
	/* this routine written in C has no difference from TVPFillARGB. */ 
	{
		int ___index = 0;
		len -= (8-1);

		while(___index < len)
		{
{
	dest[(___index+0)] = value;
}
{
	dest[(___index+1)] = value;
}
{
	dest[(___index+2)] = value;
}
{
	dest[(___index+3)] = value;
}
{
	dest[(___index+4)] = value;
}
{
	dest[(___index+5)] = value;
}
{
	dest[(___index+6)] = value;
}
{
	dest[(___index+7)] = value;
}
			___index += 8;
		}

		len += (8-1);

		while(___index < len)
		{
{
	dest[___index] = value;
}
			___index ++;
		}
	}
}

void TVPTLG6InitLeadingZeroTable(void)
{
	/* table which indicates first set bit position + 1. */
	/* this may be replaced by BSF (IA32 instrcution). */

	int i;
	for(i = 0; i < TVP_TLG6_LeadingZeroTable_SIZE; i++)
	{
		int cnt = 0;
		int j;
		for(j = 1; j != TVP_TLG6_LeadingZeroTable_SIZE && !(i & j);
			j <<= 1, cnt++);
		cnt ++;
		if(j == TVP_TLG6_LeadingZeroTable_SIZE) cnt = 0;
		TVPTLG6LeadingZeroTable[i] = cnt;
	}
}

void TVPTLG6InitGolombTable(void)
{
	int n, i, j;
	for(n = 0; n < TVP_TLG6_GOLOMB_N_COUNT; n++)
	{
		int a = 0;
		for(i = 0; i < 9; i++)
		{
			for(j = 0; j < TVPTLG6GolombCompressed[n][i]; j++)
				TVPTLG6GolombBitLengthTable[a++][n] = (char)i;
		}
		if(a != TVP_TLG6_GOLOMB_N_COUNT*2*128)
			*(char*)0 = 0;   /* THIS MUST NOT BE EXECUETED! */
				/* (this is for compressed table data check) */
	}
}

#define TLG6_AVG_PACKED(x, y) ((((x) & (y)) + ((((x) ^ (y)) & 0xfefefefe) >> 1)) +\
			(((x)^(y))&0x01010101))

static tjs_uint32 packed_bytes_add(tjs_uint32 a, tjs_uint32 b)
{
	tjs_uint32 tmp = (((a & b)<<1) + ((a ^ b) & 0xfefefefe) ) & 0x01010100;
	return a+b-tmp;
}

static tjs_uint32 avg(tjs_uint32 a, tjs_uint32 b, tjs_uint32 c, tjs_uint32 v){
	return packed_bytes_add(TLG6_AVG_PACKED(a, b), v);
}

static tjs_uint32 make_gt_mask(tjs_uint32 a, tjs_uint32 b){
	tjs_uint32 tmp2 = ~b;
	tjs_uint32 tmp = ((a & tmp2) + (((a ^ tmp2) >> 1) & 0x7f7f7f7f) ) & 0x80808080;
	tmp = ((tmp >> 7) + 0x7f7f7f7f) ^ 0x7f7f7f7f;
	return tmp;
}

static tjs_uint32 med2(tjs_uint32 a, tjs_uint32 b, tjs_uint32 c){
	/* do Median Edge Detector   thx, Mr. sugi  at    kirikiri.info */
	tjs_uint32 aa_gt_bb = make_gt_mask(a, b);
	tjs_uint32 a_xor_b_and_aa_gt_bb = ((a ^ b) & aa_gt_bb);
	tjs_uint32 aa = a_xor_b_and_aa_gt_bb ^ a;
	tjs_uint32 bb = a_xor_b_and_aa_gt_bb ^ b;
	tjs_uint32 n = make_gt_mask(c, bb);
	tjs_uint32 nn = make_gt_mask(aa, c);
	tjs_uint32 m = ~(n | nn);
	return (n & aa) | (nn & bb) | ((bb & m) - (c & m) + (aa & m));
}

static tjs_uint32 med(tjs_uint32 a, tjs_uint32 b, tjs_uint32 c, tjs_uint32 v){
	return packed_bytes_add(med2(a, b, c), v);
}

void TVPTLG6DecodeGolombValuesForFirst_c(tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool)
{
	/*
		decode values packed in "bit_pool".
		values are coded using golomb code.

		"ForFirst" function do dword access to pixelbuf,
		clearing with zero except for blue (least siginificant byte).
	*/

	int n = TVP_TLG6_GOLOMB_N_COUNT - 1; /* output counter */
	int a = 0; /* summary of absolute values of errors */

	tjs_int bit_pos = 1;
	tjs_uint8 zero = (*bit_pool & 1)?0:1;

	tjs_int8 * limit = pixelbuf + pixel_count*4;

	while(pixelbuf < limit)
	{
		/* get running count */
		int count;

		{
			tjs_uint32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
			tjs_int b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
			int bit_count = b;
			while(!b)
			{
				bit_count += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;
				t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
				bit_count += b;
			}


			bit_pos += b;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;

			bit_count --;
			count = 1 << bit_count;
			count += ((TVP_TLG6_FETCH_32BITS(bit_pool) >> (bit_pos)) & (count-1));

			bit_pos += bit_count;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;

		}

		if(zero)
		{
			/* zero values */

			/* fill distination with zero */
			do { *(tjs_uint32*)pixelbuf = 0; pixelbuf+=4;} while(--count);

			zero ^= 1;
		}
		else
		{
			/* non-zero values */

			/* fill distination with glomb code */

			do
			{
				int k = TVPTLG6GolombBitLengthTable[a][n], v, sign;

				tjs_uint32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				tjs_int bit_count;
				tjs_int b;
				if(t)
				{
					b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
					bit_count = b;
					while(!b)
					{
						bit_count += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pool += bit_pos >> 3;
						bit_pos &= 7;
						t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
						b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
						bit_count += b;
					}
					bit_count --;
				}
				else
				{
					bit_pool += 5;
					bit_count = bit_pool[-1];
					bit_pos = 0;
					t = TVP_TLG6_FETCH_32BITS(bit_pool);
					b = 0;
				}


				v = (bit_count << k) + ((t >> b) & ((1<<k)-1));
				sign = (v & 1) - 1;
				v >>= 1;
				a += v;

				*(tjs_uint32*)pixelbuf = (uint8) ((v ^ sign) + sign + 1);
				pixelbuf += 4;

				bit_pos += b;
				bit_pos += k;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;

				if (--n < 0) {
					a >>= 1;  n = TVP_TLG6_GOLOMB_N_COUNT - 1;
				}
			} while(--count);
			zero ^= 1;
		}
	}
}

void TVPTLG6DecodeGolombValues_c(tjs_int8 *pixelbuf, tjs_int pixel_count, tjs_uint8 *bit_pool)
{
	/*
		decode values packed in "bit_pool".
		values are coded using golomb code.
	*/

	int n = TVP_TLG6_GOLOMB_N_COUNT - 1; /* output counter */
	int a = 0; /* summary of absolute values of errors */

	tjs_int bit_pos = 1;
	tjs_uint8 zero = (*bit_pool & 1)?0:1;

	tjs_int8 * limit = pixelbuf + pixel_count*4;

	while(pixelbuf < limit)
	{
		/* get running count */
		int count;

		{
			tjs_uint32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
			tjs_int b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
			int bit_count = b;
			while(!b)
			{
				bit_count += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;
				t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
				bit_count += b;
			}


			bit_pos += b;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;

			bit_count --;
			count = 1 << bit_count;
			count += ((TVP_TLG6_FETCH_32BITS(bit_pool) >> (bit_pos)) & (count-1));

			bit_pos += bit_count;
			bit_pool += bit_pos >> 3;
			bit_pos &= 7;

		}

		if(zero)
		{
			/* zero values */

			/* fill distination with zero */
			do { *pixelbuf = 0; pixelbuf+=4;} while(--count);

			zero ^= 1;
		}
		else
		{
			/* non-zero values */

			/* fill distination with glomb code */

			do
			{
				int k = TVPTLG6GolombBitLengthTable[a][n], v, sign;

				tjs_uint32 t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
				tjs_int bit_count;
				tjs_int b;
				if(t)
				{
					b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
					bit_count = b;
					while(!b)
					{
						bit_count += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pos += TVP_TLG6_LeadingZeroTable_BITS;
						bit_pool += bit_pos >> 3;
						bit_pos &= 7;
						t = TVP_TLG6_FETCH_32BITS(bit_pool) >> bit_pos;
						b = TVPTLG6LeadingZeroTable[t&(TVP_TLG6_LeadingZeroTable_SIZE-1)];
						bit_count += b;
					}
					bit_count --;
				}
				else
				{
					bit_pool += 5;
					bit_count = bit_pool[-1];
					bit_pos = 0;
					t = TVP_TLG6_FETCH_32BITS(bit_pool);
					b = 0;
				}


				v = (bit_count << k) + ((t >> b) & ((1<<k)-1));
				sign = (v & 1) - 1;
				v >>= 1;
				a += v;
				*pixelbuf = (char) ((v ^ sign) + sign + 1);
				pixelbuf += 4;

				bit_pos += b;
				bit_pos += k;
				bit_pool += bit_pos >> 3;
				bit_pos &= 7;

				if (--n < 0) {
					a >>= 1;  n = TVP_TLG6_GOLOMB_N_COUNT - 1;
				}
			} while(--count);
			zero ^= 1;
		}
	}
}

void TVPTLG6DecodeLineGeneric(tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int start_block, tjs_int block_limit, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *in, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir)
{
	/*
		chroma/luminosity decoding
		(this does reordering, color correlation filter, MED/AVG  at a time)
	*/
	tjs_uint32 p, up;
	int step, i;

	if(start_block)
	{
		prevline += start_block * TVP_TLG6_W_BLOCK_SIZE;
		curline  += start_block * TVP_TLG6_W_BLOCK_SIZE;
		p  = curline[-1];
		up = prevline[-1];
	}
	else
	{
		p = up = initialp;
	}

	in += skipblockbytes * start_block;
	step = (dir&1)?1:-1;

	for(i = start_block; i < block_limit; i ++)
	{
		int w = width - i*TVP_TLG6_W_BLOCK_SIZE, ww;
		if(w > TVP_TLG6_W_BLOCK_SIZE) w = TVP_TLG6_W_BLOCK_SIZE;
		ww = w;
		if(step==-1) in += ww-1;
		if(i&1) in += oddskip * ww;
		switch(filtertypes[i])
		{
#define IA	(char)((*in>>24)&0xff)
#define IR	(char)((*in>>16)&0xff)
#define IG  (char)((*in>>8 )&0xff)
#define IB  (char)((*in    )&0xff)
		TVP_TLG6_DO_CHROMA_DECODE( 0, IB, IG, IR); 
		TVP_TLG6_DO_CHROMA_DECODE( 1, IB+IG, IG, IR+IG); 
		TVP_TLG6_DO_CHROMA_DECODE( 2, IB, IG+IB, IR+IB+IG); 
		TVP_TLG6_DO_CHROMA_DECODE( 3, IB+IR+IG, IG+IR, IR); 
		TVP_TLG6_DO_CHROMA_DECODE( 4, IB+IR, IG+IB+IR, IR+IB+IR+IG); 
		TVP_TLG6_DO_CHROMA_DECODE( 5, IB+IR, IG+IB+IR, IR); 
		TVP_TLG6_DO_CHROMA_DECODE( 6, IB+IG, IG, IR); 
		TVP_TLG6_DO_CHROMA_DECODE( 7, IB, IG+IB, IR); 
		TVP_TLG6_DO_CHROMA_DECODE( 8, IB, IG, IR+IG); 
		TVP_TLG6_DO_CHROMA_DECODE( 9, IB+IG+IR+IB, IG+IR+IB, IR+IB); 
		TVP_TLG6_DO_CHROMA_DECODE(10, IB+IR, IG+IR, IR); 
		TVP_TLG6_DO_CHROMA_DECODE(11, IB, IG+IB, IR+IB); 
		TVP_TLG6_DO_CHROMA_DECODE(12, IB, IG+IR+IB, IR+IB); 
		TVP_TLG6_DO_CHROMA_DECODE(13, IB+IG, IG+IR+IB+IG, IR+IB+IG); 
		TVP_TLG6_DO_CHROMA_DECODE(14, IB+IG+IR, IG+IR, IR+IB+IG+IR); 
		TVP_TLG6_DO_CHROMA_DECODE(15, IB, IG+(IB<<1), IR+(IB<<1));

		default: return;
		}
		if(step == 1)
			in += skipblockbytes - ww;
		else
			in += skipblockbytes + 1;
		if(i&1) in -= oddskip * ww;
#undef IR
#undef IG
#undef IB
	}
}

void TVPTLG6DecodeLine_c(tjs_uint32 *prevline, tjs_uint32 *curline, tjs_int width, tjs_int block_count, tjs_uint8 *filtertypes, tjs_int skipblockbytes, tjs_uint32 *in, tjs_uint32 initialp, tjs_int oddskip, tjs_int dir)
{
	TVPTLG6DecodeLineGeneric(prevline, curline, width, 0, block_count,
		filtertypes, skipblockbytes, in, initialp, oddskip, dir);
}

/* CPUID の存在チェック */
extern int IsCPUID(tjs_uint32 dw_tag, tjs_uint32 cpuid[4]);

static int TVPGL_IA32_Inited = 0;
uint8 TVPOpacityOnOpacityTable[256*256];
uint8 TVPNegativeMulTable[256*256];

void TVPGL_IA32_Init()
{
	if(TVPGL_IA32_Inited)
	{
		return;
	}
	else
	{
		tjs_uint32 cpuid[4] = {0};
		TVPGL_IA32_Inited = 1;

#if defined(USE_SIMD_DECODE) && USE_SIMD_DECODE != 0
		if(IsCPUID(1, cpuid) == 0)
		{
			TVPTLG5DecompressSlide =  TVPTLG5DecompressSlide_c;
			TVPTLG5ComposeColors3To4 =  TVPTLG5ComposeColors3To4_c;
			TVPTLG5ComposeColors4To4 =  TVPTLG5ComposeColors4To4_c;
			TVPTLG6DecodeLine =  TVPTLG6DecodeLine_c;
			TVPMakeAlphaFromKey = TVPMakeAlphaFromKey_c;

			return;
		}

		// MMXサポートのチェック
		if((cpuid[3] & 0x00800000) != 0)
		{
			TVPTLG5DecompressSlide =  TVPTLG5DecompressSlide_a;
			TVPTLG5ComposeColors3To4 =  TVPTLG5ComposeColors3To4_mmx_a;
			TVPTLG5ComposeColors4To4 =  TVPTLG5ComposeColors4To4_mmx_a;
			TVPTLG6DecodeLine =  TVPTLG6DecodeLine_mmx_a;
			TVPMakeAlphaFromKey = TVPMakeAlphaFromKey_cmovcc_a;
		}
		else
		{
			TVPTLG5DecompressSlide =  TVPTLG5DecompressSlide_c;
			TVPTLG5ComposeColors3To4 =  TVPTLG5ComposeColors3To4_c;
			TVPTLG5ComposeColors4To4 =  TVPTLG5ComposeColors4To4_c;
			TVPTLG6DecodeLine =  TVPTLG6DecodeLine_c;
			TVPMakeAlphaFromKey = TVPMakeAlphaFromKey_c;
		}

		// MMXとSSEのサポートのチェック
		if((cpuid[3] & 0x00800000) != 0
			&& (cpuid[3] & 0x02000000) != 0)
		{
			TVPTLG6DecodeLine =  TVPTLG6DecodeLine_sse_a;
			TVPTLG6DecodeGolombValuesForFirst =  TVPTLG6DecodeGolombValuesForFirst_emmx_a;
			TVPTLG6DecodeGolombValues =  TVPTLG6DecodeGolombValues_emmx_a;
			TVPFillARGB = TVPFillARGB_sse_a;
		}
		else if((cpuid[3] & 0x00800000) != 0)
		{
			TVPTLG6DecodeGolombValuesForFirst =  TVPTLG6DecodeGolombValuesForFirst_mmx_a;
			TVPTLG6DecodeGolombValues =  TVPTLG6DecodeGolombValues_mmx_a;
			TVPTLG6DecodeLine = TVPTLG6DecodeLine_c;
			TVPFillARGB = TVPFillARGB_mmx_a;
		}
		else
		{
			TVPTLG6DecodeGolombValuesForFirst = TVPTLG6DecodeGolombValuesForFirst_c;
			TVPTLG6DecodeGolombValues = TVPTLG6DecodeGolombValues_c;
			TVPTLG6DecodeLine = TVPTLG6DecodeLine_c;
			TVPFillARGB = TVPFillARGB_c;
		}
#else
		TVPTLG5DecompressSlide =  TVPTLG5DecompressSlide_c;
		TVPTLG5ComposeColors3To4 =  TVPTLG5ComposeColors3To4_c;
		TVPTLG5ComposeColors4To4 =  TVPTLG5ComposeColors4To4_c;
		TVPTLG6DecodeLine =  TVPTLG6DecodeLine_c;
		TVPTLG6DecodeGolombValuesForFirst = TVPTLG6DecodeGolombValuesForFirst_c;
		TVPTLG6DecodeGolombValues = TVPTLG6DecodeGolombValues_c;
		TVPTLG6DecodeLine = TVPTLG6DecodeLine_c;
		TVPFillARGB = TVPFillARGB_c;
		TVPMakeAlphaFromKey = TVPMakeAlphaFromKey_c;
#endif
	}
}

void TVPLoadGraphic_SizeCallback(void *callbackdata, tjs_uint w,
	tjs_uint h)
{
	tTVPLoadGraphicData * data = (tTVPLoadGraphicData *)callbackdata;
	data->Dest = (tTVPBaseBitmap*)MEM_ALLOC_FUNC(sizeof(tTVPBaseBitmap));
	data->Dest->bitmap = (tTVPBitmap*)MEM_ALLOC_FUNC(sizeof(tTVPBitmap));
	Create_tTVPBaseBitmap(data->Dest);

	// check size
	data->OrgW = w;
	data->OrgH = h;
	//if(data->DesW && w < data->DesW) w = data->DesW;
	//if(data->DesH && h < data->DesH) h = data->DesH;
	data->BufW = w;
	data->BufH = h;

	if(!(data->Type == lgtMask || data->Type == lgtFullColor || data->Type == lgtPalGray))
	{
		data->Type = lgtFullColor;
	}

	// create buffer
	if(data->Type == lgtMask)
	{
		// mask ( _m ) load

		// check the image previously loaded
		//if(data->Dest->GetWidth() != w || data->Dest->GetHeight() != h)
		//	TVPThrowExceptionMessage(TVPMaskSizeMismatch);

		// allocate line buffer
		data->Buffer = (tjs_uint8*)MEM_ALLOC_FUNC(w);
	}
	else
	{
		// normal load or province load
		data->Dest->Recreate(data->Dest, w, h, data->Type!=lgtFullColor?8:32);
	}
}
//---------------------------------------------------------------------------

void * TVPLoadGraphic_ScanLineCallback(void *callbackdata, tjs_int y, tjs_uint color_key)
{
	tTVPLoadGraphicData * data = (tTVPLoadGraphicData *)callbackdata;
	tjs_uint i;

	if(!data->Dest->Isindependent(data->Dest))
	{
		data->Dest->bitmap->RefCount = 1;
	}

	if(y >= 0)
	{
		// query of line buffer

		data->ScanLineNum = y;
		if(data->Type == lgtMask)
		{
			// mask
			return data->Buffer;
		}
		else
		{
			// return the scanline for writing
			return data->Dest->GetScanLineForWrite(data->Dest, y);
		}
	}
	else
	{
		// y==-1 indicates the buffer previously returned was written

		if(data->Type == lgtMask)
		{
			// mask

			// tile for horizontal direction
			tjs_uint i;
			for(i = data->OrgW; i<data->BufW; i+=data->OrgW)
			{
				tjs_uint w = data->BufW - i;
				w = w > data->OrgW ? data->OrgW : w;
				memcpy(data->Buffer + i, data->Buffer, w);
			}

			// bind mask buffer to main image buffer ( and tile for vertical )
			for(i = data->ScanLineNum; i<data->BufH; i+=data->OrgH)
			{
				TVPBindMaskToMain(
					(tjs_uint32*)data->Dest->GetScanLineForWrite(data->Dest, i),
					data->Buffer, data->BufW);
			}
			return NULL;
		}
		else if(data->Type == lgtFullColor)
		{
			tjs_uint32 * sl =
				(tjs_uint32*)data->Dest->GetScanLineForWrite(data->Dest, data->ScanLineNum);
			//if((data->ColorKey & 0xff000000) == 0x00000000)
			if((color_key & 0xff000000) == 0x00000000)
			{
				// make alpha from color key
				TVPMakeAlphaFromKey(
					sl,
					data->BufW,
					color_key);
			}

			// tile for horizontal direction
			for(i = data->OrgW; i<data->BufW; i+=data->OrgW)
			{
				tjs_uint w = data->BufW - i;
				w = w > data->OrgW ? data->OrgW : w;
				memcpy(sl + i, sl, w * sizeof(tjs_uint32));
			}

			// tile for vertical direction
			for(i = data->ScanLineNum + data->OrgH; i<data->BufH; i+=data->OrgH)
			{
				memcpy(
					(tjs_uint32*)data->Dest->GetScanLineForWrite(data->Dest, i),
					sl,
					data->BufW * sizeof(tjs_uint32) );
			}

			return NULL;
		}
		else if(data->Type == lgtPalGray)
		{
			// nothing to do
			if(data->OrgW < data->BufW || data->OrgH < data->BufH)
			{
				tjs_uint8 * sl =
					(tjs_uint8*)data->Dest->GetScanLineForWrite(data->Dest, data->ScanLineNum);
				tjs_uint i;

				// tile for horizontal direction
				for(i = data->OrgW; i<data->BufW; i+=data->OrgW)
				{
					tjs_uint w = data->BufW - i;
					w = w > data->OrgW ? data->OrgW : w;
					memcpy(sl + i, sl, w * sizeof(tjs_uint8));
				}

				// tile for vertical direction
				for(i = data->ScanLineNum + data->OrgH; i<data->BufH; i+=data->OrgH)
				{
					memcpy(
						(tjs_uint8*)data->Dest->GetScanLineForWrite(data->Dest, i),
						sl,
						data->BufW * sizeof(tjs_uint8));
				}
			}

			return NULL;
		}
	}
	return NULL;
}

//---------------------------------------------------------------------------
// TLG5 loading handler
//---------------------------------------------------------------------------
int TVPLoadTLG5(void* formatdata, void *callbackdata,
	tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback,
	tTJSBinaryStream *src,
	tjs_uint keyidx,
	tTVPGraphicLoadMode mode)
{
	tjs_int i;
	tjs_uint8 *inbuf = NULL;
	tjs_uint8 *outbuf[4];
	tjs_uint8 *text = NULL;
	tjs_int r = 0;
	tjs_int c;
	tjs_int y_blk;
	tjs_int y_lim;
	int blockcount;
	tjs_int width, height, colors, blockheight;
	uint8 mark[12];
	tjs_uint8 * outbufp[4];
	tjs_uint8 *prevline;

	// load TLG v5.0 lossless compressed graphic
	if(mode != glmNormal)
	{
		return 1;
	}

	(void)src->read(&mark, 1, 1, src->pData);
	colors = mark[0];
	width = TJS_ReadI32LE(src);
	height = TJS_ReadI32LE(src);
	blockheight = TJS_ReadI32LE(src);

	if(colors != 3 && colors != 4)
	{
		return 2;
	}

	blockcount = (int)((height - 1) / blockheight) + 1;

	// skip block size section
	(void)src->seek(src->pData, src->tell(src->pData) + blockcount * sizeof(tjs_uint32), SEEK_SET);

	// decomperss
	sizecallback(callbackdata, width, height);

	for(i = 0; i < colors; i++)
	{
		outbuf[i] = NULL;
	}

	if(NULL == (text = (tjs_uint8*)TJSAlignedAlloc(4096, 4)))
	{
		return 3;
	}
	(void)memset(text, 0, 4096);

	if(NULL == (inbuf = (tjs_uint8*)TJSAlignedAlloc(blockheight * width + 10, 4)))
	{
		return 4;
	}
	for(i = 0; i < colors; i++)
	{
		if(NULL == (outbuf[i] = (tjs_uint8*)TJSAlignedAlloc(blockheight * width + 10, 4)))
		{
			return 5;
		}
	}

	prevline = NULL;
	for(y_blk = 0; y_blk < height; y_blk += blockheight)
	{
		// read file and decompress
		for(c = 0; c < colors; c++)
		{
			tjs_int size;
			(void)src->read(&mark, 1, 1, src->pData);
			size = TJS_ReadI32LE(src);
			if(mark[0] == 0)
			{
				// modified LZSS compressed data
				(void)src->read(inbuf, 1, size, src->pData);
				r = TVPTLG5DecompressSlide(outbuf[c], inbuf, size, text, r);
			}
			else
			{
				// raw data
				(void)src->read(outbuf[c], 1, size, src->pData);
			}
		}

		// compose colors and store
		y_lim = y_blk + blockheight;
		if(y_lim > height) y_lim = height;

		for(i = 0; i < colors; i++) outbufp[i] = outbuf[i];
		for(c = y_blk; c < y_lim; c++)
		{
			tjs_uint8 *current = (tjs_uint8*)scanlinecallback(callbackdata, c, keyidx);
			tjs_uint8 *current_org = current;
			tjs_int pg, pb, pa, x;
			if(prevline)
			{
				// not first line
				switch(colors)
				{
				case 3:
					TVPTLG5ComposeColors3To4(current, prevline, outbufp, width);
					outbufp[0] += width; outbufp[1] += width;
					outbufp[2] += width;
					break;
				case 4:
					TVPTLG5ComposeColors4To4(current, prevline, outbufp, width);
					outbufp[0] += width; outbufp[1] += width;
					outbufp[2] += width; outbufp[3] += width;
					break;
				}
			}
			else
			{
				// first line
				switch(colors)
				{
				case 3:
					for(i = 0, pg = 0, pb = 0, x = 0;
						x < width; x++)
					{
						tjs_int b = outbufp[0][x];
						tjs_int g = outbufp[1][x];
						tjs_int r = outbufp[2][x];
						b += g; r += g;
						0[current++] = pb += b;
						0[current++] = pg += g;
						0[current++] = i += r;
						0[current++] = 0xff;
					}
					outbufp[0] += width;
					outbufp[1] += width;
					outbufp[2] += width;
					break;
				case 4:
					for(i = 0, pg = 0, pb = 0, pa = 0, x = 0;
						x < width; x++)
					{
						tjs_int b = outbufp[0][x];
						tjs_int g = outbufp[1][x];
						tjs_int r = outbufp[2][x];
						tjs_int a = outbufp[3][x];
						b += g; r += g;
						0[current++] = pb += b;
						0[current++] = pg += g;
						0[current++] = i += r;
						0[current++] = pa += a;
					}
					outbufp[0] += width;
					outbufp[1] += width;
					outbufp[2] += width;
					outbufp[3] += width;
					break;
				}
			}
	
			scanlinecallback(callbackdata, -1, keyidx);
			prevline = current_org;
		}
	}
	
	MEM_FREE_FUNC(inbuf);
	MEM_FREE_FUNC(text);
	for(i = 0; i < colors; i++)
	{
		MEM_FREE_FUNC(outbuf[i]);
	}

	return 0;
}
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// TLG6 loading handler
//---------------------------------------------------------------------------
int TVPLoadTLG6(void* formatdata, void *callbackdata,
	tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback,
	tTJSBinaryStream *src,
	tjs_uint keyidx,
	BOOL palettized)
{
	uint8 buf[12];

	// compute some values
	tjs_int x_block_count;
	tjs_int y_block_count;
	tjs_int main_count;
	tjs_int fraction;
	tjs_int colors;
	tjs_int width, height;
	tjs_int max_bit_length;

	tjs_uint32 i, j;
	tjs_uint32 *prevline;

	// prepare memory pointers
	tjs_uint8 *bit_pool = NULL;
	tjs_uint32 *pixelbuf = NULL; // pixel buffer
	tjs_uint8 *filter_types = NULL;
	tjs_uint8 *LZSS_text = NULL;
	tjs_uint32 *zeroline = NULL;

	static int initialized = 0;

	// load TLG v6.0 lossless/near-lossless compressed graphic
	if(palettized)
	{
		exit(1);
	}

	if(initialized == 0)
	{
		TVPTLG6InitLeadingZeroTable();
		TVPTLG6InitGolombTable();
		initialized++;
	}

	(void)src->read(buf, 1, 4, src->pData);

	colors = buf[0]; // color component count

	if(colors != 1 && colors != 4 && colors != 3)
	{
		return 1;
	}

	if(buf[1] != 0) // data flag
	{
		return 2;
	}

	if(buf[2] != 0) // color type  (currently always zero)
	{
		return 3;
	}

	if(buf[3] != 0) // external golomb table (currently always zero)
	{
		return 4;
	}

	width = TJS_ReadI32LE(src);
	height = TJS_ReadI32LE(src);

	max_bit_length = TJS_ReadI32LE(src);

	// set destination size
	sizecallback(callbackdata, width, height);

	// compute some values
	x_block_count = (tjs_int)((width - 1)/ TVP_TLG6_W_BLOCK_SIZE) + 1;
	y_block_count = (tjs_int)((height - 1)/ TVP_TLG6_H_BLOCK_SIZE) + 1;
	main_count = width / TVP_TLG6_W_BLOCK_SIZE;
	fraction = width -  main_count * TVP_TLG6_W_BLOCK_SIZE;

	// allocate memories
	if(NULL == (bit_pool = (tjs_uint8*)TJSAlignedAlloc(max_bit_length / 8 + 5, 4)))
	{
		return 5;
	}

	if(NULL == (pixelbuf = (tjs_uint32*)TJSAlignedAlloc(sizeof(tjs_uint32) * width * TVP_TLG6_H_BLOCK_SIZE + 1, 4))) exit(1);
	if(NULL == (filter_types = (tjs_uint8*)TJSAlignedAlloc(x_block_count * y_block_count, 4))) exit(1);
	if(NULL == (zeroline = (tjs_uint32*)TJSAlignedAlloc(width * sizeof(tjs_uint32), 4))) exit(1);
	if(NULL == (LZSS_text = (tjs_uint8*)TJSAlignedAlloc(4096, 4))) exit(1);

	// initialize zero line (virtual y=-1 line)
	TVPFillARGB(zeroline, width, colors==3?0xff000000:0x00000000);
		// 0xff000000 for colors=3 makes alpha value opaque

	// initialize LZSS text (used by chroma filter type codes)
	{
		tjs_uint32 *p = (tjs_uint32*)LZSS_text;
		for(i = 0; i < 32*0x01010101; i+=0x01010101)
		{
			for(j = 0; j < 16*0x01010101; j+=0x01010101)
				p[0] = i, p[1] = j, p += 2;
		}
	}

	// read chroma filter types.
	// chroma filter types are compressed via LZSS as used by TLG5.
	{
		tjs_int inbuf_size = TJS_ReadI32LE(src);
		tjs_uint8* inbuf;
		if(NULL == (inbuf = (tjs_uint8*)TJSAlignedAlloc(inbuf_size, 4))) exit(1);

		(void)src->read(inbuf, 1, inbuf_size, src->pData);
		TVPTLG5DecompressSlide(filter_types, inbuf, inbuf_size,
					LZSS_text, 0);

		MEM_FREE_FUNC(inbuf);
		inbuf = NULL;
	}

	// for each horizontal block group ...
	prevline = zeroline;
	for(j = 0; j < (tjs_uint32)height; j += TVP_TLG6_H_BLOCK_SIZE)
	{
		tjs_int ylim = j + TVP_TLG6_H_BLOCK_SIZE;
		uint8 * ft;
		tjs_int pixel_count;
		int skipbytes;
		if(ylim >= height) ylim = height;

		pixel_count = (ylim - j) * width;

		// decode values
		for(i = 0; i < (tjs_uint32)colors; i++)
		{
			// read bit length
			tjs_int bit_length = TJS_ReadI32LE(src);
			tjs_int byte_length;

			// get compress method
			int method = (bit_length >> 30)&3;
			bit_length &= 0x3fffffff;

			// compute byte length
			byte_length = bit_length / 8;
			if(bit_length % 8) byte_length++;

			// read source from input
			(void)src->read(bit_pool, 1, byte_length, src->pData);

			// decode values
			// two most significant bits of bitlength are
			// entropy coding method;
			// 00 means Golomb method,
			// 01 means Gamma method (not yet suppoted),
			// 10 means modified LZSS method (not yet supported),
			// 11 means raw (uncompressed) data (not yet supported).

			switch(method)
			{
			case 0:
				if(i == 0 && colors != 1)
					TVPTLG6DecodeGolombValuesForFirst((tjs_int8*)pixelbuf,
						pixel_count, bit_pool);
				else
					TVPTLG6DecodeGolombValues((tjs_int8*)pixelbuf + i,
						pixel_count, bit_pool);
				break;
			default:
				return 1;
			}
		}

		// for each line
		ft =
			filter_types + (j / TVP_TLG6_H_BLOCK_SIZE)*x_block_count;
		skipbytes = (ylim-j)*TVP_TLG6_W_BLOCK_SIZE;

		for(i = j; i < (tjs_uint32)ylim; i++)
		{
			tjs_uint32* curline = (tjs_uint32*)scanlinecallback(callbackdata, i, keyidx);

			int dir = (i&1)^1;
			int oddskip = ((ylim - i -1) - (i-j));
			if(main_count)
			{
				int start =
					((width < TVP_TLG6_W_BLOCK_SIZE) ? width : TVP_TLG6_W_BLOCK_SIZE) *
						(i - j);
				TVPTLG6DecodeLine(
					prevline,
					curline,
					width,
					main_count,
					ft,
					skipbytes,
					pixelbuf + start, colors==3?0xff000000:0, oddskip, dir);
			}

			if(main_count != x_block_count)
			{
				int ww = fraction;
				int start;
				if(ww > TVP_TLG6_W_BLOCK_SIZE) ww = TVP_TLG6_W_BLOCK_SIZE;
				start = ww * (i - j);
				TVPTLG6DecodeLineGeneric(
					prevline,
					curline,
					width,
					main_count,
					x_block_count,
					ft,
					skipbytes,
					pixelbuf + start, colors==3?0xff000000:0, oddskip, dir);
			}

			scanlinecallback(callbackdata, -1, keyidx);
			prevline = curline;
		}

	}

	MEM_FREE_FUNC(bit_pool);
	MEM_FREE_FUNC(pixelbuf);
	MEM_FREE_FUNC(filter_types);
	MEM_FREE_FUNC(zeroline);
	MEM_FREE_FUNC(LZSS_text);
	/*
	if(bit_pool) TJSAlignedDealloc(bit_pool);
	if(pixelbuf) TJSAlignedDealloc(pixelbuf);
	if(filter_types) TJSAlignedDealloc(filter_types);
	if(zeroline) TJSAlignedDealloc(zeroline);
	if(LZSS_text) TJSAlignedDealloc(LZSS_text);
	*/

	return 0;
}

//---------------------------------------------------------------------------
// TLG loading handler
//---------------------------------------------------------------------------
static int TVPInternalLoadTLG(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_uint keyidx,  tTVPGraphicLoadMode mode)
{
	// read header
	uint8 mark[12];
	(void)src->read(mark, 1, 11, src->pData);

	// check for TLG raw data
	if(!memcmp("TLG5.0\x00raw\x1a\x00", mark, 11))
	{
		TVPLoadTLG5(formatdata, callbackdata, sizecallback,
			scanlinecallback, src, keyidx, mode);
	}
	else if(!memcmp("TLG6.0\x00raw\x1a\x00", mark, 11))
	{
		TVPLoadTLG6(formatdata, callbackdata, sizecallback,
			scanlinecallback, src, keyidx, mode);
	}
	else
	{
		return 1;
	}

	return 0;
}
//---------------------------------------------------------------------------

int TVPLoadTLG(void* formatdata, void *callbackdata, tTVPGraphicSizeCallback sizecallback,
	tTVPGraphicScanLineCallback scanlinecallback, tTVPMetaInfoPushCallback metainfopushcallback,
	tTJSBinaryStream *src, tjs_uint keyidx,  tTVPGraphicLoadMode mode)
{
	// read header
	uint8 mark[12];
	src->read(mark, 1, 11, src->pData);

	TVPGL_IA32_Init();

	// check for TLG0.0 sds
	if(!memcmp("TLG0.0\x00sds\x1a\x00", mark, 11))
	{
		// read TLG0.0 Structured Data Stream

		// TLG0.0 SDS tagged data is simple "NAME=VALUE," string;
		// Each NAME and VALUE have length:content expression.
		// eg: 4:LEFT=2:20,3:TOP=3:120,4:TYPE=1:3,
		// The last ',' cannot be ommited.
		// Each string (name and value) must be encoded in utf-8.

		// read raw data size
		tjs_uint rawlen = TJS_ReadI32LE(src);

		// try to load TLG raw data
		TVPInternalLoadTLG(formatdata, callbackdata, sizecallback,
			scanlinecallback, metainfopushcallback, src, keyidx, mode);

		// seek to meta info data point
		src->seek(src->pData, rawlen + 11 + 4, SEEK_SET);

		// read tag data
		while(1)
		{
			char chunkname[4];
			tjs_uint chunksize;
			if(4 != src->read(chunkname, 1, 4, src->pData))
			{
				break;
			}	// cannot read more

			chunksize = TJS_ReadI32LE(src);
			if(!memcmp(chunkname, "tags", 4))
			{
				// tag information
				char *tag = NULL;
				char *name = NULL;
				char *value = NULL;

				tag = (char*)MEM_ALLOC_FUNC(chunksize+1);
				(void)src->read(tag, 1, chunksize, src->pData);
				tag[chunksize] = 0;
				if(metainfopushcallback)
				{
					const char *tagp = tag;
					const char *tagp_lim = tag + chunksize;
					while(tagp < tagp_lim)
					{
						tjs_uint namelen = 0;
						tjs_uint valuelen = 0;
						while(*tagp >= '0' && *tagp <= '9')
							namelen = namelen * 10 + *tagp - '0', tagp++;
						if(*tagp != ':')
						{
							return 2;
						}
						tagp ++;
						name = (char*)MEM_ALLOC_FUNC(namelen + 1);
						memcpy(name, tagp, namelen);
						name[namelen] = '\0';
						tagp += namelen;
						if(*tagp != '=')
						{
							return 3;
						}
						tagp++;
						
						while(*tagp >= '0' && *tagp <= '9')
							valuelen = valuelen * 10 + *tagp - '0', tagp++;
						if(*tagp != ':')
						{
							return 4;
						}
						tagp++;
						value = (char*)MEM_ALLOC_FUNC(valuelen+1);
						memcpy(value, tagp, valuelen);
						value[valuelen] = '\0';
						tagp += valuelen;
						if(*tagp != ',')
						{
							return 5;
						}
						tagp++;

						// insert into name-value pairs ... TODO: utf-8 decode
						metainfopushcallback(callbackdata, name, value);

						MEM_FREE_FUNC(name);	name = NULL;
						MEM_FREE_FUNC(value);	value = NULL;
					}
				}

				MEM_FREE_FUNC(tag);
				tag = NULL;
				MEM_FREE_FUNC(name);
				name = NULL;
				MEM_FREE_FUNC(value);
				value = NULL;
				break;
			}
			else
			{
				// skip the chunk
				src->seek(src->pData, src->tell(src->pData) + chunksize, SEEK_SET);
			}
		} // while
	}
	else
	{
		src->seek(src->pData, 0, SEEK_SET);	// rewind

		// try to load TLG raw data
		TVPInternalLoadTLG(formatdata, callbackdata, sizecallback,
			scanlinecallback, metainfopushcallback, src, keyidx, mode);
	}

	return 0;
}

unsigned char* ReadTlgStream(
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	long (*tell_func)(void*),
	int* width,
	int* height,
	int* channel
)
{
	tTJSBinaryStream stream;
	tTVPLoadGraphicData *image;
	uint8 *call_back = (uint8*)MEM_ALLOC_FUNC(sizeof(tTVPLoadGraphicData));
	uint8 *bits;
	uint8 *ret;

	stream.pData = src;
	stream.read = read_func;
	stream.seek = seek_func;
	stream.tell = tell_func;

	if(TVPLoadTLG(NULL, call_back, TVPLoadGraphic_SizeCallback,
		TVPLoadGraphic_ScanLineCallback, NULL, &stream, 0xff000000, glmNormal) != 0)
	{
		return NULL;
	}

	image = (tTVPLoadGraphicData*)call_back;
	bits = (uint8*)image->Dest->bitmap->Bits;

	if(width != NULL)
	{
		*width = image->Dest->bitmap->width;
	}
	if(height != NULL)
	{
		*height = image->Dest->bitmap->height;
	}
	if(channel != NULL)
	{
		*channel = image->Dest->bitmap->PitchBytes / image->Dest->bitmap->width;
		//*channel = image->Dest->base_bitmap->bitmap->PitchBytes / image->Dest->bitmap->width;
	}

	ret = MEM_ALLOC_FUNC(image->Dest->bitmap->BitmapInfo->bmiHeader.biSizeImage);
	if(ret == NULL)
	{
		return NULL;
	}
	(void)memcpy(ret, bits, image->Dest->bitmap->BitmapInfo->bmiHeader.biSizeImage);

	MEM_FREE_FUNC(bits - 16 - sizeof(tTVPLayerBitmapMemoryRecord));
	MEM_FREE_FUNC(image->Dest->bitmap);
	MEM_FREE_FUNC(image->Dest);
	MEM_FREE_FUNC(call_back);

	return ret;
}

#ifdef __cplusplus
}
#endif
