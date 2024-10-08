/*
 ***********************************************************************************************************************
 * @file    VS1053.c
 * @author  Piotr Sperka
 * @date    07.08.2015
 * @brief   This file provides VS1053 usage and control functions. Based on VS1003 library by Przemyslaw Stasiak.
 * Copyright 2016 karawin (http://www.karawin.fr)
 * added control treble, bass and spacialisation
 ***********************************************************************************************************************
 */

#include "c_types.h"
#include <math.h>

#include "esp8266.h"
#include "espressif/esp_common.h"

#include <espressif/esp8266/eagle_soc.h>
#include <espressif/esp8266/gpio_register.h>
#include <espressif/esp8266/pin_mux_register.h>

#include <FreeRTOS.h>
#include <semphr.h>
#include <task.h>

#include "vs1053.h"
#include "eeprom.h"
#include "interface.h"


extern uint8_t clientIvol;

int vsVersion; // the version of the chip
//SS_VER is 0 for VS1001, 1 for VS1011, 2 for VS1002, 3 for VS1003, 4 for VS1053 and VS8053, 5 for VS1033, 7 for VS1103, and 6 for VS1063.
//char strvMODE[] ICACHE_RODATA_ATTR STORE_ATTR  = {"SCI_Mode (0x4800) = 0x%X\n"};
const char strvSTATUS[] ICACHE_RODATA_ATTR STORE_ATTR  = {"SCI_Status (0x48) = 0x%X\n"};
const char strvVERSION[] ICACHE_RODATA_ATTR STORE_ATTR  = {"VS Version (VS1053 is 4) = %d\n"};
const char strvCLOCK[] ICACHE_RODATA_ATTR STORE_ATTR  = {"SCI_ClockF = 0x%X\n"};
const char strvI2S[] ICACHE_RODATA_ATTR STORE_ATTR  = {"I2S Speed: %d\n"};

#define TMAX 4096

SemaphoreHandle_t sSPI = NULL;

ICACHE_FLASH_ATTR uint8_t spi_take_semaphore() {
  if (sSPI)
    if (xSemaphoreTake(sSPI, portMAX_DELAY)) return 1;
  return 0;
}

ICACHE_FLASH_ATTR void spi_give_semaphore() {
  if (sSPI) xSemaphoreGive(sSPI);
}

ICACHE_FLASH_ATTR void VS1053_HW_init() {
  printf(PSTR("\nVS1053 init%c"), 0x0d);

  WRITE_PERI_REG(PERIPHS_IO_MUX,0x105|(0<<9));			//Set bit 9 if 80MHz sysclock required
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTDI_U,FUNC_HSPIQ_MISO);	//GPIO12 is SPI_BUS MISO pin (Master Data In)
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTCK_U,FUNC_HSPID_MOSI);	//GPIO13 is SPI_BUS MOSI pin (Master Data Out)
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U,FUNC_HSPI_CLK);		//GPIO14 is SPI_BUS CLK pin (Clock)
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_GPIO0_U,FUNC_GPIO0);		//GPIO0  is set as CS pin (Chip Select / Slave Select)

  SET_PERI_REG_MASK(SPI_USER(SPI_BUS),SPI_WR_BYTE_ORDER);		//SPI TX Byte order High to Low
  SET_PERI_REG_MASK(SPI_USER(SPI_BUS),SPI_RD_BYTE_ORDER);		//SPI RX Byte order High to Low

  SET_PERI_REG_MASK(SPI_USER(SPI_BUS),SPI_CS_SETUP|SPI_CS_HOLD|SPI_USR_COMMAND);
  CLEAR_PERI_REG_MASK(SPI_USER(SPI_BUS),SPI_FLASH_MODE);

  // Enable GPIO 9 and 10 (DREQ and RST)
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA2_U, FUNC_GPIO9);
  PIN_FUNC_SELECT(PERIPHS_IO_MUX_SD_DATA3_U, FUNC_GPIO10);

  gpio_enable(DREQ_PIN, GPIO_INPUT);
  gpio_enable(RST_PIN, GPIO_OUTPUT);
  gpio_enable(DCS_PIN, GPIO_OUTPUT);
  gpio_enable(CS_PIN, GPIO_OUTPUT);

  // Set the CS and DCS pins high (idle state)
  gpio_write(CS_PIN, 1);
  gpio_write(DCS_PIN, 1);
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

