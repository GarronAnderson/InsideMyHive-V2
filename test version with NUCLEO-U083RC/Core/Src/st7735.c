#include "st7735.h"

/*
 * ============================================================================
 * STM32 HAL ST7735 DRIVER
 * ============================================================================
 *
 * Target:
 *   Adafruit #3533
 *   0.96" 160x80 ST7735 TFT
 *
 * This is the revised/plugin-FPC initialization variant corresponding to:
 *
 *   INITR_MINI160x80_PLUGIN
 *
 * Display:
 *   160 x 80 pixels
 *
 * STM32:
 *   NUCLEO-U083RC
 *
 * SPI:
 *   SPI1
 *
 * GPIO:
 *   CS  = PB0
 *   DC  = PB1
 *   RST = PB2
 *
 * SPI mode:
 *   Mode 0
 *
 * Color:
 *   RGB565
 *
 * ============================================================================
 */


/* CubeMX-generated SPI handle */
extern SPI_HandleTypeDef hspi1;


/* ------------------------------------------------------------------------- */
/* Current display state                                                     */
/* ------------------------------------------------------------------------- */

static uint16_t _width  = 160;
static uint16_t _height = 80;

static uint8_t _rotation = 1;

/*
 * IMPORTANT:
 *
 * The revised Adafruit #3533/plugin display uses:
 *
 *     colstart = 26
 *     rowstart = 1
 *
 * After rotation 1 these become:
 *
 *     xstart = 1
 *     ystart = 26
 *
 * This is the mapping that fixes the edge artifacts on the revised
 * Adafruit #3533 display.
 */
static uint16_t _xstart = 1;
static uint16_t _ystart = 26;


/* ------------------------------------------------------------------------- */
/* GPIO helpers                                                              */
/* ------------------------------------------------------------------------- */

static void ST7735_Select(void)
{
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port,
                      TFT_CS_Pin,
                      GPIO_PIN_RESET);
}


static void ST7735_Unselect(void)
{
    HAL_GPIO_WritePin(TFT_CS_GPIO_Port,
                      TFT_CS_Pin,
                      GPIO_PIN_SET);
}


static void ST7735_DC_Command(void)
{
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port,
                      TFT_DC_Pin,
                      GPIO_PIN_RESET);
}


static void ST7735_DC_Data(void)
{
    HAL_GPIO_WritePin(TFT_DC_GPIO_Port,
                      TFT_DC_Pin,
                      GPIO_PIN_SET);
}


/* ------------------------------------------------------------------------- */
/* SPI helper                                                                */
/* ------------------------------------------------------------------------- */

static HAL_StatusTypeDef ST7735_SPI_Write(const uint8_t *data,
                                          uint16_t size)
{
    if ((data == NULL) || (size == 0))
    {
        return HAL_OK;
    }

    return HAL_SPI_Transmit(&hspi1,
                            (uint8_t *)data,
                            size,
                            HAL_MAX_DELAY);
}


/* ------------------------------------------------------------------------- */
/* Command                                                                   */
/* ------------------------------------------------------------------------- */

void ST7735_WriteCommand(uint8_t command)
{
    ST7735_Select();

    ST7735_DC_Command();

    ST7735_SPI_Write(&command, 1);

    ST7735_Unselect();
}


/* ------------------------------------------------------------------------- */
/* Data                                                                      */
/* ------------------------------------------------------------------------- */

void ST7735_WriteData(const uint8_t *data,
                      uint16_t size)
{
    if ((data == NULL) || (size == 0))
    {
        return;
    }

    ST7735_Select();

    ST7735_DC_Data();

    ST7735_SPI_Write(data, size);

    ST7735_Unselect();
}

/* ------------------------------------------------------------------------- */
/* Write raw pixel data                                                      */
/* ------------------------------------------------------------------------- */

void ST7735_WritePixels(const uint8_t *data,
                        uint16_t size)
{
    if ((data == NULL) || (size == 0))
    {
        return;
    }

    /*
     * CS remains asserted for the entire pixel transfer.
     *
     * ST7735_SetAddressWindow() has already sent RAMWR.
     */
    ST7735_Select();

    ST7735_DC_Data();

    ST7735_SPI_Write(data, size);

    ST7735_Unselect();
}

/* ------------------------------------------------------------------------- */
/* Command + data                                                            */
/* ------------------------------------------------------------------------- */

static void ST7735_WriteCommandData(uint8_t command,
                                    const uint8_t *data,
                                    uint16_t size)
{
    ST7735_Select();

    /*
     * Command phase.
     */
    ST7735_DC_Command();

    ST7735_SPI_Write(&command, 1);

    /*
     * Data phase.
     */
    if ((data != NULL) && (size > 0))
    {
        ST7735_DC_Data();

        ST7735_SPI_Write(data, size);
    }

    ST7735_Unselect();
}


