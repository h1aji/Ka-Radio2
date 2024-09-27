/*
 * Copyright 2016 Piotr Sperka (http://www.piotrsperka.info)
 */

#include <string.h>
#include <stdio.h>

#include <espressif/esp8266/eagle_soc.h>
#include <espressif/esp8266/gpio_register.h>
#include <espressif/esp8266/pin_mux_register.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include "extram.h"
#include "buffer.h"
#include "interface.h"
#include "vs1053.h"

void extramInit() {
  bool externram   = false;
  char test[17]    = {"FFFFFFFF"};
  char testram[17] = {"01234567"};

  gpio_enable(SRAM_CS, GPIO_OUTPUT);
  gpio_write(SRAM_CS, 1);

  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDO_U, FUNC_GPIO15); //GPIO15 is HSPI CS pin (Chip Select / Slave Select)

  spi_speed_down();

  extramWrite(strlen(test), 0, testram);
  extramRead(strlen(test), 0, test);

  if (memcmp(test, testram, 16) == 0) {
    externram = true;
    printf(PSTR("\nSPIRAM detected%c"), 0x0d);
  } else {
    printf(PSTR("\nSPIRAM not detected%c"), 0x0d);
  }
}

uint32_t extramRead(uint32_t size, uint32_t address, uint8_t *buffer) {
  uint32_t i = 0;
  spi_take_semaphore();
  spi_speed_up();
  gpio_write(SRAM_CS, 0);
  spi_put_char(0x03);
  spi_put_char((address >> 16) & 0xFF);
  spi_put_char((address >> 8) & 0xFF);
  spi_put_char(address & 0xFF);
  for (i = 0; i < size; i++) {
    buffer[i] = spi_get_char();
  }
  gpio_write(SRAM_CS, 1);
//  spi_speed_down();
  spi_give_semaphore();
  return i;
}

uint32_t extramWrite(uint32_t size, uint32_t address, uint8_t *data) {
  uint32_t i = 0;
  spi_take_semaphore();
  spi_speed_up();
  gpio_write(SRAM_CS, 0);
  spi_put_char(0x02);
  spi_put_char((address >> 16) & 0xFF);
  spi_put_char((address >> 8) & 0xFF);
  spi_put_char(address & 0xFF);
  for (i = 0; i < size; i++) {
    spi_put_char(data[i]);
  }
  gpio_write(SRAM_CS, 1);
//  spi_speed_down();
  spi_give_semaphore();
  return i;
}