ICACHE_FLASH_ATTR void Delay(uint32_t nTime) {
  unsigned int i;
  unsigned long j;
  for (i = nTime; i > 0; i--)
    for (j = 1000; j > 0; j--);
}

ICACHE_FLASH_ATTR void ControlReset(uint8_t State) {
  gpio_write(RST_PIN, State);
}

ICACHE_FLASH_ATTR void SCI_ChipSelect(uint8_t State) {
  gpio_write(CS_PIN, State);
}

ICACHE_FLASH_ATTR void SDI_ChipSelect(uint8_t State) {
  gpio_write(DCS_PIN, State);
}

ICACHE_FLASH_ATTR uint8_t VS1053_checkDREQ() {
  return gpio_read(DREQ_PIN);
}

ICACHE_FLASH_ATTR void WaitDREQ() {
  uint16_t  time_out = 0;
  while (gpio_read(DREQ_PIN) == 0 && time_out++ < TMAX)
  {
    ;
  }
}

ICACHE_FLASH_ATTR void VS1053_WriteRegister(uint8_t addressbyte, uint8_t highbyte, uint8_t lowbyte) {
  spi_take_semaphore();
  spi_speed_down();
  SDI_ChipSelect(RESET);
  WaitDREQ();
  SCI_ChipSelect(SET);
  spi_put_char(VS_WRITE_COMMAND);
  spi_put_char(addressbyte);
  spi_put_char(highbyte);
  spi_put_char(lowbyte);
  WaitDREQ();
  SCI_ChipSelect(RESET);
  spi_give_semaphore();
}

ICACHE_FLASH_ATTR uint16_t VS1053_ReadRegister(uint8_t addressbyte) {
  spi_take_semaphore();
  spi_speed_down();
  uint16_t result;
  SDI_ChipSelect(RESET);
  WaitDREQ();
  SCI_ChipSelect(SET);
  spi_put_char(VS_READ_COMMAND);
  spi_put_char(addressbyte);
  result = spi_get_char() << 8;
  result |= spi_get_char();
  WaitDREQ();
  SCI_ChipSelect(RESET);
  spi_give_semaphore();
  return result;
}

ICACHE_FLASH_ATTR void WriteVS10xxRegister(unsigned short addr, unsigned short val) {
  VS1053_WriteRegister((uint8_t) addr & 0xff, (uint8_t)((val & 0xFF00) >> 8), (uint8_t)(val & 0xFF));
}

ICACHE_FLASH_ATTR void VS1053_ResetChip() {
  ControlReset(SET);
  Delay(10);
  ControlReset(RESET);
  Delay(5);
  if (VS1053_checkDREQ() == 1) return;
  Delay(10);
}

ICACHE_FLASH_ATTR uint16_t MaskAndShiftRight(uint16_t Source, uint16_t Mask, uint16_t Shift) {
  return ((Source & Mask) >> Shift);
}

ICACHE_FLASH_ATTR void VS1053_regtest() {
  int MP3Status = VS1053_ReadRegister(SPI_STATUS);
  int MP3Mode = VS1053_ReadRegister(SPI_MODE);
  int MP3Clock = VS1053_ReadRegister(SPI_CLOCKF);
  printf(PSTR("SCI_Mode (0x4800) = 0x%X\n"), MP3Mode);
  printf(strvSTATUS, MP3Status);

  vsVersion = (MP3Status >> 4) & 0x000F; //Mask out only the four version bits
  printf(strvVERSION, vsVersion);
  //The VS1053B should respond with 4. VS1001 = 0, VS1011 = 1, VS1002 = 2, VS1003 = 3, VS1054 = 4
  printf(strvCLOCK, MP3Clock);
}

/*
void VS1053_PluginLoad()
{
  int i;
  for (i=0;i<CODE_SIZE;i++) {
    VS1053_WriteRegister(atab[i], (dtab[i]>>8), (dtab[i]&0xff));
  }
}
*/

