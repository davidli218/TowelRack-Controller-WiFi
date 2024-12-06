#include <stdlib.h>
#include <string.h>

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "freertos/FreeRTOS.h"

#include "ic_74hc595_driver.h"

#include "bsp/seg_display_driver.h"

typedef struct {
    ic_74hc595_handle_t ic_74_hc595_handle;
    gpio_num_t u1_ctrl;
    gpio_num_t u2_ctrl;

    uint8_t max_lens; // 数码管最大显示字符数

    bool status;     // 显示是否开启
    bool c_flag;     // 是否显示C标志
    bool h_flag;     // 是否显示H标志
    char* content;   // 显示内容
    uint8_t* buffer; // 显示缓冲区

    gptimer_handle_t gptimer; // LED数码管刷新定时器句柄
} display_driver_dev_t;

/**
* @brief 获取字符对应的数码管段亮灯配置
 *
 * @param character 目标字符
 * @return 数码管段亮灯配置。低7位从高到低分别对应G-F-E-D-C-B-A
 */
static uint8_t display_get_segment_pattern(const char character) {
    switch (character) {
        /* G - F - E - D - C - B - A */
        case '1': return 0b0000110;
        case '2': return 0b1011011;
        case '3': return 0b1001111;
        case '4': return 0b1100110;
        case '5': return 0b1101101;
        case '6': return 0b1111101;
        case '7': return 0b0000111;
        case '8': return 0b1111111;
        case '9': return 0b1101111;
        case '0': return 0b0111111;
        case 'E': return 0b1111001;
        default:
            return 0b0000000;
    }
}

/**
 * @brief 关闭所有数码管显示
 */
static void display_disable_output(const display_driver_dev_t* dev) {
    gpio_set_level(dev->u1_ctrl, 1);
    gpio_set_level(dev->u2_ctrl, 1);
}

/**
 * @brief 打开1号数码管显示
 */
static void display_enable_u1(const display_driver_dev_t* dev) {
    gpio_set_level(dev->u1_ctrl, 0);
}

/**
 * @brief 打开2号数码管
 */
static void display_enable_u2(const display_driver_dev_t* dev) {
    gpio_set_level(dev->u2_ctrl, 0);
}

/**
 * @brief 刷新显示缓冲区
 */
static void display_flush_buffer(const display_driver_dev_t* dev) {
    memset(dev->buffer, 0, sizeof(uint8_t) * dev->max_lens);

    for (int i = 0; i < strlen(dev->content); i++) {
        dev->buffer[i] = display_get_segment_pattern(dev->content[i]) << 1;
    }

    if (dev->c_flag) { dev->buffer[0] |= 0x01; }
    if (dev->h_flag) { dev->buffer[1] |= 0x01; }
}

/**
 * @brief 清空74HC595 & 清空显示上下文变量
 */
static void display_clear_all(display_driver_dev_t* dev) {
    /* 1. 清空显示状态变量 */
    dev->c_flag = false;
    dev->h_flag = false;
    memset(dev->content, 0, sizeof(char) * (dev->max_lens + 1));
    memset(dev->buffer, 0, sizeof(uint8_t) * dev->max_lens);

    /* 2. 清空74HC595 */
    ic_74hc595_reset(dev->ic_74_hc595_handle);
}

static void display_pause(display_driver_dev_t* dev) {
    if (dev->status) { ESP_ERROR_CHECK(gptimer_stop(dev->gptimer)); }

    display_disable_output(dev);
    display_clear_all(dev);

    dev->status = false;
}

static void display_resume(display_driver_dev_t* dev) {
    if (dev->status) return;

    ESP_ERROR_CHECK(gptimer_start(dev->gptimer));

    dev->status = true;
}

/**
 * @brief (数码管刷新定时器回调函数) 利用显示缓冲区刷新数码管
 */
