#pragma once
#ifndef TYPES_H
#define TYPES_H

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

#include <napi.h>
#include <assert.h>
#include <stdlib.h>

#if defined(IS_MACOSX)
  #include <ApplicationServices/ApplicationServices.h>
#elif defined(USE_X11)
  #include <X11/Xlib.h>
  #include <X11/extensions/XTest.h>
  #include <stdlib.h>
  #include "xdisplay.h"
#elif defined(IS_WINDOWS)
  #include <windows.h>
#endif

struct MMSignedPoint {
	int32_t x;
	int32_t y;
};

typedef uint32_t MMRGBHex;

struct MMSignedSize {
	int32_t width;
	int32_t height;
};


struct MMRGBColor {
	uint8_t blue;
	uint8_t green;
	uint8_t red;
};

struct MMBitmap {
	uint8_t *imageBuffer;  /* Pixels stored in Quad I format; i.e., origin is in top left. Length should be height * bytewidth. */
	int32_t width;          /* Never 0, unless image is NULL. */
	int32_t height;         /* Never 0, unless image is NULL. */
	int32_t bytewidth;      /* The aligned width (width + padding). */
	uint8_t bitsPerPixel;  /* Should be either 24 or 32. */
	uint8_t bytesPerPixel; /* For convenience; should be bitsPerPixel / 8. */
};

typedef MMBitmap *MMBitmapRef;

struct MMSignedRect {
	MMSignedPoint origin;
	MMSignedSize size;
};

#define RGB_TO_HEX(red, green, blue) (((red) << 16) | ((green) << 8) | (blue))

#define MMBitmapPointInBounds(image, p) ((p).x < (image)->width && \
                                         (p).y < (image)->height)
#define MMBitmapRectInBounds(image, r)                    \
	(((r).origin.x + (r).size.width <= (image)->width) && \
	 ((r).origin.y + (r).size.height <= (image)->height))

#define MMBitmapGetBounds(image) MMSignedRectMake(0, 0, image->width, image->height)

/* Get pointer to pixel of MMBitmapRef. No bounds checking is performed (check
 * yourself before calling this with MMBitmapPointInBounds(). */
#define MMRGBColorRefAtPoint(image, x, y) \
	(MMRGBColor *)(assert(MMBitmapPointInBounds(bitmap, MMSignedPointMake(x, y))), \
	               ((image)->imageBuffer) + (((image)->bytewidth * (y)) \
	                                      + ((x) * (image)->bytesPerPixel)))

/* Dereference pixel of MMBitmapRef. Again, no bounds checking is performed. */
#define MMRGBColorAtPoint(image, x, y) *MMRGBColorRefAtPoint(image, x, y)

/* Hex/integer value of color at point. */
#define MMRGBHexAtPoint(image, x, y) \
	hexFromMMRGB(MMRGBColorAtPoint(image, x, y))

#endif