/*
 * Copyright 2016 Piotr Sperka (http://www.piotrsperka.info)
*/

#include "extram.h"
#include "gpio.h"
#include "buffer.h"
#include "interface.h"
#include "vs1053.h"
#include "esp/spi.h"

#include <string.h>
#include <stdio.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

/*
SemaphoreHandle_t sSPI = NULL;

ICACHE_FLASH_ATTR uint8_t spi_take_semaphore()
{
        if(sSPI) if(xSemaphoreTake(sSPI, portMAX_DELAY)) return 1;
        return 0;
}

ICACHE_FLASH_ATTR void spi_give_semaphore()
{
        if(sSPI) xSemaphoreGive(sSPI);
}
*/

void extramInit()
{
	char test[17]    = {"FFFFFFFF"};
	char testram[17] = {"01234567"};
	gpio15_output_conf();
	gpio15_output_set(1);
	externram = false;
	extramWrite(strlen(test), 0, testram);
	extramRead(strlen(test), 0, test);
	if (memcmp(test,testram,16) == 0)
		externram = true;
	if (externram)
		printf(PSTR("\nExternal ram detected%c"),0x0d);
	else
		printf(PSTR("\nExternal ram not detected%c"),0x0d);
}

uint32_t extramRead(uint32_t size, uint32_t address, uint8_t *buffer)
{
	uint32_t i = 0;
	spi_take_semaphore();
//	spi_clock(HSPI, 3, 2); //13MHz
	gpio15_output_set(0);
	SPIPutChar(0x03);
	SPIPutChar((address>>16)&0xFF);
	SPIPutChar((address>>8)&0xFF);
	SPIPutChar(address&0xFF);
	for(i = 0; i < size; i++) {
		buffer[i] = SPIGetChar();
	}
	gpio15_output_set(1);
//	spi_clock(HSPI, 4, 10); //2MHz
	spi_give_semaphore();
	return i;
}

uint32_t extramWrite(uint32_t size, uint32_t address, uint8_t *data)
{
	uint32_t i = 0;
	spi_take_semaphore();
	//spi_clock(HSPI, 3, 2); //13MHz
	gpio15_output_set(0);
	SPIPutChar(0x02);
	SPIPutChar((address>>16)&0xFF);
	SPIPutChar((address>>8)&0xFF);
	SPIPutChar(address&0xFF);
	for ( i = 0; i < size; i++ )
	{
		SPIPutChar(data[i]);
	}
	gpio15_output_set(1);
//	spi_clock(HSPI, 4, 10); //2MHz
	spi_give_semaphore();
	return i;
}

