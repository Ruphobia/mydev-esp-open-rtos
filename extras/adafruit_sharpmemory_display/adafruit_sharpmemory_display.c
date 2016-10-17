
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

#include "adafruit_sharpmemory_display.h"

#define GPIO_SPI_MISO	(12)
#define GPIO_SPI_MOSI 	(13)
#define GPIO_SPI_CLK 	(14)

sys_mutex_t *pSPIMutex;
int DISPLAY_SPI_CS_GPIO_NUM;

uint8_t sharpmem_buffer[(SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8];
static const uint8_t set[] = {  1,  2,  4,  8,  16,  32,  64,  128 };
//fixed compiler warning {  ~1,  ~2,  ~4,  ~8,  ~16,  ~32,  ~64,  ~128 }
//gcc doesn't like ~128 in an unsigned 8 bit var for some reason
static const uint8_t clr[] = { 254, 253, 251, 247, 239, 223, 191, 127 };
int rotation = 0;

void Display_SendByte(uint8_t data)
{
	uint8_t i = 0;

	// LCD expects LSB first
	for (i=0; i<8; i++)
	{
		// Make sure clock starts low
		//digitalWrite(_clk, LOW);
		gpio_write(14,0);

		if (data & 0x80)
			//digitalWrite(_mosi, HIGH);
			gpio_write(13,1);
		else
			//digitalWrite(_mosi, LOW);
			gpio_write(13,0);

		// Clock is active high
		//digitalWrite(_clk, HIGH);
		gpio_write(14,1);
		data <<= 1;
	}
	// Make sure clock ends low
	//digitalWrite(_clk, LOW);
	gpio_write(14,0);
}

void Display_sendbyteLSB(uint8_t data)
{
	uint8_t i = 0;

	// LCD expects LSB first
	for (i=0; i<8; i++)
	{
		// Make sure clock starts low
		//digitalWrite(_clk, LOW);
		gpio_write(14,0);
		if (data & 0x01)
			//digitalWrite(_mosi, HIGH);
			gpio_write(13,1);
		else
			//digitalWrite(_mosi, LOW);
			gpio_write(13,0);
		// Clock is active high
		//digitalWrite(_clk, HIGH);
		gpio_write(14,1);
		data >>= 1;
	}
	// Make sure clock ends low
	//digitalWrite(_clk, LOW);
	gpio_write(14,0);
}

void setupbitbang()
{
    gpio_set_iomux_function(12, IOMUX_GPIO12_FUNC_GPIO);
    gpio_set_iomux_function(13, IOMUX_GPIO13_FUNC_GPIO);
    gpio_set_iomux_function(14, IOMUX_GPIO14_FUNC_GPIO);

	gpio_enable(GPIO_SPI_CLK, GPIO_OUTPUT);
	gpio_write(GPIO_SPI_CLK,0);
	gpio_enable(GPIO_SPI_MOSI, GPIO_OUTPUT);
	gpio_enable(GPIO_SPI_MISO, GPIO_INPUT);
	printf("Set Bitbang\n");
}

void Adafruit_Sharpmemory_Display_Setrotation(int value)
{
	rotation = value;
}

void Adafruit_Sharpmemory_Display_drawPixel(int16_t x, int16_t y, uint16_t color)
{
	if((x < 0) || (x >= SHARPMEM_LCDWIDTH) || (y < 0) || (y >= SHARPMEM_LCDHEIGHT)) return;

	switch(rotation)
	{
	case 1:
		_swap_int16_t(x, y);
		x = SHARPMEM_LCDWIDTH  - 1 - x;
		break;
	case 2:
		x = SHARPMEM_LCDWIDTH  - 1 - x;
		y = SHARPMEM_LCDHEIGHT - 1 - y;
		break;
	case 3:
		_swap_int16_t(x, y);
		y = SHARPMEM_LCDHEIGHT - 1 - y;
		break;
	}

	if(color)
	{
		sharpmem_buffer[(y*SHARPMEM_LCDWIDTH + x) / 8] |= set[x & 7];
	} else
	{
		sharpmem_buffer[(y*SHARPMEM_LCDWIDTH + x) / 8] &= clr[x & 7];
	}
}

uint8_t Adafruit_Sharpmemory_Display_getPixel(uint16_t x, uint16_t y)
{
	if((x >= SHARPMEM_LCDWIDTH) || (y >= SHARPMEM_LCDHEIGHT)) return 0; // <0 test not needed, unsigned

	switch(rotation) {
	case 1:
		_swap_uint16_t(x, y);
		x = SHARPMEM_LCDWIDTH  - 1 - x;
		break;
	case 2:
		x = SHARPMEM_LCDWIDTH  - 1 - x;
		y = SHARPMEM_LCDHEIGHT - 1 - y;
		break;
	case 3:
		_swap_uint16_t(x, y);
		y = SHARPMEM_LCDHEIGHT - 1 - y;
		break;
	}

	return sharpmem_buffer[(y*SHARPMEM_LCDWIDTH + x) / 8] & set[x & 7] ? 1 : 0;
}

void Display_Clear()
{
	memset(sharpmem_buffer, 0xff, (SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8);
	// Send the clear screen command rather than doing a HW refresh (quicker)

	Display_SendByte(_sharpmem_vcom | SHARPMEM_BIT_CLEAR);
	Display_sendbyteLSB(0x00);
	TOGGLE_VCOM;
	printf("Display Clear\n");
}

void Adafruit_Sharpmemory_Display_Clear()
{
	//make sure we have a valid mutex
	//or bail out
	if(pSPIMutex == NULL)
		return;

	//get a lock on the SPI bus
	//so the camera doesn't boink the display
	//during the update
	sys_mutex_lock(pSPIMutex);

	setupbitbang();
	// Send the clear screen command rather than doing a HW refresh (quicker)
	Adafruit_Sharpmemory_Display_ChipSelect(1);

	Display_Clear();

	Adafruit_Sharpmemory_Display_ChipSelect(0);

	//unlock the SPI bus so the
	//camera can use it
	sys_mutex_unlock(pSPIMutex);
}

void Adafruit_Sharpmemory_Display_ChipSelect(bool Value)
{
	if (DISPLAY_SPI_CS_GPIO_NUM != 16)
	{
		gpio_write(DISPLAY_SPI_CS_GPIO_NUM,Value);
	}
	else
	{
		printf("GPio16\n");
		if(Value == 1)
			GP16O |= 1; // Set GPIO_16
		else
			GP16O &= ~1; // clear GPIO_16a
	}
}

void Display_refresh(void)
{
	uint16_t i, totalbytes, currentline, oldline;
	totalbytes = (SHARPMEM_LCDWIDTH * SHARPMEM_LCDHEIGHT) / 8;

	// Send the write command
	Display_SendByte(SHARPMEM_BIT_WRITECMD | _sharpmem_vcom);
	TOGGLE_VCOM;

	// Send the address for line 1
	oldline = currentline = 1;
	Display_sendbyteLSB(currentline);

	// Send image buffer
	for (i=0; i<totalbytes; i++)
	{
		Display_sendbyteLSB(sharpmem_buffer[i]);
		currentline = ((i+1)/(SHARPMEM_LCDWIDTH/8)) + 1;
		if(currentline != oldline)
		{
			// Send end of line and address bytes
			Display_sendbyteLSB(0x00);
			if (currentline <= SHARPMEM_LCDHEIGHT)
			{
				Display_sendbyteLSB(currentline);
			}
			oldline = currentline;
		}
	}

	// Send another trailing 8 bits for the last line
	Display_sendbyteLSB(0x00);
}

void Adafruit_Sharpmemory_Display_refresh(void)
{
	//make sure we have a valid mutex
	//or bail out
	if(pSPIMutex == NULL)
		return;

	//get a lock on the SPI bus
	//so the camera doesn't boink the display
	//during the update
	sys_mutex_lock(pSPIMutex);

	setupbitbang();
	Adafruit_Sharpmemory_Display_ChipSelect(1);//Set LCD chip select to enable

	Display_refresh();

	Adafruit_Sharpmemory_Display_ChipSelect(0);

	//unlock the SPI bus so the
	//camera can use it
	sys_mutex_unlock(pSPIMutex);
}

void Adafruit_Sharpmemory_Display_Init(int SPI_CS_GPIO, sys_mutex_t *mMutex)
{
	if(mMutex == NULL)
		return;//we must have a mutex here

	DISPLAY_SPI_CS_GPIO_NUM = SPI_CS_GPIO;
	pSPIMutex = mMutex;

	if (SPI_CS_GPIO == 16)
	{
		//TODO:strange stuffingsgs for gpio16, fix this properly
		GPF16 = GP16FFS(GPFFS_GPIO(16));//Set mode to GPIO
		GPC16 = 0;//?
		GP16E |= 1;//enable gpio 16
	}

	Adafruit_Sharpmemory_Display_ChipSelect(0);//de-select display
}

void Adafruit_Sharpmemory_Display_drawChar(int16_t x, int16_t y, unsigned char c,uint16_t color, uint16_t bg, uint8_t size)
{
	if((x >= SHARPMEM_LCDWIDTH)            || // Clip right
			(y >= SHARPMEM_LCDHEIGHT)           || // Clip bottom
			((x + 6 * size - 1) < 0) || // Clip left
			((y + 8 * size - 1) < 0))   // Clip top
		return;

	if(c >= 176) c++; // Handle 'classic' charset behavior

	for(int8_t i=0; i<6; i++ )
	{
		uint8_t line;

		if(i < 5)
			line = font[(c*5)+i];
		else
			line = 0x0;

		for(int8_t j=0; j<8; j++, line >>= 1)
		{
			if(line & 0x1)
			{
				if(size == 1)
					Adafruit_Sharpmemory_Display_drawPixel(x+i, y+j, color);
				else
					Adafruit_Sharpmemory_Display_fillRect(x+(i*size), y+(j*size), size, size, color);
			}
			else
				if(bg != color)
				{
					if(size == 1)
						Adafruit_Sharpmemory_Display_drawPixel(x+i, y+j, bg);
					else
						Adafruit_Sharpmemory_Display_fillRect(x+i*size, y+j*size, size, size, bg);
				}
		}
	}

}

void Adafruit_Sharpmemory_Display_fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
	for (int16_t i=x; i<x+w; i++)
		Adafruit_Sharpmemory_Display_drawFastVLine(i, y, h, color);
}

void Adafruit_Sharpmemory_Display_drawFastVLine(int16_t x, int16_t y,int16_t h, uint16_t color)
{
	Adafruit_Sharpmemory_Display_drawLine(x, y, x, y+h-1, color);
}

// Bresenham's algorithm - thx wikpedia
void Adafruit_Sharpmemory_Display_drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
	int16_t steep = abs(y1 - y0) > abs(x1 - x0);
	if (steep)
	{
		_swap_int16_t(x0, y0);
		_swap_int16_t(x1, y1);
	}

	if (x0 > x1)
	{
		_swap_int16_t(x0, x1);
		_swap_int16_t(y0, y1);
	}

	int16_t dx, dy;
	dx = x1 - x0;
	dy = abs(y1 - y0);

	int16_t err = dx / 2;
	int16_t ystep;

	if (y0 < y1)
	{
		ystep = 1;
	}
	else
	{
		ystep = -1;
	}

	for (; x0<=x1; x0++)
	{
		if (steep)
		{
			Adafruit_Sharpmemory_Display_drawPixel(y0, x0, color);
		} else
		{
			Adafruit_Sharpmemory_Display_drawPixel(x0, y0, color);
		}
		err -= dy;
		if (err < 0)
		{
			y0 += ystep;
			err += dx;
		}
	}
}