ICACHE_FLASH_ATTR void VS1053_I2SRate(uint8_t speed) { // 0 = 48kHz, 1 = 96kHz, 2 = 128kHz
  if (speed > 2) speed = 0;
  if (vsVersion != 4) return;
//	VS1053_WriteRegister(SPI_WRAMADDR, 0xc0,0x40); //address of GPIO_ODATA is 0xC017
//	VS1053_WriteRegister(SPI_WRAM, 0x00,0x8); //reset I2S_CF_ENA
	VS1053_WriteRegister(SPI_WRAMADDR, 0xc0,0x40); //address of GPIO_ODATA is 0xC017
	VS1053_WriteRegister(SPI_WRAM, 0x00,0x8|speed); //
	VS1053_WriteRegister(SPI_WRAMADDR, 0xc0,0x40); //address of GPIO_ODATA is 0xC017
	VS1053_WriteRegister(SPI_WRAM, 0x00,0xC|speed); //
	printf(strvI2S,speed);
}

ICACHE_FLASH_ATTR void VS1053_DisableAnalog() {
  // disable analog output
  VS1053_WriteRegister(SPI_VOL, 0xFF, 0xFF);
}

// reduce the chip consumption
void VS1053_LowPower() {
  VS1053_WriteRegister(SPI_CLOCKF, 0x00, 0x00);
}

// normal chip consumption
void VS1053_HighPower() {
  if (vsVersion == 4) { // only 1053
    VS1053_WriteRegister(SPI_CLOCKF, 0xB8, 0x00); // SC_MULT = x1, SC_ADD= x1
  } else {
    VS1053_WriteRegister(SPI_CLOCKF, 0xb0, 0x00);
  }
}

// patch if GPIO1 is not wired to gnd
ICACHE_FLASH_ATTR void VS1053_GPIO1() {
  // these 4 lines makes board to run on mp3 mode, no soldering required anymore
  VS1053_WriteRegister(SPI_WRAMADDR, 0xc0, 0x17); //address of GPIO_DDR is 0xC017
  VS1053_WriteRegister(SPI_WRAM, 0x00, 0x03);     //GPIO_DDR=3
  VS1053_WriteRegister(SPI_WRAMADDR, 0xc0, 0x19); //address of GPIO_ODATA is 0xC019
  VS1053_WriteRegister(SPI_WRAM, 0x00, 0x00);     //GPIO_ODATA=0
  printf("SPI_AUDATA1 = %x\n", VS1053_ReadRegister(SPI_AUDATA));
}

// First VS10xx configuration after reset
ICACHE_FLASH_ATTR void VS1053_InitVS() {
  //vsVersion = (VS1053_ReadRegister(SPI_STATUS) >> 4) & 0x000F; //Mask out only the four version bits
  //0 for VS1001, 1 for VS1011, 2 for VS1002, 3 for VS1003, 4 for VS1053 and VS8053,
  //5 for VS1033, 7 for VS1103, and 6 for VS1063
  if (vsVersion == 4) // only 1053
  {
    //VS1053_WriteRegister(SPI_CLOCKF,0x78,0x00); // SC_MULT = x3, SC_ADD= x2
    VS1053_WriteRegister(SPI_CLOCKF, 0xB8, 0x00); // SC_MULT = x1, SC_ADD= x1
    //VS1053_WriteRegister(SPI_CLOCKF,0x90,0x00); // SC_MULT = x3.5, SC_ADD= x1.5
  } else {
	VS1053_WriteRegister(SPI_CLOCKF,0xb0,0x00);
  }

  VS1053_WriteRegister(SPI_MODE, (SM_SDINEW|SM_LINE1)>>8,SM_RESET);
  VS1053_WriteRegister(SPI_MODE, (SM_SDINEW|SM_LINE1)>>8, SM_LAYER12); //mode
  WaitDREQ();

  VS1053_regtest();

  // enable I2C dac output
  if (vsVersion == 4) // only 1053
  {
    VS1053_WriteRegister(SPI_WRAMADDR, 0xc0, 0x17); //
    VS1053_WriteRegister(SPI_WRAM, 0x00, 0xF0); //
    VS1053_I2SRate(0);
  }
}

