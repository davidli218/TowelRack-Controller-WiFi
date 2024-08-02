#include "driver/gpio.h"
#include "esp_log.h"
#include "iot_knob.h"

#define KNOB_PIN_A GPIO_NUM_0
#define KNOB_PIN_B GPIO_NUM_1

void system_knob_init(void);
