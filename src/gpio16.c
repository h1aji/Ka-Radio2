
#include <espressif/esp_common.h>
#include "portmacro.h"
#include "gpio.h"
#include "c_types.h"

extern  uint32_t PIN_OUT;
extern  uint32_t PIN_OUT_SET;
extern  uint32_t PIN_OUT_CLEAR;
extern  uint32_t PIN_DIR;
extern  uint32_t PIN_DIR_OUTPUT;
extern  uint32_t PIN_DIR_INPUT;

#define PERIPHS_IO_MUX_GPIO2_U          (PERIPHS_IO_MUX + 0x38)
#define FUNC_GPIO2                      0

void ICACHE_FLASH_ATTR gpio2_output_conf(void)
{
    PIN_DIR_OUTPUT = (1<<GPIO2);
    PIN_OUT_SET = (1<<GPIO2);
}

void ICACHE_FLASH_ATTR gpio2_output_set(uint8_t value)
{
    if (value == 0)
            PIN_OUT_CLEAR = (1<<GPIO2);
    else
            PIN_OUT_SET = (1<<GPIO2);
}

void ICACHE_FLASH_ATTR gpio16_output_conf(void)
{
    WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
                   (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32_t)0x1); // mux configuration for XPD_DCDC to output rtc_gpio0

    WRITE_PERI_REG(RTC_GPIO_CONF,
                   (READ_PERI_REG(RTC_GPIO_CONF) & (uint32_t)0xfffffffe) | (uint32_t)0x0); //mux configuration for out enable

    WRITE_PERI_REG(RTC_GPIO_ENABLE,
                   (READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32_t)0xfffffffe) | (uint32_t)0x1); //out enable
}

void ICACHE_FLASH_ATTR gpio16_output_set(uint8_t value)
{
    WRITE_PERI_REG(RTC_GPIO_OUT,
                   (READ_PERI_REG(RTC_GPIO_OUT) & (uint32_t)0xfffffffe) | (uint32_t)(value & 1));
}

void ICACHE_FLASH_ATTR gpio16_input_conf(void)
{
    WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
                   (READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32_t)0x1); // mux configuration for XPD_DCDC and rtc_gpio0 connection

    WRITE_PERI_REG(RTC_GPIO_CONF,
                   (READ_PERI_REG(RTC_GPIO_CONF) & (uint32_t)0xfffffffe) | (uint32_t)0x0); //mux configuration for out enable

    WRITE_PERI_REG(RTC_GPIO_ENABLE,
                   READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32_t)0xfffffffe); //out disable
}

uint8_t ICACHE_FLASH_ATTR gpio16_input_get(void)
{
    return (uint8_t)(READ_PERI_REG(RTC_GPIO_IN_DATA) & 1);
}