/* ------------------------------------------------------------------------- */
/* Hardware reset                                                             */
/* ------------------------------------------------------------------------- */

void ST7735_Reset(void)
{
    /*
     * Display must not be selected during reset.
     */
    ST7735_Unselect();

    /*
     * Reset high initially.
     */
    HAL_GPIO_WritePin(TFT_RST_GPIO_Port,
                      TFT_RST_Pin,
                      GPIO_PIN_SET);

    HAL_Delay(5);

    /*
     * Assert reset.
     */
    HAL_GPIO_WritePin(TFT_RST_GPIO_Port,
                      TFT_RST_Pin,
                      GPIO_PIN_RESET);

    HAL_Delay(20);

    /*
     * Release reset.
     */
    HAL_GPIO_WritePin(TFT_RST_GPIO_Port,
                      TFT_RST_Pin,
                      GPIO_PIN_SET);

    /*
     * Allow controller to start.
     */
    HAL_Delay(120);
}


/* ------------------------------------------------------------------------- */
/* Initialization                                                             */
/* ------------------------------------------------------------------------- */

void ST7735_Init(void)
{
    uint8_t data[16];


    /*
     * Make sure TFT is deselected.
     */
    ST7735_Unselect();


    /*
     * Hardware reset.
     */
    ST7735_Reset();


    /* ===================================================================== */
    /* Rcmd1 - ST7735R common initialization                                 */
    /* ===================================================================== */


    /*
     * 1. Software reset
     */
    ST7735_WriteCommand(ST7735_SWRESET);

    HAL_Delay(150);


    /*
     * 2. Sleep out
     */
    ST7735_WriteCommand(ST7735_SLPOUT);

    HAL_Delay(150);


    /*
     * 3. Frame rate control - normal mode
     */
    data[0] = 0x01;
    data[1] = 0x2C;
    data[2] = 0x2D;

    ST7735_WriteCommandData(ST7735_FRMCTR1,
                            data,
                            3);


    /*
     * 4. Frame rate control - idle mode
     */
    data[0] = 0x01;
    data[1] = 0x2C;
    data[2] = 0x2D;

    ST7735_WriteCommandData(ST7735_FRMCTR2,
                            data,
                            3);


    /*
     * 5. Frame rate control - partial mode
     */
    data[0] = 0x01;
    data[1] = 0x2C;
    data[2] = 0x2D;

    data[3] = 0x01;
    data[4] = 0x2C;
    data[5] = 0x2D;

    ST7735_WriteCommandData(ST7735_FRMCTR3,
                            data,
                            6);


    /*
     * 6. Display inversion control
     */
    data[0] = 0x07;

    ST7735_WriteCommandData(ST7735_INVCTR,
                            data,
                            1);


    /*
     * 7. Power control 1
     */
    data[0] = 0xA2;
    data[1] = 0x02;
    data[2] = 0x84;

    ST7735_WriteCommandData(ST7735_PWCTR1,
                            data,
                            3);


    /*
     * 8. Power control 2
     */
    data[0] = 0xC5;

    ST7735_WriteCommandData(ST7735_PWCTR2,
                            data,
                            1);


    /*
     * 9. Power control 3
     */
    data[0] = 0x0A;
    data[1] = 0x00;

    ST7735_WriteCommandData(ST7735_PWCTR3,
                            data,
                            2);


    /*
     * 10. Power control 4
     */
    data[0] = 0x8A;
    data[1] = 0x2A;

    ST7735_WriteCommandData(ST7735_PWCTR4,
                            data,
                            2);


    /*
     * 11. Power control 5
     */
    data[0] = 0x8A;
    data[1] = 0xEE;

    ST7735_WriteCommandData(ST7735_PWCTR5,
                            data,
                            2);


    /*
     * 12. VCOM control
     */
    data[0] = 0x0E;

    ST7735_WriteCommandData(ST7735_VMCTR1,
                            data,
                            1);


    /*
     * 13. Display inversion for revised #3533.
     *
     * The plugin-FPC variant requires INVON.
     */
    ST7735_WriteCommand(ST7735_INVON);


    /*
     * 14. Initial memory access control.
     */
    data[0] = 0xC8;

    ST7735_WriteCommandData(ST7735_MADCTL,
                            data,
                            1);


    /*
     * 15. 16-bit RGB565 color.
     */
    data[0] = 0x05;

    ST7735_WriteCommandData(ST7735_COLMOD,
                            data,
                            1);


    /* ===================================================================== */
    /* Rcmd2green160x80plugin                                                */
    /* ===================================================================== */

    /*
     * The revised/plugin #3533 explicitly uses INVON.
     */
    ST7735_WriteCommand(ST7735_INVON);


    /*
     * Column address initialization.
     *
     * Controller-native dimensions:
     *     0 .. 79
     */
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x4F;

    ST7735_WriteCommandData(ST7735_CASET,
                            data,
                            4);


    /*
     * Row address initialization.
     *
     * Controller-native dimensions:
     *     0 .. 159
     */
    data[0] = 0x00;
    data[1] = 0x00;
    data[2] = 0x00;
    data[3] = 0x9F;

    ST7735_WriteCommandData(ST7735_RASET,
                            data,
                            4);


    /* ===================================================================== */
    /* Rcmd3 - gamma + display on                                             */
    /* ===================================================================== */


    /*
     * Positive gamma.
     */
    data[0]  = 0x02;
    data[1]  = 0x1C;
    data[2]  = 0x07;
    data[3]  = 0x12;
    data[4]  = 0x37;
    data[5]  = 0x32;
    data[6]  = 0x29;
    data[7]  = 0x2D;
    data[8]  = 0x29;
    data[9]  = 0x25;
    data[10] = 0x2B;
    data[11] = 0x39;
    data[12] = 0x00;
    data[13] = 0x01;
    data[14] = 0x03;
    data[15] = 0x10;

    ST7735_WriteCommandData(ST7735_GMCTRP1,
                            data,
                            16);


    /*
     * Negative gamma.
     */
    data[0]  = 0x03;
    data[1]  = 0x1D;
    data[2]  = 0x07;
    data[3]  = 0x06;
    data[4]  = 0x2E;
    data[5]  = 0x2C;
    data[6]  = 0x29;
    data[7]  = 0x2D;
    data[8]  = 0x2E;
    data[9]  = 0x2E;
    data[10] = 0x37;
    data[11] = 0x3F;
    data[12] = 0x00;
    data[13] = 0x00;
    data[14] = 0x02;
    data[15] = 0x10;

    ST7735_WriteCommandData(ST7735_GMCTRN1,
                            data,
                            16);


    /*
     * Normal display mode.
     */
    ST7735_WriteCommand(ST7735_NORON);

    HAL_Delay(10);


    /*
     * Display on.
     */
    ST7735_WriteCommand(ST7735_DISPON);

    HAL_Delay(100);


    /*
     * Set the desired 160x80 landscape orientation.
     *
     * This is important because the plugin offsets are transformed
     * by the rotation.
     */
    ST7735_SetRotation(1);


    /*
     * Start with a completely black screen.
     */
    ST7735_Fill(ST7735_BLACK);
}


