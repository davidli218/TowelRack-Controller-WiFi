#ifndef TOWERRACK_CONTROLLER_SYS_INPUT_H
#define TOWERRACK_CONTROLLER_SYS_INPUT_H

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "iot_button.h"
#include "iot_knob.h"
#include "nvs_flash.h"

#define KNOB_PIN_A GPIO_NUM_0
#define KNOB_PIN_B GPIO_NUM_1

#define BUTTON_PIN GPIO_NUM_10

void system_input_init(void);

#endif // TOWERRACK_CONTROLLER_SYS_INPUT_H
