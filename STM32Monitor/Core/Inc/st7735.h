#ifndef ST7735_H
#define ST7735_H

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"
#include <stdint.h>

/*
 * Adafruit #3533
 * 0.96" 160x80 ST7735 TFT
 *
 * This driver targets the revised/plugin-FPC version of the display.
 *
 * STM32:
 *   NUCLEO-U083RC
 *
 * SPI:
 *   SPI1
 *
 * TFT:
 *   CS  = PB0
 *   DC  = PB1
 *   RST = PB2
 */

/* ------------------------------------------------------------------------- */
/* Display dimensions                                                        */
/* ------------------------------------------------------------------------- */

#define ST7735_WIDTH       160
#define ST7735_HEIGHT       80


/* ------------------------------------------------------------------------- */
/* ST7735 commands                                                           */
/* ------------------------------------------------------------------------- */

#define ST7735_NOP          0x00
#define ST7735_SWRESET      0x01
#define ST7735_RDDID        0x04
#define ST7735_RDDST        0x09

#define ST7735_SLPIN        0x10
#define ST7735_SLPOUT       0x11
#define ST7735_PTLON        0x12
#define ST7735_NORON        0x13

#define ST7735_INVOFF       0x20
#define ST7735_INVON        0x21
#define ST7735_GAMSET       0x26

#define ST7735_DISPOFF      0x28
#define ST7735_DISPON       0x29

#define ST7735_CASET        0x2A
#define ST7735_RASET        0x2B
#define ST7735_RAMWR        0x2C
#define ST7735_RAMRD        0x2E

#define ST7735_PTLAR        0x30
#define ST7735_VSCRDEF      0x33
#define ST7735_MADCTL       0x36
#define ST7735_VSCRSADD     0x37
#define ST7735_COLMOD       0x3A

#define ST7735_FRMCTR1      0xB1
#define ST7735_FRMCTR2      0xB2
#define ST7735_FRMCTR3      0xB3
#define ST7735_INVCTR       0xB4
#define ST7735_DISSET5      0xB6

#define ST7735_PWCTR1       0xC0
#define ST7735_PWCTR2       0xC1
#define ST7735_PWCTR3       0xC2
#define ST7735_PWCTR4       0xC3
#define ST7735_PWCTR5       0xC4
#define ST7735_VMCTR1       0xC5
#define ST7735_PWCTR6       0xFC

#define ST7735_GMCTRP1      0xE0
#define ST7735_GMCTRN1      0xE1


/* ------------------------------------------------------------------------- */
/* MADCTL bits                                                               */
/* ------------------------------------------------------------------------- */

#define ST7735_MADCTL_MY    0x80
#define ST7735_MADCTL_MX    0x40
#define ST7735_MADCTL_MV    0x20
#define ST7735_MADCTL_ML    0x10
#define ST7735_MADCTL_RGB   0x00
#define ST7735_MADCTL_BGR   0x08
#define ST7735_MADCTL_MH    0x04


/* ------------------------------------------------------------------------- */
/* RGB565 colors                                                             */
/* ------------------------------------------------------------------------- */

#define ST7735_BLACK        0x0000
#define ST7735_BLUE         0x001F
#define ST7735_RED          0xF800
#define ST7735_GREEN        0x07E0
#define ST7735_CYAN         0x07FF
#define ST7735_MAGENTA      0xF81F
#define ST7735_YELLOW       0xFFE0
#define ST7735_WHITE        0xFFFF
#define ST7735_ORANGE       0xFD20
#define ST7735_GREENYELLOW  0xAFE5
#define ST7735_PINK         0xF81F


/* ------------------------------------------------------------------------- */
/* Public functions                                                          */
/* ------------------------------------------------------------------------- */

void ST7735_Init(void);

void ST7735_Reset(void);

void ST7735_SetRotation(uint8_t rotation);

void ST7735_SetAddressWindow(uint16_t x0,
                             uint16_t y0,
                             uint16_t x1,
                             uint16_t y1);

void ST7735_Fill(uint16_t color);

void ST7735_DrawPixel(uint16_t x,
                      uint16_t y,
                      uint16_t color);

void ST7735_WritePixels(const uint8_t *data,
                        uint16_t size);

void ST7735_WriteCommand(uint8_t command);

void ST7735_WriteData(const uint8_t *data,
                      uint16_t size);

void ST7735_DisplayOn(void);

void ST7735_DisplayOff(void);

void ST7735_InvertDisplay(uint8_t invert);

#ifdef __cplusplus
}
#endif

#endif /* ST7735_H */
