#include "flirlipton.h"

sys_mutex_t *pSPIMutex;
int FLIR_SPI_CS_GPIO_NUM;

void FLIR_Lipton_CaptureImage(FLIRBuffer ImageBuffer)
{
	//lock the SPI port
	sys_mutex_lock(pSPIMutex);

	FLIR_Lipton_ChipSelect(1);//chip disabled
	//setup up the spi port
	spi_init(1, SPI_MODE3, SPI_FREQ_DIV_20M, true, SPI_BIG_ENDIAN, true);


	bool done = false;

	//loop until we get a good image
	while( done == false )
	{
		int x = 0;
		int y = 0;
		vTaskDelay((1000)/portTICK_RATE_MS);
		//we are looking for a sync, so clear frist word
		ImageBuffer[0][0] = 0;

		printf("Starting Camera\n");
		FLIR_Lipton_ChipSelect(0);//enable chip


		//loop until we find the begining of a status frame
		while ((ImageBuffer[0][0] & 0x0FFFFFFF) != 0x0FFFFFFF)
		{
			ImageBuffer[0][0] = spi_transfer_32(1,0x00);
		}

		printf("Sync?\n");

		//eat up the rest of the status frame
		for(x = 1; x < 41; x++)
		{
			ImageBuffer[x][y] = spi_transfer_32(1,0x00);
		}
		FLIR_Lipton_ChipSelect(1);//disable chip

		vTaskDelay((200)/portTICK_RATE_MS);
		//we are now sync'd so lets keep eating up status frames
		//until we get the start of an image frame
		while ((ImageBuffer[0][0] & 0x0FFFFFFF) == 0x0FFFFFFF)
		{
			FLIR_Lipton_ChipSelect(0);//enable chip

			//capture an entire row or a status frame
			for(x = 0; x < 41; x++)
			{
				//grab the data 32 bits at a time
				ImageBuffer[x][y] = spi_transfer_32(1,0x00);
			}
			FLIR_Lipton_ChipSelect(1);//disable chip
		}

		printf("Sync fo sure\n");

		//we are now capturing the image data + 1
		//status frame to keep things running
		for(y = 1; y < 61; y++)
		{
			FLIR_Lipton_ChipSelect(0);//enable chip
			//loop through the packets
			for(x = 0; x < 41; x++)
			{	//grab data 32 bits at a time
				ImageBuffer[x][y] = spi_transfer_32(1,0x00);
			}
			FLIR_Lipton_ChipSelect(1);//disable chip
		}

		//set to finished, we will set false
		//if we detect an out of order packet
		done = true;

		//loop through the packets to make sure
		//the sequence numbers line up
		for (y = 0; y < 60; y++)
			if (((ImageBuffer[0][y] & 0x00FF0000) >> 16) != y)
			{
				done = false;
			}


	}

	sys_mutex_unlock(pSPIMutex);
}

void FLIR_Lipton_ChipSelect(bool Value)
{
	if (FLIR_SPI_CS_GPIO_NUM != 16)
	{
		gpio_write(FLIR_SPI_CS_GPIO_NUM,Value);
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
	printf("Init Lipton\n");
	if(mMutex == NULL)
		return;//we must have a mutex here

	FLIR_SPI_CS_GPIO_NUM = SPI_CS_GPIO;
	pSPIMutex = mMutex;

	if (SPI_CS_GPIO == 16)
	{
		//TODO:strange stuffingsgs for gpio16, fix this properly
		GPF16 = GP16FFS(GPFFS_GPIO(16));//Set mode to GPIO
		GPC16 = 0;//?
		GP16E |= 1;//enable gpio 16
		printf("camera16\n");
	}

	FLIR_Lipton_ChipSelect(1);//disable FLIR chip select

}
