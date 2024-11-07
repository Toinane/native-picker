#include "services.h"

MMBitmapRef createMMBitmap(uint8_t *buffer,
                           int32_t width,
                           int32_t height,
                           int32_t bytewidth,
                           uint8_t bitsPerPixel,
                           uint8_t bytesPerPixel) {
	MMBitmapRef bitmap = (MMBitmapRef)malloc(sizeof(MMBitmap));
	if (bitmap == NULL) return NULL;

	bitmap->imageBuffer = buffer;
	bitmap->width = width;
	bitmap->height = height;
	bitmap->bytewidth = bytewidth;
	bitmap->bitsPerPixel = bitsPerPixel;
	bitmap->bytesPerPixel = bytesPerPixel;

	return bitmap;
}

void destroyMMBitmap(MMBitmapRef bitmap)
{
	assert(bitmap != NULL);

	if (bitmap->imageBuffer != NULL) {
		free(bitmap->imageBuffer);
		bitmap->imageBuffer = NULL;
	}

	free(bitmap);
}

MMBitmapRef copyMMBitmapFromDisplayInRect(MMSignedRect rect) {
#if defined(IS_MACOSX)
	MMBitmapRef bitmap = NULL;
	uint8_t *buffer = NULL;
	int32_t bufferSize = 0;

	CGDirectDisplayID displayID = CGMainDisplayID();

	CGImageRef image = CGDisplayCreateImageForRect(displayID,
		CGRectMake(rect.origin.x,
			rect.origin.y,
			rect.size.width,
			rect.size.height));

	if (!image) { return NULL; }

	CFDataRef imageData = CGDataProviderCopyData(CGImageGetDataProvider(image));

	if (!imageData) { return NULL; }

	bufferSize = CFDataGetLength(imageData);
	buffer = malloc(bufferSize);

	CFDataGetBytes(imageData, CFRangeMake(0,bufferSize), buffer);

	bitmap = createMMBitmap(buffer,
		CGImageGetWidth(image),
		CGImageGetHeight(image),
		CGImageGetBytesPerRow(image),
		CGImageGetBitsPerPixel(image),
		CGImageGetBitsPerPixel(image) / 8);

	CFRelease(imageData);

	CGImageRelease(image);

	return bitmap;

#elif defined(USE_X11)
	MMBitmapRef bitmap;

	Display *display = XOpenDisplay(NULL);
	XImage *image = XGetImage(display,
	                          XDefaultRootWindow(display),
	                          (int)rect.origin.x,
	                          (int)rect.origin.y,
	                          (unsigned int)rect.size.width,
	                          (unsigned int)rect.size.height,
	                          AllPlanes, ZPixmap);
	XCloseDisplay(display);
	if (image == NULL) return NULL;

	bitmap = createMMBitmap((uint8_t *)image->data,
	                        rect.size.width,
	                        rect.size.height,
	                        (int32_t)image->bytes_per_line,
	                        (uint8_t)image->bits_per_pixel,
	                        (uint8_t)image->bits_per_pixel / 8);
	image->data = NULL; /* Steal ownership of bitmap data so we don't have to
	                     * copy it. */
	XDestroyImage(image);

	return bitmap;
#elif defined(IS_WINDOWS)
	MMBitmapRef bitmap;
	void *data;
	HDC screen = NULL, screenMem = NULL;
	HBITMAP dib;
	BITMAPINFO bi;

	/* Initialize bitmap info. */
	bi.bmiHeader.biSize = sizeof(bi.bmiHeader);
  bi.bmiHeader.biWidth = (long)rect.size.width;
  bi.bmiHeader.biHeight = -(long)rect.size.height; /* Non-cartesian, please */
  bi.bmiHeader.biPlanes = 1;
  bi.bmiHeader.biBitCount = 32;
  bi.bmiHeader.biCompression = BI_RGB;
  bi.bmiHeader.biSizeImage = (DWORD)(4 * rect.size.width * rect.size.height);
	bi.bmiHeader.biXPelsPerMeter = 0;
	bi.bmiHeader.biYPelsPerMeter = 0;
	bi.bmiHeader.biClrUsed = 0;
	bi.bmiHeader.biClrImportant = 0;

	screen = GetDC(NULL); /* Get entire screen */
	if (screen == NULL) return NULL;

	/* Get screen data in display device context. */
  dib = CreateDIBSection(screen, &bi, DIB_RGB_COLORS, &data, NULL, 0);

	/* Copy the data into a bitmap struct. */
	if ((screenMem = CreateCompatibleDC(screen)) == NULL ||
	    SelectObject(screenMem, dib) == NULL ||
	    !BitBlt(screenMem,
	            (int)0,
	            (int)0,
	            (int)rect.size.width,
	            (int)rect.size.height,
				screen,
				rect.origin.x,
				rect.origin.y,
				SRCCOPY)) {
		
		/* Error copying data. */
		ReleaseDC(NULL, screen);
		DeleteObject(dib);
		if (screenMem != NULL) DeleteDC(screenMem);

		return NULL;
	}

	bitmap = createMMBitmap(NULL,
	                        rect.size.width,
	                        rect.size.height,
	                        4 * rect.size.width,
	                        (uint8_t)bi.bmiHeader.biBitCount,
	                        4);

	/* Copy the data to our pixel buffer. */
	if (bitmap != NULL) {
		bitmap->imageBuffer = (uint8_t *)malloc(bitmap->bytewidth * bitmap->height);
		memcpy(bitmap->imageBuffer, data, bitmap->bytewidth * bitmap->height);
	}

	ReleaseDC(NULL, screen);
	DeleteObject(dib);
	DeleteDC(screenMem);

	return bitmap;
#endif
}

char* GetHexPixelColor(MMSignedPoint pos) {
  MMBitmapRef bitmap = copyMMBitmapFromDisplayInRect(MMSignedRectMake(pos.x, pos.y, 1, 1));
	MMRGBHex color = MMRGBHexAtPoint(bitmap, 0, 0);
	char hex[7];
	padHex(color, hex);
	destroyMMBitmap(bitmap);

  return hex;
}