#ifndef GFX_H
#define GFX_H

#ifdef __cplusplus
extern "C" {
#endif

#include "st7735.h"
#include <stdint.h>

/*
 * Text/graphics layer for the ST7735.
 *
 * Coordinates:
 *   (0,0) = top-left
 *
 * Text font:
 *   5x7 pixels
 *
 * Character spacing:
 *   1 pixel
 *
 * Text size:
 *   1 = 5x7
 *   2 = 10x14
 *   3 = 15x21
 *   etc.
 */

void GFX_Init(void);

void GFX_SetCursor(int16_t x, int16_t y);

void GFX_SetTextColor(uint16_t color);

void GFX_SetTextBackground(uint16_t color);

void GFX_SetTextSize(uint8_t size);

void GFX_SetTextWrap(uint8_t wrap);

void GFX_WriteChar(char c);

void GFX_Print(const char *str);

void GFX_FillRect(int16_t x,
                  int16_t y,
                  int16_t w,
                  int16_t h,
                  uint16_t color);

void GFX_DrawRect(int16_t x,
                  int16_t y,
                  int16_t w,
                  int16_t h,
                  uint16_t color);

void GFX_DrawFastHLine(int16_t x,
                       int16_t y,
                       int16_t w,
                       uint16_t color);

void GFX_DrawFastVLine(int16_t x,
                       int16_t y,
                       int16_t h,
                       uint16_t color);

#ifdef __cplusplus
}
#endif

#endif /* GFX_H */
