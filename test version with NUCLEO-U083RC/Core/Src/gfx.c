#include "gfx.h"
#include "font5x7.h"
#include <stddef.h>


/* ------------------------------------------------------------------------- */
/* Text state                                                                */
/* ------------------------------------------------------------------------- */

static int16_t cursor_x = 0;
static int16_t cursor_y = 0;

static uint16_t text_color = ST7735_WHITE;
static uint16_t text_bg    = ST7735_BLACK;

static uint8_t text_size = 1;
static uint8_t text_wrap = 1;


/* ------------------------------------------------------------------------- */
/* Display dimensions                                                        */
/* ------------------------------------------------------------------------- */

/*
 * Our current display is 160x80.
 *
 * Since ST7735_SetRotation() maintains its dimensions internally,
 * the graphics layer uses these values for clipping.
 */
#define GFX_WIDTH  160
#define GFX_HEIGHT  80


/* ------------------------------------------------------------------------- */
/* Initialization                                                            */
/* ------------------------------------------------------------------------- */

void GFX_Init(void)
{
    cursor_x = 0;
    cursor_y = 0;

    text_color = ST7735_WHITE;
    text_bg    = ST7735_BLACK;

    text_size = 1;
    text_wrap = 1;
}


/* ------------------------------------------------------------------------- */
/* Cursor                                                                     */
/* ------------------------------------------------------------------------- */

void GFX_SetCursor(int16_t x, int16_t y)
{
    cursor_x = x;
    cursor_y = y;
}


/* ------------------------------------------------------------------------- */
/* Text color                                                                 */
/* ------------------------------------------------------------------------- */

void GFX_SetTextColor(uint16_t color)
{
    text_color = color;
}


/* ------------------------------------------------------------------------- */
/* Text background                                                            */
/* ------------------------------------------------------------------------- */

void GFX_SetTextBackground(uint16_t color)
{
    text_bg = color;
}


/* ------------------------------------------------------------------------- */
/* Text size                                                                  */
/* ------------------------------------------------------------------------- */

void GFX_SetTextSize(uint8_t size)
{
    if (size == 0)
    {
        size = 1;
    }

    text_size = size;
}


/* ------------------------------------------------------------------------- */
/* Text wrapping                                                              */
/* ------------------------------------------------------------------------- */

void GFX_SetTextWrap(uint8_t wrap)
{
    text_wrap = wrap ? 1 : 0;
}


/* ------------------------------------------------------------------------- */
/* Filled rectangle                                                           */
/* ------------------------------------------------------------------------- */

void GFX_FillRect(int16_t x,
                  int16_t y,
                  int16_t w,
                  int16_t h,
                  uint16_t color)
{
    /*
     * Completely outside display?
     */
    if (w <= 0 || h <= 0)
    {
        return;
    }

    if (x >= GFX_WIDTH || y >= GFX_HEIGHT)
    {
        return;
    }

    if ((x + w) <= 0 || (y + h) <= 0)
    {
        return;
    }


    /*
     * Clip left.
     */
    if (x < 0)
    {
        w += x;
        x = 0;
    }


    /*
     * Clip top.
     */
    if (y < 0)
    {
        h += y;
        y = 0;
    }


    /*
     * Clip right.
     */
    if ((x + w) > GFX_WIDTH)
    {
        w = GFX_WIDTH - x;
    }


    /*
     * Clip bottom.
     */
    if ((y + h) > GFX_HEIGHT)
    {
        h = GFX_HEIGHT - y;
    }


    if (w <= 0 || h <= 0)
    {
        return;
    }


    /*
     * Set display address window.
     */
    ST7735_SetAddressWindow((uint16_t)x,
                            (uint16_t)y,
                            (uint16_t)(x + w - 1),
                            (uint16_t)(y + h - 1));


    /*
     * Build a reasonably sized RGB565 transfer buffer.
     *
     * 128 pixels = 256 bytes.
     */
    uint8_t buffer[256];

    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)(color & 0xFF);

    for (uint16_t i = 0; i < sizeof(buffer); i += 2)
    {
        buffer[i]     = hi;
        buffer[i + 1] = lo;
    }


    uint32_t pixels =
        (uint32_t)w * (uint32_t)h;

    uint32_t sent = 0;


    /*
     * We need direct access to the SPI transfer.
     *
     * ST7735_WriteData() would toggle CS for every buffer.
     *
     * For rectangles, keep the display selected throughout.
     */
    extern SPI_HandleTypeDef hspi1;


    HAL_GPIO_WritePin(TFT_CS_GPIO_Port,
                      TFT_CS_Pin,
                      GPIO_PIN_RESET);

    HAL_GPIO_WritePin(TFT_DC_GPIO_Port,
                      TFT_DC_Pin,
                      GPIO_PIN_SET);


    while (sent < pixels)
    {
        uint32_t remaining = pixels - sent;

        uint16_t count;

        if (remaining > 128)
        {
            count = 128;
        }
        else
        {
            count = (uint16_t)remaining;
        }


        HAL_SPI_Transmit(&hspi1,
                         buffer,
                         count * 2,
                         HAL_MAX_DELAY);

        sent += count;
    }


    HAL_GPIO_WritePin(TFT_CS_GPIO_Port,
                      TFT_CS_Pin,
                      GPIO_PIN_SET);
}


/* ------------------------------------------------------------------------- */
/* Rectangle                                                                  */
/* ------------------------------------------------------------------------- */

void GFX_DrawRect(int16_t x,
                  int16_t y,
                  int16_t w,
                  int16_t h,
                  uint16_t color)
{
    if (w <= 0 || h <= 0)
    {
        return;
    }

    GFX_DrawFastHLine(x, y, w, color);

    GFX_DrawFastHLine(x, y + h - 1, w, color);

    GFX_DrawFastVLine(x, y, h, color);

    GFX_DrawFastVLine(x + w - 1, y, h, color);
}


