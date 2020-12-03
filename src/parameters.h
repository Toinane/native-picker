#pragma once

#define __PRETTY_FUNCTION__ __FUNCSIG__

/*
        /+/       <---------------------------
         +                          | GRID_NUMUBER_L*2 + 1
         +      /- GRID_NUMUBER_L   |
         +     /                    |
         +<---v-->|                 |
/+/+/+/+/#/+/+/+/+/                 |
         +                          |
         +                          |
         +                          |
         +                          |
        /+/       <---------------------------
^_________________^
|                 |
|  GRID_NUMUBER_L*2 + 1 |
*/

const int GRID_PIXEL = 9;
const int GRID_NUMUBER_L = 8;

const int GRID_NUMUBER = GRID_NUMUBER_L*2 + 1;

const int CAPTURE_WIDTH  = GRID_NUMUBER;
const int CAPTURE_HEIGHT = GRID_NUMUBER;

#if defined(OS_MACOS)

const int UI_WINDOW_SIZE = 16 + // <- window shadow
                           GRID_PIXEL + 2 + // center pixel
                           ((GRID_PIXEL + 1)*GRID_NUMUBER_L)*2;

#elif defined(OS_WINDOWS)

const int UI_WINDOW_SIZE = 0 + // <- without window shadow
                           GRID_PIXEL + 2 + // center pixel
                           ((GRID_PIXEL + 1)*GRID_NUMUBER_L)*2;

#endif // defined(OS_WINDOWS)

const uint32_t CURSOR_REFRESH_FREQUENCY = 144;
// const uint32_t CURSOR_REFRESH_FREQUENCY = 20;
const uint32_t SCREEN_CAPTURE_FREQUENCY_TO_CURSOR_REFRESH_RATIO = 1; // 60 hz
// const uint32_t SCREEN_CAPTURE_FREQUENCY_TO_CURSOR_REFRESH_RATIO = 3; // 20 hz
// const uint32_t SCREEN_CAPTURE_FREQUENCY_TO_CURSOR_REFRESH_RATIO = 30; // 2 hz



