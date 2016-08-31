#ifndef FLIRLIPTON_H
#define FLIRLIPTON_H

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

void FLIR_Lipton_Init(int SPI_CS_GPIO, sys_mutex_t *mMutex);
void FLIR_Lipton_CaptureImage(uint16_t *ImageBuffer);

#endif