// ReSharper disable once CppDFAConstantFunctionResult
static bool IRAM_ATTR display_refresh_timer_cb(
    gptimer_handle_t timer, const gptimer_alarm_event_data_t* event, void* user_data
) {
    static int current_digit;

    const display_driver_dev_t* dev = (display_driver_dev_t*)user_data;

    if (current_digit >= dev->max_lens) { current_digit = 0; }

    ic_74hc595_write(dev->ic_74_hc595_handle, dev->buffer[current_digit]);
    display_disable_output(dev);
    ic_74hc595_latch(dev->ic_74_hc595_handle);
    current_digit == 0 ? display_enable_u1(dev) : display_enable_u2(dev);

    current_digit++;

    return pdFALSE;
}

void display_write_str(const display_device_handle_t handle, const char* str) {
    display_driver_dev_t* dev = handle;

    if (str == NULL || strlen(str) == 0) {
        display_pause(dev);
        return;
    }

    display_resume(dev);

    strncpy(dev->content, str, dev->max_lens);

    if (strlen(str) < dev->max_lens) {
        dev->content[strlen(str)] = '\0';
    } else {
        dev->content[dev->max_lens] = '\0';
    }

    display_flush_buffer(dev);
}

void display_write_int(const display_device_handle_t handle, const int num) {
    display_driver_dev_t* dev = handle;

    char str[dev->max_lens + 1];

    snprintf(str, sizeof(str), "%d", num);

    display_write_str(handle, str);
}

void display_set_c_flag(const display_device_handle_t handle, const bool flag) {
    display_driver_dev_t* dev = handle;
    dev->c_flag = flag;
}

void display_set_h_flag(const display_device_handle_t handle, const bool flag) {
    display_driver_dev_t* dev = handle;
    dev->h_flag = flag;
}

void display_enable_all(const display_device_handle_t handle) {
    display_driver_dev_t* dev = handle;

    if (dev->status) display_pause(dev);

    ic_74hc595_write(dev->ic_74_hc595_handle, 0xFF);
    ic_74hc595_latch(dev->ic_74_hc595_handle);
    display_enable_u1(dev);
    display_enable_u2(dev);
}

void display_init(const display_config_t* config, display_device_handle_t* handle) {
    *handle = NULL;

    if (config == NULL) return;

    display_driver_dev_t* dev = malloc(sizeof(display_driver_dev_t));
    if (dev == NULL) return;

    const ic_74hc595_config_t ic_config = {
        .ds = config->ds,
        .shcp = config->shcp,
        .stcp = config->stcp,
        .oe_ = GPIO_NUM_NC,
        .mr_ = GPIO_NUM_NC,
    };
    ESP_ERROR_CHECK(ic_74hc595_init(&ic_config, &dev->ic_74_hc595_handle));

    dev->u1_ctrl = config->u1_ctrl;
    dev->u2_ctrl = config->u2_ctrl;
    dev->max_lens = config->max_lens;
    dev->content = malloc(sizeof(char) * (config->max_lens + 1));
    dev->buffer = malloc(sizeof(uint8_t) * config->max_lens);
    dev->gptimer = NULL;

    // 初始化GPIO引脚
    const gpio_config_t io_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << dev->u1_ctrl | 1ULL << dev->u2_ctrl,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .pull_up_en = GPIO_PULLUP_DISABLE,
    };
    gpio_config(&io_config);

    // 初始化全局变量与74HC595
    display_disable_output(dev);
    display_clear_all(dev);
    dev->status = false;

    // 初始化数码管刷新定时器
    const gptimer_config_t gptimer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 100000, // 100kHz
    };
    const gptimer_event_callbacks_t gptimer_callbacks = {
        .on_alarm = display_refresh_timer_cb,
    };
    const gptimer_alarm_config_t alarm_config = {
        .alarm_count = 100, // 1ms
        .flags.auto_reload_on_alarm = true,
    };

    // 开启数码管刷新定时器
    ESP_ERROR_CHECK(gptimer_new_timer(&gptimer_config, &dev->gptimer));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(dev->gptimer, &gptimer_callbacks, dev));
    ESP_ERROR_CHECK(gptimer_enable(dev->gptimer));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(dev->gptimer, &alarm_config));

    *handle = dev;
}