/* ------------------------------------------------------------------------- */
/* Rotation                                                                   */
/* ------------------------------------------------------------------------- */

void ST7735_SetRotation(uint8_t rotation)
{
    uint8_t madctl;

    rotation &= 3;

    _rotation = rotation;


    switch (rotation)
    {
        /*
         * ---------------------------------------------------------------
         * Rotation 0
         *
         * 80 x 160
         * ---------------------------------------------------------------
         */
        case 0:

            madctl = ST7735_MADCTL_MX |
                     ST7735_MADCTL_MY |
                     ST7735_MADCTL_BGR;

            _width  = 80;
            _height = 160;

            /*
             * Plugin offsets:
             *
             * colstart = 26
             * rowstart = 1
             */
            _xstart = 26;
            _ystart = 1;

            break;


        /*
         * ---------------------------------------------------------------
         * Rotation 1
         *
         * 160 x 80
         *
         * NORMAL LANDSCAPE ORIENTATION
         * ---------------------------------------------------------------
         */
        case 1:

            madctl = ST7735_MADCTL_MY |
                     ST7735_MADCTL_MV |
                     ST7735_MADCTL_BGR;

            _width  = 160;
            _height = 80;

            /*
             * Rotation swaps the plugin offsets:
             *
             * xstart = rowstart = 1
             * ystart = colstart = 26
             */
            _xstart = 1;
            _ystart = 26;

            break;


        /*
         * ---------------------------------------------------------------
         * Rotation 2
         *
         * 80 x 160
         * ---------------------------------------------------------------
         */
        case 2:

            madctl = ST7735_MADCTL_BGR;

            _width  = 80;
            _height = 160;

            _xstart = 26;
            _ystart = 1;

            break;


        /*
         * ---------------------------------------------------------------
         * Rotation 3
         *
         * 160 x 80
         * ---------------------------------------------------------------
         */
        case 3:

            madctl = ST7735_MADCTL_MX |
                     ST7735_MADCTL_MV |
                     ST7735_MADCTL_BGR;

            _width  = 160;
            _height = 80;

            _xstart = 1;
            _ystart = 26;

            break;


        default:

            return;
    }


    ST7735_WriteCommandData(ST7735_MADCTL,
                            &madctl,
                            1);
}


/* ------------------------------------------------------------------------- */
/* Address window                                                             */
/* ------------------------------------------------------------------------- */