ICACHE_FLASH_ATTR void VS1053_Start() {
  struct device_settings *device;

  VS1053_ResetChip();
  VS1053_GPIO1(); // patch if GPIO1 is not wired to gnd

  if (VS1053_ReadRegister(SPI_AUDATA) == 0xAC45) // still midi mode?
  {
    VS1053_ResetChip();
    VS1053_GPIO1();
  }

  vsVersion = (VS1053_ReadRegister(SPI_STATUS) >> 4) & 0x000F; //Mask out only the four
  device = getDeviceSettings();
  printf(PSTR("device: %x\n"), device);
  if (device != NULL) {
      if ((vsVersion == 4) && ((device->options&T_PATCH) == 0)) {
      /*The patch must be re-loaded after each
                          hardware or software reset. If you replace software reset by writing 0x50 to AIADDR,
                          you do not need to reload the patch.*/
      LoadUserCodes(); // vs1053b patch and admix
      printf("SPI_AUDATA2 = %x\n", VS1053_ReadRegister(SPI_AUDATA));
      if (VS1053_ReadRegister(SPI_AUDATA) == 0xAC45) //midi mode?
      {
        VS1053_WriteRegister(SPI_AIADDR, 0x00, 0x50); // reset soft but let  patch loaded
        VS1053_GPIO1(); // patch if GPIO1 is not wired to gnd
        if (VS1053_ReadRegister(SPI_AUDATA) == 0xAC45) // in midi mode
        {
          printf(PSTR("midi mode on\n"));
          device->options |= T_PATCH; // force no patch
          saveDeviceSettings(device);
          sdk_system_restart();
        }
      }
      VS1053_InitVS();
      VS1053_Admix(false);
    } else // no patch
    {
      VS1053_InitVS();
    }
    VS1053_DisableAnalog();
    VS1053_SetVolumeLine(-31);
    Delay(1);
		printf(PSTR("volume: %d\n"),device->vol);
		clientIvol = device->vol;
		VS1053_SetVolume(device->vol);
		VS1053_SetTreble(device->treble);
		VS1053_SetBass(device->bass);
		VS1053_SetTrebleFreq(device->freqtreble);
		VS1053_SetBassFreq(device->freqbass);
		VS1053_SetSpatial(device->spacial);
    free(device);
  }
}

ICACHE_FLASH_ATTR int VS1053_SendMusicBytes(uint8_t* music, uint16_t quantity) {
  if (quantity == 0) return 0;
  spi_take_semaphore();
  int o = 0;
  while (VS1053_checkDREQ() == 0) {
    vTaskDelay(1);
  }
  spi_speed_up();
  SDI_ChipSelect(SET);
  while (quantity) {
    if (VS1053_checkDREQ()) {
      int t = quantity;
      int k;
      if (t > 32) t = 32;
      for (k = o; k < o + t; k++) {
        spi_put_char(music[k]);
      }
      o += t;
      quantity -= t;
    }
  }
  SDI_ChipSelect(RESET);
  spi_speed_down();
  spi_give_semaphore();
  return o;
}

ICACHE_FLASH_ATTR void VS1053_SoftwareReset() {
  VS1053_WriteRegister(SPI_MODE, (SM_SDINEW|SM_LINE1)>>8,SM_RESET);
  VS1053_WriteRegister(SPI_MODE, (SM_SDINEW|SM_LINE1)>>8, SM_LAYER12); //mode
}

// Set the volume of the line1 (for admix plugin) // -31 to -3
ICACHE_FLASH_ATTR void VS1053_SetVolumeLine(int16_t vol) {
  if (vol > -3) vol = -3;
  if (vol < -31) vol = -31;
  VS1053_WriteRegister(SPI_AICTRL0,(vol&0xFF00)>>8,vol&0xFF);
}

// activate or stop admix plugin (true = activate)
ICACHE_FLASH_ATTR void VS1053_Admix(bool val) {
  uint16_t Mode = VS1053_ReadRegister(SPI_MODE);
  VS1053_WriteRegister(SPI_MODE, MaskAndShiftRight(Mode|SM_LINE1,0xFF00,8), (Mode & 0x00FF));
  if (val) {
    VS1053_WriteRegister(SPI_AIADDR, 0x0F, 0);
  } else {
    VS1053_WriteRegister(SPI_AIADDR, 0x0F, 1);
  }
}

ICACHE_FLASH_ATTR uint8_t VS1053_GetVolume() {
  uint8_t i, j;
  uint8_t value = VS1053_ReadRegister(SPI_VOL) & 0x00FF;
  for (i = 0; i < 255; i++) {
  j = (log10(255/((float)i+1)) * 105.54571334);
    //		printf("i=%d  j=%d value=%d\n",i,j,value);
    if (value == j) {
      return i;
    }
  }
  return 127;
}

