#pragma once

#include <stdbool.h>

#include "esp_err.h"

typedef struct {
    uint8_t magic;
    bool dev_adopted;
} sys_param_t;

esp_err_t settings_read_parameter_from_nvs(void);

esp_err_t settings_write_parameter_to_nvs(void);

bool settings_get_dev_adopted(void);

void settings_set_dev_adopted(void);
