#ifndef _LOAD_IMAGE_H_
#define _LOAD_IMAGE_H_

#include "types.h"

extern uint8* LoadImage(const char* utf8_path, int* width, int* height, int* channel);

#endif	// #ifndef _LOAD_IMAGE_H_