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

typedef enum {
    USER_UNPAIRED = 0x00,
    USER_PAIRED = 0xFF,
} user_paired_status;

void system_wifi_init(void);

#endif // TOWERRACK_CONTROLLER_WIFI_H
