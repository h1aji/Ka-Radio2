
#include "flash.h"
#include "esp/spi.h"
#include <espressif/esp_common.h>

#define SPI_BUS       1 //HSPI_HOST

ICACHE_FLASH_ATTR uint32_t flashRead (void *to, uint32_t fromaddr, uint32_t size )
{
  uint32_t ret;
  fromaddr -= INTERNAL_FLASH_START_ADDRESS;
//  int r;
//  printf("flasRead from %x, size: %d  TO:%x\n",fromaddr,size,to);
  WRITE_PERI_REG(0x60000914, 0x73);
  spi_take_semaphore();
  spi_set_frequency_div(SPI_BUS, SPI_FREQ_DIV_2M);
  ret = sdk_spi_flash_read(fromaddr, (uint32_t *)to, size);
  spi_give_semaphore();
  return ret;
}

ICACHE_FLASH_ATTR uint32_t flashWrite (void *data, uint32_t fromaddr, uint32_t size )
{
  uint32_t ret;
  fromaddr -= INTERNAL_FLASH_START_ADDRESS;
//  printf("flasWrite from %x, size: %d  Data: %x\n",fromaddr,size,data);
//  int r;
  WRITE_PERI_REG(0x60000914, 0x73);
  spi_take_semaphore();
  spi_set_frequency_div(SPI_BUS, SPI_FREQ_DIV_2M);
  ret = sdk_spi_flash_write(fromaddr, (uint32_t *)data, size);
  spi_give_semaphore();
  return ret;
}

