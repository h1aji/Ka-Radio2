

#include "esp8266.h"
#include "espressif/esp_common.h"

#include <espressif/esp8266/eagle_soc.h>
#include <espressif/esp8266/gpio_register.h>
#include <espressif/esp8266/pin_mux_register.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include "c_types.h"
#include "spilib.h"

SemaphoreHandle_t sSPI = NULL;

ICACHE_FLASH_ATTR uint8_t spi_take_semaphore() {
  if (sSPI)
    if (xSemaphoreTake(sSPI, portMAX_DELAY)) return 1;
  return 0;
}

ICACHE_FLASH_ATTR void spi_give_semaphore() {
  if (sSPI) xSemaphoreGive(sSPI);
}

ICACHE_FLASH_ATTR void spi_speed_up() {
  // Set SPI clock to 10MHz
  WRITE_PERI_REG(SPI_CLOCK(SPI_BUS),
		((1&SPI_CLKDIV_PRE)<<SPI_CLKDIV_PRE_S)|
		((3&SPI_CLKCNT_N)<<SPI_CLKCNT_N_S)|
		((1&SPI_CLKCNT_H)<<SPI_CLKCNT_H_S)|
		((3&SPI_CLKCNT_L)<<SPI_CLKCNT_L_S));
}

ICACHE_FLASH_ATTR void spi_speed_down() {
  // Set SPI clock to 2MHz
  WRITE_PERI_REG(SPI_CLOCK(SPI_BUS),
		((9&SPI_CLKDIV_PRE)<<SPI_CLKDIV_PRE_S)|
		((3&SPI_CLKCNT_N)<<SPI_CLKCNT_N_S)|
		((1&SPI_CLKCNT_H)<<SPI_CLKCNT_H_S)|
		((3&SPI_CLKCNT_L)<<SPI_CLKCNT_L_S));
}

ICACHE_FLASH_ATTR void spi_put_char(uint8_t data) {
  while(READ_PERI_REG(SPI_CMD(SPI_BUS))&SPI_USR);	//wait for SPI to be ready

  CLEAR_PERI_REG_MASK(SPI_USER(SPI_BUS),SPI_USR_MOSI|SPI_USR_MISO|SPI_USR_COMMAND|SPI_USR_ADDR|SPI_USR_DUMMY);

  WRITE_PERI_REG(SPI_USER1(SPI_BUS), SPI_USR_ADDR_BITLEN<<SPI_USR_ADDR_BITLEN_S |		//Number of bits in Address
					(7&SPI_USR_MOSI_BITLEN)<<SPI_USR_MOSI_BITLEN_S |	//Number of bits to Send
					SPI_USR_MISO_BITLEN<<SPI_USR_MISO_BITLEN_S |		//Number of bits to Receive
					SPI_USR_DUMMY_CYCLELEN<<SPI_USR_DUMMY_CYCLELEN_S);	//Number of Dummy bits to insert

  SET_PERI_REG_MASK(SPI_USER(SPI_BUS),SPI_USR_MOSI); //enable MOSI function in SPI module

  if (READ_PERI_REG(SPI_USER(SPI_BUS))&SPI_WR_BYTE_ORDER) {
	WRITE_PERI_REG(SPI_W0(SPI_BUS),(uint32_t)data<<24);
  } else {
	WRITE_PERI_REG(SPI_W0(SPI_BUS),(uint32_t)data);
  }

  SET_PERI_REG_MASK(SPI_CMD(SPI_BUS),SPI_USR);
  while(READ_PERI_REG(SPI_CMD(SPI_BUS))&SPI_USR);
}

ICACHE_FLASH_ATTR uint8_t spi_get_char() {
  while(READ_PERI_REG(SPI_CMD(SPI_BUS))&SPI_USR);	//wait for SPI to be ready

  CLEAR_PERI_REG_MASK(SPI_USER(SPI_BUS),SPI_USR_MOSI|SPI_USR_MISO|SPI_USR_COMMAND|SPI_USR_ADDR|SPI_USR_DUMMY);
  SET_PERI_REG_MASK(SPI_USER(SPI_BUS),SPI_USR_MISO);

  WRITE_PERI_REG(SPI_USER1(SPI_BUS),SPI_USR_ADDR_BITLEN<<SPI_USR_ADDR_BITLEN_S |			//Number of bits in Address
					SPI_USR_MOSI_BITLEN<<SPI_USR_MOSI_BITLEN_S |		//Number of bits to Send
					(7&SPI_USR_MISO_BITLEN)<<SPI_USR_MISO_BITLEN_S |	//Number of bits to Receive
					SPI_USR_DUMMY_CYCLELEN<<SPI_USR_DUMMY_CYCLELEN_S);	//Number of Dummy bits to insert

  SET_PERI_REG_MASK(SPI_CMD(SPI_BUS),SPI_USR);
  while(READ_PERI_REG(SPI_CMD(SPI_BUS))&SPI_USR);
  if (READ_PERI_REG(SPI_USER(SPI_BUS))&SPI_RD_BYTE_ORDER) {
	return READ_PERI_REG(SPI_W0(SPI_BUS))>>24;	//assuming data in is written to MSB. TBC
  } else {
	return READ_PERI_REG(SPI_W0(SPI_BUS));
  }
}


