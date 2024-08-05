#ifndef TOWERRACK_CONTROLLER_WIFI_H
#define TOWERRACK_CONTROLLER_WIFI_H

#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_smartconfig.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include <string.h>

void system_wifi_init(void);

#endif // TOWERRACK_CONTROLLER_WIFI_H
