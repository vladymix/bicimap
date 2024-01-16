#include <stdio.h>
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "driver/adc_types_legacy.h"
#include "driver/adc.h"
#include "libs/bme280.h"

/******** OLED ************/
#define CONFIG_SCL_GPIO 18 // SCK
#define CONFIG_SDA_GPIO 19
#define CONFIG_RESET_GPIO -1 // no contains reset pin display

#define I2C_MASTER_FREQ_HZ 400000 /*!< I2C clock of SSD1306 can run at 400 kHz max. */
#define I2C_MASTER_FREQ_100kHZ 100000 /*!< I2C clock of BMP280 can run at 100 kHz max. */
#define I2CAddress 0x3C

#define OLED_CONTROL_BYTE_CMD_STREAM 0x00
#define OLED_CONTROL_BYTE_DATA_STREAM 0x40

// Fundamental commands (pg.28)
#define OLED_CMD_SET_CONTRAST 0x81 // follow with 0x7F
#define OLED_CMD_DISPLAY_RAM 0xA4
#define OLED_CMD_DISPLAY_OFF 0xAE
#define OLED_CMD_SET_MUX_RATIO 0xA8      // follow with 0x3F = 64 MUX
#define OLED_CMD_SET_DISPLAY_OFFSET 0xD3 // follow with 0x00
#define OLED_CMD_DISPLAY_NORMAL 0xA6
#define OLED_CMD_DISPLAY_ON 0xAF

// Hardware Config (pg.31)
#define OLED_CMD_SET_DISPLAY_START_LINE 0x40
#define OLED_CMD_SET_SEGMENT_REMAP_0 0xA0
#define OLED_CMD_SET_SEGMENT_REMAP_1 0xA1
#define OLED_CMD_SET_COM_SCAN_MODE 0xC8
#define OLED_CMD_SET_DISPLAY_CLK_DIV 0xD5 // follow with 0x80
#define OLED_CMD_SET_COM_PIN_MAP 0xDA     // follow with 0x12

// Timing and Driving Scheme (pg.32)
#define OLED_CMD_SET_VCOMH_DESELCT 0xDB // follow with 0x30

// Addressing Command Table (pg.30)
#define OLED_CMD_SET_MEMORY_ADDR_MODE 0x20
#define OLED_CMD_SET_PAGE_ADDR_MODE 0x02 // Page Addressing Mode

// Charge Pump (pg.62)
#define OLED_CMD_SET_CHARGE_PUMP 0x8D // follow with 0x14

// Scrolling Command
#define OLED_CMD_HORIZONTAL_RIGHT 0x26
#define OLED_CMD_HORIZONTAL_LEFT 0x27
#define OLED_CMD_CONTINUOUS_SCROLL 0x29
#define OLED_CMD_DEACTIVE_SCROLL 0x2E
#define OLED_CMD_ACTIVE_SCROLL 0x2F
#define OLED_CMD_VERTICAL 0xA3

#define CONFIG_OFFSETX 0

