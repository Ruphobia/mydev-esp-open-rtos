#include "flirlipton.h"

sys_mutex_t *pSPIMutex;
int SPI_CS_GPIO_NUM;


void FLIR_Lipton_CaptureImage(uint16_t *ImageBuffer)
{
	//get a lock on the SPI bus
	//so the LCD display doesn't
	//boink the camera while we
	//are capturing an image
	sys_mutex_lock(pSPIMutex);

	//clear buffer
	memset(ImageBuffer,0,sizeof(uint16_t) * 4800);

	//setup SPI bus
	//bool spi_init(uint8_t bus, spi_mode_t mode, uint32_t freq_divider, bool msb, spi_endianness_t endianness, bool minimal_pins)
	//TODO: experiment with msb and endianness to make this work
	spi_init(1, 3, SPI_FREQ_DIV_4M, true, true, true);

	//setup buffers
	uint8_t TXpayload[164];
	uint8_t RXpayload[164];
	int packetNb;
	int i;
	uint8_t checkByte = 0x0f;

	//sync
	//set chip select to 1
	gpio_write(SPI_CS_GPIO_NUM,1);

	//delay for 200ms
	vTaskDelay(200/portTICK_RATE_MS);
	
	//set chip select to 0
	gpio_write(SPI_CS_GPIO_NUM,0);

	//loop while discard packets
	while((checkByte & 0x0f) == 0x0f)
	{
		memset(RXpayload,0,sizeof(RXpayload));
		spi_transfer(1,TXpayload, RXpayload, sizeof(TXpayload), 8);

		checkByte = RXpayload[0];
		packetNb = RXpayload[1];
	}

	//sync done, first packet is ready, store packets
	while(packetNb < 60)
	{
		// ignore discard packets
		if((RXpayload[0] & 0x0f) != 0x0f)
		{
			for(i=0;i<80;i++)
			{
				ImageBuffer[packetNb * 80 + i] = (RXpayload[2*i+4] << 8 | RXpayload[2*i+5]);
			}
		}

		// read next packet
		memset(RXpayload,0,sizeof(RXpayload));
		spi_transfer(1,TXpayload, RXpayload, sizeof(TXpayload), 8);
		packetNb = RXpayload[1];
	}

	//mraa_gpio_write(cs, 0); //why does this code set this to 0?
	//set chip select to 1
	gpio_write(SPI_CS_GPIO_NUM,1);

	//release the lock on the SPI bus
	//so the LCD can use it
	sys_mutex_unlock(pSPIMutex);
}

void FLIR_Lipton_ChipSelect(bool Value)
{
	if (SPI_CS_GPIO_NUM != 16)
	{
		gpio_write(SPI_CS_GPIO_NUM,Value);
	}
	else
	{
		if(Value == 1)
			GP16O |= 1; // Set GPIO_16
		else
			GP16O &= ~1; // clear GPIO_16a
	}
}

void FLIR_Lipton_Init(int SPI_CS_GPIO, sys_mutex_t *mMutex)
{
	if(mMutex == NULL)
		return;//we must have a mutex here

	SPI_CS_GPIO_NUM = SPI_CS_GPIO;
	pSPIMutex = mMutex;

	if (SPI_CS_GPIO == 16)
	{
		//TODO:strange stuffingsgs for gpio16, fix this properly
		GPF16 = GP16FFS(GPFFS_GPIO(16));//Set mode to GPIO
		GPC16 = 0;//?
		GP16E |= 1;//enable gpio 16
	}
	
	FLIR_Lipton_ChipSelect(1);//disable FLIR chip select
}