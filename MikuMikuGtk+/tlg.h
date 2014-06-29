#ifndef _INCLUDED_TLG_H_
#define _INCLUDED_TLG_H_

#include <stdio.h>

// #define USE_SIMD_DECODE 1

#ifdef __cplusplus
extern "C" {
#endif

unsigned char* ReadTlgStream(
	void* src,
	size_t (*read_func)(void*, size_t, size_t, void*),
	int (*seek_func)(void*, long, int),
	long (*tell_func)(void*),
	int* width,
	int* height,
	int* channel
);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _INCLUDED_TLG_H_