ICACHE_FLASH_ATTR uint8_t VS1053_GetVolumeLinear() {
  return VS1053_ReadRegister(SPI_VOL) & 0x00FF;
}

/**
 * Function sets the same volume level to both channels.
 * @param xMinusHalfdB describes damping level as a multiple
 * of 0.5dB. Maximum volume is 0 and silence is 0xFEFE.
 */
ICACHE_FLASH_ATTR void VS1053_SetVolume(uint8_t xMinusHalfdB) {
  uint8_t value = (log10(255/((float)xMinusHalfdB+1)) * 105.54571334);
  //printf("setvol: %d\n",value);
  if (value == 255) value = 254;
  //printf("xMinusHalfdB=%d  value=%d\n",xMinusHalfdB,value);
  VS1053_WriteRegister(SPI_VOL, value, value);
}

/**
 * Functions returns level of treble enhancement.
 * @return Returned value describes enhancement in multiplies
 * of 1.5dB. 0 value means no enhancement, 8 max (12dB).
 */
ICACHE_FLASH_ATTR int8_t VS1053_GetTreble() {
	int8_t  treble = (VS1053_ReadRegister(SPI_BASS) & 0xF000) >> 12;
	if ( (treble&0x08)) treble |= 0xF0; // negative value
	return ( treble);
}

/**
 * Sets treble level.
 * @note If xOneAndHalfdB is greater than max value, sets treble
 * to maximum.
 * @param xOneAndHalfdB describes level of enhancement. It is a multiplier
 * of 1.5dB. 0 - no enhancement, -8 minimum -12dB , 7 - maximum, 10.5dB.
 * @return void
 */
ICACHE_FLASH_ATTR void VS1053_SetTreble(int8_t xOneAndHalfdB) {
  uint16_t bassReg = VS1053_ReadRegister(SPI_BASS);
  if ((xOneAndHalfdB <= 7) && (xOneAndHalfdB >=-8)) {
    VS1053_WriteRegister(SPI_BASS, MaskAndShiftRight(bassReg,0x0F00,8) | (xOneAndHalfdB << 4), bassReg & 0x00FF);
  }
}

/**
 * Sets low limit frequency of treble enhancer.
 * @note new frequency is set only if argument is valid.
 * @param xkHz The lowest frequency enhanced by treble enhancer.
 * Values from 0 to 15 (in kHz)
 * @return void
 */
ICACHE_FLASH_ATTR void VS1053_SetTrebleFreq(uint8_t xkHz) {
  uint16_t bassReg = VS1053_ReadRegister(SPI_BASS);
  if (xkHz <= 15) {
    VS1053_WriteRegister( SPI_BASS, MaskAndShiftRight(bassReg,0xF000,8) | xkHz, bassReg & 0x00FF );
  }
}

ICACHE_FLASH_ATTR int8_t VS1053_GetTrebleFreq() {
  return ((VS1053_ReadRegister(SPI_BASS) & 0x0F00) >> 8);
}

/**
 * Returns level of bass boost in dB.
 * @return Value of bass enhancement from 0 (off) to 15(dB).
 */
ICACHE_FLASH_ATTR uint8_t VS1053_GetBass() {
  return ((VS1053_ReadRegister(SPI_BASS) & 0x00F0) >> 4);
}

/**
 * Sets bass enhancement level (in dB).
 * @note If xdB is greater than max value, bass enhancement is set to its max (15dB).
 * @param xdB Value of bass enhancement from 0 (off) to 15(dB).
 * @return void
 */
ICACHE_FLASH_ATTR void VS1053_SetBass(uint8_t xdB) {
  uint16_t bassReg = VS1053_ReadRegister(SPI_BASS);
  if (xdB <= 15)
    VS1053_WriteRegister(SPI_BASS, (bassReg & 0xFF00) >> 8, (bassReg & 0x000F) | (xdB << 4));
  else
    VS1053_WriteRegister(SPI_BASS, (bassReg & 0xFF00) >> 8, (bassReg & 0x000F) | 0xF0);
}

