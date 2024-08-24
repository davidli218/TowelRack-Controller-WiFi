#pragma once

#include "driver/gpio.h"
#include "esp_adc/adc_oneshot.h"

#define BSP_NTC_ADC_CHANNEL (ADC_CHANNEL_0)
#define BSP_NTC_ADC_UNIT (ADC_UNIT_1)

#define BSP_HEATING_CTRL_PORT (GPIO_NUM_1)

void bsp_heating_init(void);

void bsp_heating_deinit(void);

int bsp_heating_get_temp(void);

void bsp_heating_enable(void);

void bsp_heating_disable(void);
