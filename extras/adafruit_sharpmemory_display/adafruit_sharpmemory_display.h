/*********************************************************************
 * Adafruit Sharp Memory Display Breakout driver for ESP8266
 * Modified / used code from the Adafruit arduino lib, see below :)
 *
 *	Uses SPI Port 1
 *
 *	SPI_CLK = GPIO_14
 *	SPI_MOSI = GPIO_13
 *  SPI_MISO = GPIO_12
 *  SPI_CS = User defined in Adafruit_Sharpmemory_Display_Init
 *
 * Uses mutex to keep from stepping on other SPI stuff you may have
 * connected to the SPI bus
 *
This is an Arduino library for our Monochrome SHARP Memory Displays
  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/1393
These displays use SPI to communicate, 3 pins are required to
interface
Adafruit invests time and resources providing this open source code,
please support Adafruit and open-source hardware by purchasing
products from Adafruit!
Written by Limor Fried/Ladyada  for Adafruit Industries.
BSD license, check license.txt for more information
All text above, and the splash screen must be included in any redistribution
 *********************************************************************/

#ifndef ADAFRUITSHARPMEMORYDISPLAY_H
#define ADAFRUITSHARPMEMORYDISPLAY_H

#include <string.h>
#include <stddef.h>
#include <esp/spi.h>
#include <lwip/sys.h>

#ifndef _USEGPIO_16_
#define _USEGPIO_16_
#define ESP8266_REG(addr) *((volatile uint32_t *)(0x60000000+(addr)))
#define GPFFS_GPIO(p) (((p)==0||(p)==2||(p)==4||(p)==5)?0:((p)==16)?1:3)

//GPIO 16 Control Registers
#define GP16O  ESP8266_REG(0x768)
#define GP16E  ESP8266_REG(0x774)
#define GP16I  ESP8266_REG(0x78C)

//GPIO 16 PIN Control Register
#define GP16C  ESP8266_REG(0x790)
#define GPC16  GP16C

//GPIO 16 PIN Function Register
#define GP16F  ESP8266_REG(0x7A0)
#define GPF16  GP16F

#define GP16FFS0 0 //Function Select bit 0
#define GP16FFS1 1 //Function Select bit 1
#define GP16FPD  3 //Pulldown
#define GP16FSPD 5 //Sleep Pulldown
#define GP16FFS2 6 //Function Select bit 2
#define GP16FFS(f) (((f) & 0x03) | (((f) & 0x04) << 4))
#endif

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) { int16_t t = a; a = b; b = t; }
#endif
#ifndef _swap_uint16_t
#define _swap_uint16_t(a, b) { uint16_t t = a; a = b; b = t; }
#endif

uint8_t _sharpmem_vcom, datapinmask, clkpinmask;

// LCD Dimensions
#define SHARPMEM_LCDWIDTH       (96)
#define SHARPMEM_LCDHEIGHT 		(96)

// LCD Registers
#define SHARPMEM_BIT_WRITECMD   (0x80)
#define SHARPMEM_BIT_VCOM       (0x40)
#define SHARPMEM_BIT_CLEAR      (0x20)

#define TOGGLE_VCOM             do { _sharpmem_vcom = _sharpmem_vcom ? 0x00 : SHARPMEM_BIT_VCOM; } while(0);

typedef struct { // Data stored PER GLYPH
	uint16_t bitmapOffset;     // Pointer into GFXfont->bitmap
	uint8_t  width, height;    // Bitmap dimensions in pixels
	uint8_t  xAdvance;         // Distance to advance cursor (x axis)
	int8_t   xOffset, yOffset; // Dist from cursor pos to UL corner
} GFXglyph;

typedef struct { // Data stored for FONT AS A WHOLE:
	uint8_t  *bitmap;      // Glyph bitmaps, concatenated
	GFXglyph *glyph;       // Glyph array
	uint8_t   first, last; // ASCII extents
	uint8_t   yAdvance;    // Newline distance (y axis)
} GFXfont;

