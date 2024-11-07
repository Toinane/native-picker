#ifndef TYPES_H
#define TYPES_H

#include <napi.h>

#if !defined(IS_MACOS) && defined(__APPLE__) && defined(__MACH__)
	#define IS_MACOS
#endif
#if !defined(IS_WINDOWS) && (defined(WIN32) || defined(_WIN32) || \
                             defined(__WIN32__) || defined(__WINDOWS__))
	#define IS_WINDOWS
#endif
#if !defined(USE_X11) && !defined(NUSE_X11) && !defined(IS_MACOSX) && !defined(IS_WINDOWS)
	#define USE_X11
#endif

struct _MMSignedPoint {
	int32_t x;
	int32_t y;
};

typedef struct _MMSignedPoint MMSignedPoint;

#endif