#include <string.h>

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#include "bsp_display.h"

__unused static const char *TAG = "bsp_display";

static bool g_display_status = false;                       // 显示是否开启
static bool g_display_c_flag = false;                       // 是否显示C标志
static bool g_display_h_flag = false;                       // 是否显示H标志
static char g_display_content[BSP_DISP_MAX_CHAR + 1] = {0}; // 显示内容
static uint8_t g_display_buffer[BSP_DISP_MAX_CHAR] = {0};   // 显示缓冲区

static gptimer_handle_t display_gptimer_handle; // LED数码管刷新定时器句柄

/* LED数码管字符列表 */
static const uint8_t display_pattern_array[12][7] = {
    {0, 0, 0, 0, 0, 0, 0}, // None
    {0, 1, 1, 0, 0, 0, 0}, // 1
    {1, 1, 0, 1, 1, 0, 1}, // 2
    {1, 1, 1, 1, 0, 0, 1}, // 3
    {0, 1, 1, 0, 0, 1, 1}, // 4
    {1, 0, 1, 1, 0, 1, 1}, // 5
    {1, 0, 1, 1, 1, 1, 1}, // 6
    {1, 1, 1, 0, 0, 0, 0}, // 7
    {1, 1, 1, 1, 1, 1, 1}, // 8
    {1, 1, 1, 1, 0, 1, 1}, // 9
    {1, 1, 1, 1, 1, 1, 0}, // 0
    {1, 0, 0, 1, 1, 1, 1}, // E
};

/**
 * @brief 获取字符在LED数码管字符列表中的索引
 *
 * @param character 查询的字符
 *
 * @return 字符在数码管字符手册中的索引, 无法识别的字符返回空白字符索引
 */
static int display_get_pattern_index(char character) {
    switch (character) {
        case '1': return 1;
        case '2': return 2;
        case '3': return 3;
        case '4': return 4;
        case '5': return 5;
        case '6': return 6;
        case '7': return 7;
        case '8': return 8;
        case '9': return 9;
        case '0': return 10;
        case 'E': return 11;
        default: return 0;
    }
}

/**
 * @brief 向74HC595发送一个字节
 */
static void display_ic_send_byte(uint8_t data) {
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
static void display_ic_refresh(void) {
    gpio_set_level(BSP_DISP_IC_STCP, 0);
    gpio_set_level(BSP_DISP_IC_STCP, 1);
}

/**
 * @brief 关闭所有数码管段
 */
static void display_disable_all_seg() {
    gpio_set_level(BSP_DISP_U1_CTRL, 1);
    gpio_set_level(BSP_DISP_U2_CTRL, 1);
}

/**
 * @brief 刷新显示缓冲区
 */
static void display_flush_buffer(void) {
    for (int i = 0; i < BSP_DISP_MAX_CHAR; i++) {
        int pattern_index = display_get_pattern_index(g_display_content[i]);

        uint8_t buf_temp = 0;
        for (int j = 0; j < 7; j++) {
            buf_temp |= display_pattern_array[pattern_index][j] << j;
        }

        g_display_buffer[i] = buf_temp << 1;
    }

    if (g_display_c_flag) g_display_buffer[0] |= 0x01;
    if (g_display_h_flag) g_display_buffer[1] |= 0x01;
}

/**
 * @brief 清空显示缓冲区 & 关闭所有数码管 & 清空74HC595
 */
static void display_clear_all(void) {
    g_display_c_flag = false;
    g_display_h_flag = false;
    memset(g_display_content, 0, sizeof(g_display_content));
    memset(g_display_buffer, 0, sizeof(g_display_buffer));

    display_disable_all_seg();
    display_ic_send_byte(0);
    display_ic_refresh();
}

/**
 * @brief (数码管刷新定时器回调函数) 利用显示缓冲区刷新数码管
 */
static bool IRAM_ATTR display_refresh_timer_cb(gptimer_handle_t timer, const gptimer_alarm_event_data_t *event,
                                               void *user_data) {
    static int current_digit;

    if (current_digit >= BSP_DISP_MAX_CHAR) current_digit = 0;

    display_ic_send_byte(g_display_buffer[current_digit]);
    display_disable_all_seg();
    display_ic_refresh();
    gpio_set_level((current_digit == 0) ? BSP_DISP_U1_CTRL : BSP_DISP_U2_CTRL, 0);

    current_digit++;

    return pdFALSE;
}

void bsp_display_set_string(const char *str) {
    if (strlen(str) > BSP_DISP_MAX_CHAR) {
        ESP_LOGW(TAG, "Display content overflow, truncating display content");
    }

    memset(g_display_content, 0, sizeof(g_display_content));
    strncpy(g_display_content, str, BSP_DISP_MAX_CHAR);

    display_flush_buffer();
}

void bsp_display_set_int(int num) {
    char str[BSP_DISP_MAX_CHAR + 1];
    snprintf(str, sizeof(str), "%d", num);
    bsp_display_set_string(str);
}

void bsp_display_set_c_flag(bool flag) {
    g_display_c_flag = flag;
    display_flush_buffer();
}

void bsp_display_set_h_flag(bool flag) {
    g_display_h_flag = flag;
    display_flush_buffer();
}

void bsp_display_pause(void) {
    if (!g_display_status) return;

    ESP_ERROR_CHECK(gptimer_stop(display_gptimer_handle));
    display_clear_all();

    g_display_status = false;
}

void bsp_display_resume(void) {
    if (g_display_status) return;

    ESP_ERROR_CHECK(gptimer_start(display_gptimer_handle));

    g_display_status = true;
}

void bsp_display_init(void) {
    // 初始化74HC595所需的引脚
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = (1ULL << BSP_DISP_IC_DS) | (1ULL << BSP_DISP_IC_SHCP) | (1ULL << BSP_DISP_IC_STCP) |
                        (1ULL << BSP_DISP_U1_CTRL) | (1ULL << BSP_DISP_U2_CTRL),
        .pull_down_en = 0,
        .pull_up_en = 0,
    };
    gpio_config(&io_conf);

    // 初始化数码管刷新定时器
    gptimer_config_t gptimer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 100000, // 100kHz
    };
    gptimer_event_callbacks_t gptimer_callbacks = {
        .on_alarm = display_refresh_timer_cb,
    };
    gptimer_alarm_config_t alarm_config = {
        .alarm_count = 100, // 1ms
        .flags.auto_reload_on_alarm = true,
    };

    // 开启数码管刷新定时器
    ESP_ERROR_CHECK(gptimer_new_timer(&gptimer_config, &display_gptimer_handle));
    ESP_ERROR_CHECK(gptimer_register_event_callbacks(display_gptimer_handle, &gptimer_callbacks, NULL));
    ESP_ERROR_CHECK(gptimer_enable(display_gptimer_handle));
    ESP_ERROR_CHECK(gptimer_set_alarm_action(display_gptimer_handle, &alarm_config));

    // 关闭数码管显示 & 重置全局变量
    g_display_status = false;
    display_clear_all();
}