static const unsigned char font[] = {
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x3E, 0x5B, 0x4F, 0x5B, 0x3E,
		0x3E, 0x6B, 0x4F, 0x6B, 0x3E,
		0x1C, 0x3E, 0x7C, 0x3E, 0x1C,
		0x18, 0x3C, 0x7E, 0x3C, 0x18,
		0x1C, 0x57, 0x7D, 0x57, 0x1C,
		0x1C, 0x5E, 0x7F, 0x5E, 0x1C,
		0x00, 0x18, 0x3C, 0x18, 0x00,
		0xFF, 0xE7, 0xC3, 0xE7, 0xFF,
		0x00, 0x18, 0x24, 0x18, 0x00,
		0xFF, 0xE7, 0xDB, 0xE7, 0xFF,
		0x30, 0x48, 0x3A, 0x06, 0x0E,
		0x26, 0x29, 0x79, 0x29, 0x26,
		0x40, 0x7F, 0x05, 0x05, 0x07,
		0x40, 0x7F, 0x05, 0x25, 0x3F,
		0x5A, 0x3C, 0xE7, 0x3C, 0x5A,
		0x7F, 0x3E, 0x1C, 0x1C, 0x08,
		0x08, 0x1C, 0x1C, 0x3E, 0x7F,
		0x14, 0x22, 0x7F, 0x22, 0x14,
		0x5F, 0x5F, 0x00, 0x5F, 0x5F,
		0x06, 0x09, 0x7F, 0x01, 0x7F,
		0x00, 0x66, 0x89, 0x95, 0x6A,
		0x60, 0x60, 0x60, 0x60, 0x60,
		0x94, 0xA2, 0xFF, 0xA2, 0x94,
		0x08, 0x04, 0x7E, 0x04, 0x08,
		0x10, 0x20, 0x7E, 0x20, 0x10,
		0x08, 0x08, 0x2A, 0x1C, 0x08,
		0x08, 0x1C, 0x2A, 0x08, 0x08,
		0x1E, 0x10, 0x10, 0x10, 0x10,
		0x0C, 0x1E, 0x0C, 0x1E, 0x0C,
		0x30, 0x38, 0x3E, 0x38, 0x30,
		0x06, 0x0E, 0x3E, 0x0E, 0x06,
		0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x5F, 0x00, 0x00,
		0x00, 0x07, 0x00, 0x07, 0x00,
		0x14, 0x7F, 0x14, 0x7F, 0x14,
		0x24, 0x2A, 0x7F, 0x2A, 0x12,
		0x23, 0x13, 0x08, 0x64, 0x62,
		0x36, 0x49, 0x56, 0x20, 0x50,
		0x00, 0x08, 0x07, 0x03, 0x00,
		0x00, 0x1C, 0x22, 0x41, 0x00,
		0x00, 0x41, 0x22, 0x1C, 0x00,
		0x2A, 0x1C, 0x7F, 0x1C, 0x2A,
		0x08, 0x08, 0x3E, 0x08, 0x08,
		0x00, 0x80, 0x70, 0x30, 0x00,
		0x08, 0x08, 0x08, 0x08, 0x08,
		0x00, 0x00, 0x60, 0x60, 0x00,
		0x20, 0x10, 0x08, 0x04, 0x02,
		0x3E, 0x51, 0x49, 0x45, 0x3E,
		0x00, 0x42, 0x7F, 0x40, 0x00,
		0x72, 0x49, 0x49, 0x49, 0x46,
		0x21, 0x41, 0x49, 0x4D, 0x33,
		0x18, 0x14, 0x12, 0x7F, 0x10,
		0x27, 0x45, 0x45, 0x45, 0x39,
		0x3C, 0x4A, 0x49, 0x49, 0x31,
		0x41, 0x21, 0x11, 0x09, 0x07,
		0x36, 0x49, 0x49, 0x49, 0x36,
		0x46, 0x49, 0x49, 0x29, 0x1E,
		0x00, 0x00, 0x14, 0x00, 0x00,
		0x00, 0x40, 0x34, 0x00, 0x00,
		0x00, 0x08, 0x14, 0x22, 0x41,
		0x14, 0x14, 0x14, 0x14, 0x14,
		0x00, 0x41, 0x22, 0x14, 0x08,
		0x02, 0x01, 0x59, 0x09, 0x06,
		0x3E, 0x41, 0x5D, 0x59, 0x4E,
		0x7C, 0x12, 0x11, 0x12, 0x7C,
		0x7F, 0x49, 0x49, 0x49, 0x36,
		0x3E, 0x41, 0x41, 0x41, 0x22,
		0x7F, 0x41, 0x41, 0x41, 0x3E,
		0x7F, 0x49, 0x49, 0x49, 0x41,
		0x7F, 0x09, 0x09, 0x09, 0x01,
		0x3E, 0x41, 0x41, 0x51, 0x73,
		0x7F, 0x08, 0x08, 0x08, 0x7F,
		0x00, 0x41, 0x7F, 0x41, 0x00,
		0x20, 0x40, 0x41, 0x3F, 0x01,
		0x7F, 0x08, 0x14, 0x22, 0x41,
		0x7F, 0x40, 0x40, 0x40, 0x40,
		0x7F, 0x02, 0x1C, 0x02, 0x7F,
		0x7F, 0x04, 0x08, 0x10, 0x7F,
		0x3E, 0x41, 0x41, 0x41, 0x3E,
		0x7F, 0x09, 0x09, 0x09, 0x06,
		0x3E, 0x41, 0x51, 0x21, 0x5E,
		0x7F, 0x09, 0x19, 0x29, 0x46,
		0x26, 0x49, 0x49, 0x49, 0x32,
		0x03, 0x01, 0x7F, 0x01, 0x03,
		0x3F, 0x40, 0x40, 0x40, 0x3F,
		0x1F, 0x20, 0x40, 0x20, 0x1F,
		0x3F, 0x40, 0x38, 0x40, 0x3F,
		0x63, 0x14, 0x08, 0x14, 0x63,
		0x03, 0x04, 0x78, 0x04, 0x03,
		0x61, 0x59, 0x49, 0x4D, 0x43,
		0x00, 0x7F, 0x41, 0x41, 0x41,
		0x02, 0x04, 0x08, 0x10, 0x20,
		0x00, 0x41, 0x41, 0x41, 0x7F,
		0x04, 0x02, 0x01, 0x02, 0x04,
		0x40, 0x40, 0x40, 0x40, 0x40,
		0x00, 0x03, 0x07, 0x08, 0x00,
		0x20, 0x54, 0x54, 0x78, 0x40,
		0x7F, 0x28, 0x44, 0x44, 0x38,
		0x38, 0x44, 0x44, 0x44, 0x28,
		0x38, 0x44, 0x44, 0x28, 0x7F,
		0x38, 0x54, 0x54, 0x54, 0x18,
		0x00, 0x08, 0x7E, 0x09, 0x02,
		0x18, 0xA4, 0xA4, 0x9C, 0x78,
		0x7F, 0x08, 0x04, 0x04, 0x78,
		0x00, 0x44, 0x7D, 0x40, 0x00,
		0x20, 0x40, 0x40, 0x3D, 0x00,
		0x7F, 0x10, 0x28, 0x44, 0x00,
		0x00, 0x41, 0x7F, 0x40, 0x00,
		0x7C, 0x04, 0x78, 0x04, 0x78,
		0x7C, 0x08, 0x04, 0x04, 0x78,
		0x38, 0x44, 0x44, 0x44, 0x38,
		0xFC, 0x18, 0x24, 0x24, 0x18,
		0x18, 0x24, 0x24, 0x18, 0xFC,
		0x7C, 0x08, 0x04, 0x04, 0x08,
		0x48, 0x54, 0x54, 0x54, 0x24,
		0x04, 0x04, 0x3F, 0x44, 0x24,
		0x3C, 0x40, 0x40, 0x20, 0x7C,
		0x1C, 0x20, 0x40, 0x20, 0x1C,
		0x3C, 0x40, 0x30, 0x40, 0x3C,
		0x44, 0x28, 0x10, 0x28, 0x44,
		0x4C, 0x90, 0x90, 0x90, 0x7C,
		0x44, 0x64, 0x54, 0x4C, 0x44,
		0x00, 0x08, 0x36, 0x41, 0x00,
		0x00, 0x00, 0x77, 0x00, 0x00,
		0x00, 0x41, 0x36, 0x08, 0x00,
		0x02, 0x01, 0x02, 0x04, 0x02,
		0x3C, 0x26, 0x23, 0x26, 0x3C,
		0x1E, 0xA1, 0xA1, 0x61, 0x12,
		0x3A, 0x40, 0x40, 0x20, 0x7A,
		0x38, 0x54, 0x54, 0x55, 0x59,
		0x21, 0x55, 0x55, 0x79, 0x41,
		0x22, 0x54, 0x54, 0x78, 0x42, // a-umlaut
		0x21, 0x55, 0x54, 0x78, 0x40,
		0x20, 0x54, 0x55, 0x79, 0x40,
		0x0C, 0x1E, 0x52, 0x72, 0x12,
		0x39, 0x55, 0x55, 0x55, 0x59,
		0x39, 0x54, 0x54, 0x54, 0x59,
		0x39, 0x55, 0x54, 0x54, 0x58,
		0x00, 0x00, 0x45, 0x7C, 0x41,
		0x00, 0x02, 0x45, 0x7D, 0x42,
		0x00, 0x01, 0x45, 0x7C, 0x40,
		0x7D, 0x12, 0x11, 0x12, 0x7D, // A-umlaut
		0xF0, 0x28, 0x25, 0x28, 0xF0,
		0x7C, 0x54, 0x55, 0x45, 0x00,
		0x20, 0x54, 0x54, 0x7C, 0x54,
		0x7C, 0x0A, 0x09, 0x7F, 0x49,
		0x32, 0x49, 0x49, 0x49, 0x32,
		0x3A, 0x44, 0x44, 0x44, 0x3A, // o-umlaut
		0x32, 0x4A, 0x48, 0x48, 0x30,
		0x3A, 0x41, 0x41, 0x21, 0x7A,
		0x3A, 0x42, 0x40, 0x20, 0x78,
		0x00, 0x9D, 0xA0, 0xA0, 0x7D,
		0x3D, 0x42, 0x42, 0x42, 0x3D, // O-umlaut
		0x3D, 0x40, 0x40, 0x40, 0x3D,
		0x3C, 0x24, 0xFF, 0x24, 0x24,
		0x48, 0x7E, 0x49, 0x43, 0x66,
		0x2B, 0x2F, 0xFC, 0x2F, 0x2B,
		0xFF, 0x09, 0x29, 0xF6, 0x20,
		0xC0, 0x88, 0x7E, 0x09, 0x03,
		0x20, 0x54, 0x54, 0x79, 0x41,
		0x00, 0x00, 0x44, 0x7D, 0x41,
		0x30, 0x48, 0x48, 0x4A, 0x32,
		0x38, 0x40, 0x40, 0x22, 0x7A,
		0x00, 0x7A, 0x0A, 0x0A, 0x72,
		0x7D, 0x0D, 0x19, 0x31, 0x7D,
		0x26, 0x29, 0x29, 0x2F, 0x28,
		0x26, 0x29, 0x29, 0x29, 0x26,
		0x30, 0x48, 0x4D, 0x40, 0x20,
		0x38, 0x08, 0x08, 0x08, 0x08,
		0x08, 0x08, 0x08, 0x08, 0x38,
		0x2F, 0x10, 0xC8, 0xAC, 0xBA,
		0x2F, 0x10, 0x28, 0x34, 0xFA,
		0x00, 0x00, 0x7B, 0x00, 0x00,
		0x08, 0x14, 0x2A, 0x14, 0x22,
		0x22, 0x14, 0x2A, 0x14, 0x08,
		0x55, 0x00, 0x55, 0x00, 0x55, // #176 (25% block) missing in old code
		0xAA, 0x55, 0xAA, 0x55, 0xAA, // 50% block
		0xFF, 0x55, 0xFF, 0x55, 0xFF, // 75% block
		0x00, 0x00, 0x00, 0xFF, 0x00,
		0x10, 0x10, 0x10, 0xFF, 0x00,
		0x14, 0x14, 0x14, 0xFF, 0x00,
		0x10, 0x10, 0xFF, 0x00, 0xFF,
		0x10, 0x10, 0xF0, 0x10, 0xF0,
		0x14, 0x14, 0x14, 0xFC, 0x00,
		0x14, 0x14, 0xF7, 0x00, 0xFF,
		0x00, 0x00, 0xFF, 0x00, 0xFF,
		0x14, 0x14, 0xF4, 0x04, 0xFC,
		0x14, 0x14, 0x17, 0x10, 0x1F,
		0x10, 0x10, 0x1F, 0x10, 0x1F,
		0x14, 0x14, 0x14, 0x1F, 0x00,
		0x10, 0x10, 0x10, 0xF0, 0x00,
		0x00, 0x00, 0x00, 0x1F, 0x10,
		0x10, 0x10, 0x10, 0x1F, 0x10,
		0x10, 0x10, 0x10, 0xF0, 0x10,
		0x00, 0x00, 0x00, 0xFF, 0x10,
		0x10, 0x10, 0x10, 0x10, 0x10,
		0x10, 0x10, 0x10, 0xFF, 0x10,
		0x00, 0x00, 0x00, 0xFF, 0x14,
		0x00, 0x00, 0xFF, 0x00, 0xFF,
		0x00, 0x00, 0x1F, 0x10, 0x17,
		0x00, 0x00, 0xFC, 0x04, 0xF4,
		0x14, 0x14, 0x17, 0x10, 0x17,
		0x14, 0x14, 0xF4, 0x04, 0xF4,
		0x00, 0x00, 0xFF, 0x00, 0xF7,
		0x14, 0x14, 0x14, 0x14, 0x14,
		0x14, 0x14, 0xF7, 0x00, 0xF7,
		0x14, 0x14, 0x14, 0x17, 0x14,
		0x10, 0x10, 0x1F, 0x10, 0x1F,
		0x14, 0x14, 0x14, 0xF4, 0x14,
		0x10, 0x10, 0xF0, 0x10, 0xF0,
		0x00, 0x00, 0x1F, 0x10, 0x1F,
		0x00, 0x00, 0x00, 0x1F, 0x14,
		0x00, 0x00, 0x00, 0xFC, 0x14,
		0x00, 0x00, 0xF0, 0x10, 0xF0,
		0x10, 0x10, 0xFF, 0x10, 0xFF,
		0x14, 0x14, 0x14, 0xFF, 0x14,
		0x10, 0x10, 0x10, 0x1F, 0x00,
		0x00, 0x00, 0x00, 0xF0, 0x10,
		0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		0xF0, 0xF0, 0xF0, 0xF0, 0xF0,
		0xFF, 0xFF, 0xFF, 0x00, 0x00,
		0x00, 0x00, 0x00, 0xFF, 0xFF,
		0x0F, 0x0F, 0x0F, 0x0F, 0x0F,
		0x38, 0x44, 0x44, 0x38, 0x44,
		0xFC, 0x4A, 0x4A, 0x4A, 0x34, // sharp-s or beta
		0x7E, 0x02, 0x02, 0x06, 0x06,
		0x02, 0x7E, 0x02, 0x7E, 0x02,
		0x63, 0x55, 0x49, 0x41, 0x63,
		0x38, 0x44, 0x44, 0x3C, 0x04,
		0x40, 0x7E, 0x20, 0x1E, 0x20,
		0x06, 0x02, 0x7E, 0x02, 0x02,
		0x99, 0xA5, 0xE7, 0xA5, 0x99,
		0x1C, 0x2A, 0x49, 0x2A, 0x1C,
		0x4C, 0x72, 0x01, 0x72, 0x4C,
		0x30, 0x4A, 0x4D, 0x4D, 0x30,
		0x30, 0x48, 0x78, 0x48, 0x30,
		0xBC, 0x62, 0x5A, 0x46, 0x3D,
		0x3E, 0x49, 0x49, 0x49, 0x00,
		0x7E, 0x01, 0x01, 0x01, 0x7E,
		0x2A, 0x2A, 0x2A, 0x2A, 0x2A,
		0x44, 0x44, 0x5F, 0x44, 0x44,
		0x40, 0x51, 0x4A, 0x44, 0x40,
		0x40, 0x44, 0x4A, 0x51, 0x40,
		0x00, 0x00, 0xFF, 0x01, 0x03,
		0xE0, 0x80, 0xFF, 0x00, 0x00,
		0x08, 0x08, 0x6B, 0x6B, 0x08,
		0x36, 0x12, 0x36, 0x24, 0x36,
		0x06, 0x0F, 0x09, 0x0F, 0x06,
		0x00, 0x00, 0x18, 0x18, 0x00,
		0x00, 0x00, 0x10, 0x10, 0x00,
		0x30, 0x40, 0xFF, 0x01, 0x01,
		0x00, 0x1F, 0x01, 0x01, 0x1E,
		0x00, 0x19, 0x1D, 0x17, 0x12,
		0x00, 0x3C, 0x3C, 0x3C, 0x3C,
		0x00, 0x00, 0x00, 0x00, 0x00  // #255 NBSP
};


void Adafruit_Sharpmemory_Display_Setrotation(int value);
void Adafruit_Sharpmemory_Display_drawPixel(int16_t x, int16_t y, uint16_t color);
uint8_t Adafruit_Sharpmemory_Display_getPixel(uint16_t x, uint16_t y);
void Adafruit_Sharpmemory_Display_Init(int SPI_CS_GPIO, sys_mutex_t *mMutex);
void Adafruit_Sharpmemory_Display_refresh(void);
void Adafruit_Sharpmemory_Display_ChipSelect(bool Value);
void Adafruit_Sharpmemory_Display_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void Adafruit_Sharpmemory_Display_drawFastVLine(int16_t x, int16_t y,int16_t h, uint16_t color);
void Adafruit_Sharpmemory_Display_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void Adafruit_Sharpmemory_Display_drawChar(int16_t x, int16_t y, unsigned char c,uint16_t color, uint16_t bg, uint8_t size);

#endif
