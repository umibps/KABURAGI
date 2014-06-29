#ifndef _LOAD_IMAGE_H_
#define _LOAD_IMAGE_H_

#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern uint8* LoadImage(const char* utf8_path, int* width, int* height, int* channel);

#ifdef __cplusplus
}
#endif

#endif	// #ifndef _LOAD_IMAGE_H_