/* ------------------------------------------------------------------------- */
/* Horizontal line                                                            */
/* ------------------------------------------------------------------------- */

void GFX_DrawFastHLine(int16_t x,
                       int16_t y,
                       int16_t w,
                       uint16_t color)
{
    GFX_FillRect(x, y, w, 1, color);
}


/* ------------------------------------------------------------------------- */
/* Vertical line                                                              */
/* ------------------------------------------------------------------------- */

void GFX_DrawFastVLine(int16_t x,
                       int16_t y,
                       int16_t h,
                       uint16_t color)
{
    GFX_FillRect(x, y, 1, h, color);
}


/* ------------------------------------------------------------------------- */
/* Write character - optimized                                               */
/* ------------------------------------------------------------------------- */

void GFX_WriteChar(char c)
{
    /*
     * Newline.
     */
    if (c == '\n')
    {
        cursor_x = 0;
        cursor_y += 8 * text_size;
        return;
    }


    /*
     * Carriage return.
     */
    if (c == '\r')
    {
        cursor_x = 0;
        return;
    }


    /*
     * Printable ASCII only.
     */
    if (c < 32 || c > 126)
    {
        c = '?';
    }


    /*
     * Character dimensions including the one-pixel spacing column.
     */
    int16_t char_width  = 6 * text_size;
    int16_t char_height = 8 * text_size;


    /*
     * Automatic wrapping.
     */
    if (text_wrap &&
        (cursor_x + char_width > GFX_WIDTH))
    {
        cursor_x = 0;
        cursor_y += char_height;
    }


    /*
     * Completely below the display.
     */
    if (cursor_y >= GFX_HEIGHT)
    {
        return;
    }


    /*
     * Completely above the display.
     */
    if (cursor_y + 7 * text_size <= 0)
    {
        cursor_x += char_width;
        return;
    }


    const uint8_t *bitmap =
        Font5x7_GetChar((uint8_t)c);


    /*
     * Maximum character size supported by this local buffer:
     *
     *     6 x 7 x 2 x 4^2 = 672 bytes
     *
     * So size 1..4 is safe.
     */
    if (text_size > 4)
    {
        text_size = 4;
    }


    /*
     * A 5x7 character plus one spacing column.
     *
     * For example, at size 2:
     *
     *       12 pixels wide
     *       14 pixels high
     *
     * The buffer contains RGB565 pixels in display order.
     */
    uint16_t pixel_width =
        6 * text_size;

    uint16_t pixel_height =
        7 * text_size;


    uint8_t buffer[672];

    uint32_t index = 0;


    /*
     * Generate the complete character bitmap.
     */
    for (uint16_t y = 0; y < pixel_height; y++)
    {
        uint16_t source_y =
            y / text_size;


        for (uint16_t x = 0; x < pixel_width; x++)
        {
            uint16_t source_x =
                x / text_size;


            uint8_t pixel_on = 0;


            /*
             * The sixth column is the character spacing.
             */
            if (source_x < 5)
            {
                uint8_t column =
                    bitmap[source_x];

                pixel_on =
                    (column >> source_y) & 0x01;
            }


            uint16_t color;

            if (pixel_on)
            {
                color = text_color;
            }
            else
            {
                color = text_bg;
            }


            /*
             * RGB565 MSB first.
             */
            buffer[index++] =
                (uint8_t)(color >> 8);

            buffer[index++] =
                (uint8_t)(color & 0xFF);
        }
    }


    /*
     * Clip if necessary.
     *
     * For normal use, characters are entirely on-screen.
     *
     * We handle the common case directly and fall back to the
     * pixel renderer when a character crosses an edge.
     */
    if ((cursor_x >= 0) &&
        (cursor_y >= 0) &&
        (cursor_x + pixel_width <= GFX_WIDTH) &&
        (cursor_y + pixel_height <= GFX_HEIGHT))
    {
        /*
         * Entire character is visible.
         */
        ST7735_SetAddressWindow(
            (uint16_t)cursor_x,
            (uint16_t)cursor_y,
            (uint16_t)(cursor_x + pixel_width - 1),
            (uint16_t)(cursor_y + pixel_height - 1));


        ST7735_WritePixels(buffer,
                           (uint16_t)index);
    }
    else
    {
        /*
         * Edge case.
         *
         * Render individual pixels only when clipping is required.
         */
        for (uint16_t y = 0; y < pixel_height; y++)
        {
            for (uint16_t x = 0; x < pixel_width; x++)
            {
                int16_t px = cursor_x + x;
                int16_t py = cursor_y + y;

                if (px < 0 || px >= GFX_WIDTH ||
                    py < 0 || py >= GFX_HEIGHT)
                {
                    continue;
                }


                uint16_t source_x =
                    x / text_size;

                uint16_t source_y =
                    y / text_size;


                uint16_t color;


                if (source_x < 5)
                {
                    uint8_t column =
                        bitmap[source_x];

                    if ((column >> source_y) & 0x01)
                    {
                        color = text_color;
                    }
                    else
                    {
                        color = text_bg;
                    }
                }
                else
                {
                    color = text_bg;
                }


                ST7735_DrawPixel(
                    (uint16_t)px,
                    (uint16_t)py,
                    color);
            }
        }
    }


    /*
     * Advance cursor.
     */
    cursor_x += char_width;
}

/* ------------------------------------------------------------------------- */
/* Print string                                                               */
/* ------------------------------------------------------------------------- */

void GFX_Print(const char *str)
{
    if (str == NULL)
    {
        return;
    }


    while (*str)
    {
        GFX_WriteChar(*str);

        str++;
    }
}
