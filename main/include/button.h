#include "esp_log.h"
#include "esp_system.h"
#include "iot_button.h"
#include "nvs_flash.h"

#define BUTTON_PIN GPIO_NUM_10

void system_button_init(void);