/**
 * Sets low limit frequency of bass enhancer.
 * @note new frequency is set only if argument is valid.
 * @param xTenHz The lowest frequency enhanced by bass enhancer.
 * Values from 2 to 15 ( equal to 20 - 150 Hz).
 * @return void
 */
ICACHE_FLASH_ATTR void VS1053_SetBassFreq(uint8_t xTenHz) {
  uint16_t bassReg = VS1053_ReadRegister(SPI_BASS);
  if (xTenHz >=2 && xTenHz <= 15) {
    VS1053_WriteRegister(SPI_BASS, MaskAndShiftRight(bassReg,0xFF00,8), (bassReg & 0x00F0) | xTenHz);
  }
}

ICACHE_FLASH_ATTR uint8_t VS1053_GetBassFreq() {
  return ((VS1053_ReadRegister(SPI_BASS) & 0x000F));
}

ICACHE_FLASH_ATTR uint8_t VS1053_GetSpatial() {
  if (vsVersion != 4) return 0;
  uint16_t spatial = (VS1053_ReadRegister(SPI_MODE) & 0x0090) >>4;
  //printf("GetSpatial: %d\n",(spatial&1) | ((spatial>>2) & 2));
  return ((spatial&1) | ((spatial>>2) & 2));
}

ICACHE_FLASH_ATTR void VS1053_SetSpatial(uint8_t num) {
  if (vsVersion != 4) return ;
  uint16_t spatial = VS1053_ReadRegister(SPI_MODE);
  if (num >=0 && num <= 3) {
    num = (((num <<2)&8) | (num&1))<<4;
    VS1053_WriteRegister(SPI_MODE, MaskAndShiftRight(spatial,0xFF00,8), (spatial & 0x006F) | num );
  }
}

ICACHE_FLASH_ATTR uint16_t VS1053_GetDecodeTime() {
  return VS1053_ReadRegister(SPI_DECODE_TIME);
}

ICACHE_FLASH_ATTR uint16_t VS1053_GetBitrate() {
  uint16_t bitrate = (VS1053_ReadRegister(SPI_HDAT0) & 0xf000) >> 12;
  uint8_t ID = (VS1053_ReadRegister(SPI_HDAT1) & 0x18) >> 3;
  uint16_t res;
	if (ID == 3)
	{	res = 32;
		while(bitrate>13)
		{
			res+=64;
			bitrate--;
		}
		while (bitrate>9)
		{
			res+=32;
			bitrate--;
		}
		while (bitrate>5)
		{
			res+=16;
			bitrate--;
		}
		while (bitrate>1)
		{
			res+=8;
			bitrate--;
		}
	}
	else
	{
	    res = 8;
		while (bitrate>8)
		{
			res+=16;
			bitrate--;
		}
		while (bitrate>1)
		{
			res+=8;
			bitrate--;
		}
	}
	return res;
}

ICACHE_FLASH_ATTR uint16_t VS1053_GetSampleRate() {
  return (VS1053_ReadRegister(SPI_AUDATA) & 0xFFFE);
}

/* to start and stop a new stream */
ICACHE_FLASH_ATTR void VS1053_flush_cancel() {
  int8_t endFillByte;
  int16_t y;
  uint8_t buf[33];
  // set spimode with SM_CANCEL
  uint16_t spimode = VS1053_ReadRegister(SPI_MODE)|SM_CANCEL;
  // set CANCEL
  VS1053_WriteRegister(SPI_MODE, MaskAndShiftRight(spimode, 0xFF00, 8), (spimode & 0x00FF));
  // wait CANCEL
  VS1053_WriteRegister(SPI_WRAMADDR, MaskAndShiftRight(para_endFillByte, 0xFF00, 8), (para_endFillByte & 0x00FF));
  endFillByte = (int8_t)(VS1053_ReadRegister(SPI_WRAM) & 0xFF);
  for (y = 0; y < 32; y++) buf[y] = endFillByte;
  y = 0;
  while (VS1053_ReadRegister(SPI_MODE)&SM_CANCEL) {
    VS1053_SendMusicBytes(buf, 32);
    if (y++ > 64) {
      printf("VS1053 Reset\n");
      //VS1053_Start();
      break;
    }
  }
  for (y = 0; y < 64; y++)
    VS1053_SendMusicBytes(buf, 32); //2080 bytes
}

