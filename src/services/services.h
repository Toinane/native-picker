#pragma once
#ifndef SERVICES_H
#define SERVICES_H

#include "../types.h"

MMSignedPoint MMSignedPointMake(int32_t x, int32_t y);
MMSignedSize MMSignedSizeMake(int32_t width, int32_t height);
MMSignedRect MMSignedRectMake(int32_t x, int32_t y, int32_t width, int32_t height);
MMRGBHex hexFromMMRGB(MMRGBColor rgb);
void padHex(MMRGBHex color, char* hex);

MMSignedPoint GetMousePos();
char* GetHexPixelColor(MMSignedPoint pos);

#endif