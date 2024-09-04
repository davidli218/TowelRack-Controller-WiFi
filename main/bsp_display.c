#include <string.h>

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "bsp_display.h"

__unused static const char* TAG = "bsp_display";

static struct {
    bool status;                         // 显示是否开启
    bool c_flag;                         // 是否显示C标志
    bool h_flag;                         // 是否显示H标志
    char content[BSP_DISP_MAX_CHAR + 1]; // 显示内容
    uint8_t buffer[BSP_DISP_MAX_CHAR];   // 显示缓冲区
} display_context = {
    .status = false,
    .c_flag = false,
    .h_flag = false,
    .content = {0},
    .buffer = {0},
};

static gptimer_handle_t display_gptimer_handle; // LED数码管刷新定时器句柄

/**
 * @brief 关闭所有数码管显示
 */
static void display_disable_output() {
    gpio_set_level(BSP_DISP_U1_CTRL, 1);
    gpio_set_level(BSP_DISP_U2_CTRL, 1);
}

/**
 * @brief 打开1号数码管显示
 */
static void display_enable_u1() { gpio_set_level(BSP_DISP_U1_CTRL, 0); }

/**
 * @brief 打开2号数码管
 */
static void display_enable_u2() { gpio_set_level(BSP_DISP_U2_CTRL, 0); }

/**
 * @brief 获取字符对应的数码管段亮灯配置
 *
 * @param character 目标字符
 * @return 数码管段亮灯配置。低7位从高到低为[A-G]
 */
static uint8_t display_get_segment_pattern(const char character) {
    switch (character) {
        /* G F E D C B A */
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
 * @brief 向74HC595发送一个字节
 */
static void display_74hc595_send_byte(uint8_t data) {
    for (int i = 0; i < 8; i++) {
        gpio_set_level(BSP_DISP_IC_DS, (data & 0x80) ? 1 : 0);
        data <<= 1;
        gpio_set_level(BSP_DISP_IC_SHCP, 0);
        gpio_set_level(BSP_DISP_IC_SHCP, 1);
    }
}

/**
 * @brief 刷新74HC595输出
 */
static void display_74hc595_toggle_output(void) {
    gpio_set_level(BSP_DISP_IC_STCP, 0);
    gpio_set_level(BSP_DISP_IC_STCP, 1);
}

/**
 * @brief 刷新显示缓冲区
 */
static void display_flush_buffer(void) {
    memset(display_context.buffer, 0, sizeof(display_context.buffer));

    for (int i = 0; i < strlen(display_context.content); i++) {
        display_context.buffer[i] = display_get_segment_pattern(display_context.content[i]) << 1;
    }

    if (display_context.c_flag) { display_context.buffer[0] |= 0x01; }
    if (display_context.h_flag) { display_context.buffer[1] |= 0x01; }
}

/**
 * @brief 清空74HC595 & 清空显示上下文变量
 */
static void display_clear_all(void) {
    /* 1. 清空显示状态变量 */
    display_context.c_flag = false;
    display_context.h_flag = false;
    memset(display_context.content, 0, sizeof(display_context.content));
    memset(display_context.buffer, 0, sizeof(display_context.buffer));

    /* 2. 清空74HC595 */
    display_74hc595_send_byte(0);
    display_74hc595_toggle_output();
}

static void display_pause(void) {
    if (!display_context.status) return;

    ESP_ERROR_CHECK(gptimer_stop(display_gptimer_handle));
    display_disable_output();
    display_clear_all();

    display_context.status = false;
}

static void display_resume(void) {
    if (display_context.status) return;

    ESP_ERROR_CHECK(gptimer_start(display_gptimer_handle));

    display_context.status = true;
}

/**
 * @brief (数码管刷新定时器回调函数) 利用显示缓冲区刷新数码管
 */
// ReSharper disable once CppDFAConstantFunctionResult
static bool IRAM_ATTR display_refresh_timer_cb(
    gptimer_handle_t timer, const gptimer_alarm_event_data_t* event, void* user_data
) {
    static int current_digit;

    if (current_digit >= BSP_DISP_MAX_CHAR) current_digit = 0;

    display_74hc595_send_byte(display_context.buffer[current_digit]);
    display_disable_output();
    display_74hc595_toggle_output();
    current_digit == 0 ? display_enable_u1() : display_enable_u2();

    current_digit++;

    return pdFALSE;
}

void bsp_display_write_str(const char* str) {
    if (str == NULL || strlen(str) == 0) {
        display_pause();
        return;
    }

    display_resume();

    if (strlen(str) > BSP_DISP_MAX_CHAR) {
        ESP_LOGW(TAG, "Display content overflow, truncating display content");
    }

    strncpy(display_context.content, str, BSP_DISP_MAX_CHAR);

    if (strlen(str) < BSP_DISP_MAX_CHAR) {
        display_context.content[strlen(str)] = '\0';
    } else {
        display_context.content[BSP_DISP_MAX_CHAR] = '\0';
    }

    display_flush_buffer();
}

void bsp_display_write_int(const int num) {
    char str[BSP_DISP_MAX_CHAR + 1];

    snprintf(str, sizeof(str), "%d", num);

    bsp_display_write_str(str);
}

void bsp_display_set_c_flag(const bool flag) { display_context.c_flag = flag; }

void bsp_display_set_h_flag(const bool flag) { display_context.h_flag = flag; }

void bsp_display_init(void) {
    // 初始化74HC595与数码管控制IO
    const gpio_config_t io_config = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << BSP_DISP_IC_DS | 1ULL << BSP_DISP_IC_SHCP | 1ULL << BSP_DISP_IC_STCP |
                        1ULL << BSP_DISP_U1_CTRL | 1ULL << BSP_DISP_U2_CTRL,
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_config);

    // 初始化全局变量与74HC595
    display_disable_output();
    display_clear_all();
    display_context.status = false;

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
    ESP_ERROR_CHECK(gptimer_new_timer(&gptimer_config, &display_gptimer_handle));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(display_gptimer_handle, &gptimer_callbacks, NULL));
    ESP_ERROR_CHECK(gptimer_enable(display_gptimer_handle));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(display_gptimer_handle, &alarm_config));
}