void ST7735_SetAddressWindow(uint16_t x0,
                             uint16_t y0,
                             uint16_t x1,
                             uint16_t y1)
{
    uint8_t data[4];

    /*
     * Convert logical display coordinates into controller RAM coordinates.
     *
     * For our normal rotation 1:
     *
     *     logical X 0..159 -> physical X 1..160
     *     logical Y 0..79  -> physical Y 26..105
     */
    x0 += _xstart;
    x1 += _xstart;

    y0 += _ystart;
    y1 += _ystart;


    /*
     * CASET - column address.
     */
    data[0] = (uint8_t)(x0 >> 8);
    data[1] = (uint8_t)(x0 & 0xFF);
    data[2] = (uint8_t)(x1 >> 8);
    data[3] = (uint8_t)(x1 & 0xFF);

    ST7735_WriteCommandData(ST7735_CASET,
                            data,
                            4);


    /*
     * RASET - row address.
     */
    data[0] = (uint8_t)(y0 >> 8);
    data[1] = (uint8_t)(y0 & 0xFF);
    data[2] = (uint8_t)(y1 >> 8);
    data[3] = (uint8_t)(y1 & 0xFF);

    ST7735_WriteCommandData(ST7735_RASET,
                            data,
                            4);


    /*
     * RAM write.
     */
    ST7735_WriteCommand(ST7735_RAMWR);
}


/* ------------------------------------------------------------------------- */
/* Fill entire display                                                        */
/* ------------------------------------------------------------------------- */

void ST7735_Fill(uint16_t color)
{
    /*
     * 256 bytes = 128 RGB565 pixels.
     *
     * This dramatically reduces the number of HAL_SPI_Transmit()
     * calls compared with sending one pixel at a time.
     */
    uint8_t buffer[256];

    uint8_t hi;
    uint8_t lo;

    uint32_t total_pixels;
    uint32_t pixels_sent;


    /*
     * Build the complete display window.
     */
    ST7735_SetAddressWindow(0,
                            0,
                            _width - 1,
                            _height - 1);


    /*
     * RGB565 is transmitted MSB first.
     */
    hi = (uint8_t)(color >> 8);
    lo = (uint8_t)(color & 0xFF);


    /*
     * Fill the transfer buffer.
     */
    for (uint16_t i = 0; i < sizeof(buffer); i += 2)
    {
        buffer[i]     = hi;
        buffer[i + 1] = lo;
    }


    total_pixels = (uint32_t)_width *
                   (uint32_t)_height;

    pixels_sent = 0;


    /*
     * Keep CS asserted for the entire RAM write.
     */
    ST7735_Select();

    ST7735_DC_Data();


    while (pixels_sent < total_pixels)
    {
        uint32_t remaining;

        uint16_t pixels_this_transfer;


        remaining = total_pixels - pixels_sent;


        if (remaining > 128)
        {
            pixels_this_transfer = 128;
        }
        else
        {
            pixels_this_transfer = (uint16_t)remaining;
        }


        ST7735_SPI_Write(buffer,
                         pixels_this_transfer * 2);


        pixels_sent += pixels_this_transfer;
    }


    ST7735_Unselect();
}


/* ------------------------------------------------------------------------- */
/* Draw one pixel                                                             */
/* ------------------------------------------------------------------------- */

void ST7735_DrawPixel(uint16_t x,
                      uint16_t y,
                      uint16_t color)
{
    uint8_t data[2];


    /*
     * Bounds check.
     */
    if (x >= _width || y >= _height)
    {
        return;
    }


    /*
     * Select single-pixel address.
     */
    ST7735_SetAddressWindow(x,
                            y,
                            x,
                            y);


    /*
     * RGB565, MSB first.
     */
    data[0] = (uint8_t)(color >> 8);
    data[1] = (uint8_t)(color & 0xFF);


    ST7735_WriteData(data, 2);
}


/* ------------------------------------------------------------------------- */
/* Display ON                                                                */
/* ------------------------------------------------------------------------- */

void ST7735_DisplayOn(void)
{
    ST7735_WriteCommand(ST7735_DISPON);

    HAL_Delay(10);
}


/* ------------------------------------------------------------------------- */
/* Display OFF                                                               */
/* ------------------------------------------------------------------------- */

void ST7735_DisplayOff(void)
{
    ST7735_WriteCommand(ST7735_DISPOFF);

    HAL_Delay(10);
}


/* ------------------------------------------------------------------------- */
/* Display inversion                                                          */
/* ------------------------------------------------------------------------- */

void ST7735_InvertDisplay(uint8_t invert)
{
    if (invert)
    {
        ST7735_WriteCommand(ST7735_INVON);
    }
    else
    {
        ST7735_WriteCommand(ST7735_INVOFF);
    }
}
