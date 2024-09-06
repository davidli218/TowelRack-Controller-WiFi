#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"

typedef struct {
    gpio_num_t ds;
    gpio_num_t shcp;
    gpio_num_t stcp;
    gpio_num_t u1_ctrl;
    gpio_num_t u2_ctrl;
    uint8_t max_lens;
} display_config_t;

typedef void* display_device_handle_t;

void display_write_str(display_device_handle_t handle, const char* str);

void display_write_int(display_device_handle_t handle, int num);

void display_set_c_flag(display_device_handle_t handle, bool flag);

void display_set_h_flag(display_device_handle_t handle, bool flag);

void display_enable_all(display_device_handle_t handle);

void display_init(const display_config_t* config, display_device_handle_t* handle);