static uint8_t font8x8_basic_tr[128][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0000 (nul)
    {0x00, 0x04, 0x02, 0xFF, 0x02, 0x04, 0x00, 0x00}, // U+0001 (Up Allow)
    {0x00, 0x20, 0x40, 0xFF, 0x40, 0x20, 0x00, 0x00}, // U+0002 (Down Allow)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0003
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0004
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0005
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0006
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0007
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0008
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0009
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000A
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000B
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000C
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000D
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000E
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000F
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0010
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0011
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0012
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0013
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0014
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0015
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0016
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0017
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0018
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0019
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001A
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001B
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001C
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001D
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001E
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001F
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0020 (space)
    {0x00, 0x00, 0x06, 0x5F, 0x5F, 0x06, 0x00, 0x00}, // U+0021 (!)
    {0x00, 0x03, 0x03, 0x00, 0x03, 0x03, 0x00, 0x00}, // U+0022 (")
    {0x14, 0x7F, 0x7F, 0x14, 0x7F, 0x7F, 0x14, 0x00}, // U+0023 (#)
    {0x24, 0x2E, 0x6B, 0x6B, 0x3A, 0x12, 0x00, 0x00}, // U+0024 ($)
    {0x46, 0x66, 0x30, 0x18, 0x0C, 0x66, 0x62, 0x00}, // U+0025 (%)
    {0x30, 0x7A, 0x4F, 0x5D, 0x37, 0x7A, 0x48, 0x00}, // U+0026 (&)
    {0x04, 0x07, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0027 (')
    {0x00, 0x1C, 0x3E, 0x63, 0x41, 0x00, 0x00, 0x00}, // U+0028 (()
    {0x00, 0x41, 0x63, 0x3E, 0x1C, 0x00, 0x00, 0x00}, // U+0029 ())
    {0x08, 0x2A, 0x3E, 0x1C, 0x1C, 0x3E, 0x2A, 0x08}, // U+002A (*)
    {0x08, 0x08, 0x3E, 0x3E, 0x08, 0x08, 0x00, 0x00}, // U+002B (+)
    {0x00, 0x80, 0xE0, 0x60, 0x00, 0x00, 0x00, 0x00}, // U+002C (,)
    {0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x00, 0x00}, // U+002D (-)
    {0x00, 0x00, 0x60, 0x60, 0x00, 0x00, 0x00, 0x00}, // U+002E (.)
    {0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00}, // U+002F (/)
    {0x3E, 0x7F, 0x71, 0x59, 0x4D, 0x7F, 0x3E, 0x00}, // U+0030 (0)
    {0x40, 0x42, 0x7F, 0x7F, 0x40, 0x40, 0x00, 0x00}, // U+0031 (1)
    {0x62, 0x73, 0x59, 0x49, 0x6F, 0x66, 0x00, 0x00}, // U+0032 (2)
    {0x22, 0x63, 0x49, 0x49, 0x7F, 0x36, 0x00, 0x00}, // U+0033 (3)
    {0x18, 0x1C, 0x16, 0x53, 0x7F, 0x7F, 0x50, 0x00}, // U+0034 (4)
    {0x27, 0x67, 0x45, 0x45, 0x7D, 0x39, 0x00, 0x00}, // U+0035 (5)
    {0x3C, 0x7E, 0x4B, 0x49, 0x79, 0x30, 0x00, 0x00}, // U+0036 (6)
    {0x03, 0x03, 0x71, 0x79, 0x0F, 0x07, 0x00, 0x00}, // U+0037 (7)
    {0x36, 0x7F, 0x49, 0x49, 0x7F, 0x36, 0x00, 0x00}, // U+0038 (8)
    {0x06, 0x4F, 0x49, 0x69, 0x3F, 0x1E, 0x00, 0x00}, // U+0039 (9)
    {0x00, 0x00, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00}, // U+003A (:)
    {0x00, 0x80, 0xE6, 0x66, 0x00, 0x00, 0x00, 0x00}, // U+003B (;)
    {0x08, 0x1C, 0x36, 0x63, 0x41, 0x00, 0x00, 0x00}, // U+003C (<)
    {0x24, 0x24, 0x24, 0x24, 0x24, 0x24, 0x00, 0x00}, // U+003D (=)
    {0x00, 0x41, 0x63, 0x36, 0x1C, 0x08, 0x00, 0x00}, // U+003E (>)
    {0x02, 0x03, 0x51, 0x59, 0x0F, 0x06, 0x00, 0x00}, // U+003F (?)
    {0x3E, 0x7F, 0x41, 0x5D, 0x5D, 0x1F, 0x1E, 0x00}, // U+0040 (@)
    {0x7C, 0x7E, 0x13, 0x13, 0x7E, 0x7C, 0x00, 0x00}, // U+0041 (A)
    {0x41, 0x7F, 0x7F, 0x49, 0x49, 0x7F, 0x36, 0x00}, // U+0042 (B)
    {0x1C, 0x3E, 0x63, 0x41, 0x41, 0x63, 0x22, 0x00}, // U+0043 (C)
    {0x41, 0x7F, 0x7F, 0x41, 0x63, 0x3E, 0x1C, 0x00}, // U+0044 (D)
    {0x41, 0x7F, 0x7F, 0x49, 0x5D, 0x41, 0x63, 0x00}, // U+0045 (E)
    {0x41, 0x7F, 0x7F, 0x49, 0x1D, 0x01, 0x03, 0x00}, // U+0046 (F)
    {0x1C, 0x3E, 0x63, 0x41, 0x51, 0x73, 0x72, 0x00}, // U+0047 (G)
    {0x7F, 0x7F, 0x08, 0x08, 0x7F, 0x7F, 0x00, 0x00}, // U+0048 (H)
    {0x00, 0x41, 0x7F, 0x7F, 0x41, 0x00, 0x00, 0x00}, // U+0049 (I)
    {0x30, 0x70, 0x40, 0x41, 0x7F, 0x3F, 0x01, 0x00}, // U+004A (J)
    {0x41, 0x7F, 0x7F, 0x08, 0x1C, 0x77, 0x63, 0x00}, // U+004B (K)
    {0x41, 0x7F, 0x7F, 0x41, 0x40, 0x60, 0x70, 0x00}, // U+004C (L)
    {0x7F, 0x7F, 0x0E, 0x1C, 0x0E, 0x7F, 0x7F, 0x00}, // U+004D (M)
    {0x7F, 0x7F, 0x06, 0x0C, 0x18, 0x7F, 0x7F, 0x00}, // U+004E (N)
    {0x1C, 0x3E, 0x63, 0x41, 0x63, 0x3E, 0x1C, 0x00}, // U+004F (O)
    {0x41, 0x7F, 0x7F, 0x49, 0x09, 0x0F, 0x06, 0x00}, // U+0050 (P)
    {0x1E, 0x3F, 0x21, 0x71, 0x7F, 0x5E, 0x00, 0x00}, // U+0051 (Q)
    {0x41, 0x7F, 0x7F, 0x09, 0x19, 0x7F, 0x66, 0x00}, // U+0052 (R)
    {0x26, 0x6F, 0x4D, 0x59, 0x73, 0x32, 0x00, 0x00}, // U+0053 (S)
    {0x03, 0x41, 0x7F, 0x7F, 0x41, 0x03, 0x00, 0x00}, // U+0054 (T)
    {0x7F, 0x7F, 0x40, 0x40, 0x7F, 0x7F, 0x00, 0x00}, // U+0055 (U)
    {0x1F, 0x3F, 0x60, 0x60, 0x3F, 0x1F, 0x00, 0x00}, // U+0056 (V)
    {0x7F, 0x7F, 0x30, 0x18, 0x30, 0x7F, 0x7F, 0x00}, // U+0057 (W)
    {0x43, 0x67, 0x3C, 0x18, 0x3C, 0x67, 0x43, 0x00}, // U+0058 (X)
    {0x07, 0x4F, 0x78, 0x78, 0x4F, 0x07, 0x00, 0x00}, // U+0059 (Y)
    {0x47, 0x63, 0x71, 0x59, 0x4D, 0x67, 0x73, 0x00}, // U+005A (Z)
    {0x00, 0x7F, 0x7F, 0x41, 0x41, 0x00, 0x00, 0x00}, // U+005B ([)
    {0x01, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x00}, // U+005C (\)
    {0x00, 0x41, 0x41, 0x7F, 0x7F, 0x00, 0x00, 0x00}, // U+005D (])
    {0x08, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x08, 0x00}, // U+005E (^)
    {0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80}, // U+005F (_)
    {0x00, 0x00, 0x03, 0x07, 0x04, 0x00, 0x00, 0x00}, // U+0060 (`)
    {0x20, 0x74, 0x54, 0x54, 0x3C, 0x78, 0x40, 0x00}, // U+0061 (a)
    {0x41, 0x7F, 0x3F, 0x48, 0x48, 0x78, 0x30, 0x00}, // U+0062 (b)
    {0x38, 0x7C, 0x44, 0x44, 0x6C, 0x28, 0x00, 0x00}, // U+0063 (c)
    {0x30, 0x78, 0x48, 0x49, 0x3F, 0x7F, 0x40, 0x00}, // U+0064 (d)
    {0x38, 0x7C, 0x54, 0x54, 0x5C, 0x18, 0x00, 0x00}, // U+0065 (e)
    {0x48, 0x7E, 0x7F, 0x49, 0x03, 0x02, 0x00, 0x00}, // U+0066 (f)
    {0x98, 0xBC, 0xA4, 0xA4, 0xF8, 0x7C, 0x04, 0x00}, // U+0067 (g)
    {0x41, 0x7F, 0x7F, 0x08, 0x04, 0x7C, 0x78, 0x00}, // U+0068 (h)
    {0x00, 0x44, 0x7D, 0x7D, 0x40, 0x00, 0x00, 0x00}, // U+0069 (i)
    {0x60, 0xE0, 0x80, 0x80, 0xFD, 0x7D, 0x00, 0x00}, // U+006A (j)
    {0x41, 0x7F, 0x7F, 0x10, 0x38, 0x6C, 0x44, 0x00}, // U+006B (k)
    {0x00, 0x41, 0x7F, 0x7F, 0x40, 0x00, 0x00, 0x00}, // U+006C (l)
    {0x7C, 0x7C, 0x18, 0x38, 0x1C, 0x7C, 0x78, 0x00}, // U+006D (m)
    {0x7C, 0x7C, 0x04, 0x04, 0x7C, 0x78, 0x00, 0x00}, // U+006E (n)
    {0x38, 0x7C, 0x44, 0x44, 0x7C, 0x38, 0x00, 0x00}, // U+006F (o)
    {0x84, 0xFC, 0xF8, 0xA4, 0x24, 0x3C, 0x18, 0x00}, // U+0070 (p)
    {0x18, 0x3C, 0x24, 0xA4, 0xF8, 0xFC, 0x84, 0x00}, // U+0071 (q)
    {0x44, 0x7C, 0x78, 0x4C, 0x04, 0x1C, 0x18, 0x00}, // U+0072 (r)
    {0x48, 0x5C, 0x54, 0x54, 0x74, 0x24, 0x00, 0x00}, // U+0073 (s)
    {0x00, 0x04, 0x3E, 0x7F, 0x44, 0x24, 0x00, 0x00}, // U+0074 (t)
    {0x3C, 0x7C, 0x40, 0x40, 0x3C, 0x7C, 0x40, 0x00}, // U+0075 (u)
    {0x1C, 0x3C, 0x60, 0x60, 0x3C, 0x1C, 0x00, 0x00}, // U+0076 (v)
    {0x3C, 0x7C, 0x70, 0x38, 0x70, 0x7C, 0x3C, 0x00}, // U+0077 (w)
    {0x44, 0x6C, 0x38, 0x10, 0x38, 0x6C, 0x44, 0x00}, // U+0078 (x)
    {0x9C, 0xBC, 0xA0, 0xA0, 0xFC, 0x7C, 0x00, 0x00}, // U+0079 (y)
    {0x4C, 0x64, 0x74, 0x5C, 0x4C, 0x64, 0x00, 0x00}, // U+007A (z)
    {0x08, 0x08, 0x3E, 0x77, 0x41, 0x41, 0x00, 0x00}, // U+007B ({)
    {0x00, 0x00, 0x00, 0x77, 0x77, 0x00, 0x00, 0x00}, // U+007C (|)
    {0x41, 0x41, 0x77, 0x3E, 0x08, 0x08, 0x00, 0x00}, // U+007D (})
    {0x02, 0x03, 0x01, 0x03, 0x02, 0x03, 0x01, 0x00}, // U+007E (~)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // U+007F
};

typedef struct
{
    bool _valid; // Not using it anymore
    int _segLen; // Not using it anymore
    uint8_t _segs[128];
} PAGE_t;

typedef struct
{
    int _sda;
    int _slc;
    int _reset;
    int _address;
    int _width;
    int _height;
    int _pages;
    int _dc;
    spi_device_handle_t _SPIHandle;
    bool _scEnable;
    int _scStart;
    int _scEnd;
    int _scDirection;
    PAGE_t _page[8];
    bool _flip;
} OLed;

void i2c_init(OLed *dev);
void i2c_contrast(OLed *dev, int contrast);
void initOled(OLed *dev);
uint8_t ssd1306_rotate_byte(uint8_t ch1);
uint8_t ssd1306_copy_bit(uint8_t src, int srcBits, uint8_t dst, int dstBits);
void ssd1306_flip(uint8_t *buf, size_t blen);
void ssd1306_invert(uint8_t *buf, size_t blen);
void ssd1306_show_buffer(OLed *dev);

void oled_display_clear(OLed *dev, int line);
void oled_display_image(OLed *dev, int page, int seg, uint8_t *images, int width);
void oled_display_text(OLed *dev, int line, char *text, bool invert);
void oled_print_text(OLed *dev, int page, char *text, int text_len, bool invert);
void oled_clear_screen(OLed *dev, bool invert);
void oled_display_bitmap(OLed *dev, int xpos, int ypos, uint8_t *bitmap, int width, int height, bool invert);

// ** Commons ****
void delayms(int time);
void loadInfo();
void chronometer(int ms, char *text);

// ********* ADC *******

/*
    ADC2_CHANNEL_0 = 0, !< ADC2 channel 0 is GPIO4  (ESP32), GPIO11 (ESP32-S2)
    ADC2_CHANNEL_1,     ADC2 channel 1 is GPIO0  (ESP32), GPIO12 (ESP32-S2)
    ADC2_CHANNEL_2,     ADC2 channel 2 is GPIO2  (ESP32), GPIO13 (ESP32-S2)
    ADC2_CHANNEL_3,     ADC2 channel 3 is GPIO15 (ESP32), GPIO14 (ESP32-S2)
    ADC2_CHANNEL_4,     ADC2 channel 4 is GPIO13 (ESP32), GPIO15 (ESP32-S2)
    ADC2_CHANNEL_5,     ADC2 channel 5 is GPIO12 (ESP32), GPIO16 (ESP32-S2)
    ADC2_CHANNEL_6,     ADC2 channel 6 is GPIO14 (ESP32), GPIO17 (ESP32-S2)
    ADC2_CHANNEL_7,     ADC2 channel 7 is GPIO27 (ESP32), GPIO18 (ESP32-S2)
    ADC2_CHANNEL_8,     ADC2 channel 8 is GPIO25 (ESP32), GPIO19 (ESP32-S2)
    ADC2_CHANNEL_9,     ADC2 channel 9 is GPIO26 (ESP32), GPIO20 (ESP32-S2)

******  Value Channel *****
    ADC_CHANNEL_0
    ADC_CHANNEL_1
    ADC_CHANNEL_2
    ADC_CHANNEL_3
    ADC_CHANNEL_4
    ADC_CHANNEL_5
    ADC_CHANNEL_6
    ADC_CHANNEL_7
    ADC_CHANNEL_8
    ADC_CHANNEL_9
    ADC_CHANNEL_MAX

*****  Value adc_atten *****
    ADC_ATTEN_DB_0 = 0
    The input voltage of ADC will be reduced to about 1/1

    ADC_ATTEN_DB_2_5 = 1
    The input voltage of ADC will be reduced to about 1/1.34

    ADC_ATTEN_DB_6 = 2
    The input voltage of ADC will be reduced to about 1/2

    ADC_ATTEN_DB_11 = 3
    The input voltage of ADC will be reduced to about 1/3.6
    ADC_ATTEN_MAX
 *
 *** adc_bits_width_t ****
    ADC_WIDTH_BIT_9 = 0
    ADC capture width is 9Bit
    ADC_WIDTH_BIT_10 = 1
    ADC capture width is 10Bit
    ADC_WIDTH_BIT_11 = 2
    ADC capture width is 11Bit
    ADC_WIDTH_BIT_12 = 3
    ADC capture width is 12Bit
    ADC_WIDTH_MAX
 */

typedef struct
{
    int channel;
    int adc_atten;        // Atenuacion
    int adc_bits_width_t; // output bits
} AnalogicDevice;

void initAdc2(AnalogicDevice *device);

int readAdc2Value(AnalogicDevice *device);

void initAdc1(AnalogicDevice *device);

int readAdc1Value(AnalogicDevice *device);

//********* OTA ********

/**
 * @brief Bit set for application events
 */
#define WIFI_CONNECTED_EVENT BIT0
#define WIFI_DISCONNECTED_EVENT BIT1
#define MQTT_CONNECTED_EVENT BIT2
#define MQTT_DISCONNECTED_EVENT BIT3
#define OTA_CONFIG_FETCHED_EVENT BIT4
#define OTA_CONFIG_UPDATED_EVENT BIT5
#define OTA_TASK_IN_NORMAL_STATE_EVENT BIT6

/*! Client attribute key to send the firmware version value to ThingsBoard */
#define TB_CLIENT_ATTR_FIELD_CURRENT_FW "current_fw_version"
/**
 * @brief  MQTT topic to request the specified shared attributes from ThingsBoard.
 *         44332 is a request id, any integer number can be used.
 */
#define TB_ATTRIBUTES_REQUEST_TOPIC "v1/devices/me/attributes/request/44332"

/**
 * @brief  MQTT topic to receive the requested specified shared attributes from ThingsBoard.
 *         44332 is a request id, have to be the same as used for the request.
 */
#define TB_ATTRIBUTES_RESPONSE_TOPIC "v1/devices/me/attributes/response/44332"

#define TB_SHARED_ATTR_FIELD_TARGET_FW_URL "fw_url"
#define TB_SHARED_ATTR_FIELD_TARGET_FW_VER "fw_version"
#define TB_ATTRIBUTES_TOPIC "v1/devices/me/attributes"

/*! MQTT topic to subscribe for the receiving of the specified shared attribute after the request to ThingsBoard 
https://thingsboard.io/docs/reference/mqtt-api/#firmware-api
*/
#define TB_ATTRIBUTES_SUBSCRIBE_TO_RESPONSE_TOPIC "v1/devices/me/attributes/response/+"

#define HASH_LEN 32

//extern const uint8_t server_cert_pem_start[] asm("_binary_github_pem_start");
//extern const uint8_t server_cert_pem_end[] asm("_binary_github_pem_end");

extern const uint8_t vladymix_cert_pem_start[] asm("_binary_vladymix_pem_start");
extern const uint8_t vladymix_cert_pem_end[] asm("_binary_vladymix_pem_end");

#define cer_vlady "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"

/*! Firmware version used for comparison after OTA config was received from ThingsBoard */
#define FIRMWARE_VERSION "2.0"
/*! Body of the request of specified shared attributes */
#define TB_SHARED_ATTR_KEYS_REQUEST "{\"sharedKeys\":\"fw_version,fw_url\"}"

//***********+ END OTA

#define APP_ABORT_ON_ERROR(x)                                     \
    do                                                            \
    {                                                             \
        esp_err_t __err_rc = (x);                                 \
        if (__err_rc != ESP_OK)                                   \
        {                                                         \
            _esp_error_check_failed(__err_rc, __FILE__, __LINE__, \
                                    __ASSERT_FUNC, #x);           \
        }                                                         \
    } while (0);



typedef enum
{
    BUTTON_STATE_TOUCH = 0,
    BUTTON_STATE_RELEASE = 1
} StateTouch;

typedef struct
{
    int gpio;
    // Sensitivity response ms
    int sensitivity;
    StateTouch status;
    int time;
} TouchButton;

typedef enum
{
    DISPLAY_LUX,
    DISPLAY_HUMIDITY,
    DISPLAY_TEMPERATURE,
    DISPLAY_PRESSURE,
    DISPLAY_NOISE,
} DisplayMode;

typedef struct
{
    DisplayMode mode;
    int lux;
    double temperature;
    double humidity;
    int noise;
    double pressure;
    double latitude;
    double longitude;
} Sensor;



void initBMP(struct bme280_t *dev);

void readDataBmp(struct bme280_t dev, Sensor *sensor);

/**
 * @brief Set of states for @ref ota_task(void)
 */
enum state
{
    STATE_RETRY_CONECTED,
    STATE_INITIAL,
    STATE_WAIT_WIFI,
    STATE_WIFI_CONNECTED,
    STATE_WAIT_MQTT,
    STATE_MQTT_CONNECTED,
    STATE_WAIT_OTA_CONFIG_FETCHED,
    STATE_OTA_CONFIG_FETCHED,
    STATE_APP_LOOP,
    STATE_CONNECTION_IS_OK
};
