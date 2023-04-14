/*
 * Copyright 2016 Piotr Sperka (http://www.piotrsperka.info)
 */

#include <string.h>
#include <stdio.h>
#include "esp/spi.h"

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include "extram.h"
#include "buffer.h"
#include "interface.h"
#include "vs1053.h"

void extramInit() {
  char test[17] = {
    "FFFFFFFF"
  };
  char testram[17] = {
    "01234567"
  };

  gpio_enable(SRAM_CS, GPIO_OUTPUT);
  spi_init(SPI_BUS, SPI_MODE0, SPI_FREQ_DIV_20M, true, SPI_LITTLE_ENDIAN, false);
  gpio_write(SRAM_CS, 0);

  externram = false;
  extramWrite(strlen(test), 0, testram);
  extramRead(strlen(test), 0, test);

  if (memcmp(test, testram, 16) == 0)
    externram = true;
  if (externram)
    printf(PSTR("\nExternal ram detected%c"), 0x0d);
  else
    printf(PSTR("\nExternal ram not detected%c"), 0x0d);

  gpio_write(SRAM_CS, 1);
}

uint32_t extramRead(uint32_t size, uint32_t address, uint8_t *buffer) {
  uint32_t i = 0;
  //	spi_clock(HSPI, 3, 2); //13MHz
  gpio_write(SRAM_CS, 0);

  SPIPutChar(0x03);
  SPIPutChar((address >> 16) & 0xFF);
  SPIPutChar((address >> 8) & 0xFF);
  SPIPutChar(address & 0xFF);
  for (i = 0; i < size; i++) {
    buffer[i] = SPIGetChar();
  }

  gpio_write(SRAM_CS, 1);
  //	spi_clock(HSPI, 4, 10); //2MHz
  return i;
}

uint32_t extramWrite(uint32_t size, uint32_t address, uint8_t *data) {
  uint32_t i = 0;
  //spi_clock(HSPI, 3, 2); //13MHz
  gpio_write(SRAM_CS, 0);

  SPIPutChar(0x02);
  SPIPutChar((address >> 16) & 0xFF);
  SPIPutChar((address >> 8) & 0xFF);
  SPIPutChar(address & 0xFF);
  for (i = 0; i < size; i++) {
    SPIPutChar(data[i]);
  }

  gpio_write(SRAM_CS, 1);
  //	spi_clock(HSPI, 4, 10); //2MHz
  return i;
}

