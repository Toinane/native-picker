#include "services.h"

MMSignedPoint MMSignedPointMake(int32_t x, int32_t y) {
	MMSignedPoint point;
	point.x = x;
	point.y = y;
	return point;
}

MMSignedSize MMSignedSizeMake(int32_t width, int32_t height) {
	MMSignedSize size;
	size.width = width;
	size.height = height;
	return size;
}

MMSignedRect MMSignedRectMake(int32_t x, int32_t y, int32_t width, int32_t height) {
	MMSignedRect rect;
	rect.origin = MMSignedPointMake(x, y);
	rect.size = MMSignedSizeMake(width, height);
	return rect;
}

MMRGBHex hexFromMMRGB(MMRGBColor rgb) {
	return RGB_TO_HEX(rgb.red, rgb.green, rgb.blue);
}

/**
 * Pad hex color code with leading zeros.
 * @param color Hex value to pad.
 * @param hex   Hex value to output.
 */
void padHex(MMRGBHex color, char* hex) {
	//Length needs to be 7 because snprintf includes a terminating null.
	//Use %06x to pad hex value with leading 0s.
	snprintf(hex, 7, "%06x", color);
}