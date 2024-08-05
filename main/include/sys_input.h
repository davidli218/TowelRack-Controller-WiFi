#ifndef TOWERRACK_CONTROLLER_SYS_INPUT_H
#define TOWERRACK_CONTROLLER_SYS_INPUT_H

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "iot_button.h"
#include "iot_knob.h"
#include "nvs_flash.h"

#define KNOB_PIN_A GPIO_NUM_0
#define KNOB_PIN_B GPIO_NUM_1

#define BUTTON_PIN GPIO_NUM_10

typedef enum {
    BUTTON_EVENT_SINGLE_CLICK = 0,
    BUTTON_EVENT_DOUBLE_CLICK,
    BUTTON_EVENT_LONG_PRESS,
    BUTTON_EVENT_PRESS_REPEAT,
    KNOB_EVENT_LEFT,
    KNOB_EVENT_RIGHT,
} sys_input_event_t;

void system_input_init(void);

#endif // TOWERRACK_CONTROLLER_SYS_INPUT_H
